#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <SDL.h>
#include <SDL_ttf.h>

#include "game.h"
#include "net_client.h"
#include "../common/config.h"
#include "../common/net_protocol.h"

/* Game states */
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

/* Global variables */
static SDL_Surface *screen = NULL;
static TTF_Font *font_L = NULL;
static TTF_Font *font_S = NULL;
static TTF_Font *font_XS = NULL;
static GameState game;
static int current_state = ST_MENU;
static int mouse_x = 0, mouse_y = 0;
static int selected_piece_idx = -1;

/* Network settings */
static char online_ip[32] = "127.0.0.1";
static int online_port = PORT;

/* Options editing */
static char edit_ip[32] = "127.0.0.1";
static char edit_port[16] = "5000";
static int active_input = 0;  /* 0=none, 1=IP, 2=port */
static int test_result = 0;   /* -1=fail, 0=none, 1=success */

/* Player info */
static char my_pseudo[32] = "";
static char current_turn_pseudo[32] = "";
static char input_buffer[32] = "";
static char popup_msg[128] = "";

/* Multiplayer state */
static LobbyState current_lobby;
static LeaderboardData leaderboard;

/* Server browser state */
static ServerListData server_list;
static int browser_scroll_offset = 0;

/* Game mode settings (host) */
static int selected_game_mode = GAME_MODE_CLASSIC;
static int selected_timer_minutes = 3;  /* Default 3 minutes for Rush */

/* Spectator mode */
static int is_spectator = 0;
static int spectate_view_idx = 0;  /* Which player's grid to view in Rush */

/* Rush mode state */
static RushPlayerState rush_states[4];
static int rush_player_count = 0;
static int rush_time_remaining = 0;  /* Seconds remaining */
static Uint32 last_time_update = 0;

/* Frame timing */
static Uint32 last_frame_time = 0;
static float delta_time = 0.016f;

/* High score for solo mode */
static int solo_high_score = 0;

/* Piece display area */
#define PIECE_AREA_Y 680
#define PIECE_SLOT_W 160
#define PIECE_SLOT_H 200

/* Forward declarations */
static void render_game_grid_ex(GameState *gs, int offset_x, int offset_y);
static void render_rush_game(void);
static void render_mini_grid(int grid[GRID_H][GRID_W], int x, int y, int block_size, const char *label, int score, int is_selected);

/* UI Helper Functions */

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

static int draw_button(int x, int y, int w, int h, const char *lbl, Uint32 color, int disabled) {
    int hover;
    Uint32 draw_color;
    int r, g, b;
    
    if (disabled) {
        /* Draw disabled button */
        fill_rect(x + 4, y + 4, w, h, 0x111111);  /* Shadow */
        fill_rect(x, y, w, h, COLOR_BTN_DISABLED);
        draw_text(font_S, lbl, x + w/2, y + h/2, 0x888888);
        return 0;
    }
    
    hover = point_in_rect(mouse_x, mouse_y, x, y, w, h);
    
    /* Draw shadow */
    fill_rect(x + 4, y + 4, w, h, 0x111111);
    
    /* Calculate hover color */
    if (hover) {
        r = ((color >> 16) & 0xFF) + 32;
        g = ((color >> 8) & 0xFF) + 32;
        b = (color & 0xFF) + 32;
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;
        draw_color = (r << 16) | (g << 8) | b;
    } else {
        draw_color = color;
    }
    
    fill_rect(x, y, w, h, draw_color);
    draw_text(font_S, lbl, x + w/2, y + h/2, COLOR_WHITE);
    
    return hover;
}

static void draw_input_field(int x, int y, int w, int h, const char *text, int focused) {
    Uint32 bg_color = focused ? COLOR_INPUT_FOCUS : COLOR_INPUT;
    
    fill_rect(x, y, w, h, bg_color);
    
    /* Border */
    if (focused) {
        fill_rect(x, y, w, 2, COLOR_CYAN);
        fill_rect(x, y + h - 2, w, 2, COLOR_CYAN);
        fill_rect(x, y, 2, h, COLOR_CYAN);
        fill_rect(x + w - 2, y, 2, h, COLOR_CYAN);
    }
    
    /* Text */
    if (text && text[0]) {
        draw_text_left(font_S, text, x + 10, y + (h - 22) / 2, COLOR_WHITE);
    }
    
    /* Cursor */
    if (focused) {
        int text_w = 0;
        if (text && text[0]) {
            TTF_SizeUTF8(font_S, text, &text_w, NULL);
        }
        fill_rect(x + 10 + text_w + 2, y + 8, 2, h - 16, COLOR_CYAN);
    }
}

static void draw_styled_block(int x, int y, int size, Uint32 color) {
    int r, g, b;
    Uint32 inner_color;
    
    /* Outer block */
    fill_rect(x, y, size, size, color);
    
    /* Inner block (darker for 3D effect) */
    r = (int)(((color >> 16) & 0xFF) * 0.7);
    g = (int)(((color >> 8) & 0xFF) * 0.7);
    b = (int)((color & 0xFF) * 0.7);
    inner_color = (r << 16) | (g << 8) | b;
    
    fill_rect(x + 3, y + 3, size - 6, size - 6, inner_color);
}

static void draw_styled_block_scaled(int x, int y, int size, Uint32 color, float scale) {
    int r, g, b;
    Uint32 inner_color;
    int scaled_size = (int)(size * scale);
    int offset = (size - scaled_size) / 2;
    
    /* Outer block */
    fill_rect(x + offset, y + offset, scaled_size, scaled_size, color);
    
    /* Inner block (darker for 3D effect) */
    r = (int)(((color >> 16) & 0xFF) * 0.7);
    g = (int)(((color >> 8) & 0xFF) * 0.7);
    b = (int)((color & 0xFF) * 0.7);
    inner_color = (r << 16) | (g << 8) | b;
    
    int inner_offset = (int)(3 * scale);
    fill_rect(x + offset + inner_offset, y + offset + inner_offset, 
              scaled_size - inner_offset * 2, scaled_size - inner_offset * 2, inner_color);
}

/* Draw visual effects */
static void render_effects(EffectsManager *em, int offset_x, int offset_y) {
    int i;
    
    /* Draw particles */
    for (i = 0; i < MAX_PARTICLES; i++) {
        if (em->particles[i].active) {
            Particle *p = &em->particles[i];
            int alpha = (int)(p->life * 255);
            if (alpha > 255) alpha = 255;
            if (alpha < 0) alpha = 0;
            
            /* Draw particle with fading */
            Uint32 color = p->color;
            int r = ((color >> 16) & 0xFF) * alpha / 255;
            int g = ((color >> 8) & 0xFF) * alpha / 255;
            int b = (color & 0xFF) * alpha / 255;
            color = (r << 16) | (g << 8) | b;
            
            fill_rect((int)p->x + offset_x - p->size/2, 
                      (int)p->y + offset_y - p->size/2, 
                      p->size, p->size, color);
        }
    }
    
    /* Draw line clear effects (flash) */
    for (i = 0; i < 20; i++) {
        if (em->line_clears[i].active) {
            LineClearEffect *lc = &em->line_clears[i];
            int alpha = (int)((1.0f - lc->progress) * 200);
            if (alpha > 200) alpha = 200;
            if (alpha < 0) alpha = 0;
            
            /* Create flash color with alpha */
            int flash_intensity = alpha;
            Uint32 flash_color = (flash_intensity << 16) | (flash_intensity << 8) | flash_intensity;
            
            if (lc->row >= 0) {
                /* Row flash */
                fill_rect(GRID_OFFSET_X + offset_x, 
                          GRID_OFFSET_Y + lc->row * BLOCK_SIZE + offset_y,
                          GRID_W * BLOCK_SIZE, BLOCK_SIZE, flash_color);
            }
            if (lc->col >= 0) {
                /* Column flash */
                fill_rect(GRID_OFFSET_X + lc->col * BLOCK_SIZE + offset_x,
                          GRID_OFFSET_Y + offset_y,
                          BLOCK_SIZE, GRID_H * BLOCK_SIZE, flash_color);
            }
        }
    }
    
    /* Draw placement effects (scale pop) */
    for (i = 0; i < 25; i++) {
        if (em->place_effects[i].active) {
            PlaceEffect *pe = &em->place_effects[i];
            if (pe->scale < 1.0f) {
                int px = GRID_OFFSET_X + pe->col * BLOCK_SIZE + 1 + offset_x;
                int py = GRID_OFFSET_Y + pe->row * BLOCK_SIZE + 1 + offset_y;
                
                /* Draw a glowing outline effect */
                int glow_size = (int)((1.0f - pe->scale) * 10);
                Uint32 glow_color = 0xFFFFFF;
                
                fill_rect(px - glow_size, py - glow_size, 
                          BLOCK_SIZE - 2 + glow_size * 2, 
                          BLOCK_SIZE - 2 + glow_size * 2, glow_color);
            }
        }
    }
}

/* Draw eye icon for spectators */
static void draw_eye_icon(int x, int y, Uint32 color) {
    /* Simple eye shape using rectangles */
    fill_rect(x + 2, y + 6, 12, 4, color);  /* Eye body */
    fill_rect(x + 6, y + 4, 4, 8, color);   /* Vertical part */
    fill_rect(x + 7, y + 7, 2, 2, 0x000000); /* Pupil */
}

static void draw_popup(void) {
    if (!popup_msg[0]) return;
    
    /* Dark overlay */
    fill_rect(40, WINDOW_H / 2 - 80, WINDOW_W - 80, 160, 0x1A1A2A);
    
    /* Border */
    fill_rect(40, WINDOW_H / 2 - 80, WINDOW_W - 80, 4, COLOR_CYAN);
    fill_rect(40, WINDOW_H / 2 + 76, WINDOW_W - 80, 4, COLOR_CYAN);
    
    /* Message */
    draw_text(font_S, popup_msg, WINDOW_W / 2, WINDOW_H / 2 - 20, COLOR_WHITE);
    draw_text(font_S, "(Cliquez pour fermer)", WINDOW_W / 2, WINDOW_H / 2 + 30, COLOR_GREY);
}

/* Input handling */
static void handle_input_char(Uint16 unicode) {
    char c = (char)unicode;
    int len;
    
    if (current_state == ST_OPTIONS) {
        if (active_input == 1) {
            /* IP input */
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
            /* Port input */
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
        /* Other states use input_buffer */
        len = strlen(input_buffer);
        
        if (unicode == 8 && len > 0) {
            /* Backspace */
            input_buffer[len - 1] = '\0';
        } else if (len < 12) {
            /* Allow alphanumeric and space */
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') || c == ' ') {
                
                /* Auto-uppercase for room codes */
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

/* Rendering functions for each state */

static void render_menu(void) {
    /* Title */
    draw_text(font_L, "BLOCKBLAST V4", WINDOW_W / 2, 120, COLOR_ORANGE);
    
    /* Buttons */
    draw_button(WINDOW_W / 2 - 150, 280, 300, 60, "SOLO", COLOR_BUTTON, 0);
    draw_button(WINDOW_W / 2 - 150, 380, 300, 60, "MULTI ONLINE", COLOR_BUTTON, 0);
    draw_button(WINDOW_W / 2 - 150, 480, 300, 60, "OPTIONS", COLOR_BUTTON, 0);
}

static void render_options(void) {
    draw_text(font_L, "CONFIGURATION", WINDOW_W / 2, 80, COLOR_ORANGE);
    
    /* IP field */
    draw_text(font_S, "Adresse IP:", WINDOW_W / 2, 180, COLOR_WHITE);
    draw_input_field(70, 210, 400, 50, edit_ip, active_input == 1);
    
    /* Port field */
    draw_text(font_S, "Port:", WINDOW_W / 2, 300, COLOR_WHITE);
    draw_input_field(70, 330, 400, 50, edit_port, active_input == 2);
    
    /* Test result */
    if (test_result == 1) {
        draw_text(font_S, "Connexion OK!", WINDOW_W / 2, 420, COLOR_SUCCESS);
    } else if (test_result == -1) {
        draw_text(font_S, "Echec connexion!", WINDOW_W / 2, 420, COLOR_DANGER);
    }
    
    /* Buttons */
    draw_button(70, 480, 180, 50, "TESTER", COLOR_BUTTON, 0);
    draw_button(290, 480, 180, 50, "SAUVEGARDER", COLOR_SUCCESS, test_result != 1);
    draw_button(WINDOW_W / 2 - 90, 580, 180, 50, "RETOUR", COLOR_DANGER, 0);
}

static void render_login(void) {
    draw_text(font_L, "CONNEXION", WINDOW_W / 2, 120, COLOR_ORANGE);
    
    draw_text(font_S, "PSEUDO :", WINDOW_W / 2, 280, COLOR_WHITE);
    draw_input_field(70, 310, 400, 50, input_buffer, 1);
    
    draw_button(WINDOW_W / 2 - 90, 420, 180, 50, "VALIDER", COLOR_SUCCESS, strlen(input_buffer) == 0);
    draw_button(WINDOW_W / 2 - 90, 500, 180, 50, "RETOUR", COLOR_DANGER, 0);
}

static void render_multi_choice(void) {
    int i;
    char line[64];
    
    draw_text(font_L, "MULTIJOUEUR", WINDOW_W / 2, 80, COLOR_ORANGE);
    
    /* Leaderboard */
    draw_text(font_S, "- MEILLEURS SCORES -", WINDOW_W / 2, 160, COLOR_CYAN);
    
    for (i = 0; i < leaderboard.count && i < 5; i++) {
        snprintf(line, sizeof(line), "%d. %s (%d)", i + 1, leaderboard.names[i], leaderboard.scores[i]);
        draw_text(font_S, line, WINDOW_W / 2, 200 + i * 35, COLOR_WHITE);
    }
    
    if (leaderboard.count == 0) {
        draw_text(font_S, "Aucun score", WINDOW_W / 2, 220, COLOR_GREY);
    }
    
    /* Buttons */
    draw_button(WINDOW_W / 2 - 150, 420, 300, 55, "CREER UNE PARTIE", COLOR_SUCCESS, 0);
    draw_button(WINDOW_W / 2 - 150, 495, 300, 55, "REJOINDRE (CODE)", COLOR_BUTTON, 0);
    draw_button(WINDOW_W / 2 - 150, 570, 300, 55, "SERVEURS OUVERTS", COLOR_PURPLE, 0);
    draw_button(WINDOW_W / 2 - 90, 660, 180, 50, "RETOUR", COLOR_DANGER, 0);
}

/* Server browser rendering */
static void render_server_browser(void) {
    int i;
    char line[128];
    int y_start = 160;
    int entry_height = 110;
    
    draw_text(font_L, "SERVEURS OUVERTS", WINDOW_W / 2, 50, COLOR_ORANGE);
    
    if (server_list.count == 0) {
        draw_text(font_S, "Aucun serveur disponible", WINDOW_W / 2, 350, COLOR_GREY);
        draw_text(font_S, "Cliquez sur Actualiser", WINDOW_W / 2, 390, COLOR_GREY);
    } else {
        /* Server list panel */
        fill_rect(20, 120, WINDOW_W - 40, 520, COLOR_PANEL);
        
        for (i = 0; i < server_list.count && i < 5; i++) {
            int idx = i + browser_scroll_offset;
            if (idx >= server_list.count) break;
            
            ServerInfo *srv = &server_list.servers[idx];
            int y = y_start + i * entry_height;
            
            /* Server entry background */
            fill_rect(30, y, WINDOW_W - 60, entry_height - 10, 0x2A2A3A);
            
            /* Server info */
            snprintf(line, sizeof(line), "Code: %s | Hote: %s", srv->room_code, srv->host_name);
            draw_text_left(font_S, line, 45, y + 10, COLOR_WHITE);
            
            /* Player count */
            snprintf(line, sizeof(line), "Joueurs: %d/4", srv->player_count);
            draw_text_left(font_S, line, 45, y + 35, COLOR_CYAN);
            
            /* Game mode */
            const char *mode_str = srv->game_mode == GAME_MODE_RUSH ? "Rush" : "Classic";
            snprintf(line, sizeof(line), "Mode: %s", mode_str);
            draw_text_left(font_S, line, 200, y + 35, COLOR_GOLD);
            
            /* Status and buttons - stacked vertically */
            if (srv->game_started) {
                draw_text_left(font_S, "En cours", 45, y + 60, COLOR_DANGER);
                /* Join button (disabled) */
                draw_button(WINDOW_W - 140, y + 10, 100, 35, "Rejoindre", COLOR_BTN_DISABLED, 1);
                /* Spectate button below */
                draw_button(WINDOW_W - 140, y + 52, 100, 35, "Observer", COLOR_PURPLE, 0);
            } else {
                draw_text_left(font_S, "En attente", 45, y + 60, COLOR_SUCCESS);
                /* Join button */
                draw_button(WINDOW_W - 140, y + 10, 100, 35, "Rejoindre", COLOR_SUCCESS, 0);
                /* Spectate button below */
                draw_button(WINDOW_W - 140, y + 52, 100, 35, "Observer", COLOR_PURPLE, 0);
            }
        }
        
        /* Scroll indicators */
        if (browser_scroll_offset > 0) {
            draw_text(font_S, "^ Defiler vers le haut ^", WINDOW_W / 2, 130, COLOR_GREY);
        }
        if (browser_scroll_offset + 5 < server_list.count) {
            draw_text(font_S, "v Defiler vers le bas v", WINDOW_W / 2, 650, COLOR_GREY);
        }
    }
    
    /* Buttons at bottom - Refresh above Back */
    draw_button(WINDOW_W / 2 - 90, 680, 180, 45, "Actualiser", COLOR_BUTTON, 0);
    draw_button(WINDOW_W / 2 - 90, 735, 180, 45, "RETOUR", COLOR_DANGER, 0);
}

static void render_join_input(void) {
    draw_text(font_L, "REJOINDRE", WINDOW_W / 2, 120, COLOR_ORANGE);
    
    draw_text(font_S, "CODE :", WINDOW_W / 2, 280, COLOR_WHITE);
    draw_input_field(120, 310, 300, 50, input_buffer, 1);
    
    draw_button(WINDOW_W / 2 - 90, 420, 180, 50, "ENTRER", COLOR_SUCCESS, strlen(input_buffer) != 4);
    draw_button(WINDOW_W / 2 - 90, 500, 180, 50, "RETOUR", COLOR_DANGER, 0);
}

static void render_lobby(void) {
    int i;
    char title[64];
    char mode_str[32];
    
    /* Room code */
    snprintf(title, sizeof(title), "CODE PARTIE : %s", current_lobby.room_code);
    draw_text(font_L, title, WINDOW_W / 2, 60, COLOR_ORANGE);
    
    /* Public/Private indicator */
    if (current_lobby.is_public) {
        draw_text(font_S, "(Partie publique)", WINDOW_W / 2, 95, COLOR_SUCCESS);
    } else {
        draw_text(font_S, "(Partie privee)", WINDOW_W / 2, 95, COLOR_GREY);
    }
    
    /* Players header */
    draw_text(font_S, "JOUEURS :", WINDOW_W / 2, 140, COLOR_WHITE);
    
    /* Player list */
    for (i = 0; i < current_lobby.player_count; i++) {
        Uint32 color = COLOR_WHITE;
        int name_x = WINDOW_W / 2;
        
        /* Highlight own name */
        if (strcmp(current_lobby.players[i], my_pseudo) == 0) {
            color = COLOR_CYAN;
        }
        
        /* Draw spectator eye icon if applicable */
        if (current_lobby.is_spectator[i]) {
            draw_eye_icon(name_x - 100, 175 + i * 35, COLOR_PURPLE);
            draw_text(font_S, current_lobby.players[i], name_x, 180 + i * 35, COLOR_PURPLE);
        } else {
            draw_text(font_S, current_lobby.players[i], name_x, 180 + i * 35, color);
        }
        
        /* Show host indicator */
        if (i == 0) {
            draw_text(font_S, "(Host)", name_x + 100, 180 + i * 35, COLOR_GOLD);
        }
    }
    
    /* Game mode section (host only) */
    if (current_lobby.is_host && !current_lobby.game_started) {
        int mode_y = 320;
        
        draw_text(font_S, "MODE DE JEU :", WINDOW_W / 2, mode_y, COLOR_WHITE);
        
        /* Classic mode button */
        int classic_selected = (selected_game_mode == GAME_MODE_CLASSIC);
        Uint32 classic_color = classic_selected ? COLOR_CYAN : COLOR_BUTTON;
        draw_button(WINDOW_W / 2 - 160, mode_y + 25, 150, 35, "Classic", classic_color, 0);
        
        /* Rush mode button */
        int rush_selected = (selected_game_mode == GAME_MODE_RUSH);
        Uint32 rush_color = rush_selected ? COLOR_CYAN : COLOR_BUTTON;
        draw_button(WINDOW_W / 2 + 10, mode_y + 25, 150, 35, "Rush", rush_color, 0);
        
        /* Timer selection for Rush mode */
        if (selected_game_mode == GAME_MODE_RUSH) {
            draw_text(font_S, "DUREE :", WINDOW_W / 2, mode_y + 80, COLOR_WHITE);
            
            /* Timer buttons - compact layout */
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
        
        /* Make room public/private toggle */
        int toggle_y = selected_game_mode == GAME_MODE_RUSH ? mode_y + 150 : mode_y + 80;
        const char *visibility_text = current_lobby.is_public ? "Rendre Privee" : "Rendre Publique";
        draw_button(WINDOW_W / 2 - 90, toggle_y, 180, 35, visibility_text, COLOR_PURPLE, 0);
        
        /* Start game button and waiting text */
        int start_y = selected_game_mode == GAME_MODE_RUSH ? 530 : 470;
        int need_players = current_lobby.player_count < 2 - current_lobby.spectator_count;
        
        if (need_players) {
            draw_text(font_S, "En attente d'autres joueurs...", WINDOW_W / 2, start_y, COLOR_GREY);
            start_y += 40;
        }
        
        draw_button(WINDOW_W / 2 - 150, start_y, 300, 55, "LANCER LA PARTIE", COLOR_SUCCESS, need_players);
    } else if (!current_lobby.is_host) {
        /* Show current game mode for non-hosts */
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
    
    /* Kick instruction for host */
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
    
    /* Apply screen shake */
    if (gs->effects.screen_shake > 0 && gs->effects.shake_time > 0) {
        shake_x = (rand() % (gs->effects.screen_shake * 2 + 1)) - gs->effects.screen_shake;
        shake_y = (rand() % (gs->effects.screen_shake * 2 + 1)) - gs->effects.screen_shake;
    }
    
    int base_x = GRID_OFFSET_X + offset_x + shake_x;
    int base_y = GRID_OFFSET_Y + offset_y + shake_y;
    
    /* Grid background */
    fill_rect(base_x - 5, base_y - 5, 
              GRID_W * BLOCK_SIZE + 10, GRID_H * BLOCK_SIZE + 10, 
              0x151520);
    
    /* Grid lines */
    for (i = 0; i <= GRID_H; i++) {
        fill_rect(base_x, base_y + i * BLOCK_SIZE, 
                  GRID_W * BLOCK_SIZE, 1, COLOR_GRID);
    }
    for (j = 0; j <= GRID_W; j++) {
        fill_rect(base_x + j * BLOCK_SIZE, base_y, 
                  1, GRID_H * BLOCK_SIZE, COLOR_GRID);
    }
    
    /* Placed blocks */
    for (i = 0; i < GRID_H; i++) {
        for (j = 0; j < GRID_W; j++) {
            if (gs->grid[i][j] != 0) {
                draw_styled_block(base_x + j * BLOCK_SIZE + 1,
                                  base_y + i * BLOCK_SIZE + 1,
                                  BLOCK_SIZE - 2, gs->grid[i][j]);
            }
        }
    }
    
    /* Render visual effects */
    render_effects(&gs->effects, shake_x + offset_x, shake_y + offset_y);
}

/* Render a mini grid for Rush mode spectator view */
static void render_mini_grid(int grid[GRID_H][GRID_W], int x, int y, int block_size, const char *label, int score, int is_selected) {
    int i, j;
    int grid_w = GRID_W * block_size;
    int grid_h = GRID_H * block_size;
    char score_str[32];
    
    /* Selection highlight */
    if (is_selected) {
        fill_rect(x - 4, y - 25, grid_w + 8, grid_h + 55, COLOR_CYAN);
    }
    
    /* Background */
    fill_rect(x - 2, y - 2, grid_w + 4, grid_h + 4, 0x151520);
    
    /* Grid lines (simplified) */
    fill_rect(x, y, grid_w, grid_h, 0x202030);
    
    /* Blocks */
    for (i = 0; i < GRID_H; i++) {
        for (j = 0; j < GRID_W; j++) {
            if (grid[i][j] != 0) {
                fill_rect(x + j * block_size, y + i * block_size,
                          block_size - 1, block_size - 1, grid[i][j]);
            }
        }
    }
    
    /* Player name */
    if (font_XS) {
        draw_text(font_XS, label, x + grid_w / 2, y - 12, COLOR_WHITE);
    }
    
    /* Score */
    snprintf(score_str, sizeof(score_str), "%d pts", score);
    if (font_XS) {
        draw_text(font_XS, score_str, x + grid_w / 2, y + grid_h + 12, COLOR_GOLD);
    }
}

static void render_pieces(int greyed) {
    int i, j, k;
    int slot_x;
    
    for (i = 0; i < 3; i++) {
        if (!game.pieces_available[i]) continue;
        if (selected_piece_idx == i) continue;  /* Don't draw selected piece here */
        
        Piece *p = &game.current_pieces[i];
        slot_x = 30 + i * PIECE_SLOT_W;
        
        /* Center piece in slot */
        int piece_px = slot_x + (PIECE_SLOT_W - p->w * 30) / 2;
        int piece_py = PIECE_AREA_Y + (PIECE_SLOT_H - p->h * 30) / 2;
        
        for (j = 0; j < p->h; j++) {
            for (k = 0; k < p->w; k++) {
                if (p->data[j][k]) {
                    Uint32 color = p->color;
                    if (greyed) {
                        /* Grey out if not player's turn */
                        int r = ((color >> 16) & 0xFF) / 3;
                        int g = ((color >> 8) & 0xFF) / 3;
                        int b = (color & 0xFF) / 3;
                        color = (r << 16) | (g << 8) | b;
                    }
                    draw_styled_block(piece_px + k * 30, piece_py + j * 30, 28, color);
                }
            }
        }
    }
}

static void render_dragged_piece(void) {
    int j, k;
    
    if (selected_piece_idx < 0 || selected_piece_idx >= 3) return;
    
    Piece *p = &game.current_pieces[selected_piece_idx];
    
    /* Calculate position centered on mouse */
    int base_x = mouse_x - (p->w * BLOCK_SIZE) / 2;
    int base_y = mouse_y - (p->h * BLOCK_SIZE) / 2;
    
    for (j = 0; j < p->h; j++) {
        for (k = 0; k < p->w; k++) {
            if (p->data[j][k]) {
                draw_styled_block(base_x + k * BLOCK_SIZE, 
                                  base_y + j * BLOCK_SIZE, 
                                  BLOCK_SIZE - 2, p->color);
            }
        }
    }
}

static void render_solo(void) {
    char score_text[64];
    
    /* Update visual effects */
    update_effects(&game.effects, delta_time);
    
    /* Update high score if current score is higher */
    if (game.score > solo_high_score) {
        solo_high_score = game.score;
    }
    
    /* Score */
    snprintf(score_text, sizeof(score_text), "Score: %d", game.score);
    draw_text(font_L, score_text, WINDOW_W / 2, 35, COLOR_WHITE);
    
    /* High score */
    snprintf(score_text, sizeof(score_text), "Record: %d", solo_high_score);
    draw_text(font_S, score_text, WINDOW_W / 2, 65, COLOR_GOLD);
    
    /* Grid with effects */
    render_game_grid_ex(&game, 0, 0);
    
    /* Pieces */
    render_pieces(0);
    
    /* Dragged piece */
    render_dragged_piece();
    
    /* Game over message */
    if (game.game_over) {
        fill_rect(0, WINDOW_H / 2 - 80, WINDOW_W, 160, 0x000000DD);
        draw_text(font_L, "GAME OVER!", WINDOW_W / 2, WINDOW_H / 2 - 40, COLOR_DANGER);
        snprintf(score_text, sizeof(score_text), "Score final: %d", game.score);
        draw_text(font_S, score_text, WINDOW_W / 2, WINDOW_H / 2, COLOR_WHITE);
        if (game.score >= solo_high_score) {
            draw_text(font_S, "Nouveau record!", WINDOW_W / 2, WINDOW_H / 2 + 30, COLOR_GOLD);
        }
        draw_text(font_S, "Cliquez pour revenir au menu", WINDOW_W / 2, WINDOW_H / 2 + 60, COLOR_GREY);
    }
}

static void render_multi_game(void) {
    char score_text[32];
    char turn_text[64];
    char timer_text[32];
    
    /* Update visual effects */
    update_effects(&game.effects, delta_time);
    
    /* Check if Rush mode */
    if (current_lobby.game_mode == GAME_MODE_RUSH) {
        render_rush_game();
        return;
    }
    
    /* Classic mode rendering */
    
    /* Turn indicator */
    if (is_spectator) {
        draw_text(font_S, "Mode Spectateur", WINDOW_W / 2, 30, COLOR_PURPLE);
        snprintf(turn_text, sizeof(turn_text), "Tour de %s", current_turn_pseudo);
        draw_text(font_S, turn_text, WINDOW_W / 2, 55, COLOR_GREY);
    } else if (is_my_turn()) {
        draw_text(font_L, "A TOI DE JOUER !", WINDOW_W / 2, 40, COLOR_SUCCESS);
    } else {
        snprintf(turn_text, sizeof(turn_text), "Tour de %s...", current_turn_pseudo);
        draw_text(font_S, turn_text, WINDOW_W / 2, 40, COLOR_GREY);
    }
    
    /* Score */
    snprintf(score_text, sizeof(score_text), "Score: %d", game.score);
    draw_text(font_S, score_text, WINDOW_W / 2, 70, COLOR_WHITE);
    
    /* Grid with effects */
    render_game_grid_ex(&game, 0, 0);
    
    /* Pieces - greyed if not our turn or spectator */
    render_pieces(!is_my_turn() || is_spectator);
    
    /* Dragged piece (only if our turn and not spectator) */
    if (is_my_turn() && !is_spectator) {
        render_dragged_piece();
    }
}

/* Render Rush mode game */
static void render_rush_game(void) {
    char timer_text[32];
    char score_text[64];
    int i;
    
    /* Timer display */
    int mins = rush_time_remaining / 60;
    int secs = rush_time_remaining % 60;
    snprintf(timer_text, sizeof(timer_text), "%d:%02d", mins, secs);
    
    /* Timer color changes based on time remaining */
    Uint32 timer_color = COLOR_WHITE;
    if (rush_time_remaining <= 30) timer_color = COLOR_DANGER;
    else if (rush_time_remaining <= 60) timer_color = COLOR_ORANGE;
    
    draw_text(font_L, timer_text, WINDOW_W / 2, 35, timer_color);
    
    if (is_spectator) {
        /* Spectator view - show all player grids */
        draw_text(font_L, "SPECTATEUR", WINDOW_W / 2, 70, COLOR_PURPLE);
        
        /* Mini grids for all players */
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
        
        /* Show selected player's grid larger */
        if (spectate_view_idx >= 0 && spectate_view_idx < rush_player_count) {
            /* Create a temporary game state for rendering */
            memcpy(game.grid, rush_states[spectate_view_idx].grid, sizeof(game.grid));
            game.score = rush_states[spectate_view_idx].score;
            
            draw_text(font_S, rush_states[spectate_view_idx].pseudo, WINDOW_W / 2, 310, COLOR_CYAN);
            snprintf(score_text, sizeof(score_text), "Score: %d", rush_states[spectate_view_idx].score);
            draw_text(font_S, score_text, WINDOW_W / 2, 335, COLOR_GOLD);
            
            /* Render the main grid (offset down) */
            render_game_grid_ex(&game, 0, 280);
        }
        
        /* Navigation hint */
        draw_text(font_S, "< > pour changer de joueur", WINDOW_W / 2, WINDOW_H - 30, COLOR_GREY);
    } else {
        /* Player view - own grid */
        
        /* Grid - render first */
        render_game_grid_ex(&game, 0, 0);
        
        /* Score display - positioned to the left of the grid */
        snprintf(score_text, sizeof(score_text), "%d", game.score);
        draw_text(font_S, "Score", 35, 100, COLOR_WHITE);
        draw_text(font_L, score_text, 35, 135, COLOR_GOLD);
        
        /* Show mini leaderboard on the left */
        draw_text(font_XS ? font_XS : font_S, "Classement", 35, 180, COLOR_WHITE);
        for (i = 0; i < rush_player_count && i < 4; i++) {
            char entry[48];
            int entry_y = 205 + i * 40;  /* 40 pixels between each player entry */
            Uint32 entry_color = (strcmp(rush_states[i].pseudo, my_pseudo) == 0) ? COLOR_CYAN : COLOR_WHITE;
            
            /* Player name */
            snprintf(entry, sizeof(entry), "%d. %s", i + 1, rush_states[i].pseudo);
            draw_text_left(font_XS ? font_XS : font_S, entry, 10, entry_y, entry_color);
            
            /* Score on next line */
            snprintf(entry, sizeof(entry), "   %d pts", rush_states[i].score);
            draw_text_left(font_XS ? font_XS : font_S, entry, 10, entry_y + 16, entry_color);
        }
        
        /* Pieces */
        render_pieces(0);
        
        /* Dragged piece */
        render_dragged_piece();
    }
    
    /* Game over for Rush */
    if (rush_time_remaining <= 0 && !is_spectator) {
        fill_rect(0, WINDOW_H / 2 - 80, WINDOW_W, 160, 0x000000DD);
        draw_text(font_L, "TEMPS ECOULE !", WINDOW_W / 2, WINDOW_H / 2 - 40, COLOR_ORANGE);
        
        /* Find winner */
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

/* Render spectate view (for Classic mode or between games) */
static void render_spectate(void) {
    if (current_lobby.game_mode == GAME_MODE_RUSH) {
        /* Rush mode has its own spectator header */
        render_rush_game();
    } else {
        /* Classic mode - show spectator header */
        draw_text(font_L, "MODE SPECTATEUR", WINDOW_W / 2, 40, COLOR_PURPLE);
        
        /* Classic mode spectate */
        char info[64];
        snprintf(info, sizeof(info), "Tour de %s", current_turn_pseudo);
        draw_text(font_S, info, WINDOW_W / 2, 75, COLOR_GREY);
        
        render_game_grid_ex(&game, 0, 10);
        
        /* Score display */
        char score_text[32];
        snprintf(score_text, sizeof(score_text), "Score: %d", game.score);
        draw_text(font_S, score_text, WINDOW_W / 2, WINDOW_H - 80, COLOR_WHITE);
    }
    
    draw_button(WINDOW_W / 2 - 90, WINDOW_H - 50, 180, 40, "QUITTER", COLOR_DANGER, 0);
}

/* Event handling */

static void handle_menu_click(void) {
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 150, 280, 300, 60)) {
        /* Solo */
        init_game(&game);
        current_state = ST_SOLO;
    } else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 150, 380, 300, 60)) {
        /* Multi Online */
        if (net_connect(online_ip, online_port)) {
            memset(input_buffer, 0, sizeof(input_buffer));
            current_state = ST_LOGIN;
        } else {
            strcpy(popup_msg, "Impossible de se connecter au serveur!");
        }
    } else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 150, 480, 300, 60)) {
        /* Options */
        strcpy(edit_ip, online_ip);
        snprintf(edit_port, sizeof(edit_port), "%d", online_port);
        active_input = 0;
        test_result = 0;
        current_state = ST_OPTIONS;
    }
}

static void handle_options_click(void) {
    /* IP field click */
    if (point_in_rect(mouse_x, mouse_y, 70, 210, 400, 50)) {
        active_input = 1;
    }
    /* Port field click */
    else if (point_in_rect(mouse_x, mouse_y, 70, 330, 400, 50)) {
        active_input = 2;
    }
    /* Test button */
    else if (point_in_rect(mouse_x, mouse_y, 70, 480, 180, 50)) {
        test_result = perform_connection_test() ? 1 : -1;
    }
    /* Save button */
    else if (point_in_rect(mouse_x, mouse_y, 290, 480, 180, 50) && test_result == 1) {
        strcpy(online_ip, edit_ip);
        online_port = atoi(edit_port);
        strcpy(popup_msg, "Configuration sauvegardee!");
    }
    /* Back button */
    else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 580, 180, 50)) {
        current_state = ST_MENU;
    }
}

static void handle_login_click(void) {
    Packet pkt;
    
    /* Validate button */
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 420, 180, 50) && strlen(input_buffer) > 0) {
        strcpy(my_pseudo, input_buffer);
        
        /* Send login */
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_LOGIN;
        strcpy(pkt.text, my_pseudo);
        net_send(&pkt);
        
        /* Request leaderboard */
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_LEADERBOARD_REQ;
        net_send(&pkt);
        
        memset(input_buffer, 0, sizeof(input_buffer));
        current_state = ST_MULTI_CHOICE;
    }
    /* Back button */
    else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 500, 180, 50)) {
        net_close();
        current_state = ST_MENU;
    }
}

static void handle_multi_choice_click(void) {
    Packet pkt;
    
    /* Create room */
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 150, 420, 300, 55)) {
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_CREATE_ROOM;
        net_send(&pkt);
        /* Reset game mode selection to defaults */
        selected_game_mode = GAME_MODE_CLASSIC;
        selected_timer_minutes = 3;
    }
    /* Join room by code */
    else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 150, 495, 300, 55)) {
        memset(input_buffer, 0, sizeof(input_buffer));
        current_state = ST_JOIN_INPUT;
    }
    /* Open servers browser */
    else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 150, 570, 300, 55)) {
        /* Request server list */
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_SERVER_LIST_REQ;
        net_send(&pkt);
        browser_scroll_offset = 0;
        current_state = ST_SERVER_BROWSER;
    }
    /* Back button */
    else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 660, 180, 50)) {
        net_close();
        current_state = ST_MENU;
    }
}

static void handle_join_input_click(void) {
    Packet pkt;
    
    /* Enter button */
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 420, 180, 50) && strlen(input_buffer) == 4) {
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_JOIN_ROOM;
        strcpy(pkt.text, input_buffer);
        net_send(&pkt);
    }
    /* Back button */
    else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 500, 180, 50)) {
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
    
    if (current_lobby.is_host && !current_lobby.game_started) {
        /* Classic mode button */
        if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 160, mode_y + 25, 150, 35)) {
            selected_game_mode = GAME_MODE_CLASSIC;
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_SET_GAME_MODE;
            pkt.game_mode = GAME_MODE_CLASSIC;
            net_send(&pkt);
        }
        /* Rush mode button */
        else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 + 10, mode_y + 25, 150, 35)) {
            selected_game_mode = GAME_MODE_RUSH;
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_SET_GAME_MODE;
            pkt.game_mode = GAME_MODE_RUSH;
            pkt.timer_value = selected_timer_minutes * 60;
            net_send(&pkt);
        }
        
        /* Timer buttons (Rush mode only) - updated positions */
        if (selected_game_mode == GAME_MODE_RUSH) {
            int timer_buttons[] = {1, 2, 3, 5, 10};
            int timer_x[] = {WINDOW_W / 2 - 200, WINDOW_W / 2 - 130, WINDOW_W / 2 - 60, 
                            WINDOW_W / 2 + 10, WINDOW_W / 2 + 80};
            int i;
            for (i = 0; i < 5; i++) {
                if (point_in_rect(mouse_x, mouse_y, timer_x[i], mode_y + 100, 55, 30)) {
                    selected_timer_minutes = timer_buttons[i];
                    memset(&pkt, 0, sizeof(pkt));
                    pkt.type = MSG_SET_TIMER;
                    pkt.timer_value = selected_timer_minutes * 60;
                    net_send(&pkt);
                    break;
                }
            }
        }
        
        /* Public/Private toggle */
        if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, toggle_y, 180, 35)) {
            /* Toggle and send update - server handles the toggle */
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_SET_GAME_MODE;  /* Reuse this message type */
            pkt.game_mode = selected_game_mode;
            pkt.lobby.is_public = !current_lobby.is_public;
            net_send(&pkt);
        }
        
        /* Start game button */
        if (!need_players && point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 150, start_y, 300, 55)) {
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_START_GAME;
            pkt.game_mode = selected_game_mode;
            pkt.timer_value = selected_timer_minutes * 60;
            net_send(&pkt);
        }
    }
    
    /* Quit button */
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 660, 180, 50)) {
        net_close();
        current_state = ST_MENU;
        is_spectator = 0;
    }
}

/* Handle server browser clicks */
static void handle_server_browser_click(void) {
    Packet pkt;
    int i;
    int y_start = 160;
    int entry_height = 110;
    
    /* Refresh button */
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 680, 180, 45)) {
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_SERVER_LIST_REQ;
        net_send(&pkt);
        return;
    }
    
    /* Server entries */
    for (i = 0; i < server_list.count && i < 5; i++) {
        int idx = i + browser_scroll_offset;
        if (idx >= server_list.count) break;
        
        ServerInfo *srv = &server_list.servers[idx];
        int y = y_start + i * entry_height;
        
        /* Join button */
        if (!srv->game_started && point_in_rect(mouse_x, mouse_y, WINDOW_W - 140, y + 10, 100, 35)) {
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_JOIN_ROOM;
            strcpy(pkt.text, srv->room_code);
            is_spectator = 0;
            net_send(&pkt);
            return;
        }
        
        /* Spectate button (below Join) */
        if (point_in_rect(mouse_x, mouse_y, WINDOW_W - 140, y + 52, 100, 35)) {
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_JOIN_SPECTATE;
            strcpy(pkt.text, srv->room_code);
            is_spectator = 1;
            spectate_view_idx = 0;
            net_send(&pkt);
            return;
        }
    }
    
    /* Back button */
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 735, 180, 45)) {
        current_state = ST_MULTI_CHOICE;
    }
}

/* Handle scroll in server browser */
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
    
    /* Check if clicking on a player name (not self) */
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
    
    /* In Classic multiplayer, check if it's our turn. In Rush mode, everyone plays */
    if (is_multi && current_lobby.game_mode == GAME_MODE_CLASSIC && !is_my_turn()) return;
    
    if (selected_piece_idx >= 0) {
        /* Try to place piece */
        Piece *p = &game.current_pieces[selected_piece_idx];
        
        /* Calculate grid position from mouse */
        grid_x = (mouse_x - GRID_OFFSET_X - (p->w * BLOCK_SIZE) / 2 + BLOCK_SIZE / 2) / BLOCK_SIZE;
        grid_y = (mouse_y - GRID_OFFSET_Y - (p->h * BLOCK_SIZE) / 2 + BLOCK_SIZE / 2) / BLOCK_SIZE;
        
        if (can_place(&game, grid_y, grid_x, p)) {
            place_piece_logic(&game, grid_y, grid_x, p);
            game.pieces_available[selected_piece_idx] = 0;
            
            /* Check if all pieces are placed */
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
            
            /* Send update in multiplayer */
            if (is_multi) {
                memset(&pkt, 0, sizeof(pkt));
                pkt.type = MSG_PLACE_PIECE;
                memcpy(pkt.grid_data, game.grid, sizeof(game.grid));
                pkt.score = game.score;
                net_send(&pkt);
            }
            
            /* Check for game over */
            if (!check_valid_moves_exist(&game)) {
                game.game_over = 1;
            }
        }
        
        selected_piece_idx = -1;
    }
}

static void handle_game_mousedown(int is_multi) {
    int i;
    int slot_x;
    
    /* In Classic multiplayer, check if it's our turn. In Rush mode, everyone plays */
    if (is_multi && current_lobby.game_mode == GAME_MODE_CLASSIC && !is_my_turn()) return;
    
    /* Check if clicking on available pieces */
    if (mouse_y >= PIECE_AREA_Y) {
        for (i = 0; i < 3; i++) {
            if (!game.pieces_available[i]) continue;
            
            slot_x = 30 + i * PIECE_SLOT_W;
            if (mouse_x >= slot_x && mouse_x < slot_x + PIECE_SLOT_W) {
                selected_piece_idx = i;
                break;
            }
        }
    }
}

/* Network message processing */
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
                /* Update local game mode selection from lobby */
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
                /* Initialize game based on mode */
                current_lobby.game_mode = pkt.game_mode;
                
                if (pkt.game_mode == GAME_MODE_RUSH) {
                    /* Rush mode - initialize own grid */
                    memset(game.grid, 0, sizeof(game.grid));
                    game.score = 0;
                    generate_pieces(&game);
                    init_effects(&game.effects);
                    
                    rush_time_remaining = pkt.time_remaining;
                    last_time_update = SDL_GetTicks();
                    rush_player_count = 0;
                    
                    current_state = is_spectator ? ST_SPECTATE : ST_MULTI_GAME;
                } else {
                    /* Classic mode */
                    memcpy(game.grid, pkt.grid_data, sizeof(game.grid));
                    strcpy(current_turn_pseudo, pkt.turn_pseudo);
                    game.score = 0;
                    generate_pieces(&game);
                    init_effects(&game.effects);
                    
                    current_state = is_spectator ? ST_SPECTATE : ST_MULTI_GAME;
                }
                break;
            
            case MSG_UPDATE_GRID:
                /* Classic mode grid update */
                memcpy(game.grid, pkt.grid_data, sizeof(game.grid));
                strcpy(current_turn_pseudo, pkt.turn_pseudo);
                break;
            
            case MSG_RUSH_UPDATE:
                /* Rush mode update - copy player states */
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
                /* Game ended - show results */
                rush_time_remaining = 0;
                if (current_lobby.game_mode == GAME_MODE_RUSH) {
                    /* Results will be shown in render_rush_game */
                }
                break;
            
            case MSG_SERVER_LIST_REP:
                /* Received server list */
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

/* Main function */
int main(int argc, char *argv[]) {
    SDL_Event e;
    int running = 1;
    
    (void)argc;
    (void)argv;
    
    /* Initialize random seed */
    srand((unsigned int)time(NULL));
    
    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL init failed: %s\n", SDL_GetError());
        return 1;
    }
    
    /* Initialize TTF */
    if (TTF_Init() < 0) {
        printf("TTF init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    
    /* Enable Unicode for keyboard */
    SDL_EnableUNICODE(1);
    
    /* Create window */
    screen = SDL_SetVideoMode(WINDOW_W, WINDOW_H, 32, SDL_SWSURFACE);
    if (!screen) {
        printf("Video mode failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    SDL_WM_SetCaption("BlockBlast V4.0", NULL);
    
    /* Load fonts - try multiple paths */
    {
        const char *font_paths[] = {
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
    
    /* Initialize game */
    init_game(&game);
    memset(&current_lobby, 0, sizeof(current_lobby));
    memset(&leaderboard, 0, sizeof(leaderboard));
    
    /* Main loop */
    while (running) {
        /* Process network messages */
        process_network();
        
        /* Handle events */
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    running = 0;
                    break;
                
                case SDL_KEYDOWN:
                    if (e.key.keysym.unicode) {
                        handle_input_char(e.key.keysym.unicode);
                    }
                    /* Escape to go back */
                    if (e.key.keysym.sym == SDLK_ESCAPE) {
                        if (current_state == ST_SOLO) {
                            current_state = ST_MENU;
                        } else if (current_state == ST_SPECTATE || current_state == ST_MULTI_GAME) {
                            /* Leave game/spectate mode */
                            net_close();
                            current_state = ST_MENU;
                            is_spectator = 0;
                        } else if (current_state != ST_MENU) {
                            net_close();
                            current_state = ST_MENU;
                        }
                    }
                    /* Arrow keys for spectator view switching in Rush */
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
                    break;
                
                case SDL_MOUSEBUTTONDOWN:
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        if (current_state == ST_SOLO) {
                            handle_game_mousedown(0);
                        } else if (current_state == ST_MULTI_GAME && !is_spectator) {
                            handle_game_mousedown(1);
                        }
                    }
                    /* Scroll wheel for server browser */
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
                        /* Dismiss popup first */
                        if (popup_msg[0]) {
                            popup_msg[0] = '\0';
                            break;
                        }
                        
                        /* Handle click based on state */
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
                                if (game.game_over) {
                                    current_state = ST_MENU;
                                } else {
                                    handle_game_click(0);
                                }
                                break;
                            case ST_MULTI_GAME:
                                if (is_spectator) {
                                    /* Spectators can click to switch view in Rush */
                                    if (current_lobby.game_mode == GAME_MODE_RUSH && rush_player_count > 0) {
                                        spectate_view_idx = (spectate_view_idx + 1) % rush_player_count;
                                    }
                                } else if (current_lobby.game_mode == GAME_MODE_RUSH) {
                                    /* Rush mode - handle game over click */
                                    if (rush_time_remaining <= 0) {
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
                                handle_server_browser_click();
                                break;
                            case ST_SPECTATE:
                                /* Quit button in spectate */
                                if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, WINDOW_H - 50, 180, 40)) {
                                    net_close();
                                    current_state = ST_MENU;
                                    is_spectator = 0;
                                }
                                /* Click to switch view in Rush spectate */
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
        
        /* Update delta time for animations */
        {
            Uint32 current_time = SDL_GetTicks();
            if (last_frame_time > 0) {
                delta_time = (current_time - last_frame_time) / 1000.0f;
                if (delta_time > 0.1f) delta_time = 0.1f;  /* Cap delta time */
            }
            last_frame_time = current_time;
        }
        
        /* Update Rush timer locally for smoother display */
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
        
        /* Clear screen */
        fill_rect(0, 0, WINDOW_W, WINDOW_H, COLOR_BG);
        
        /* Render current state */
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
        
        /* Draw popup on top */
        draw_popup();
        
        /* Update screen */
        SDL_Flip(screen);
        
        /* Cap to ~60 FPS */
        SDL_Delay(16);
    }
    
    /* Cleanup */
    net_close();
    if (font_L) TTF_CloseFont(font_L);
    if (font_S) TTF_CloseFont(font_S);
    if (font_XS) TTF_CloseFont(font_XS);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}

