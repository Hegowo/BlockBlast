#ifndef GLOBALS_H
#define GLOBALS_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include "game.h"
#include "../common/config.h"
#include "../common/net_protocol.h"

enum GameScreenState {
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

#define SAVE_FILE "data.arthur"
#define SAVE_MAGIC 0xBB5A7E01
#define SAVE_KEY "N30N_BL0CK_BL4ST_K3Y_2024!"
#define GAME_SAVE_FILE "game_session.arthur"
#define GAME_SAVE_MAGIC 0xBB5E5510

#define DEFAULT_PIECE_AREA_Y 680
#define PIECE_SLOT_W 160
#define PIECE_SLOT_H 200

extern float global_time;
extern float glow_pulse;
extern float delta_time;
extern Uint32 last_frame_time;

extern int window_w;
extern int window_h;
extern int layout_horizontal;
extern int grid_offset_x;
extern int grid_offset_y;
extern int piece_area_x;
extern int piece_area_y;
extern int score_area_x;
extern int score_area_y;
extern int block_size;

extern SDL_Surface *screen;
extern TTF_Font *font_L;
extern TTF_Font *font_S;
extern TTF_Font *font_XS;

extern GameState game;
extern int current_state;
extern int mouse_x, mouse_y;
extern int selected_piece_idx;
extern int solo_high_score;

extern char online_ip[32];
extern int online_port;
extern char edit_ip[32];
extern char edit_port[16];
extern int active_input;
extern int test_result;

extern char my_pseudo[32];
extern char current_turn_pseudo[32];
extern char input_buffer[32];
extern char popup_msg[128];
extern LobbyState current_lobby;
extern LeaderboardData leaderboard;
extern ServerListData server_list;
extern int browser_scroll_offset;
extern int selected_game_mode;
extern int selected_timer_minutes;
extern int is_spectator;
extern int spectate_view_idx;

extern RushPlayerState rush_states[4];
extern int rush_player_count;
extern int rush_time_remaining;
extern Uint32 last_time_update;

extern int music_volume;
extern int sfx_volume;
extern int audio_enabled;

extern int show_settings_overlay;
extern int dragging_music_slider;
extern int dragging_sfx_slider;
extern int show_pause_menu;
extern int has_saved_game;
extern int settings_tab;

void recalculate_layout(void);
int is_my_turn(void);

#endif
