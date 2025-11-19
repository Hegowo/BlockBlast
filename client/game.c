#include <stdlib.h>
#include <string.h>
#include "game.h"

Piece shapes_templates[] = {
    { {{1}}, 1, 1, 0xFF0000 },
    { {{1,1}}, 2, 1, 0x00FF00 },
    { {{1,1,1}}, 3, 1, 0x0000FF },
    { {{1,1},{1,1}}, 2, 2, 0xFFFF00 },
    { {{1,0},{1,1},{1,0}}, 2, 3, 0xFF00FF }
};

void init_game(GameState *gs) {
    memset(gs->grid, 0, sizeof(gs->grid));
    gs->score = 0;
    gs->game_over = 0;
    generate_pieces(gs);
}

void generate_pieces(GameState *gs) {
    for(int i=0; i<3; i++) {
        int r = rand() % 5;
        gs->current_pieces[i] = shapes_templates[r];
        gs->pieces_available[i] = 1;
    }
}

int can_place(GameState *gs, int r, int c, Piece *p) {
    for(int i=0; i < p->h; i++) {
        for(int j=0; j < p->w; j++) {
            if (p->data[i][j]) {
                int gr = r + i;
                int gc = c + j;
                if (gr < 0 || gr >= GRID_H || gc < 0 || gc >= GRID_W) return 0;
                if (gs->grid[gr][gc] != 0) return 0;
            }
        }
    }
    return 1;
}

void place_piece_logic(GameState *gs, int r, int c, Piece *p) {
    for(int i=0; i < p->h; i++) {
        for(int j=0; j < p->w; j++) {
            if (p->data[i][j]) {
                gs->grid[r+i][c+j] = p->color;
            }
        }
    }
    gs->score += 10;

    for(int i=0; i<GRID_H; i++) {
        int full = 1;
        for(int j=0; j<GRID_W; j++) if(gs->grid[i][j] == 0) full = 0;
        if(full) {
            for(int j=0; j<GRID_W; j++) gs->grid[i][j] = 0;
            gs->score += 100;
        }
    }
}

int check_valid_moves_exist(GameState *gs) {
    for(int i=0; i<3; i++) {
        if(!gs->pieces_available[i]) continue;
        Piece *p = &gs->current_pieces[i];
        for(int r=0; r<GRID_H; r++) {
            for(int c=0; c<GRID_W; c++) {
                if(can_place(gs, r, c, p)) return 1;
            }
        }
    }
    return 0; 
}