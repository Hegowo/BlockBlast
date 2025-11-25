#include <stdlib.h>
#include <string.h>
#include "game.h"

/* Piece templates - 5 basic shapes */
static Piece piece_templates[] = {
    /* Single block (1x1) - Red */
    {
        .data = {{1, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0}},
        .w = 1, .h = 1,
        .color = 0xFF0000
    },
    /* Horizontal 2-block (2x1) - Green */
    {
        .data = {{1, 1, 0, 0, 0},
                 {0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0}},
        .w = 2, .h = 1,
        .color = 0x00FF00
    },
    /* Horizontal 3-block (3x1) - Blue */
    {
        .data = {{1, 1, 1, 0, 0},
                 {0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0}},
        .w = 3, .h = 1,
        .color = 0x0000FF
    },
    /* 2x2 Square - Yellow */
    {
        .data = {{1, 1, 0, 0, 0},
                 {1, 1, 0, 0, 0},
                 {0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0}},
        .w = 2, .h = 2,
        .color = 0xFFFF00
    },
    /* T-shape (2x3) - Magenta */
    {
        .data = {{1, 0, 0, 0, 0},
                 {1, 1, 0, 0, 0},
                 {1, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0}},
        .w = 2, .h = 3,
        .color = 0xFF00FF
    }
};

#define NUM_TEMPLATES 5

void init_game(GameState *gs) {
    /* Initialize empty grid */
    memset(gs->grid, 0, sizeof(gs->grid));
    
    /* Reset score and game over flag */
    gs->score = 0;
    gs->game_over = 0;
    
    /* Generate initial pieces */
    generate_pieces(gs);
}

void generate_pieces(GameState *gs) {
    int i;
    for (i = 0; i < 3; i++) {
        int r = rand() % NUM_TEMPLATES;
        gs->current_pieces[i] = piece_templates[r];
        gs->pieces_available[i] = 1;
    }
}

int can_place(GameState *gs, int row, int col, Piece *p) {
    int i, j;
    
    for (i = 0; i < p->h; i++) {
        for (j = 0; j < p->w; j++) {
            if (p->data[i][j]) {
                int gr = row + i;
                int gc = col + j;
                
                /* Check bounds */
                if (gr < 0 || gr >= GRID_H || gc < 0 || gc >= GRID_W) {
                    return 0;
                }
                
                /* Check for overlap */
                if (gs->grid[gr][gc] != 0) {
                    return 0;
                }
            }
        }
    }
    
    return 1;
}

void place_piece_logic(GameState *gs, int row, int col, Piece *p) {
    int i, j;
    
    /* Place piece on grid */
    for (i = 0; i < p->h; i++) {
        for (j = 0; j < p->w; j++) {
            if (p->data[i][j]) {
                gs->grid[row + i][col + j] = p->color;
            }
        }
    }
    
    /* Add base points for placing piece */
    gs->score += 10;
    
    /* Check for complete rows */
    for (i = 0; i < GRID_H; i++) {
        int full = 1;
        for (j = 0; j < GRID_W; j++) {
            if (gs->grid[i][j] == 0) {
                full = 0;
                break;
            }
        }
        if (full) {
            /* Clear the row */
            for (j = 0; j < GRID_W; j++) {
                gs->grid[i][j] = 0;
            }
            gs->score += 100;
        }
    }
    
    /* Check for complete columns */
    for (j = 0; j < GRID_W; j++) {
        int full = 1;
        for (i = 0; i < GRID_H; i++) {
            if (gs->grid[i][j] == 0) {
                full = 0;
                break;
            }
        }
        if (full) {
            /* Clear the column */
            for (i = 0; i < GRID_H; i++) {
                gs->grid[i][j] = 0;
            }
            gs->score += 100;
        }
    }
}

int check_valid_moves_exist(GameState *gs) {
    int i, r, c;
    
    for (i = 0; i < 3; i++) {
        if (!gs->pieces_available[i]) {
            continue;
        }
        
        Piece *p = &gs->current_pieces[i];
        
        for (r = 0; r < GRID_H; r++) {
            for (c = 0; c < GRID_W; c++) {
                if (can_place(gs, r, c, p)) {
                    return 1;
                }
            }
        }
    }
    
    return 0;
}

