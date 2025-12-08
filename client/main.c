#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#include "game.h"
#include "net_client.h"
#include "../common/config.h"
#include "../common/net_protocol.h"

#define SAVE_FILE "data.arthur"
#define SAVE_MAGIC 0xBB5A7E01
#define SAVE_KEY "N30N_BL0CK_BL4ST_K3Y_2024!"

typedef struct {
    unsigned int magic;
    int high_score;
    int music_vol;
    int sfx_vol;
    char server_ip[32];
    int server_port;
    unsigned int checksum;
} SaveData;

#define GAME_SAVE_FILE "game_session.arthur"
#define GAME_SAVE_MAGIC 0xBB5E5510

typedef struct {
    unsigned int magic;
    int grid[GRID_H][GRID_W];
    int score;
    int piece_data[3][5][5];
    int piece_w[3];
    int piece_h[3];
    int piece_color[3];
    int pieces_available[3];
    unsigned int checksum;
} GameSaveData;

static unsigned int calculate_checksum(SaveData *data) {
    unsigned char *ptr = (unsigned char *)data;
    unsigned int sum = 0;
    size_t i;
    size_t len = sizeof(SaveData) - sizeof(unsigned int);
    for (i = 0; i < len; i++) {
        sum = ((sum << 5) + sum) + ptr[i];
    }
    return sum ^ 0xDEADBEEF;
}

static void encrypt_data(unsigned char *data, size_t len) {
    const char *key = SAVE_KEY;
    size_t key_len = strlen(key);
    size_t i;
    for (i = 0; i < len; i++) {
        data[i] ^= key[i % key_len];
        data[i] = (unsigned char)((data[i] << 3) | (data[i] >> 5));
        data[i] ^= (unsigned char)(i * 17);
    }
}

static void decrypt_data(unsigned char *data, size_t len) {
    const char *key = SAVE_KEY;
    size_t key_len = strlen(key);
    size_t i;
    for (i = 0; i < len; i++) {
        data[i] ^= (unsigned char)(i * 17);
        data[i] = (unsigned char)((data[i] >> 3) | (data[i] << 5));
        data[i] ^= key[i % key_len];
    }
}

static float global_time = 0.0f;
static float glow_pulse = 0.0f;

static int window_w = WINDOW_W;
static int window_h = WINDOW_H;
static int layout_horizontal = 0;  

static int grid_offset_x = 0;
static int grid_offset_y = 0;
static int piece_area_x = 0;
static int piece_area_y = 0;
static int score_area_x = 0;
static int score_area_y = 0;
static int block_size = BLOCK_SIZE;

static Mix_Chunk *snd_place = NULL;
static Mix_Chunk *snd_clear = NULL;
static Mix_Chunk *snd_gameover = NULL;
static Mix_Chunk *snd_click = NULL;
static Mix_Music *music_bg = NULL;
static int audio_enabled = 0;

static int music_volume = 80;      
static int sfx_volume = 100;       
static int show_settings_overlay = 0;  
static int dragging_music_slider = 0;  
static int dragging_sfx_slider = 0;
static int show_pause_menu = 0;
static int has_saved_game = 0;    

enum {
    ST_MENU,
    ST_OPTIONS,
    ST_SOLO,
    ST_LOGIN,
    ST_MULTI_CHOICE,
    ST_JOIN_INPUT,
    ST_LOBBY,
    ST_MULTI_GAME,
    ST_SERVER_BROWSER,
    ST_SPECTATE
};

static SDL_Surface *screen = NULL;
static TTF_Font *font_L = NULL;
static TTF_Font *font_S = NULL;
static TTF_Font *font_XS = NULL;
static GameState game;
static int current_state = ST_MENU;
static int mouse_x = 0, mouse_y = 0;
static int selected_piece_idx = -1;

static char online_ip[32] = "127.0.0.1";
static int online_port = PORT;

static char edit_ip[32] = "127.0.0.1";
static char edit_port[16] = "5000";
static int active_input = 0;  
static int test_result = 0;   

static char my_pseudo[32] = "";
static char current_turn_pseudo[32] = "";
static char input_buffer[32] = "";
static char popup_msg[128] = "";

static LobbyState current_lobby;
static LeaderboardData leaderboard;

static ServerListData server_list;
static int browser_scroll_offset = 0;

static int selected_game_mode = GAME_MODE_CLASSIC;
static int selected_timer_minutes = 3;  

static int is_spectator = 0;
static int spectate_view_idx = 0;  

static RushPlayerState rush_states[4];
static int rush_player_count = 0;
static int rush_time_remaining = 0;  
static Uint32 last_time_update = 0;

static Uint32 last_frame_time = 0;
static float delta_time = 0.016f;

static int solo_high_score = 0;

#define DEFAULT_PIECE_AREA_Y 680
#define PIECE_SLOT_W 160
#define PIECE_SLOT_H 200

static void recalculate_layout(void) {
    float aspect_ratio = (float)window_w / (float)window_h;
    
    
    layout_horizontal = (aspect_ratio > 1.2f) ? 1 : 0;
    
    if (layout_horizontal) {
        
        
        int available_h = window_h - 40;  
        int available_w = (window_w / 2) - 40;
        
        int size_from_h = available_h / GRID_H;
        int size_from_w = available_w / GRID_W;
        block_size = (size_from_h < size_from_w) ? size_from_h : size_from_w;
        if (block_size < 20) block_size = 20;
        if (block_size > 50) block_size = 50;
        
        
        int grid_total_w = GRID_W * block_size;
        int grid_total_h = GRID_H * block_size;
        grid_offset_x = (window_w / 2 - grid_total_w) / 2;
        grid_offset_y = (window_h - grid_total_h) / 2;
        
        
        score_area_x = window_w / 2 + 20;
        score_area_y = 30;
        
        
        piece_area_x = window_w / 2 + 20;
        piece_area_y = 150;
    } else {
        
        
        int available_h = window_h - 300;  
        int available_w = window_w - 40;
        
        int size_from_h = available_h / GRID_H;
        int size_from_w = available_w / GRID_W;
        block_size = (size_from_h < size_from_w) ? size_from_h : size_from_w;
        if (block_size < 20) block_size = 20;
        if (block_size > 50) block_size = 50;
        
        
        int grid_total_w = GRID_W * block_size;
        grid_offset_x = (window_w - grid_total_w) / 2;
        grid_offset_y = 100;
        
        
        score_area_x = window_w / 2;
        score_area_y = 30;
        
        
        piece_area_x = 30;
        piece_area_y = grid_offset_y + GRID_H * block_size + 30;
    }
}

static void render_game_grid_ex(GameState *gs, int offset_x, int offset_y);
static void render_rush_game(void);
static void render_mini_grid(int grid[GRID_H][GRID_W], int x, int y, int block_size, const char *label, int score, int is_selected);

static Uint32 get_pixel(int r, int g, int b) {
    return SDL_MapRGB(screen->format, r, g, b);
}

static Uint32 color_to_pixel(Uint32 color) {
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;
    return get_pixel(r, g, b);
}

static void fill_rect(int x, int y, int w, int h, Uint32 color) {
    SDL_Rect rect = {x, y, w, h};
    SDL_FillRect(screen, &rect, color_to_pixel(color));
}

static Uint32 blend_colors(Uint32 c1, Uint32 c2, float t) {
    int r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
    int r2 = (c2 >> 16) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = c2 & 0xFF;
    int r = (int)(r1 + (r2 - r1) * t);
    int g = (int)(g1 + (g2 - g1) * t);
    int b = (int)(b1 + (b2 - b1) * t);
    return (r << 16) | (g << 8) | b;
}

static Uint32 darken_color(Uint32 color, float factor) {
    int r = (int)(((color >> 16) & 0xFF) * factor);
    int g = (int)(((color >> 8) & 0xFF) * factor);
    int b = (int)((color & 0xFF) * factor);
    return (r << 16) | (g << 8) | b;
}

static Uint32 lighten_color(Uint32 color, float factor) {
    int r = (int)(((color >> 16) & 0xFF) + (255 - ((color >> 16) & 0xFF)) * factor);
    int g = (int)(((color >> 8) & 0xFF) + (255 - ((color >> 8) & 0xFF)) * factor);
    int b = (int)((color & 0xFF) + (255 - (color & 0xFF)) * factor);
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return (r << 16) | (g << 8) | b;
}

static void draw_gradient_v(int x, int y, int w, int h, Uint32 color_top, Uint32 color_bottom) {
    int i;
    for (i = 0; i < h; i++) {
        float t = (float)i / (float)h;
        Uint32 color = blend_colors(color_top, color_bottom, t);
        fill_rect(x, y + i, w, 1, color);
    }
}

static void draw_gradient_h(int x, int y, int w, int h, Uint32 color_left, Uint32 color_right) {
    int i;
    for (i = 0; i < w; i++) {
        float t = (float)i / (float)w;
        Uint32 color = blend_colors(color_left, color_right, t);
        fill_rect(x + i, y, 1, h, color);
    }
}

static void draw_neon_border(int x, int y, int w, int h, Uint32 color, int glow_size) {
    int i;
    
    for (i = glow_size; i > 0; i--) {
        float alpha = (float)(glow_size - i + 1) / (float)(glow_size + 1) * 0.3f;
        Uint32 glow_color = darken_color(color, alpha);
        
        fill_rect(x - i, y - i, w + i * 2, 1, glow_color);
        
        fill_rect(x - i, y + h + i - 1, w + i * 2, 1, glow_color);
        
        fill_rect(x - i, y - i, 1, h + i * 2, glow_color);
        
        fill_rect(x + w + i - 1, y - i, 1, h + i * 2, glow_color);
    }
    
    fill_rect(x, y, w, 2, color);           
    fill_rect(x, y + h - 2, w, 2, color);   
    fill_rect(x, y, 2, h, color);           
    fill_rect(x + w - 2, y, 2, h, color);   
}

static void draw_neon_rect(int x, int y, int w, int h, Uint32 bg_color, Uint32 border_color, int glow_size) {
    
    int i;
    for (i = glow_size; i > 0; i--) {
        float alpha = (float)(glow_size - i + 1) / (float)(glow_size + 1) * 0.2f;
        Uint32 glow_color = darken_color(border_color, alpha);
        fill_rect(x - i, y - i, w + i * 2, h + i * 2, glow_color);
    }
    
    fill_rect(x, y, w, h, bg_color);
    
    fill_rect(x, y, w, 2, border_color);
    fill_rect(x, y + h - 2, w, 2, border_color);
    fill_rect(x, y, 2, h, border_color);
    fill_rect(x + w - 2, y, 2, h, border_color);
}

static void draw_cyberpunk_background_responsive(void) {
    int i, j;
    int grid_spacing = 60;
    
    
    draw_gradient_v(0, 0, window_w, window_h, COLOR_BG, COLOR_BG_DARK);
    
    
    for (i = 0; i < window_h; i += 3) {
        fill_rect(0, i, window_w, 1, darken_color(COLOR_BG, 0.8f));
    }
    
    
    for (i = 0; i < window_w; i += grid_spacing) {
        
        fill_rect(i - 1, 0, 1, window_h, darken_color(COLOR_NEON_CYAN, 0.05f));
        fill_rect(i + 1, 0, 1, window_h, darken_color(COLOR_NEON_CYAN, 0.05f));
        
        fill_rect(i, 0, 1, window_h, darken_color(COLOR_NEON_CYAN, 0.15f));
    }
    
    
    for (j = 0; j < window_h; j += grid_spacing) {
        
        fill_rect(0, j - 1, window_w, 1, darken_color(COLOR_NEON_MAGENTA, 0.05f));
        fill_rect(0, j + 1, window_w, 1, darken_color(COLOR_NEON_MAGENTA, 0.05f));
        
        fill_rect(0, j, window_w, 1, darken_color(COLOR_NEON_MAGENTA, 0.15f));
    }
    
    
    fill_rect(10, 10, 50, 2, COLOR_NEON_CYAN);
    fill_rect(10, 10, 2, 30, COLOR_NEON_CYAN);
    
    
    fill_rect(window_w - 60, 10, 50, 2, COLOR_NEON_CYAN);
    fill_rect(window_w - 12, 10, 2, 30, COLOR_NEON_CYAN);
    
    
    fill_rect(10, window_h - 12, 50, 2, COLOR_NEON_MAGENTA);
    fill_rect(10, window_h - 40, 2, 30, COLOR_NEON_MAGENTA);
    
    
    fill_rect(window_w - 60, window_h - 12, 50, 2, COLOR_NEON_MAGENTA);
    fill_rect(window_w - 12, window_h - 40, 2, 30, COLOR_NEON_MAGENTA);
}

static void draw_neon_block(int x, int y, int size, Uint32 color) {
    int glow_size = 3;
    int i;
    Uint32 inner_color = darken_color(color, 0.4f);
    Uint32 highlight = lighten_color(color, 0.5f);
    
    
    for (i = glow_size; i > 0; i--) {
        float alpha = (float)(glow_size - i + 1) / (float)(glow_size + 1) * 0.25f;
        Uint32 glow_color = darken_color(color, alpha);
        fill_rect(x - i, y - i, size + i * 2, size + i * 2, glow_color);
    }
    
    
    fill_rect(x, y, size, size, color);
    
    
    fill_rect(x + 4, y + 4, size - 8, size - 8, inner_color);
    
    
    fill_rect(x + 6, y + 6, size - 12, size - 12, darken_color(color, 0.6f));
    
    
    fill_rect(x + 2, y + 2, size - 4, 2, highlight);
    
    
    fill_rect(x + 2, y + 2, 2, size - 4, lighten_color(color, 0.3f));
    
    
    fill_rect(x + 2, y + size - 4, size - 4, 2, darken_color(color, 0.3f));
    
    
    fill_rect(x + size - 4, y + 2, 2, size - 4, darken_color(color, 0.3f));
    
    
    fill_rect(x + 3, y + 3, 4, 2, 0xFFFFFF);
    fill_rect(x + 3, y + 3, 2, 4, 0xFFFFFF);
}

static void draw_text(TTF_Font *f, const char *txt, int cx, int cy, Uint32 col_val) {
    SDL_Color col;
    SDL_Surface *surface;
    SDL_Rect dest;
    
    if (!txt || !txt[0]) return;
    
    col.r = (col_val >> 16) & 0xFF;
    col.g = (col_val >> 8) & 0xFF;
    col.b = col_val & 0xFF;
    
    surface = TTF_RenderUTF8_Blended(f, txt, col);
    if (!surface) return;
    
    dest.x = cx - surface->w / 2;
    dest.y = cy - surface->h / 2;
    dest.w = surface->w;
    dest.h = surface->h;
    
    SDL_BlitSurface(surface, NULL, screen, &dest);
    SDL_FreeSurface(surface);
}

static void draw_text_left(TTF_Font *f, const char *txt, int x, int y, Uint32 col_val) {
    SDL_Color col;
    SDL_Surface *surface;
    SDL_Rect dest;
    
    if (!txt || !txt[0]) return;
    
    col.r = (col_val >> 16) & 0xFF;
    col.g = (col_val >> 8) & 0xFF;
    col.b = col_val & 0xFF;
    
    surface = TTF_RenderUTF8_Blended(f, txt, col);
    if (!surface) return;
    
    dest.x = x;
    dest.y = y;
    dest.w = surface->w;
    dest.h = surface->h;
    
    SDL_BlitSurface(surface, NULL, screen, &dest);
    SDL_FreeSurface(surface);
}

static int point_in_rect(int px, int py, int x, int y, int w, int h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

static void init_audio(void) {
    
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer init failed: %s\n", Mix_GetError());
        audio_enabled = 0;
        return;
    }
    
    audio_enabled = 1;
    
    
    snd_place = Mix_LoadWAV("assets/sounds/place.wav");
    if (!snd_place) snd_place = Mix_LoadWAV("../assets/sounds/place.wav");
    if (!snd_place) snd_place = Mix_LoadWAV("assets/place.wav");
    if (!snd_place) printf("WARNING: Could not load place.wav: %s\n", Mix_GetError());
    
    snd_clear = Mix_LoadWAV("assets/sounds/clear.wav");
    if (!snd_clear) snd_clear = Mix_LoadWAV("../assets/sounds/clear.wav");
    if (!snd_clear) snd_clear = Mix_LoadWAV("assets/clear.wav");
    if (!snd_clear) printf("WARNING: Could not load clear.wav: %s\n", Mix_GetError());
    
    snd_gameover = Mix_LoadWAV("assets/sounds/gameover.wav");
    if (!snd_gameover) snd_gameover = Mix_LoadWAV("../assets/sounds/gameover.wav");
    if (!snd_gameover) snd_gameover = Mix_LoadWAV("assets/gameover.wav");
    if (!snd_gameover) printf("WARNING: Could not load gameover.wav: %s\n", Mix_GetError());
    
    snd_click = Mix_LoadWAV("assets/sounds/click.wav");
    if (!snd_click) snd_click = Mix_LoadWAV("../assets/sounds/click.wav");
    if (!snd_click) snd_click = Mix_LoadWAV("assets/click.wav");
    if (!snd_click) printf("WARNING: Could not load click.wav: %s\n", Mix_GetError());
    
    
    const char *music_paths[] = {
        "assets/sounds/music.wav",
        "assets/sounds/music.ogg",
        "assets/sounds/music.mp3",
        "assets/music.wav",
        "assets/music.ogg",
        "../assets/sounds/music.wav",
        "../assets/sounds/music.ogg",
        NULL
    };
    
    int m;
    for (m = 0; music_paths[m] != NULL && !music_bg; m++) {
        music_bg = Mix_LoadMUS(music_paths[m]);
        if (music_bg) {
            printf("Loaded music from: %s\n", music_paths[m]);
        }
    }
    
    
    Mix_VolumeMusic(80);   
    Mix_Volume(-1, 100);   
    
    printf("Audio system initialized.\n");
    if (snd_place) printf("  - place.wav loaded\n");
    if (snd_clear) printf("  - clear.wav loaded\n");
    if (snd_gameover) printf("  - gameover.wav loaded\n");
    if (snd_click) printf("  - click.wav loaded\n");
    if (music_bg) {
        printf("  - background music loaded\n");
    } else {
        printf("  - WARNING: No music file found! Place music.wav in assets/sounds/\n");
    }
}

static void cleanup_audio(void) {
    if (snd_place) Mix_FreeChunk(snd_place);
    if (snd_clear) Mix_FreeChunk(snd_clear);
    if (snd_gameover) Mix_FreeChunk(snd_gameover);
    if (snd_click) Mix_FreeChunk(snd_click);
    if (music_bg) Mix_FreeMusic(music_bg);
    
    Mix_CloseAudio();
}

static void play_sound(Mix_Chunk *sound) {
    if (audio_enabled && sound) {
        int channel = Mix_PlayChannel(-1, sound, 0);
        if (channel == -1) {
            printf("Error playing sound: %s\n", Mix_GetError());
        }
    }
}

static void play_click(void) {
    play_sound(snd_click);
}

static void play_place(void) {
    play_sound(snd_place);
}

static void play_clear(void) {
    play_sound(snd_clear);
}

static void play_gameover(void) {
    Mix_PauseMusic();
    play_sound(snd_gameover);
}

static void resume_music(void) {
    if (audio_enabled && music_bg) {
        if (Mix_PausedMusic()) {
            Mix_ResumeMusic();
        } else if (!Mix_PlayingMusic()) {
            Mix_PlayMusic(music_bg, -1);
        }
    }
}

static void start_music(void) {
    if (audio_enabled && music_bg) {
        if (Mix_PlayMusic(music_bg, -1) == -1) {
            printf("Error playing music: %s\n", Mix_GetError());
        } else {
            printf("Background music started.\n");
        }
    } else {
        if (!audio_enabled) printf("Music not started: audio disabled\n");
        if (!music_bg) printf("Music not started: no music file loaded\n");
    }
}

static void stop_music(void) {
    Mix_HaltMusic();
}

static void update_music_volume(void) {
    Mix_VolumeMusic(music_volume);
}

static void update_sfx_volume(void) {
    Mix_Volume(-1, sfx_volume);
}

static void save_game_data(void) {
    FILE *f;
    SaveData data;
    unsigned char *raw;
    
    memset(&data, 0, sizeof(data));
    data.magic = SAVE_MAGIC;
    data.high_score = solo_high_score;
    data.music_vol = music_volume;
    data.sfx_vol = sfx_volume;
    strncpy(data.server_ip, online_ip, 31);
    data.server_ip[31] = '\0';
    data.server_port = online_port;
    data.checksum = calculate_checksum(&data);
    
    raw = (unsigned char *)&data;
    encrypt_data(raw, sizeof(SaveData));
    
    f = fopen(SAVE_FILE, "wb");
    if (f) {
        fwrite(&data, sizeof(SaveData), 1, f);
        fclose(f);
    }
}

static void load_game_data(void) {
    FILE *f;
    SaveData data;
    unsigned char *raw;
    unsigned int expected_checksum;
    
    f = fopen(SAVE_FILE, "rb");
    if (!f) {
        printf("Save file not found, using defaults.\n");
        return;
    }
    
    if (fread(&data, sizeof(SaveData), 1, f) != 1) {
        printf("Save file corrupted (read error), using defaults.\n");
        fclose(f);
        return;
    }
    fclose(f);
    
    raw = (unsigned char *)&data;
    decrypt_data(raw, sizeof(SaveData));
    
    if (data.magic != SAVE_MAGIC) {
        printf("Save file corrupted (bad magic), using defaults.\n");
        return;
    }
    
    expected_checksum = data.checksum;
    data.checksum = 0;
    data.checksum = calculate_checksum(&data);
    
    if (data.checksum != expected_checksum) {
        printf("Save file corrupted (bad checksum), using defaults.\n");
        return;
    }
    
    solo_high_score = data.high_score;
    music_volume = data.music_vol;
    sfx_volume = data.sfx_vol;
    strncpy(online_ip, data.server_ip, 31);
    online_ip[31] = '\0';
    online_port = data.server_port;
    
    if (music_volume < 0) music_volume = 0;
    if (music_volume > 128) music_volume = 128;
    if (sfx_volume < 0) sfx_volume = 0;
    if (sfx_volume > 128) sfx_volume = 128;
    if (online_port <= 0 || online_port > 65535) online_port = PORT;
    
    printf("Save data loaded: score=%d, music=%d, sfx=%d, ip=%s, port=%d\n",
           solo_high_score, music_volume, sfx_volume, online_ip, online_port);
}

static unsigned int calculate_game_checksum(GameSaveData *data) {
    unsigned char *ptr = (unsigned char *)data;
    unsigned int sum = 0;
    size_t i;
    size_t len = sizeof(GameSaveData) - sizeof(unsigned int);
    for (i = 0; i < len; i++) {
        sum = ((sum << 5) + sum) + ptr[i];
    }
    return sum ^ 0xCAFEBABE;
}

static void save_current_game(void) {
    FILE *f;
    GameSaveData data;
    unsigned char *raw;
    int i, j, k;
    
    memset(&data, 0, sizeof(data));
    data.magic = GAME_SAVE_MAGIC;
    data.score = game.score;
    
    for (i = 0; i < GRID_H; i++) {
        for (j = 0; j < GRID_W; j++) {
            data.grid[i][j] = game.grid[i][j];
        }
    }
    
    for (i = 0; i < 3; i++) {
        data.piece_w[i] = game.current_pieces[i].w;
        data.piece_h[i] = game.current_pieces[i].h;
        data.piece_color[i] = game.current_pieces[i].color;
        data.pieces_available[i] = game.pieces_available[i];
        for (j = 0; j < 5; j++) {
            for (k = 0; k < 5; k++) {
                data.piece_data[i][j][k] = game.current_pieces[i].data[j][k];
            }
        }
    }
    
    data.checksum = calculate_game_checksum(&data);
    
    raw = (unsigned char *)&data;
    encrypt_data(raw, sizeof(GameSaveData));
    
    f = fopen(GAME_SAVE_FILE, "wb");
    if (f) {
        fwrite(&data, sizeof(GameSaveData), 1, f);
        fclose(f);
        has_saved_game = 1;
    }
}

static int load_current_game(void) {
    FILE *f;
    GameSaveData data;
    unsigned char *raw;
    unsigned int expected_checksum;
    int i, j, k;
    
    f = fopen(GAME_SAVE_FILE, "rb");
    if (!f) {
        return 0;
    }
    
    if (fread(&data, sizeof(GameSaveData), 1, f) != 1) {
        fclose(f);
        return 0;
    }
    fclose(f);
    
    raw = (unsigned char *)&data;
    decrypt_data(raw, sizeof(GameSaveData));
    
    if (data.magic != GAME_SAVE_MAGIC) {
        return 0;
    }
    
    expected_checksum = data.checksum;
    data.checksum = 0;
    data.checksum = calculate_game_checksum(&data);
    
    if (data.checksum != expected_checksum) {
        return 0;
    }
    
    game.score = data.score;
    game.game_over = 0;
    init_effects(&game.effects);
    
    for (i = 0; i < GRID_H; i++) {
        for (j = 0; j < GRID_W; j++) {
            game.grid[i][j] = data.grid[i][j];
        }
    }
    
    for (i = 0; i < 3; i++) {
        game.current_pieces[i].w = data.piece_w[i];
        game.current_pieces[i].h = data.piece_h[i];
        game.current_pieces[i].color = data.piece_color[i];
        game.pieces_available[i] = data.pieces_available[i];
        for (j = 0; j < 5; j++) {
            for (k = 0; k < 5; k++) {
                game.current_pieces[i].data[j][k] = data.piece_data[i][j][k];
            }
        }
    }
    
    return 1;
}

static void delete_saved_game(void) {
    remove(GAME_SAVE_FILE);
    has_saved_game = 0;
}

static void check_saved_game_exists(void) {
    FILE *f = fopen(GAME_SAVE_FILE, "rb");
    if (f) {
        has_saved_game = 1;
        fclose(f);
    } else {
        has_saved_game = 0;
    }
}

static int draw_button(int x, int y, int w, int h, const char *lbl, Uint32 color, int disabled);
static int point_in_rect(int px, int py, int rx, int ry, int rw, int rh);
static void play_click(void);
static void draw_text(TTF_Font *f, const char *txt, int cx, int cy, Uint32 col_val);
static void fill_rect(int x, int y, int w, int h, Uint32 col);
static Uint32 darken_color(Uint32 color, float factor);
static void draw_input_field(int x, int y, int w, int h, const char *text, int focused);

static int settings_tab = 0;  

static void draw_settings_gear(int x, int y) {
    int size = 28;
    int cx = x + size / 2;
    int cy = y + size / 2;
    int hover = point_in_rect(mouse_x, mouse_y, x, y, size, size);
    Uint32 color = hover ? COLOR_NEON_CYAN : COLOR_GREY;
    
    
    fill_rect(cx - 10, cy - 2, 20, 4, color);
    fill_rect(cx - 2, cy - 10, 4, 20, color);
    
    
    fill_rect(cx - 8, cy - 8, 4, 4, color);
    fill_rect(cx + 4, cy - 8, 4, 4, color);
    fill_rect(cx - 8, cy + 4, 4, 4, color);
    fill_rect(cx + 4, cy + 4, 4, 4, color);
    
    
    fill_rect(cx - 4, cy - 4, 8, 8, COLOR_PANEL);
    fill_rect(cx - 3, cy - 3, 6, 6, color);
    fill_rect(cx - 2, cy - 2, 4, 4, COLOR_PANEL);
}

static void render_pause_menu(void) {
    int i;
    int overlay_w = 350;
    int overlay_h = 280;
    int overlay_x = (window_w - overlay_w) / 2;
    int overlay_y = (window_h - overlay_h) / 2;
    
    fill_rect(0, 0, window_w, window_h, darken_color(COLOR_BG, 0.7f));
    
    for (i = 6; i > 0; i--) {
        float alpha = (float)(6 - i + 1) / 7.0f * 0.2f;
        Uint32 glow_color = darken_color(COLOR_NEON_ORANGE, alpha);
        fill_rect(overlay_x - i, overlay_y - i, overlay_w + i * 2, overlay_h + i * 2, glow_color);
    }
    
    fill_rect(overlay_x, overlay_y, overlay_w, overlay_h, COLOR_PANEL);
    
    fill_rect(overlay_x, overlay_y, overlay_w, 3, COLOR_NEON_ORANGE);
    fill_rect(overlay_x, overlay_y + overlay_h - 3, overlay_w, 3, COLOR_NEON_ORANGE);
    fill_rect(overlay_x, overlay_y, 3, overlay_h, COLOR_NEON_ORANGE);
    fill_rect(overlay_x + overlay_w - 3, overlay_y, 3, overlay_h, COLOR_NEON_ORANGE);
    
    draw_text(font_L, "PAUSE", overlay_x + overlay_w / 2, overlay_y + 45, COLOR_NEON_ORANGE);
    fill_rect(overlay_x + 50, overlay_y + 70, overlay_w - 100, 2, COLOR_NEON_ORANGE);
    
    draw_button(overlay_x + overlay_w / 2 - 120, overlay_y + 95, 240, 50, "REPRENDRE", COLOR_SUCCESS, 0);
    draw_button(overlay_x + overlay_w / 2 - 120, overlay_y + 155, 240, 50, "RECOMMENCER", COLOR_NEON_CYAN, 0);
    draw_button(overlay_x + overlay_w / 2 - 120, overlay_y + 215, 240, 50, "QUITTER", COLOR_DANGER, 0);
}

static int handle_pause_menu_click(void) {
    int overlay_w = 350;
    int overlay_h = 280;
    int overlay_x = (window_w - overlay_w) / 2;
    int overlay_y = (window_h - overlay_h) / 2;
    
    if (point_in_rect(mouse_x, mouse_y, overlay_x + overlay_w / 2 - 120, overlay_y + 95, 240, 50)) {
        play_click();
        show_pause_menu = 0;
        return 1;
    }
    
    if (point_in_rect(mouse_x, mouse_y, overlay_x + overlay_w / 2 - 120, overlay_y + 155, 240, 50)) {
        play_click();
        show_pause_menu = 0;
        delete_saved_game();
        init_game(&game);
        return 1;
    }
    
    if (point_in_rect(mouse_x, mouse_y, overlay_x + overlay_w / 2 - 120, overlay_y + 215, 240, 50)) {
        play_click();
        show_pause_menu = 0;
        save_current_game();
        current_state = ST_MENU;
        resume_music();
        return 1;
    }
    
    return 0;
}

static void render_settings_overlay(void) {
    int i;
    int overlay_w = 450;
    int overlay_h = 480;
    int overlay_x = (window_w - overlay_w) / 2;
    int overlay_y = (window_h - overlay_h) / 2;
    
    
    fill_rect(0, 0, window_w, window_h, darken_color(COLOR_BG, 0.7f));
    
    
    for (i = 6; i > 0; i--) {
        float alpha = (float)(6 - i + 1) / 7.0f * 0.2f;
        Uint32 glow_color = darken_color(COLOR_NEON_CYAN, alpha);
        fill_rect(overlay_x - i, overlay_y - i, overlay_w + i * 2, overlay_h + i * 2, glow_color);
    }
    
    
    fill_rect(overlay_x, overlay_y, overlay_w, overlay_h, COLOR_PANEL);
    
    
    fill_rect(overlay_x, overlay_y, overlay_w, 3, COLOR_NEON_CYAN);
    fill_rect(overlay_x, overlay_y + overlay_h - 3, overlay_w, 3, COLOR_NEON_MAGENTA);
    fill_rect(overlay_x, overlay_y, 3, overlay_h, COLOR_NEON_CYAN);
    fill_rect(overlay_x + overlay_w - 3, overlay_y, 3, overlay_h, COLOR_NEON_MAGENTA);
    
    
    draw_text(font_L, "PARAMETRES", overlay_x + overlay_w / 2, overlay_y + 35, COLOR_NEON_CYAN);
    
    
    int tab_y = overlay_y + 65;
    int tab_w = (overlay_w - 40) / 2;
    
    
    Uint32 audio_tab_color = (settings_tab == 0) ? COLOR_NEON_CYAN : COLOR_GREY;
    fill_rect(overlay_x + 15, tab_y, tab_w, 35, (settings_tab == 0) ? darken_color(COLOR_NEON_CYAN, 0.2f) : COLOR_PANEL);
    fill_rect(overlay_x + 15, tab_y, tab_w, 2, audio_tab_color);
    draw_text(font_S, "AUDIO", overlay_x + 15 + tab_w / 2, tab_y + 18, audio_tab_color);
    
    
    Uint32 net_tab_color = (settings_tab == 1) ? COLOR_NEON_MAGENTA : COLOR_GREY;
    fill_rect(overlay_x + 25 + tab_w, tab_y, tab_w, 35, (settings_tab == 1) ? darken_color(COLOR_NEON_MAGENTA, 0.2f) : COLOR_PANEL);
    fill_rect(overlay_x + 25 + tab_w, tab_y, tab_w, 2, net_tab_color);
    draw_text(font_S, "RESEAU", overlay_x + 25 + tab_w + tab_w / 2, tab_y + 18, net_tab_color);
    
    
    fill_rect(overlay_x + 15, tab_y + 40, overlay_w - 30, 2, COLOR_GREY);
    
    int content_y = tab_y + 55;
    int slider_x = overlay_x + 40;
    int slider_w = overlay_w - 80;
    char vol_text[16];
    
    if (settings_tab == 0) {
        
        
        
        int music_slider_y = content_y + 15;
        draw_text(font_S, "Volume Musique", overlay_x + overlay_w / 2, music_slider_y, COLOR_WHITE);
        
        
        fill_rect(slider_x, music_slider_y + 30, slider_w, 24, darken_color(COLOR_PANEL, 0.5f));
        fill_rect(slider_x, music_slider_y + 30, slider_w, 2, COLOR_GREY);
        fill_rect(slider_x, music_slider_y + 52, slider_w, 2, COLOR_GREY);
        
        
        int music_fill_w = (music_volume * slider_w) / 128;
        fill_rect(slider_x, music_slider_y + 30, music_fill_w, 24, COLOR_NEON_CYAN);
        
        
        int music_handle_x = slider_x + music_fill_w - 6;
        fill_rect(music_handle_x, music_slider_y + 26, 12, 32, COLOR_WHITE);
        
        
        snprintf(vol_text, sizeof(vol_text), "%d%%", (music_volume * 100) / 128);
        draw_text(font_S, vol_text, overlay_x + overlay_w / 2, music_slider_y + 75, COLOR_NEON_CYAN);
        
        
        int sfx_slider_y = content_y + 120;
        draw_text(font_S, "Volume Effets Sonores", overlay_x + overlay_w / 2, sfx_slider_y, COLOR_WHITE);
        
        
        fill_rect(slider_x, sfx_slider_y + 30, slider_w, 24, darken_color(COLOR_PANEL, 0.5f));
        fill_rect(slider_x, sfx_slider_y + 30, slider_w, 2, COLOR_GREY);
        fill_rect(slider_x, sfx_slider_y + 52, slider_w, 2, COLOR_GREY);
        
        
        int sfx_fill_w = (sfx_volume * slider_w) / 128;
        fill_rect(slider_x, sfx_slider_y + 30, sfx_fill_w, 24, COLOR_NEON_MAGENTA);
        
        
        int sfx_handle_x = slider_x + sfx_fill_w - 6;
        fill_rect(sfx_handle_x, sfx_slider_y + 26, 12, 32, COLOR_WHITE);
        
        
        snprintf(vol_text, sizeof(vol_text), "%d%%", (sfx_volume * 100) / 128);
        draw_text(font_S, vol_text, overlay_x + overlay_w / 2, sfx_slider_y + 75, COLOR_NEON_MAGENTA);
        
    } else {
        
        
        
        int ip_y = content_y + 15;
        draw_text(font_S, "Adresse IP du serveur", overlay_x + overlay_w / 2, ip_y, COLOR_WHITE);
        draw_input_field(overlay_x + 30, ip_y + 25, overlay_w - 60, 45, edit_ip, active_input == 1);
        
        
        int port_y = content_y + 110;
        draw_text(font_S, "Port", overlay_x + overlay_w / 2, port_y, COLOR_WHITE);
        draw_input_field(overlay_x + 30, port_y + 25, overlay_w - 60, 45, edit_port, active_input == 2);
        
        
        draw_text(font_XS ? font_XS : font_S, "Cliquez sur un champ pour le modifier", 
                  overlay_x + overlay_w / 2, content_y + 200, COLOR_GREY);
    }
    
    
    int close_btn_y = overlay_y + overlay_h - 65;
    draw_button(overlay_x + overlay_w / 2 - 90, close_btn_y, 180, 45, "FERMER", COLOR_DANGER, 0);
    
    
    draw_text(font_XS ? font_XS : font_S, "Appuyez sur ESC pour fermer", 
              overlay_x + overlay_w / 2, overlay_y + overlay_h - 15, COLOR_GREY);
}

static int handle_settings_overlay_mousedown(void) {
    int overlay_w = 450;
    int overlay_h = 480;
    int overlay_x = (window_w - overlay_w) / 2;
    int overlay_y = (window_h - overlay_h) / 2;
    int slider_x = overlay_x + 40;
    int slider_w = overlay_w - 80;
    int tab_y = overlay_y + 65;
    int content_y = tab_y + 55;
    
    
    if (settings_tab == 0) {
        
        int music_slider_y = content_y + 15 + 30;
        if (mouse_y >= music_slider_y - 5 && mouse_y <= music_slider_y + 30 &&
            mouse_x >= slider_x - 10 && mouse_x <= slider_x + slider_w + 10) {
            dragging_music_slider = 1;
            int new_vol = ((mouse_x - slider_x) * 128) / slider_w;
            if (new_vol < 0) new_vol = 0;
            if (new_vol > 128) new_vol = 128;
            music_volume = new_vol;
            update_music_volume();
            return 1;
        }
        
        
        int sfx_slider_y = content_y + 120 + 30;
        if (mouse_y >= sfx_slider_y - 5 && mouse_y <= sfx_slider_y + 30 &&
            mouse_x >= slider_x - 10 && mouse_x <= slider_x + slider_w + 10) {
            dragging_sfx_slider = 1;
            int new_vol = ((mouse_x - slider_x) * 128) / slider_w;
            if (new_vol < 0) new_vol = 0;
            if (new_vol > 128) new_vol = 128;
            sfx_volume = new_vol;
            update_sfx_volume();
            return 1;
        }
    }
    
    return 0;
}

static int handle_settings_overlay_click(void) {
    int overlay_w = 450;
    int overlay_h = 480;
    int overlay_x = (window_w - overlay_w) / 2;
    int overlay_y = (window_h - overlay_h) / 2;
    int tab_y = overlay_y + 65;
    int tab_w = (overlay_w - 40) / 2;
    int content_y = tab_y + 55;
    
    
    if (dragging_sfx_slider) {
        play_click();  
    }
    dragging_music_slider = 0;
    dragging_sfx_slider = 0;
    
    
    if (mouse_y >= tab_y && mouse_y <= tab_y + 35) {
        if (mouse_x >= overlay_x + 15 && mouse_x <= overlay_x + 15 + tab_w) {
            if (settings_tab != 0) {
                settings_tab = 0;
                active_input = 0;
                play_click();
            }
            return 1;
        }
        if (mouse_x >= overlay_x + 25 + tab_w && mouse_x <= overlay_x + 25 + tab_w + tab_w) {
            if (settings_tab != 1) {
                settings_tab = 1;
                
                strcpy(edit_ip, online_ip);
                snprintf(edit_port, sizeof(edit_port), "%d", online_port);
                active_input = 0;
                play_click();
            }
            return 1;
        }
    }
    
    
    if (settings_tab == 1) {
        int ip_y = content_y + 15 + 25;
        int port_y = content_y + 110 + 25;
        
        if (point_in_rect(mouse_x, mouse_y, overlay_x + 30, ip_y, overlay_w - 60, 45)) {
            active_input = 1;
            play_click();
            return 1;
        }
        if (point_in_rect(mouse_x, mouse_y, overlay_x + 30, port_y, overlay_w - 60, 45)) {
            active_input = 2;
            play_click();
            return 1;
        }
    }
    
    
    int close_btn_y = overlay_y + overlay_h - 65;
    if (point_in_rect(mouse_x, mouse_y, overlay_x + overlay_w / 2 - 90, close_btn_y, 180, 45)) {
        if (settings_tab == 1) {
            strcpy(online_ip, edit_ip);
            online_port = atoi(edit_port);
            if (online_port <= 0) online_port = 12345;
        }
        save_game_data();
        show_settings_overlay = 0;
        play_click();
        return 1;
    }
    
    return 0;
}

static void handle_settings_overlay_drag(void) {
    
    if (!dragging_music_slider && !dragging_sfx_slider) return;
    
    int overlay_w = 450;
    int overlay_x = (window_w - overlay_w) / 2;
    int slider_x = overlay_x + 40;
    int slider_w = overlay_w - 80;
    
    
    if (dragging_music_slider) {
        int new_vol = ((mouse_x - slider_x) * 128) / slider_w;
        if (new_vol < 0) new_vol = 0;
        if (new_vol > 128) new_vol = 128;
        if (new_vol != music_volume) {
            music_volume = new_vol;
            update_music_volume();
        }
    }
    
    
    if (dragging_sfx_slider) {
        int new_vol = ((mouse_x - slider_x) * 128) / slider_w;
        if (new_vol < 0) new_vol = 0;
        if (new_vol > 128) new_vol = 128;
        if (new_vol != sfx_volume) {
            sfx_volume = new_vol;
            update_sfx_volume();
        }
    }
}

static int draw_button(int x, int y, int w, int h, const char *lbl, Uint32 color, int disabled) {
    int hover;
    Uint32 border_color;
    int glow_intensity;
    float pulse;
    
    if (disabled) {
        
        fill_rect(x, y, w, h, COLOR_BTN_DISABLED);
        
        fill_rect(x, y, w, 1, 0x444455);
        fill_rect(x, y + h - 1, w, 1, 0x444455);
        fill_rect(x, y, 1, h, 0x444455);
        fill_rect(x + w - 1, y, 1, h, 0x444455);
        draw_text(font_S, lbl, x + w/2, y + h/2, COLOR_GREY);
        return 0;
    }
    
    hover = point_in_rect(mouse_x, mouse_y, x, y, w, h);
    
    
    if (color == COLOR_SUCCESS) {
        border_color = COLOR_NEON_GREEN;
    } else if (color == COLOR_DANGER) {
        border_color = COLOR_NEON_RED;
    } else if (color == COLOR_PURPLE) {
        border_color = COLOR_NEON_PURPLE;
    } else {
        border_color = COLOR_NEON_CYAN;
    }
    
    
    pulse = (sinf(glow_pulse * BUTTON_PULSE_SPEED) + 1.0f) * 0.5f;
    glow_intensity = hover ? (int)(4 + pulse * 3) : 2;
    
    
    {
        int i;
        for (i = glow_intensity; i > 0; i--) {
            float alpha = (float)(glow_intensity - i + 1) / (float)(glow_intensity + 1) * (hover ? 0.4f : 0.15f);
            Uint32 glow_color = darken_color(border_color, alpha);
            fill_rect(x - i, y - i, w + i * 2, h + i * 2, glow_color);
        }
    }
    
    
    if (hover) {
        draw_gradient_v(x, y, w, h, lighten_color(COLOR_BUTTON, 0.3f), COLOR_BUTTON);
    } else {
        draw_gradient_v(x, y, w, h, COLOR_BUTTON, darken_color(COLOR_BUTTON, 0.7f));
    }
    
    
    {
        Uint32 actual_border = hover ? lighten_color(border_color, 0.3f) : border_color;
        fill_rect(x, y, w, 2, actual_border);
        fill_rect(x, y + h - 2, w, 2, actual_border);
        fill_rect(x, y, 2, h, actual_border);
        fill_rect(x + w - 2, y, 2, h, actual_border);
        
        
        fill_rect(x + 4, y + 4, w - 8, 1, darken_color(actual_border, 0.5f));
    }
    
    
    if (hover) {
        draw_text(font_S, lbl, x + w/2 + 1, y + h/2 + 1, darken_color(border_color, 0.3f));
    }
    draw_text(font_S, lbl, x + w/2, y + h/2, COLOR_WHITE);
    
    return hover;
}

static void draw_input_field(int x, int y, int w, int h, const char *text, int focused) {
    Uint32 bg_color = focused ? COLOR_INPUT_FOCUS : COLOR_INPUT;
    Uint32 border_color = focused ? COLOR_NEON_CYAN : darken_color(COLOR_NEON_CYAN, 0.4f);
    int glow_size = focused ? 4 : 0;
    int i;
    float pulse = (sinf(glow_pulse * 3.0f) + 1.0f) * 0.5f;
    
    
    if (focused) {
        for (i = glow_size; i > 0; i--) {
            float alpha = (float)(glow_size - i + 1) / (float)(glow_size + 1) * (0.2f + pulse * 0.1f);
            Uint32 glow_color = darken_color(COLOR_NEON_CYAN, alpha);
            fill_rect(x - i, y - i, w + i * 2, h + i * 2, glow_color);
        }
    }
    
    
    draw_gradient_v(x, y, w, h, bg_color, darken_color(bg_color, 0.7f));
    
    
    fill_rect(x, y, w, 2, border_color);
    fill_rect(x, y + h - 2, w, 2, border_color);
    fill_rect(x, y, 2, h, border_color);
    fill_rect(x + w - 2, y, 2, h, border_color);
    
    
    fill_rect(x + 2, y + 2, w - 4, 2, darken_color(bg_color, 0.5f));
    
    
    if (text && text[0]) {
        draw_text_left(font_S, text, x + 12, y + (h - 22) / 2, COLOR_WHITE);
    }
    
    
    if (focused) {
        int text_w = 0;
        if (text && text[0]) {
            TTF_SizeUTF8(font_S, text, &text_w, NULL);
        }
        
        if (((int)(glow_pulse * 2) % 2) == 0) {
            fill_rect(x + 12 + text_w + 2, y + 8, 2, h - 16, COLOR_NEON_CYAN);
        }
    }
}

static void draw_styled_block(int x, int y, int size, Uint32 color) {
    
    draw_neon_block(x, y, size, color);
}

static void draw_styled_block_scaled(int x, int y, int size, Uint32 color, float scale) {
    int scaled_size = (int)(size * scale);
    int offset = (size - scaled_size) / 2;
    int glow_size = (int)(2 * scale);
    int i;
    Uint32 inner_color = darken_color(color, 0.4f);
    Uint32 highlight = lighten_color(color, 0.4f);
    
    int bx = x + offset;
    int by = y + offset;
    
    
    for (i = glow_size; i > 0; i--) {
        float alpha = (float)(glow_size - i + 1) / (float)(glow_size + 1) * 0.2f;
        Uint32 glow_color = darken_color(color, alpha);
        fill_rect(bx - i, by - i, scaled_size + i * 2, scaled_size + i * 2, glow_color);
    }
    
    
    fill_rect(bx, by, scaled_size, scaled_size, color);
    
    
    int inner_margin = (int)(3 * scale);
    if (inner_margin < 1) inner_margin = 1;
    fill_rect(bx + inner_margin, by + inner_margin, 
              scaled_size - inner_margin * 2, scaled_size - inner_margin * 2, inner_color);
    
    
    fill_rect(bx + 1, by + 1, scaled_size - 2, 1, highlight);
    
    
    if (scale > 0.5f) {
        fill_rect(bx + 2, by + 2, 2, 1, 0xFFFFFF);
    }
}

static void render_effects(EffectsManager *em, int offset_x, int offset_y) {
    int i;
    
    
    for (i = 0; i < MAX_PARTICLES; i++) {
        if (em->particles[i].active) {
            Particle *p = &em->particles[i];
            float alpha = p->life;
            if (alpha > 1.0f) alpha = 1.0f;
            if (alpha < 0.0f) alpha = 0.0f;
            
            Uint32 color = p->color;
            int px = (int)p->x + offset_x;
            int py = (int)p->y + offset_y;
            
            
            Uint32 glow_color = darken_color(color, alpha * 0.3f);
            fill_rect(px - p->size, py - p->size, p->size * 2, p->size * 2, glow_color);
            
            
            int r = (int)(((color >> 16) & 0xFF) * alpha);
            int g = (int)(((color >> 8) & 0xFF) * alpha);
            int b = (int)((color & 0xFF) * alpha);
            Uint32 core_color = (r << 16) | (g << 8) | b;
            
            fill_rect(px - p->size/2, py - p->size/2, p->size, p->size, core_color);
            
            
            if (alpha > 0.5f) {
                int center_size = p->size / 3;
                if (center_size < 1) center_size = 1;
                fill_rect(px - center_size/2, py - center_size/2, center_size, center_size, 0xFFFFFF);
            }
        }
    }
    
    
    for (i = 0; i < 20; i++) {
        if (em->line_clears[i].active) {
            LineClearEffect *lc = &em->line_clears[i];
            float progress = lc->progress;
            float intensity = (1.0f - progress);
            if (intensity < 0) intensity = 0;
            
            
            Uint32 flash_colors[] = {COLOR_NEON_CYAN, COLOR_NEON_MAGENTA, COLOR_NEON_GREEN};
            Uint32 flash_color = flash_colors[i % 3];
            
            if (lc->row >= 0) {
                
                int sweep_pos = (int)(progress * GRID_W * BLOCK_SIZE);
                int flash_width = (int)(GRID_W * BLOCK_SIZE * (1.0f - progress * 0.5f));
                
                
                Uint32 glow = darken_color(flash_color, intensity * 0.4f);
                fill_rect(GRID_OFFSET_X + offset_x - 5, 
                          GRID_OFFSET_Y + lc->row * BLOCK_SIZE + offset_y - 5,
                          GRID_W * BLOCK_SIZE + 10, BLOCK_SIZE + 10, glow);
                
                
                Uint32 main_flash = darken_color(flash_color, intensity * 0.8f);
                fill_rect(GRID_OFFSET_X + offset_x, 
                          GRID_OFFSET_Y + lc->row * BLOCK_SIZE + offset_y,
                          flash_width, BLOCK_SIZE, main_flash);
                
                
                if (progress < 0.8f) {
                    fill_rect(GRID_OFFSET_X + offset_x + sweep_pos - 5, 
                              GRID_OFFSET_Y + lc->row * BLOCK_SIZE + offset_y,
                              10, BLOCK_SIZE, 0xFFFFFF);
                }
            }
            if (lc->col >= 0) {
                
                int sweep_pos = (int)(progress * GRID_H * BLOCK_SIZE);
                int flash_height = (int)(GRID_H * BLOCK_SIZE * (1.0f - progress * 0.5f));
                
                
                Uint32 glow = darken_color(flash_color, intensity * 0.4f);
                fill_rect(GRID_OFFSET_X + lc->col * BLOCK_SIZE + offset_x - 5,
                          GRID_OFFSET_Y + offset_y - 5,
                          BLOCK_SIZE + 10, GRID_H * BLOCK_SIZE + 10, glow);
                
                
                Uint32 main_flash = darken_color(flash_color, intensity * 0.8f);
                fill_rect(GRID_OFFSET_X + lc->col * BLOCK_SIZE + offset_x,
                          GRID_OFFSET_Y + offset_y,
                          BLOCK_SIZE, flash_height, main_flash);
                
                
                if (progress < 0.8f) {
                    fill_rect(GRID_OFFSET_X + lc->col * BLOCK_SIZE + offset_x,
                              GRID_OFFSET_Y + offset_y + sweep_pos - 5,
                              BLOCK_SIZE, 10, 0xFFFFFF);
                }
            }
        }
    }
    
    
    for (i = 0; i < 25; i++) {
        if (em->place_effects[i].active) {
            PlaceEffect *pe = &em->place_effects[i];
            int px = GRID_OFFSET_X + pe->col * BLOCK_SIZE + BLOCK_SIZE/2 + offset_x;
            int py = GRID_OFFSET_Y + pe->row * BLOCK_SIZE + BLOCK_SIZE/2 + offset_y;
            
            
            float ring_progress = 1.0f - pe->scale;
            int ring_size = (int)(ring_progress * 25);
            float ring_alpha = pe->alpha;
            
            if (ring_size > 0 && ring_alpha > 0) {
                Uint32 ring_color = darken_color(pe->color, ring_alpha * 0.6f);
                
                
                int half = ring_size;
                fill_rect(px - half, py - half, ring_size * 2, 2, ring_color);
                fill_rect(px - half, py + half - 2, ring_size * 2, 2, ring_color);
                fill_rect(px - half, py - half, 2, ring_size * 2, ring_color);
                fill_rect(px + half - 2, py - half, 2, ring_size * 2, ring_color);
            }
        }
    }
}

static void draw_eye_icon(int x, int y, Uint32 color) {
    
    fill_rect(x + 2, y + 6, 12, 4, color);  
    fill_rect(x + 6, y + 4, 4, 8, color);   
    fill_rect(x + 7, y + 7, 2, 2, 0x000000); 
}

static void draw_popup(void) {
    int i;
    int popup_x = 40;
    int popup_y = WINDOW_H / 2 - 80;
    int popup_w = WINDOW_W - 80;
    int popup_h = 160;
    
    if (!popup_msg[0]) return;
    
    
    fill_rect(0, 0, WINDOW_W, WINDOW_H, darken_color(COLOR_BG, 0.5f));
    
    
    for (i = 8; i > 0; i--) {
        float alpha = (float)(8 - i + 1) / 9.0f * 0.25f;
        Uint32 glow_color = darken_color(COLOR_NEON_CYAN, alpha);
        fill_rect(popup_x - i, popup_y - i, popup_w + i * 2, popup_h + i * 2, glow_color);
    }
    
    
    draw_gradient_v(popup_x, popup_y, popup_w, popup_h, COLOR_PANEL, darken_color(COLOR_PANEL, 0.6f));
    
    
    fill_rect(popup_x, popup_y, popup_w, 3, COLOR_NEON_CYAN);
    fill_rect(popup_x, popup_y + popup_h - 3, popup_w, 3, COLOR_NEON_MAGENTA);
    fill_rect(popup_x, popup_y, 3, popup_h, COLOR_NEON_CYAN);
    fill_rect(popup_x + popup_w - 3, popup_y, 3, popup_h, COLOR_NEON_MAGENTA);
    
    
    fill_rect(popup_x, popup_y, 20, 3, 0xFFFFFF);
    fill_rect(popup_x, popup_y, 3, 20, 0xFFFFFF);
    fill_rect(popup_x + popup_w - 20, popup_y + popup_h - 3, 20, 3, 0xFFFFFF);
    fill_rect(popup_x + popup_w - 3, popup_y + popup_h - 20, 3, 20, 0xFFFFFF);
    
    
    draw_text(font_S, popup_msg, WINDOW_W / 2 + 1, WINDOW_H / 2 - 20 + 1, darken_color(COLOR_NEON_CYAN, 0.3f));
    draw_text(font_S, popup_msg, WINDOW_W / 2, WINDOW_H / 2 - 20, COLOR_WHITE);
    
    
    draw_text(font_S, "(Cliquez pour fermer)", WINDOW_W / 2, WINDOW_H / 2 + 30, COLOR_NEON_MAGENTA);
}

static void handle_input_char(Uint16 unicode) {
    char c = (char)unicode;
    int len;
    
    if (current_state == ST_OPTIONS) {
        if (active_input == 1) {
            
            len = strlen(edit_ip);
            if (unicode == 8 && len > 0) {
                edit_ip[len - 1] = '\0';
            } else if ((c >= '0' && c <= '9') || c == '.') {
                if (len < 30) {
                    edit_ip[len] = c;
                    edit_ip[len + 1] = '\0';
                }
            }
            test_result = 0;
        } else if (active_input == 2) {
            
            len = strlen(edit_port);
            if (unicode == 8 && len > 0) {
                edit_port[len - 1] = '\0';
            } else if (c >= '0' && c <= '9') {
                if (len < 6) {
                    edit_port[len] = c;
                    edit_port[len + 1] = '\0';
                }
            }
            test_result = 0;
        }
    } else {
        
        len = strlen(input_buffer);
        
        if (unicode == 8 && len > 0) {
            
            input_buffer[len - 1] = '\0';
        } else if (len < 12) {
            
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') || c == ' ') {
                
                
                if (current_state == ST_JOIN_INPUT && c >= 'a' && c <= 'z') {
                    c = c - 'a' + 'A';
                }
                
                input_buffer[len] = c;
                input_buffer[len + 1] = '\0';
            }
        }
    }
}

static int is_my_turn(void) {
    return strcmp(my_pseudo, current_turn_pseudo) == 0;
}

static int perform_connection_test(void) {
    int port = atoi(edit_port);
    
    if (net_connect(edit_ip, port)) {
        net_close();
        return 1;
    }
    return 0;
}

static void render_menu(void) {
    
    float title_pulse = (sinf(glow_pulse * 2.0f) + 1.0f) * 0.5f;
    int title_y = 120;
    int center_x = window_w / 2;
    
    
    {
        int glow_size = (int)(8 + title_pulse * 4);
        int i;
        for (i = glow_size; i > 0; i--) {
            float alpha = (float)(glow_size - i + 1) / (float)(glow_size + 1) * 0.15f;
            
        }
    }
    
    
    draw_text(font_L, "BLOCKBLAST", center_x + 2, title_y + 2, darken_color(COLOR_NEON_CYAN, 0.3f));
    draw_text(font_L, "BLOCKBLAST", center_x, title_y, COLOR_NEON_CYAN);
    
    
    draw_text(font_S, "NEON EDITION", center_x, title_y + 50, COLOR_NEON_MAGENTA);
    
    
    int line_y = title_y + 85;
    fill_rect(center_x - 150, line_y, 100, 2, COLOR_NEON_CYAN);
    fill_rect(center_x + 50, line_y, 100, 2, COLOR_NEON_MAGENTA);
    
    
    {
        int cx = center_x;
        int cy = line_y;
        int size = 6;
        fill_rect(cx - size/2, cy - 1, size, 3, COLOR_NEON_GREEN);
        fill_rect(cx - 1, cy - size/2, 3, size, COLOR_NEON_GREEN);
    }
    
    
    int btn_y = 250;
    
    if (has_saved_game) {
        draw_button(center_x - 150, btn_y, 300, 55, "CONTINUER", COLOR_NEON_ORANGE, 0);
        btn_y += 70;
    }
    
    draw_button(center_x - 150, btn_y, 300, 55, "NOUVELLE PARTIE", COLOR_SUCCESS, 0);
    btn_y += 70;
    draw_button(center_x - 150, btn_y, 300, 55, "MULTI ONLINE", COLOR_BUTTON, 0);
    btn_y += 70;
    draw_button(center_x - 150, btn_y, 300, 55, "PARAMETRES", COLOR_PURPLE, 0);
    
    draw_settings_gear(window_w - 50, 20);
    
    draw_text(font_XS ? font_XS : font_S, "v4.0 NEON", center_x, window_h - 30, COLOR_GREY);
}

static void render_options(void) {
    
    draw_text(font_L, "CONFIGURATION", WINDOW_W / 2 + 2, 80 + 2, darken_color(COLOR_NEON_MAGENTA, 0.3f));
    draw_text(font_L, "CONFIGURATION", WINDOW_W / 2, 80, COLOR_NEON_MAGENTA);
    
    
    fill_rect(WINDOW_W / 2 - 120, 120, 240, 2, COLOR_NEON_MAGENTA);
    
    
    draw_text(font_S, "Adresse IP:", WINDOW_W / 2, 170, COLOR_NEON_CYAN);
    draw_input_field(70, 195, 400, 50, edit_ip, active_input == 1);
    
    
    draw_text(font_S, "Port:", WINDOW_W / 2, 280, COLOR_NEON_CYAN);
    draw_input_field(70, 305, 400, 50, edit_port, active_input == 2);
    
    
    if (test_result == 1) {
        draw_text(font_S, "Connexion OK!", WINDOW_W / 2 + 1, 401, darken_color(COLOR_NEON_GREEN, 0.4f));
        draw_text(font_S, "Connexion OK!", WINDOW_W / 2, 400, COLOR_NEON_GREEN);
    } else if (test_result == -1) {
        draw_text(font_S, "Echec connexion!", WINDOW_W / 2 + 1, 401, darken_color(COLOR_NEON_RED, 0.4f));
        draw_text(font_S, "Echec connexion!", WINDOW_W / 2, 400, COLOR_NEON_RED);
    }
    
    
    draw_button(70, 460, 180, 50, "TESTER", COLOR_BUTTON, 0);
    draw_button(290, 460, 180, 50, "SAUVEGARDER", COLOR_SUCCESS, test_result != 1);
    draw_button(WINDOW_W / 2 - 90, 560, 180, 50, "RETOUR", COLOR_DANGER, 0);
}

static void render_login(void) {
    
    draw_settings_gear(window_w - 50, 20);
    
    
    draw_text(font_L, "CONNEXION", WINDOW_W / 2 + 2, 120 + 2, darken_color(COLOR_NEON_CYAN, 0.3f));
    draw_text(font_L, "CONNEXION", WINDOW_W / 2, 120, COLOR_NEON_CYAN);
    
    
    fill_rect(WINDOW_W / 2 - 100, 165, 200, 2, COLOR_NEON_CYAN);
    fill_rect(WINDOW_W / 2 - 80, 170, 160, 1, darken_color(COLOR_NEON_CYAN, 0.5f));
    
    
    {
        int icon_x = WINDOW_W / 2;
        int icon_y = 220;
        
        fill_rect(icon_x - 15, icon_y, 30, 30, COLOR_NEON_MAGENTA);
        fill_rect(icon_x - 10, icon_y + 5, 20, 20, COLOR_PANEL);
        fill_rect(icon_x - 5, icon_y + 8, 10, 10, COLOR_NEON_MAGENTA);
    }
    
    draw_text(font_S, "PSEUDO :", WINDOW_W / 2, 280, COLOR_NEON_CYAN);
    draw_input_field(70, 310, 400, 50, input_buffer, 1);
    
    draw_button(WINDOW_W / 2 - 90, 420, 180, 50, "VALIDER", COLOR_SUCCESS, strlen(input_buffer) == 0);
    draw_button(WINDOW_W / 2 - 90, 500, 180, 50, "RETOUR", COLOR_DANGER, 0);
}

static void render_multi_choice(void) {
    int i;
    char line[64];
    
    
    draw_settings_gear(window_w - 50, 20);
    
    
    draw_text(font_L, "MULTIJOUEUR", WINDOW_W / 2 + 2, 80 + 2, darken_color(COLOR_NEON_PURPLE, 0.3f));
    draw_text(font_L, "MULTIJOUEUR", WINDOW_W / 2, 80, COLOR_NEON_PURPLE);
    
    
    {
        int panel_x = 50;
        int panel_y = 130;
        int panel_w = WINDOW_W - 100;
        int panel_h = 230;
        
        
        for (i = 4; i > 0; i--) {
            float alpha = (float)(4 - i + 1) / 5.0f * 0.1f;
            Uint32 glow_color = darken_color(COLOR_NEON_CYAN, alpha);
            fill_rect(panel_x - i, panel_y - i, panel_w + i * 2, panel_h + i * 2, glow_color);
        }
        
        
        fill_rect(panel_x, panel_y, panel_w, panel_h, COLOR_PANEL);
        
        
        fill_rect(panel_x, panel_y, panel_w, 2, COLOR_NEON_CYAN);
        fill_rect(panel_x, panel_y + panel_h - 2, panel_w, 2, COLOR_NEON_CYAN);
        fill_rect(panel_x, panel_y, 2, panel_h, COLOR_NEON_CYAN);
        fill_rect(panel_x + panel_w - 2, panel_y, 2, panel_h, COLOR_NEON_CYAN);
    }
    
    
    draw_text(font_S, "MEILLEURS SCORES", WINDOW_W / 2, 155, COLOR_NEON_CYAN);
    fill_rect(WINDOW_W / 2 - 80, 175, 160, 1, COLOR_NEON_CYAN);
    
    
    for (i = 0; i < leaderboard.count && i < 5; i++) {
        Uint32 entry_color;
        
        if (i == 0) entry_color = COLOR_GOLD;
        else if (i == 1) entry_color = 0xC0C0C0;
        else if (i == 2) entry_color = 0xCD7F32;
        else entry_color = COLOR_WHITE;
        
        snprintf(line, sizeof(line), "%d. %s", i + 1, leaderboard.names[i]);
        draw_text_left(font_S, line, 80, 195 + i * 32, entry_color);
        
        snprintf(line, sizeof(line), "%d pts", leaderboard.scores[i]);
        draw_text_left(font_S, line, WINDOW_W - 180, 195 + i * 32, COLOR_NEON_GREEN);
    }
    
    if (leaderboard.count == 0) {
        draw_text(font_S, "Aucun score enregistre", WINDOW_W / 2, 250, COLOR_GREY);
    }
    
    
    draw_button(WINDOW_W / 2 - 150, 400, 300, 55, "CREER UNE PARTIE", COLOR_SUCCESS, 0);
    draw_button(WINDOW_W / 2 - 150, 475, 300, 55, "REJOINDRE (CODE)", COLOR_BUTTON, 0);
    draw_button(WINDOW_W / 2 - 150, 550, 300, 55, "SERVEURS OUVERTS", COLOR_PURPLE, 0);
    draw_button(WINDOW_W / 2 - 90, 640, 180, 50, "RETOUR", COLOR_DANGER, 0);
}

static void render_server_browser(void) {
    int i, j;
    char line[128];
    int y_start = 150;
    int entry_height = 105;
    
    
    draw_settings_gear(window_w - 50, 20);
    
    
    draw_text(font_L, "SERVEURS", WINDOW_W / 2 + 2, 50 + 2, darken_color(COLOR_NEON_PURPLE, 0.3f));
    draw_text(font_L, "SERVEURS", WINDOW_W / 2, 50, COLOR_NEON_PURPLE);
    
    
    draw_text(font_S, "Parties Publiques", WINDOW_W / 2, 90, COLOR_NEON_CYAN);
    fill_rect(WINDOW_W / 2 - 80, 108, 160, 1, COLOR_NEON_CYAN);
    
    if (server_list.count == 0) {
        
        draw_neon_rect(WINDOW_W / 2 - 150, 300, 300, 120, COLOR_PANEL, COLOR_NEON_CYAN, 4);
        draw_text(font_S, "Aucun serveur", WINDOW_W / 2, 340, COLOR_GREY);
        draw_text(font_S, "disponible", WINDOW_W / 2, 370, COLOR_GREY);
    } else {
        
        for (j = 4; j > 0; j--) {
            float alpha = (float)(4 - j + 1) / 5.0f * 0.1f;
            Uint32 glow_color = darken_color(COLOR_NEON_CYAN, alpha);
            fill_rect(20 - j, 120 - j, WINDOW_W - 40 + j * 2, 520 + j * 2, glow_color);
        }
        fill_rect(20, 120, WINDOW_W - 40, 520, COLOR_PANEL);
        fill_rect(20, 120, WINDOW_W - 40, 2, COLOR_NEON_CYAN);
        fill_rect(20, 638, WINDOW_W - 40, 2, COLOR_NEON_MAGENTA);
        
        for (i = 0; i < server_list.count && i < 5; i++) {
            int idx = i + browser_scroll_offset;
            if (idx >= server_list.count) break;
            
            ServerInfo *srv = &server_list.servers[idx];
            int y = y_start + i * entry_height;
            
            
            fill_rect(30, y, WINDOW_W - 60, entry_height - 10, darken_color(COLOR_BG_LIGHTER, 0.8f));
            
            
            Uint32 status_color = srv->game_started ? COLOR_NEON_ORANGE : COLOR_NEON_GREEN;
            fill_rect(30, y, 4, entry_height - 10, status_color);
            
            
            snprintf(line, sizeof(line), "Code: %s", srv->room_code);
            draw_text_left(font_S, line, 45, y + 8, COLOR_NEON_CYAN);
            
            snprintf(line, sizeof(line), "Hote: %s", srv->host_name);
            draw_text_left(font_S, line, 180, y + 8, COLOR_WHITE);
            
            
            snprintf(line, sizeof(line), "Joueurs: %d/4", srv->player_count);
            draw_text_left(font_S, line, 45, y + 35, COLOR_GREY);
            
            
            const char *mode_str = srv->game_mode == GAME_MODE_RUSH ? "Rush" : "Classic";
            Uint32 mode_color = srv->game_mode == GAME_MODE_RUSH ? COLOR_NEON_ORANGE : COLOR_NEON_CYAN;
            snprintf(line, sizeof(line), "Mode: %s", mode_str);
            draw_text_left(font_S, line, 180, y + 35, mode_color);
            
            
            if (srv->game_started) {
                draw_text_left(font_S, "En cours", 45, y + 62, COLOR_NEON_ORANGE);
                draw_button(WINDOW_W - 140, y + 8, 100, 35, "Rejoindre", COLOR_BTN_DISABLED, 1);
            } else {
                draw_text_left(font_S, "En attente", 45, y + 62, COLOR_NEON_GREEN);
                draw_button(WINDOW_W - 140, y + 8, 100, 35, "Rejoindre", COLOR_SUCCESS, 0);
            }
            draw_button(WINDOW_W - 140, y + 50, 100, 35, "Observer", COLOR_PURPLE, 0);
        }
        
        
        if (browser_scroll_offset > 0) {
            draw_text(font_S, "^ Defiler ^", WINDOW_W / 2, 128, COLOR_NEON_CYAN);
        }
        if (browser_scroll_offset + 5 < server_list.count) {
            draw_text(font_S, "v Defiler v", WINDOW_W / 2, 648, COLOR_NEON_MAGENTA);
        }
    }
    
    
    draw_button(WINDOW_W / 2 - 90, 680, 180, 45, "Actualiser", COLOR_BUTTON, 0);
    draw_button(WINDOW_W / 2 - 90, 735, 180, 45, "RETOUR", COLOR_DANGER, 0);
}

static void render_join_input(void) {
    
    draw_settings_gear(window_w - 50, 20);
    
    
    draw_text(font_L, "REJOINDRE", WINDOW_W / 2 + 2, 120 + 2, darken_color(COLOR_NEON_GREEN, 0.3f));
    draw_text(font_L, "REJOINDRE", WINDOW_W / 2, 120, COLOR_NEON_GREEN);
    
    
    fill_rect(WINDOW_W / 2 - 80, 160, 160, 2, COLOR_NEON_GREEN);
    
    
    {
        int box_x = 110;
        int box_y = 250;
        int box_w = 320;
        int box_h = 120;
        
        
        int i;
        for (i = 5; i > 0; i--) {
            float alpha = (float)(5 - i + 1) / 6.0f * 0.15f;
            Uint32 glow_color = darken_color(COLOR_NEON_GREEN, alpha);
            fill_rect(box_x - i, box_y - i, box_w + i * 2, box_h + i * 2, glow_color);
        }
        
        fill_rect(box_x, box_y, box_w, box_h, COLOR_PANEL);
        fill_rect(box_x, box_y, box_w, 2, COLOR_NEON_GREEN);
        fill_rect(box_x, box_y + box_h - 2, box_w, 2, COLOR_NEON_GREEN);
        fill_rect(box_x, box_y, 2, box_h, COLOR_NEON_GREEN);
        fill_rect(box_x + box_w - 2, box_y, 2, box_h, COLOR_NEON_GREEN);
    }
    
    draw_text(font_S, "CODE PARTIE :", WINDOW_W / 2, 275, COLOR_NEON_CYAN);
    draw_input_field(140, 300, 260, 50, input_buffer, 1);
    
    draw_button(WINDOW_W / 2 - 90, 420, 180, 50, "ENTRER", COLOR_SUCCESS, strlen(input_buffer) != 4);
    draw_button(WINDOW_W / 2 - 90, 500, 180, 50, "RETOUR", COLOR_DANGER, 0);
}

static void render_lobby(void) {
    int i;
    char title[64];
    char mode_str[32];
    
    
    draw_settings_gear(window_w - 50, 20);
    
    
    snprintf(title, sizeof(title), "CODE: %s", current_lobby.room_code);
    draw_text(font_L, title, WINDOW_W / 2 + 2, 50 + 2, darken_color(COLOR_NEON_GREEN, 0.3f));
    draw_text(font_L, title, WINDOW_W / 2, 50, COLOR_NEON_GREEN);
    
    
    if (current_lobby.is_public) {
        draw_text(font_S, "Partie Publique", WINDOW_W / 2, 90, COLOR_NEON_GREEN);
    } else {
        draw_text(font_S, "Partie Privee", WINDOW_W / 2, 90, COLOR_GREY);
    }
    
    
    {
        int panel_x = 50;
        int panel_y = 115;
        int panel_w = WINDOW_W - 100;
        int panel_h = 170;
        
        
        for (i = 4; i > 0; i--) {
            float alpha = (float)(4 - i + 1) / 5.0f * 0.1f;
            Uint32 glow_color = darken_color(COLOR_NEON_CYAN, alpha);
            fill_rect(panel_x - i, panel_y - i, panel_w + i * 2, panel_h + i * 2, glow_color);
        }
        fill_rect(panel_x, panel_y, panel_w, panel_h, COLOR_PANEL);
        fill_rect(panel_x, panel_y, panel_w, 2, COLOR_NEON_CYAN);
        fill_rect(panel_x, panel_y + panel_h - 2, panel_w, 2, COLOR_NEON_CYAN);
    }
    
    
    draw_text(font_S, "JOUEURS", WINDOW_W / 2, 135, COLOR_NEON_CYAN);
    fill_rect(WINDOW_W / 2 - 50, 152, 100, 1, COLOR_NEON_CYAN);
    
    
    for (i = 0; i < current_lobby.player_count; i++) {
        Uint32 color = COLOR_WHITE;
        int name_x = WINDOW_W / 2;
        int name_y = 175 + i * 32;
        
        
        if (strcmp(current_lobby.players[i], my_pseudo) == 0) {
            color = COLOR_NEON_CYAN;
            
            fill_rect(70, name_y - 12, WINDOW_W - 140, 24, darken_color(COLOR_NEON_CYAN, 0.15f));
        }
        
        
        if (current_lobby.is_spectator[i]) {
            draw_eye_icon(name_x - 100, name_y - 5, COLOR_NEON_PURPLE);
            draw_text(font_S, current_lobby.players[i], name_x, name_y, COLOR_NEON_PURPLE);
        } else {
            draw_text(font_S, current_lobby.players[i], name_x, name_y, color);
        }
        
        
        if (i == 0) {
            draw_text(font_S, "[HOST]", name_x + 100, name_y, COLOR_GOLD);
        }
    }
    
    
    if (current_lobby.is_host && !current_lobby.game_started) {
        int mode_y = 320;
        
        draw_text(font_S, "MODE DE JEU :", WINDOW_W / 2, mode_y, COLOR_WHITE);
        
        
        int classic_selected = (selected_game_mode == GAME_MODE_CLASSIC);
        Uint32 classic_color = classic_selected ? COLOR_CYAN : COLOR_BUTTON;
        draw_button(WINDOW_W / 2 - 160, mode_y + 25, 150, 35, "Classic", classic_color, 0);
        
        
        int rush_selected = (selected_game_mode == GAME_MODE_RUSH);
        Uint32 rush_color = rush_selected ? COLOR_CYAN : COLOR_BUTTON;
        draw_button(WINDOW_W / 2 + 10, mode_y + 25, 150, 35, "Rush", rush_color, 0);
        
        
        if (selected_game_mode == GAME_MODE_RUSH) {
            draw_text(font_S, "DUREE :", WINDOW_W / 2, mode_y + 80, COLOR_WHITE);
            
            
            draw_button(WINDOW_W / 2 - 200, mode_y + 100, 55, 30, "1m", 
                       selected_timer_minutes == 1 ? COLOR_CYAN : COLOR_BUTTON, 0);
            draw_button(WINDOW_W / 2 - 130, mode_y + 100, 55, 30, "2m", 
                       selected_timer_minutes == 2 ? COLOR_CYAN : COLOR_BUTTON, 0);
            draw_button(WINDOW_W / 2 - 60, mode_y + 100, 55, 30, "3m", 
                       selected_timer_minutes == 3 ? COLOR_CYAN : COLOR_BUTTON, 0);
            draw_button(WINDOW_W / 2 + 10, mode_y + 100, 55, 30, "5m", 
                       selected_timer_minutes == 5 ? COLOR_CYAN : COLOR_BUTTON, 0);
            draw_button(WINDOW_W / 2 + 80, mode_y + 100, 55, 30, "10m", 
                       selected_timer_minutes == 10 ? COLOR_CYAN : COLOR_BUTTON, 0);
        }
        
        
        int toggle_y = selected_game_mode == GAME_MODE_RUSH ? mode_y + 150 : mode_y + 80;
        const char *visibility_text = current_lobby.is_public ? "Rendre Privee" : "Rendre Publique";
        draw_button(WINDOW_W / 2 - 90, toggle_y, 180, 35, visibility_text, COLOR_PURPLE, 0);
        
        
        int start_y = selected_game_mode == GAME_MODE_RUSH ? 530 : 470;
        int need_players = current_lobby.player_count < 2 - current_lobby.spectator_count;
        
        if (need_players) {
            draw_text(font_S, "En attente d'autres joueurs...", WINDOW_W / 2, start_y, COLOR_GREY);
            start_y += 40;
        }
        
        draw_button(WINDOW_W / 2 - 150, start_y, 300, 55, "LANCER LA PARTIE", COLOR_SUCCESS, need_players);
    } else if (!current_lobby.is_host) {
        
        snprintf(mode_str, sizeof(mode_str), "Mode: %s", 
                 current_lobby.game_mode == GAME_MODE_RUSH ? "Rush" : "Classic");
        draw_text(font_S, mode_str, WINDOW_W / 2, 350, COLOR_GOLD);
        
        if (current_lobby.game_mode == GAME_MODE_RUSH) {
            snprintf(mode_str, sizeof(mode_str), "Duree: %d minutes", current_lobby.timer_minutes);
            draw_text(font_S, mode_str, WINDOW_W / 2, 380, COLOR_CYAN);
        }
        
        draw_text(font_S, "En attente du lancement...", WINDOW_W / 2, 450, COLOR_GREY);
    }
    
    draw_button(WINDOW_W / 2 - 90, 660, 180, 50, "QUITTER", COLOR_DANGER, 0);
    
    
    if (current_lobby.is_host && current_lobby.player_count > 1) {
        draw_text(font_S, "(Clic droit pour expulser)", WINDOW_W / 2, 720, COLOR_GREY);
    }
}

static void render_game_grid(void) {
    render_game_grid_ex(&game, 0, 0);
}

static void render_game_grid_ex(GameState *gs, int offset_x, int offset_y) {
    int i, j;
    int shake_x = 0, shake_y = 0;
    
    
    if (gs->effects.screen_shake > 0 && gs->effects.shake_time > 0) {
        shake_x = (rand() % (gs->effects.screen_shake * 2 + 1)) - gs->effects.screen_shake;
        shake_y = (rand() % (gs->effects.screen_shake * 2 + 1)) - gs->effects.screen_shake;
    }
    
    
    int base_x = grid_offset_x + offset_x + shake_x;
    int base_y = grid_offset_y + offset_y + shake_y;
    int grid_w = GRID_W * block_size;
    int grid_h = GRID_H * block_size;
    
    
    {
        int glow_layers = 6;
        for (i = glow_layers; i > 0; i--) {
            float alpha = (float)(glow_layers - i + 1) / (float)(glow_layers + 1) * 0.15f;
            Uint32 glow_color = darken_color(COLOR_NEON_CYAN, alpha);
            fill_rect(base_x - 8 - i, base_y - 8 - i, 
                      grid_w + 16 + i * 2, grid_h + 16 + i * 2, glow_color);
        }
    }
    
    
    fill_rect(base_x - 8, base_y - 8, grid_w + 16, grid_h + 16, COLOR_PANEL);
    
    
    fill_rect(base_x - 8, base_y - 8, grid_w + 16, 3, COLOR_NEON_CYAN);
    fill_rect(base_x - 8, base_y + grid_h + 5, grid_w + 16, 3, COLOR_NEON_MAGENTA);
    fill_rect(base_x - 8, base_y - 8, 3, grid_h + 16, COLOR_NEON_CYAN);
    fill_rect(base_x + grid_w + 5, base_y - 8, 3, grid_h + 16, COLOR_NEON_MAGENTA);
    
    
    fill_rect(base_x - 8, base_y - 8, 15, 3, 0xFFFFFF);
    fill_rect(base_x - 8, base_y - 8, 3, 15, 0xFFFFFF);
    fill_rect(base_x + grid_w - 7, base_y + grid_h + 5, 15, 3, 0xFFFFFF);
    fill_rect(base_x + grid_w + 5, base_y + grid_h - 7, 3, 15, 0xFFFFFF);
    
    
    fill_rect(base_x, base_y, grid_w, grid_h, COLOR_GRID_CELL);
    
    
    for (i = 0; i < GRID_H; i++) {
        for (j = 0; j < GRID_W; j++) {
            int cell_x = base_x + j * block_size;
            int cell_y = base_y + i * block_size;
            
            
            if ((i + j) % 2 == 0) {
                fill_rect(cell_x + 1, cell_y + 1, block_size - 2, block_size - 2, 
                          darken_color(COLOR_GRID_CELL, 0.85f));
            }
        }
    }
    
    
    for (i = 0; i <= GRID_H; i++) {
        
        fill_rect(base_x, base_y + i * block_size - 1, grid_w, 1, 
                  darken_color(COLOR_NEON_CYAN, 0.1f));
        fill_rect(base_x, base_y + i * block_size + 1, grid_w, 1, 
                  darken_color(COLOR_NEON_CYAN, 0.1f));
        
        fill_rect(base_x, base_y + i * block_size, grid_w, 1, COLOR_GRID);
    }
    for (j = 0; j <= GRID_W; j++) {
        
        fill_rect(base_x + j * block_size - 1, base_y, 1, grid_h, 
                  darken_color(COLOR_NEON_MAGENTA, 0.1f));
        fill_rect(base_x + j * block_size + 1, base_y, 1, grid_h, 
                  darken_color(COLOR_NEON_MAGENTA, 0.1f));
        
        fill_rect(base_x + j * block_size, base_y, 1, grid_h, COLOR_GRID);
    }
    
    
    for (i = 0; i < GRID_H; i++) {
        for (j = 0; j < GRID_W; j++) {
            if (gs->grid[i][j] != 0) {
                draw_styled_block(base_x + j * block_size + 2,
                                  base_y + i * block_size + 2,
                                  block_size - 4, gs->grid[i][j]);
            }
        }
    }
    
    
    render_effects(&gs->effects, shake_x + offset_x, shake_y + offset_y);
}

static void render_mini_grid(int grid[GRID_H][GRID_W], int x, int y, int block_size, const char *label, int score, int is_selected) {
    int i, j;
    int grid_w = GRID_W * block_size;
    int grid_h = GRID_H * block_size;
    char score_str[32];
    
    
    if (is_selected) {
        fill_rect(x - 4, y - 25, grid_w + 8, grid_h + 55, COLOR_CYAN);
    }
    
    
    fill_rect(x - 2, y - 2, grid_w + 4, grid_h + 4, 0x151520);
    
    
    fill_rect(x, y, grid_w, grid_h, 0x202030);
    
    
    for (i = 0; i < GRID_H; i++) {
        for (j = 0; j < GRID_W; j++) {
            if (grid[i][j] != 0) {
                fill_rect(x + j * block_size, y + i * block_size,
                          block_size - 1, block_size - 1, grid[i][j]);
            }
        }
    }
    
    
    if (font_XS) {
        draw_text(font_XS, label, x + grid_w / 2, y - 12, COLOR_WHITE);
    }
    
    
    snprintf(score_str, sizeof(score_str), "%d pts", score);
    if (font_XS) {
        draw_text(font_XS, score_str, x + grid_w / 2, y + grid_h + 12, COLOR_GOLD);
    }
}

static void render_pieces(int greyed) {
    int i, j, k;
    int slot_x, slot_y;
    int piece_block_size = 28;  
    int slot_w, slot_h;
    
    if (layout_horizontal) {
        
        slot_w = (window_w / 2 - 80);
        slot_h = (window_h - 280) / 3;
        
        for (i = 0; i < 3; i++) {
            if (!game.pieces_available[i]) continue;
            if (selected_piece_idx == i) continue;
            
            Piece *p = &game.current_pieces[i];
            slot_x = window_w / 2 + 40;
            slot_y = 200 + i * slot_h;
            
            
            int piece_px = slot_x + (slot_w - p->w * piece_block_size) / 2;
            int piece_py = slot_y + (slot_h - p->h * piece_block_size) / 2;
            
            for (j = 0; j < p->h; j++) {
                for (k = 0; k < p->w; k++) {
                    if (p->data[j][k]) {
                        Uint32 color = p->color;
                        if (greyed) {
                            int r = ((color >> 16) & 0xFF) / 3;
                            int g = ((color >> 8) & 0xFF) / 3;
                            int b = (color & 0xFF) / 3;
                            color = (r << 16) | (g << 8) | b;
                        }
                        draw_styled_block(piece_px + k * piece_block_size, piece_py + j * piece_block_size, piece_block_size - 2, color);
                    }
                }
            }
        }
    } else {
        
        slot_w = (window_w - 60) / 3;
        slot_h = window_h - piece_area_y - 20;
        
        for (i = 0; i < 3; i++) {
            if (!game.pieces_available[i]) continue;
            if (selected_piece_idx == i) continue;
            
            Piece *p = &game.current_pieces[i];
            slot_x = 30 + i * slot_w;
            slot_y = piece_area_y;
            
            
            int piece_px = slot_x + (slot_w - p->w * piece_block_size) / 2;
            int piece_py = slot_y + (slot_h - p->h * piece_block_size) / 2;
            
            for (j = 0; j < p->h; j++) {
                for (k = 0; k < p->w; k++) {
                    if (p->data[j][k]) {
                        Uint32 color = p->color;
                        if (greyed) {
                            int r = ((color >> 16) & 0xFF) / 3;
                            int g = ((color >> 8) & 0xFF) / 3;
                            int b = (color & 0xFF) / 3;
                            color = (r << 16) | (g << 8) | b;
                        }
                        draw_styled_block(piece_px + k * piece_block_size, piece_py + j * piece_block_size, piece_block_size - 2, color);
                    }
                }
            }
        }
    }
}

static void render_dragged_piece(void) {
    int j, k;
    
    if (selected_piece_idx < 0 || selected_piece_idx >= 3) return;
    
    Piece *p = &game.current_pieces[selected_piece_idx];
    
    
    int base_x = mouse_x - (p->w * block_size) / 2;
    int base_y = mouse_y - (p->h * block_size) / 2;
    
    for (j = 0; j < p->h; j++) {
        for (k = 0; k < p->w; k++) {
            if (p->data[j][k]) {
                draw_styled_block(base_x + k * block_size, 
                                  base_y + j * block_size, 
                                  block_size - 2, p->color);
            }
        }
    }
}

static void render_solo(void) {
    char score_text[64];
    int i;
    
    
    draw_settings_gear(window_w - 50, 20);
    
    
    update_effects(&game.effects, delta_time);
    
    
    if (game.score > solo_high_score) {
        solo_high_score = game.score;
        save_game_data();
    }
    
    
    render_game_grid_ex(&game, 0, 0);
    
    if (layout_horizontal) {
        
        
        
        int right_x = window_w / 2 + 30;
        
        draw_text(font_S, "SCORE", right_x + 80, 40, COLOR_WHITE);
        snprintf(score_text, sizeof(score_text), "%d", game.score);
        draw_text(font_L, score_text, right_x + 80 + 2, 80 + 2, darken_color(COLOR_NEON_GREEN, 0.4f));
        draw_text(font_L, score_text, right_x + 80, 80, COLOR_NEON_GREEN);
        
        
        snprintf(score_text, sizeof(score_text), "BEST: %d", solo_high_score);
        draw_text(font_S, score_text, right_x + 80, 130, COLOR_GOLD);
        
        
        {
            int panel_x = right_x;
            int panel_y = 180;
            int panel_w = window_w / 2 - 50;
            int panel_h = window_h - 220;
            
            
            for (i = 3; i > 0; i--) {
                float alpha = (float)(3 - i + 1) / 4.0f * 0.1f;
                Uint32 glow_color = darken_color(COLOR_NEON_MAGENTA, alpha);
                fill_rect(panel_x - i, panel_y - i, panel_w + i * 2, panel_h + i * 2, glow_color);
            }
            fill_rect(panel_x, panel_y, panel_w, panel_h, COLOR_PANEL);
            fill_rect(panel_x, panel_y, panel_w, 2, COLOR_NEON_MAGENTA);
        }
    } else {
        
        
        
        snprintf(score_text, sizeof(score_text), "%d", game.score);
        draw_text(font_L, score_text, window_w / 2 + 2, 32, darken_color(COLOR_NEON_GREEN, 0.4f));
        draw_text(font_L, score_text, window_w / 2, 30, COLOR_NEON_GREEN);
        
        
        snprintf(score_text, sizeof(score_text), "BEST: %d", solo_high_score);
        draw_text(font_XS ? font_XS : font_S, score_text, window_w - 60, 25, COLOR_GOLD);
        
        
        {
            int panel_x = 20;
            int panel_y = piece_area_y - 15;
            int panel_w = window_w - 40;
            int panel_h = window_h - piece_area_y + 5;
            
            
            for (i = 3; i > 0; i--) {
                float alpha = (float)(3 - i + 1) / 4.0f * 0.1f;
                Uint32 glow_color = darken_color(COLOR_NEON_MAGENTA, alpha);
                fill_rect(panel_x - i, panel_y - i, panel_w + i * 2, panel_h + i * 2, glow_color);
            }
            fill_rect(panel_x, panel_y, panel_w, panel_h, COLOR_PANEL);
            fill_rect(panel_x, panel_y, panel_w, 2, COLOR_NEON_MAGENTA);
        }
    }
    
    
    render_pieces(0);
    
    
    render_dragged_piece();
    
    
    if (game.game_over) {
        
        fill_rect(0, 0, window_w, window_h, darken_color(COLOR_BG, 0.6f));
        
        
        int panel_x = 40;
        int panel_y = window_h / 2 - 100;
        int panel_w = window_w - 80;
        int panel_h = 200;
        
        
        for (i = 8; i > 0; i--) {
            float alpha = (float)(8 - i + 1) / 9.0f * 0.2f;
            Uint32 glow_color = darken_color(COLOR_NEON_RED, alpha);
            fill_rect(panel_x - i, panel_y - i, panel_w + i * 2, panel_h + i * 2, glow_color);
        }
        
        fill_rect(panel_x, panel_y, panel_w, panel_h, COLOR_PANEL);
        fill_rect(panel_x, panel_y, panel_w, 3, COLOR_NEON_RED);
        fill_rect(panel_x, panel_y + panel_h - 3, panel_w, 3, COLOR_NEON_RED);
        fill_rect(panel_x, panel_y, 3, panel_h, COLOR_NEON_RED);
        fill_rect(panel_x + panel_w - 3, panel_y, 3, panel_h, COLOR_NEON_RED);
        
        
        draw_text(font_L, "GAME OVER", window_w / 2 + 2, window_h / 2 - 50 + 2, darken_color(COLOR_NEON_RED, 0.4f));
        draw_text(font_L, "GAME OVER", window_w / 2, window_h / 2 - 50, COLOR_NEON_RED);
        
        
        snprintf(score_text, sizeof(score_text), "Score Final: %d", game.score);
        draw_text(font_S, score_text, window_w / 2, window_h / 2, COLOR_WHITE);
        
        
        if (game.score >= solo_high_score && solo_high_score > 0) {
            draw_text(font_S, "NOUVEAU RECORD!", window_w / 2, window_h / 2 + 35, COLOR_GOLD);
        }
        
        draw_text(font_S, "Cliquez pour continuer", window_w / 2, window_h / 2 + 70, COLOR_NEON_CYAN);
    }
}

static void render_multi_game(void) {
    char score_text[32];
    char turn_text[64];
    int i;
    
    
    draw_settings_gear(window_w - 50, 20);
    
    
    update_effects(&game.effects, delta_time);
    
    
    if (current_lobby.game_mode == GAME_MODE_RUSH) {
        render_rush_game();
        return;
    }
    
    
    
    
    if (is_spectator) {
        draw_text(font_S, "MODE SPECTATEUR", WINDOW_W / 2, 25, COLOR_NEON_PURPLE);
        snprintf(turn_text, sizeof(turn_text), "Tour de %s", current_turn_pseudo);
        draw_text(font_S, turn_text, WINDOW_W / 2, 50, COLOR_GREY);
    } else if (is_my_turn()) {
        
        float pulse = (sinf(glow_pulse * 4.0f) + 1.0f) * 0.5f;
        Uint32 turn_color = blend_colors(COLOR_NEON_GREEN, COLOR_NEON_CYAN, pulse);
        draw_text(font_L, "A TOI!", WINDOW_W / 2 + 2, 35 + 2, darken_color(turn_color, 0.4f));
        draw_text(font_L, "A TOI!", WINDOW_W / 2, 35, turn_color);
    } else {
        snprintf(turn_text, sizeof(turn_text), "Tour de %s...", current_turn_pseudo);
        draw_text(font_S, turn_text, WINDOW_W / 2, 35, COLOR_GREY);
    }
    
    
    snprintf(score_text, sizeof(score_text), "Score: %d", game.score);
    draw_text(font_S, score_text, WINDOW_W / 2, 65, COLOR_NEON_GREEN);
    
    
    render_game_grid_ex(&game, 0, 0);
    
    
    {
        int panel_x = 20;
        int panel_y = piece_area_y - 15;
        int panel_w = WINDOW_W - 40;
        int panel_h = PIECE_SLOT_H + 30;
        
        for (i = 3; i > 0; i--) {
            float alpha = (float)(3 - i + 1) / 4.0f * 0.1f;
            Uint32 glow_color = darken_color(COLOR_NEON_MAGENTA, alpha);
            fill_rect(panel_x - i, panel_y - i, panel_w + i * 2, panel_h + i * 2, glow_color);
        }
        fill_rect(panel_x, panel_y, panel_w, panel_h, COLOR_PANEL);
        fill_rect(panel_x, panel_y, panel_w, 2, COLOR_NEON_MAGENTA);
    }
    
    
    render_pieces(!is_my_turn() || is_spectator);
    
    
    if (is_my_turn() && !is_spectator) {
        render_dragged_piece();
    }
}

static void render_rush_game(void) {
    char timer_text[32];
    char score_text[64];
    int i;
    
    
    int mins = rush_time_remaining / 60;
    int secs = rush_time_remaining % 60;
    snprintf(timer_text, sizeof(timer_text), "%d:%02d", mins, secs);
    
    
    Uint32 timer_color = COLOR_NEON_CYAN;
    if (rush_time_remaining <= 30) {
        
        float pulse = (sinf(glow_pulse * 6.0f) + 1.0f) * 0.5f;
        timer_color = blend_colors(COLOR_NEON_RED, 0xFFFFFF, pulse * 0.3f);
    } else if (rush_time_remaining <= 60) {
        timer_color = COLOR_NEON_ORANGE;
    }
    
    
    draw_text(font_L, timer_text, WINDOW_W / 2 + 2, 35 + 2, darken_color(timer_color, 0.4f));
    draw_text(font_L, timer_text, WINDOW_W / 2, 35, timer_color);
    
    if (is_spectator) {
        
        draw_text(font_L, "SPECTATEUR", WINDOW_W / 2, 70, COLOR_PURPLE);
        
        
        int mini_block = 15;
        int spacing = 20;
        int total_width = rush_player_count * (GRID_W * mini_block) + (rush_player_count - 1) * spacing;
        int start_x = (WINDOW_W - total_width) / 2;
        
        for (i = 0; i < rush_player_count; i++) {
            int grid_x = start_x + i * (GRID_W * mini_block + spacing);
            int is_selected = (spectate_view_idx == i);
            render_mini_grid(rush_states[i].grid, grid_x, 100, mini_block, 
                           rush_states[i].pseudo, rush_states[i].score, is_selected);
        }
        
        
        if (spectate_view_idx >= 0 && spectate_view_idx < rush_player_count) {
            
            memcpy(game.grid, rush_states[spectate_view_idx].grid, sizeof(game.grid));
            game.score = rush_states[spectate_view_idx].score;
            
            draw_text(font_S, rush_states[spectate_view_idx].pseudo, WINDOW_W / 2, 310, COLOR_CYAN);
            snprintf(score_text, sizeof(score_text), "Score: %d", rush_states[spectate_view_idx].score);
            draw_text(font_S, score_text, WINDOW_W / 2, 335, COLOR_GOLD);
            
            
            render_game_grid_ex(&game, 0, 280);
        }
        
        
        draw_text(font_S, "< > pour changer de joueur", WINDOW_W / 2, WINDOW_H - 30, COLOR_GREY);
    } else {
        
        
        
        render_game_grid_ex(&game, 0, 0);
        
        
        snprintf(score_text, sizeof(score_text), "%d", game.score);
        draw_text(font_S, "Score", 35, 100, COLOR_WHITE);
        draw_text(font_L, score_text, 35, 135, COLOR_GOLD);
        
        
        draw_text(font_XS ? font_XS : font_S, "Classement", 35, 180, COLOR_WHITE);
        for (i = 0; i < rush_player_count && i < 4; i++) {
            char entry[48];
            int entry_y = 205 + i * 40;  
            Uint32 entry_color = (strcmp(rush_states[i].pseudo, my_pseudo) == 0) ? COLOR_CYAN : COLOR_WHITE;
            
            
            snprintf(entry, sizeof(entry), "%d. %s", i + 1, rush_states[i].pseudo);
            draw_text_left(font_XS ? font_XS : font_S, entry, 10, entry_y, entry_color);
            
            
            snprintf(entry, sizeof(entry), "   %d pts", rush_states[i].score);
            draw_text_left(font_XS ? font_XS : font_S, entry, 10, entry_y + 16, entry_color);
        }
        
        
        render_pieces(0);
        
        
        render_dragged_piece();
    }
    
    
    if (rush_time_remaining <= 0 && !is_spectator) {
        fill_rect(0, WINDOW_H / 2 - 80, WINDOW_W, 160, 0x000000DD);
        draw_text(font_L, "TEMPS ECOULE !", WINDOW_W / 2, WINDOW_H / 2 - 40, COLOR_ORANGE);
        
        
        int max_score = -1;
        int winner_idx = 0;
        for (i = 0; i < rush_player_count; i++) {
            if (rush_states[i].score > max_score) {
                max_score = rush_states[i].score;
                winner_idx = i;
            }
        }
        
        char winner_text[64];
        snprintf(winner_text, sizeof(winner_text), "Gagnant: %s (%d pts)", 
                 rush_states[winner_idx].pseudo, max_score);
        draw_text(font_S, winner_text, WINDOW_W / 2, WINDOW_H / 2 + 10, COLOR_GOLD);
        draw_text(font_S, "Cliquez pour quitter", WINDOW_W / 2, WINDOW_H / 2 + 50, COLOR_WHITE);
    }
}

static void render_spectate(void) {
    
    draw_settings_gear(window_w - 50, 20);
    
    if (current_lobby.game_mode == GAME_MODE_RUSH) {
        
        render_rush_game();
    } else {
        
        draw_text(font_L, "SPECTATEUR", WINDOW_W / 2 + 2, 35 + 2, darken_color(COLOR_NEON_PURPLE, 0.3f));
        draw_text(font_L, "SPECTATEUR", WINDOW_W / 2, 35, COLOR_NEON_PURPLE);
        
        
        char info[64];
        snprintf(info, sizeof(info), "Tour de %s", current_turn_pseudo);
        draw_text(font_S, info, WINDOW_W / 2, 70, COLOR_GREY);
        
        render_game_grid_ex(&game, 0, 10);
        
        
        char score_text[32];
        snprintf(score_text, sizeof(score_text), "Score: %d", game.score);
        draw_text(font_S, score_text, WINDOW_W / 2, WINDOW_H - 80, COLOR_NEON_GREEN);
    }
    
    draw_button(WINDOW_W / 2 - 90, WINDOW_H - 50, 180, 40, "QUITTER", COLOR_DANGER, 0);
}

static void handle_menu_click(void) {
    int center_x = window_w / 2;
    int btn_y = 250;
    
    if (point_in_rect(mouse_x, mouse_y, window_w - 50, 20, 28, 28)) {
        play_click();
        show_settings_overlay = 1;
        settings_tab = 0;
        return;
    }
    
    if (has_saved_game) {
        if (point_in_rect(mouse_x, mouse_y, center_x - 150, btn_y, 300, 55)) {
            play_click();
            if (load_current_game()) {
                current_state = ST_SOLO;
            } else {
                delete_saved_game();
                init_game(&game);
                current_state = ST_SOLO;
            }
            return;
        }
        btn_y += 70;
    }
    
    if (point_in_rect(mouse_x, mouse_y, center_x - 150, btn_y, 300, 55)) {
        play_click();
        delete_saved_game();
        init_game(&game);
        current_state = ST_SOLO;
        return;
    }
    btn_y += 70;
    
    if (point_in_rect(mouse_x, mouse_y, center_x - 150, btn_y, 300, 55)) {
        play_click();
        if (net_connect(online_ip, online_port)) {
            memset(input_buffer, 0, sizeof(input_buffer));
            current_state = ST_LOGIN;
        } else {
            strcpy(popup_msg, "Impossible de se connecter au serveur!");
        }
        return;
    }
    btn_y += 70;
    
    if (point_in_rect(mouse_x, mouse_y, center_x - 150, btn_y, 300, 55)) {
        play_click();
        show_settings_overlay = 1;
        settings_tab = 0;
        strcpy(edit_ip, online_ip);
        snprintf(edit_port, sizeof(edit_port), "%d", online_port);
        active_input = 0;
    }
}

static void handle_options_click(void) {
    
    if (point_in_rect(mouse_x, mouse_y, 70, 210, 400, 50)) {
        play_click();
        active_input = 1;
    }
    
    else if (point_in_rect(mouse_x, mouse_y, 70, 330, 400, 50)) {
        play_click();
        active_input = 2;
    }
    
    else if (point_in_rect(mouse_x, mouse_y, 70, 480, 180, 50)) {
        play_click();
        test_result = perform_connection_test() ? 1 : -1;
    }
    
    else if (point_in_rect(mouse_x, mouse_y, 290, 480, 180, 50) && test_result == 1) {
        play_click();
        strcpy(online_ip, edit_ip);
        online_port = atoi(edit_port);
        strcpy(popup_msg, "Configuration sauvegardee!");
    }
    
    else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 580, 180, 50)) {
        play_click();
        current_state = ST_MENU;
    }
}

static void handle_login_click(void) {
    Packet pkt;
    
    
    if (point_in_rect(mouse_x, mouse_y, window_w - 50, 20, 28, 28)) {
        play_click();
        show_settings_overlay = 1;
        settings_tab = 0;
        return;
    }
    
    
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 420, 180, 50) && strlen(input_buffer) > 0) {
        play_click();
        strcpy(my_pseudo, input_buffer);
        
        
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_LOGIN;
        strcpy(pkt.text, my_pseudo);
        net_send(&pkt);
        
        
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_LEADERBOARD_REQ;
        net_send(&pkt);
        
        memset(input_buffer, 0, sizeof(input_buffer));
        current_state = ST_MULTI_CHOICE;
    }
    
    else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 500, 180, 50)) {
        play_click();
        net_close();
        current_state = ST_MENU;
    }
}

static void handle_multi_choice_click(void) {
    Packet pkt;
    
    
    if (point_in_rect(mouse_x, mouse_y, window_w - 50, 20, 28, 28)) {
        play_click();
        show_settings_overlay = 1;
        settings_tab = 0;
        return;
    }
    
    
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 150, 420, 300, 55)) {
        play_click();
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_CREATE_ROOM;
        net_send(&pkt);
        
        selected_game_mode = GAME_MODE_CLASSIC;
        selected_timer_minutes = 3;
    }
    
    else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 150, 495, 300, 55)) {
        play_click();
        memset(input_buffer, 0, sizeof(input_buffer));
        current_state = ST_JOIN_INPUT;
    }
    
    else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 150, 570, 300, 55)) {
        play_click();
        
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_SERVER_LIST_REQ;
        net_send(&pkt);
        browser_scroll_offset = 0;
        current_state = ST_SERVER_BROWSER;
    }
    
    else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 660, 180, 50)) {
        play_click();
        net_close();
        current_state = ST_MENU;
    }
}

static void handle_join_input_click(void) {
    Packet pkt;
    
    
    if (point_in_rect(mouse_x, mouse_y, window_w - 50, 20, 28, 28)) {
        play_click();
        show_settings_overlay = 1;
        settings_tab = 0;
        return;
    }
    
    
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 420, 180, 50) && strlen(input_buffer) == 4) {
        play_click();
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_JOIN_ROOM;
        strcpy(pkt.text, input_buffer);
        net_send(&pkt);
    }
    
    else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 500, 180, 50)) {
        play_click();
        current_state = ST_MULTI_CHOICE;
    }
}

static void handle_lobby_click(void) {
    Packet pkt;
    int mode_y = 320;
    int toggle_y = selected_game_mode == GAME_MODE_RUSH ? mode_y + 150 : mode_y + 80;
    int need_players = current_lobby.player_count < 2 - current_lobby.spectator_count;
    int start_y = selected_game_mode == GAME_MODE_RUSH ? 530 : 470;
    if (need_players) start_y += 40;
    
    
    if (point_in_rect(mouse_x, mouse_y, window_w - 50, 20, 28, 28)) {
        play_click();
        show_settings_overlay = 1;
        settings_tab = 0;
        return;
    }
    
    if (current_lobby.is_host && !current_lobby.game_started) {
        
        if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 160, mode_y + 25, 150, 35)) {
            play_click();
            selected_game_mode = GAME_MODE_CLASSIC;
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_SET_GAME_MODE;
            pkt.game_mode = GAME_MODE_CLASSIC;
            net_send(&pkt);
        }
        
        else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 + 10, mode_y + 25, 150, 35)) {
            play_click();
            selected_game_mode = GAME_MODE_RUSH;
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_SET_GAME_MODE;
            pkt.game_mode = GAME_MODE_RUSH;
            pkt.timer_value = selected_timer_minutes * 60;
            net_send(&pkt);
        }
        
        
        if (selected_game_mode == GAME_MODE_RUSH) {
            int timer_buttons[] = {1, 2, 3, 5, 10};
            int timer_x[] = {WINDOW_W / 2 - 200, WINDOW_W / 2 - 130, WINDOW_W / 2 - 60, 
                            WINDOW_W / 2 + 10, WINDOW_W / 2 + 80};
            int i;
            for (i = 0; i < 5; i++) {
                if (point_in_rect(mouse_x, mouse_y, timer_x[i], mode_y + 100, 55, 30)) {
                    play_click();
                    selected_timer_minutes = timer_buttons[i];
                    memset(&pkt, 0, sizeof(pkt));
                    pkt.type = MSG_SET_TIMER;
                    pkt.timer_value = selected_timer_minutes * 60;
                    net_send(&pkt);
                    break;
                }
            }
        }
        
        
        if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, toggle_y, 180, 35)) {
            play_click();
            
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_SET_GAME_MODE;  
            pkt.game_mode = selected_game_mode;
            pkt.lobby.is_public = !current_lobby.is_public;
            net_send(&pkt);
        }
        
        
        if (!need_players && point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 150, start_y, 300, 55)) {
            play_click();
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_START_GAME;
            pkt.game_mode = selected_game_mode;
            pkt.timer_value = selected_timer_minutes * 60;
            net_send(&pkt);
        }
    }
    
    
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 660, 180, 50)) {
        play_click();
        net_close();
        current_state = ST_MENU;
        is_spectator = 0;
    }
}

static void handle_server_browser_click(void) {
    Packet pkt;
    int i;
    int y_start = 160;
    int entry_height = 110;
    
    
    if (point_in_rect(mouse_x, mouse_y, window_w - 50, 20, 28, 28)) {
        play_click();
        show_settings_overlay = 1;
        settings_tab = 0;
        return;
    }
    
    
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 680, 180, 45)) {
        play_click();
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_SERVER_LIST_REQ;
        net_send(&pkt);
        return;
    }
    
    
    for (i = 0; i < server_list.count && i < 5; i++) {
        int idx = i + browser_scroll_offset;
        if (idx >= server_list.count) break;
        
        ServerInfo *srv = &server_list.servers[idx];
        int y = y_start + i * entry_height;
        
        
        if (!srv->game_started && point_in_rect(mouse_x, mouse_y, WINDOW_W - 140, y + 10, 100, 35)) {
            play_click();
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_JOIN_ROOM;
            strcpy(pkt.text, srv->room_code);
            is_spectator = 0;
            net_send(&pkt);
            return;
        }
        
        
        if (point_in_rect(mouse_x, mouse_y, WINDOW_W - 140, y + 52, 100, 35)) {
            play_click();
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_JOIN_SPECTATE;
            strcpy(pkt.text, srv->room_code);
            is_spectator = 1;
            spectate_view_idx = 0;
            net_send(&pkt);
            return;
        }
    }
    
    
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 735, 180, 45)) {
        play_click();
        current_state = ST_MULTI_CHOICE;
    }
}

static void handle_server_browser_scroll(int direction) {
    if (direction < 0 && browser_scroll_offset > 0) {
        browser_scroll_offset--;
    } else if (direction > 0 && browser_scroll_offset + 5 < server_list.count) {
        browser_scroll_offset++;
    }
}

static void handle_lobby_right_click(void) {
    int i;
    Packet pkt;
    
    if (!current_lobby.is_host) return;
    
    
    for (i = 1; i < current_lobby.player_count; i++) {
        if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 100, 190 + i * 40, 200, 30)) {
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_KICK_PLAYER;
            strcpy(pkt.text, current_lobby.players[i]);
            net_send(&pkt);
            break;
        }
    }
}

static void handle_game_click(int is_multi) {
    Packet pkt;
    int grid_x, grid_y;
    int all_placed, i;
    
    
    if (is_multi && current_lobby.game_mode == GAME_MODE_CLASSIC && !is_my_turn()) return;
    
    if (selected_piece_idx >= 0) {
        
        Piece *p = &game.current_pieces[selected_piece_idx];
        
        
        grid_x = (mouse_x - grid_offset_x - (p->w * block_size) / 2 + block_size / 2) / block_size;
        grid_y = (mouse_y - grid_offset_y - (p->h * block_size) / 2 + block_size / 2) / block_size;
        
        if (can_place(&game, grid_y, grid_x, p)) {
            place_piece_logic(&game, grid_y, grid_x, p);
            game.pieces_available[selected_piece_idx] = 0;
            
            
            if (game.num_cleared_rows > 0 || game.num_cleared_cols > 0) {
                play_clear();  
            } else {
                play_place();  
            }
            
            
            all_placed = 1;
            for (i = 0; i < 3; i++) {
                if (game.pieces_available[i]) {
                    all_placed = 0;
                    break;
                }
            }
            
            if (all_placed) {
                generate_pieces(&game);
            }
            
            
            if (is_multi) {
                memset(&pkt, 0, sizeof(pkt));
                pkt.type = MSG_PLACE_PIECE;
                memcpy(pkt.grid_data, game.grid, sizeof(game.grid));
                pkt.score = game.score;
                net_send(&pkt);
            }
            
            
            if (!check_valid_moves_exist(&game)) {
                game.game_over = 1;
                play_gameover();  
            }
        }
        
        selected_piece_idx = -1;
    }
}

static void handle_game_mousedown(int is_multi) {
    int i;
    
    
    if (is_multi && current_lobby.game_mode == GAME_MODE_CLASSIC && !is_my_turn()) return;
    
    
    if (layout_horizontal) {
        
        int slot_w = (window_w / 2 - 80);
        int slot_h = (window_h - 280) / 3;
        int slot_x = window_w / 2 + 40;
        
        if (mouse_x >= slot_x) {
            for (i = 0; i < 3; i++) {
                if (!game.pieces_available[i]) continue;
                
                int slot_y = 200 + i * slot_h;
                if (mouse_y >= slot_y && mouse_y < slot_y + slot_h) {
                    selected_piece_idx = i;
                    break;
                }
            }
        }
    } else {
        
        if (mouse_y >= piece_area_y) {
            int slot_w = (window_w - 60) / 3;
            
            for (i = 0; i < 3; i++) {
                if (!game.pieces_available[i]) continue;
                
                int slot_x = 30 + i * slot_w;
                if (mouse_x >= slot_x && mouse_x < slot_x + slot_w) {
                    selected_piece_idx = i;
                    break;
                }
            }
        }
    }
}

static void process_network(void) {
    Packet pkt;
    int i;
    
    while (net_receive(&pkt)) {
        switch (pkt.type) {
            case MSG_LEADERBOARD_REP:
                leaderboard = pkt.lb;
                break;
            
            case MSG_ROOM_UPDATE:
                current_lobby = pkt.lobby;
                
                selected_game_mode = current_lobby.game_mode;
                selected_timer_minutes = current_lobby.timer_minutes;
                if (current_state != ST_MULTI_GAME && current_state != ST_SPECTATE) {
                    current_state = ST_LOBBY;
                }
                break;
            
            case MSG_KICKED:
                strcpy(popup_msg, "Vous avez ete expulse!");
                net_close();
                current_state = ST_MENU;
                is_spectator = 0;
                break;
            
            case MSG_START_GAME:
                
                current_lobby.game_mode = pkt.game_mode;
                
                if (pkt.game_mode == GAME_MODE_RUSH) {
                    
                    memset(game.grid, 0, sizeof(game.grid));
                    game.score = 0;
                    generate_pieces(&game);
                    init_effects(&game.effects);
                    
                    rush_time_remaining = pkt.time_remaining;
                    last_time_update = SDL_GetTicks();
                    rush_player_count = 0;
                    
                    current_state = is_spectator ? ST_SPECTATE : ST_MULTI_GAME;
                } else {
                    
                    memcpy(game.grid, pkt.grid_data, sizeof(game.grid));
                    strcpy(current_turn_pseudo, pkt.turn_pseudo);
                    game.score = 0;
                    generate_pieces(&game);
                    init_effects(&game.effects);
                    
                    current_state = is_spectator ? ST_SPECTATE : ST_MULTI_GAME;
                }
                break;
            
            case MSG_UPDATE_GRID:
                
                memcpy(game.grid, pkt.grid_data, sizeof(game.grid));
                strcpy(current_turn_pseudo, pkt.turn_pseudo);
                break;
            
            case MSG_RUSH_UPDATE:
                
                rush_time_remaining = pkt.time_remaining;
                rush_player_count = pkt.rush_player_count;
                
                for (i = 0; i < pkt.rush_player_count && i < 4; i++) {
                    rush_states[i] = pkt.rush_states[i];
                }
                
                last_time_update = SDL_GetTicks();
                break;
            
            case MSG_TIME_SYNC:
                rush_time_remaining = pkt.time_remaining;
                last_time_update = SDL_GetTicks();
                break;
            
            case MSG_GAME_END:
                
                rush_time_remaining = 0;
                if (current_lobby.game_mode == GAME_MODE_RUSH) {
                    
                }
                break;
            
            case MSG_SERVER_LIST_REP:
                
                server_list = pkt.server_list;
                break;
            
            case MSG_GAME_CANCELLED:
                strcpy(popup_msg, pkt.text);
                net_close();
                current_state = ST_MENU;
                is_spectator = 0;
                break;
            
            case MSG_ERROR:
                strcpy(popup_msg, pkt.text);
                break;
            
            default:
                break;
        }
    }
}

int main(int argc, char *argv[]) {
    SDL_Event e;
    int running = 1;
    
    (void)argc;
    (void)argv;
    
    
    srand((unsigned int)time(NULL));
    
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL init failed: %s\n", SDL_GetError());
        return 1;
    }
    
    
    if (TTF_Init() < 0) {
        printf("TTF init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    
    
    init_audio();
    
    load_game_data();
    check_saved_game_exists();
    
    update_music_volume();
    update_sfx_volume();
    
    SDL_EnableUNICODE(1);
    
    
    screen = SDL_SetVideoMode(WINDOW_W, WINDOW_H, 32, SDL_SWSURFACE | SDL_RESIZABLE);
    if (!screen) {
        printf("Video mode failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    
    window_w = WINDOW_W;
    window_h = WINDOW_H;
    recalculate_layout();
    
    SDL_WM_SetCaption("BlockBlast V4.0", NULL);
    
    
    {
        
        const char *font_paths[] = {
            "assets/orbitron.ttf",
            "../assets/orbitron.ttf",
            "assets/Orbitron.ttf",
            "../assets/Orbitron.ttf",
            "assets/font.ttf",
            "../assets/font.ttf",
            "font.ttf",
            NULL
        };
        int i;
        
        for (i = 0; font_paths[i] != NULL; i++) {
            font_L = TTF_OpenFont(font_paths[i], 40);
            if (font_L) {
                font_S = TTF_OpenFont(font_paths[i], 22);
                font_XS = TTF_OpenFont(font_paths[i], 14);
                if (font_S && font_XS) {
                    printf("Loaded font from: %s\n", font_paths[i]);
                    break;
                }
                TTF_CloseFont(font_L);
                font_L = NULL;
                if (font_S) { TTF_CloseFont(font_S); font_S = NULL; }
                if (font_XS) { TTF_CloseFont(font_XS); font_XS = NULL; }
            }
        }
    }
    
    if (!font_L || !font_S) {
        printf("Failed to load font: %s\n", TTF_GetError());
        printf("Tried: assets/font.ttf, ../assets/font.ttf, font.ttf\n");
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    
    init_game(&game);
    memset(&current_lobby, 0, sizeof(current_lobby));
    memset(&leaderboard, 0, sizeof(leaderboard));
    
    
    start_music();
    
    
    while (running) {
        
        process_network();
        
        
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    running = 0;
                    break;
                
                case SDL_VIDEORESIZE:
                    
                    window_w = e.resize.w;
                    window_h = e.resize.h;
                    if (window_w < 400) window_w = 400;
                    if (window_h < 400) window_h = 400;
                    screen = SDL_SetVideoMode(window_w, window_h, 32, SDL_SWSURFACE | SDL_RESIZABLE);
                    recalculate_layout();
                    break;
                
                case SDL_KEYDOWN:
                    if (e.key.keysym.unicode) {
                        handle_input_char(e.key.keysym.unicode);
                    }
                    
                    
                    if (e.key.keysym.sym == SDLK_p) {
                        show_settings_overlay = !show_settings_overlay;
                        play_click();
                    }
                    
                    
                    if (e.key.keysym.sym == SDLK_ESCAPE) {
                        if (show_settings_overlay) {
                            show_settings_overlay = 0;
                            play_click();
                        } else if (show_pause_menu) {
                            show_pause_menu = 0;
                            play_click();
                        } else if (current_state == ST_SOLO && !game.game_over) {
                            show_pause_menu = 1;
                            play_click();
                        } else if (current_state == ST_SOLO && game.game_over) {
                            delete_saved_game();
                            current_state = ST_MENU;
                            resume_music();
                        } else if (current_state == ST_SPECTATE || current_state == ST_MULTI_GAME) {
                            
                            net_close();
                            current_state = ST_MENU;
                            is_spectator = 0;
                        } else if (current_state != ST_MENU) {
                            net_close();
                            current_state = ST_MENU;
                        }
                    }
                    
                    if ((current_state == ST_MULTI_GAME || current_state == ST_SPECTATE) && 
                        is_spectator && current_lobby.game_mode == GAME_MODE_RUSH) {
                        if (e.key.keysym.sym == SDLK_LEFT) {
                            spectate_view_idx--;
                            if (spectate_view_idx < 0) spectate_view_idx = rush_player_count - 1;
                            if (spectate_view_idx < 0) spectate_view_idx = 0;
                        } else if (e.key.keysym.sym == SDLK_RIGHT) {
                            spectate_view_idx++;
                            if (spectate_view_idx >= rush_player_count) spectate_view_idx = 0;
                        }
                    }
                    break;
                
                case SDL_MOUSEMOTION:
                    SDL_GetMouseState(&mouse_x, &mouse_y);
                    
                    if (show_settings_overlay) {
                        handle_settings_overlay_drag();
                    }
                    break;
                
                case SDL_MOUSEBUTTONDOWN:
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        
                        if (show_settings_overlay) {
                            handle_settings_overlay_mousedown();
                            break;
                        }
                        if (current_state == ST_SOLO) {
                            handle_game_mousedown(0);
                        } else if (current_state == ST_MULTI_GAME && !is_spectator) {
                            handle_game_mousedown(1);
                        }
                    }
                    
                    else if (e.button.button == SDL_BUTTON_WHEELUP) {
                        if (current_state == ST_SERVER_BROWSER) {
                            handle_server_browser_scroll(-1);
                        }
                    }
                    else if (e.button.button == SDL_BUTTON_WHEELDOWN) {
                        if (current_state == ST_SERVER_BROWSER) {
                            handle_server_browser_scroll(1);
                        }
                    }
                    break;
                
                case SDL_MOUSEBUTTONUP:
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        
                        if (show_pause_menu) {
                            handle_pause_menu_click();
                            break;
                        }
                        
                        if (show_settings_overlay) {
                            handle_settings_overlay_click();
                            break;
                        }
                        
                        
                        if (popup_msg[0]) {
                            popup_msg[0] = '\0';
                            break;
                        }
                        
                        
                        switch (current_state) {
                            case ST_MENU:
                                handle_menu_click();
                                break;
                            case ST_OPTIONS:
                                handle_options_click();
                                break;
                            case ST_LOGIN:
                                handle_login_click();
                                break;
                            case ST_MULTI_CHOICE:
                                handle_multi_choice_click();
                                break;
                            case ST_JOIN_INPUT:
                                handle_join_input_click();
                                break;
                            case ST_LOBBY:
                                handle_lobby_click();
                                break;
                            case ST_SOLO:
                                
                                if (point_in_rect(mouse_x, mouse_y, window_w - 50, 20, 28, 28)) {
                                    play_click();
                                    show_settings_overlay = 1;
                                    settings_tab = 0;
                                } else if (game.game_over) {
                                    play_click();
                                    delete_saved_game();
                                    current_state = ST_MENU;
                                    resume_music();
                                } else {
                                    handle_game_click(0);
                                }
                                break;
                            case ST_MULTI_GAME:
                                
                                if (point_in_rect(mouse_x, mouse_y, window_w - 50, 20, 28, 28)) {
                                    play_click();
                                    show_settings_overlay = 1;
                                    settings_tab = 0;
                                } else if (is_spectator) {
                                    
                                    if (current_lobby.game_mode == GAME_MODE_RUSH && rush_player_count > 0) {
                                        spectate_view_idx = (spectate_view_idx + 1) % rush_player_count;
                                    }
                                } else if (current_lobby.game_mode == GAME_MODE_RUSH) {
                                    
                                    if (rush_time_remaining <= 0) {
                                        play_click();
                                        net_close();
                                        current_state = ST_MENU;
                                        is_spectator = 0;
                                    } else {
                                        handle_game_click(1);
                                    }
                                } else {
                                    handle_game_click(1);
                                }
                                break;
                            case ST_SERVER_BROWSER:
                                play_click();
                                handle_server_browser_click();
                                break;
                            case ST_SPECTATE:
                                
                                if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, WINDOW_H - 50, 180, 40)) {
                                    net_close();
                                    current_state = ST_MENU;
                                    is_spectator = 0;
                                }
                                
                                else if (current_lobby.game_mode == GAME_MODE_RUSH && rush_player_count > 0) {
                                    spectate_view_idx = (spectate_view_idx + 1) % rush_player_count;
                                }
                                break;
                        }
                    } else if (e.button.button == SDL_BUTTON_RIGHT) {
                        if (current_state == ST_LOBBY) {
                            handle_lobby_right_click();
                        }
                    }
                    break;
            }
        }
        
        
        {
            Uint32 current_time = SDL_GetTicks();
            if (last_frame_time > 0) {
                delta_time = (current_time - last_frame_time) / 1000.0f;
                if (delta_time > 0.1f) delta_time = 0.1f;  
            }
            last_frame_time = current_time;
        }
        
        
        if ((current_state == ST_MULTI_GAME || current_state == ST_SPECTATE) && 
            current_lobby.game_mode == GAME_MODE_RUSH && rush_time_remaining > 0) {
            Uint32 now = SDL_GetTicks();
            int elapsed = (now - last_time_update) / 1000;
            if (elapsed > 0) {
                rush_time_remaining -= elapsed;
                if (rush_time_remaining < 0) rush_time_remaining = 0;
                last_time_update = now;
            }
        }
        
        
        global_time += delta_time;
        glow_pulse += delta_time;
        if (glow_pulse > 6.28318f) glow_pulse -= 6.28318f;  
        
        
        draw_cyberpunk_background_responsive();
        
        
        switch (current_state) {
            case ST_MENU:
                render_menu();
                break;
            case ST_OPTIONS:
                render_options();
                break;
            case ST_LOGIN:
                render_login();
                break;
            case ST_MULTI_CHOICE:
                render_multi_choice();
                break;
            case ST_JOIN_INPUT:
                render_join_input();
                break;
            case ST_LOBBY:
                render_lobby();
                break;
            case ST_SOLO:
                render_solo();
                break;
            case ST_MULTI_GAME:
                render_multi_game();
                break;
            case ST_SERVER_BROWSER:
                render_server_browser();
                break;
            case ST_SPECTATE:
                render_spectate();
                break;
        }
        
        
        draw_popup();
        
        if (show_pause_menu && current_state == ST_SOLO) {
            render_pause_menu();
        }
        
        if (show_settings_overlay) {
            render_settings_overlay();
        }
        
        SDL_Flip(screen);
        
        
        SDL_Delay(16);
    }
    
    
    net_close();
    cleanup_audio();
    if (font_L) TTF_CloseFont(font_L);
    if (font_S) TTF_CloseFont(font_S);
    if (font_XS) TTF_CloseFont(font_XS);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}

