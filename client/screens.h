#ifndef SCREENS_H
#define SCREENS_H

#include "game.h"

void render_menu(void);
void render_options(void);
void render_login(void);
void render_multi_choice(void);
void render_join_input(void);
void render_server_browser(void);
void render_lobby(void);

void render_solo(void);
void render_multi_game(void);
void render_rush_game(void);
void render_spectate(void);

void render_game_grid(void);
void render_game_grid_ex(GameState *gs, int offset_x, int offset_y);
void render_mini_grid(int grid[GRID_H][GRID_W], int x, int y, int block_size, const char *label, int score, int is_selected);

void render_pieces(int greyed);
void render_dragged_piece(void);

#endif
