#ifndef GAME_H
#define GAME_H

#include "../common/config.h"

/* Piece structure - represents a game piece */
typedef struct {
    int data[5][5];   /* 5x5 grid for piece shape (1 = filled, 0 = empty) */
    int w, h;         /* Width and height of piece */
    int color;        /* RGB color (0xRRGGBB format) */
} Piece;

/* Game state structure */
typedef struct {
    int grid[GRID_H][GRID_W];    /* Main game grid */
    int score;                    /* Current score */
    int game_over;                /* Game over flag */
    Piece current_pieces[3];      /* Three available pieces */
    int pieces_available[3];      /* Availability flags for each piece */
} GameState;

/* Initialize game state */
void init_game(GameState *gs);

/* Generate 3 random pieces */
void generate_pieces(GameState *gs);

/* Check if piece can be placed at position (row, col) */
int can_place(GameState *gs, int row, int col, Piece *p);

/* Place piece on grid and handle scoring */
void place_piece_logic(GameState *gs, int row, int col, Piece *p);

/* Check if any valid moves exist */
int check_valid_moves_exist(GameState *gs);

#endif

