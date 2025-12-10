#include "globals.h"
#include <string.h>

float global_time = 0.0f;
float glow_pulse = 0.0f;
float delta_time = 0.016f;
Uint32 last_frame_time = 0;

int window_w = WINDOW_W;
int window_h = WINDOW_H;
int layout_horizontal = 0;
int grid_offset_x = 0;
int grid_offset_y = 0;
int piece_area_x = 0;
int piece_area_y = 0;
int score_area_x = 0;
int score_area_y = 0;
int block_size = BLOCK_SIZE;

SDL_Surface *screen = NULL;
TTF_Font *font_L = NULL;
TTF_Font *font_S = NULL;
TTF_Font *font_XS = NULL;

GameState game;
int current_state = ST_MENU;
int mouse_x = 0, mouse_y = 0;
int selected_piece_idx = -1;
int solo_high_score = 0;

char online_ip[32] = "127.0.0.1";
int online_port = PORT;
char edit_ip[32] = "127.0.0.1";
char edit_port[16] = "5000";
int active_input = 0;
int test_result = 0;

char my_pseudo[32] = "";
char current_turn_pseudo[32] = "";
char input_buffer[32] = "";
char popup_msg[128] = "";
LobbyState current_lobby;
LeaderboardData leaderboard;
ServerListData server_list;
int browser_scroll_offset = 0;
int selected_game_mode = GAME_MODE_CLASSIC;
int selected_timer_minutes = 3;
int is_spectator = 0;
int spectate_view_idx = 0;

RushPlayerState rush_states[4];
int rush_player_count = 0;
int rush_time_remaining = 0;
Uint32 last_time_update = 0;

int multi_game_over = 0;
char multi_winner_name[32] = "";

int music_volume = 80;
int sfx_volume = 100;
int audio_enabled = 0;

int show_settings_overlay = 0;
int dragging_music_slider = 0;
int dragging_sfx_slider = 0;
int show_pause_menu = 0;
int has_saved_game = 0;
int settings_tab = 0;

void recalculate_layout(void) {
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

int is_my_turn(void) {
    return strcmp(my_pseudo, current_turn_pseudo) == 0;
}
