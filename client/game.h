#ifndef GAME_H
#define GAME_H

#include "../common/config.h"

typedef struct {
    int data[5][5];
    int w, h;
    int color;
} Piece;

typedef struct {
    int grid[GRID_H][GRID_W];
    int score;
    int game_over;
    Piece current_pieces[3];
    int pieces_available[3];
} GameState;

void init_game(GameState *gs);
int can_place(GameState *gs, int px, int py, Piece *p);
void place_piece_logic(GameState *gs, int px, int py, Piece *p);
void generate_pieces(GameState *gs);
int check_valid_moves_exist(GameState *gs);

#endif