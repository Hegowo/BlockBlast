#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "game.h"

/* Piece templates - 9 shapes */
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
    },
    /* Vertical I-3 (1x3) - Cyan */
    {
        .data = {{1, 0, 0, 0, 0},
                 {1, 0, 0, 0, 0},
                 {1, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0}},
        .w = 1, .h = 3,
        .color = 0x00FFFF
    },
    /* Vertical I-4 (1x4) - Orange */
    {
        .data = {{1, 0, 0, 0, 0},
                 {1, 0, 0, 0, 0},
                 {1, 0, 0, 0, 0},
                 {1, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0}},
        .w = 1, .h = 4,
        .color = 0xFF8C00
    },
    /* Horizontal I-4 (4x1) - Light blue */
    {
        .data = {{1, 1, 1, 1, 0},
                 {0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0}},
        .w = 4, .h = 1,
        .color = 0x6699FF
    },
    /* Vertical 2-block (1x2) - Light green */
    {
        .data = {{1, 0, 0, 0, 0},
                 {1, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0},
                 {0, 0, 0, 0, 0}},
        .w = 1, .h = 2,
        .color = 0x66FF66
    }
};

#define NUM_TEMPLATES 9

/* Visual effects implementation */

void init_effects(EffectsManager *em) {
    int i;
    memset(em, 0, sizeof(EffectsManager));
    
    for (i = 0; i < MAX_PARTICLES; i++) {
        em->particles[i].active = 0;
    }
    for (i = 0; i < 20; i++) {
        em->line_clears[i].active = 0;
        em->line_clears[i].row = -1;
        em->line_clears[i].col = -1;
    }
    for (i = 0; i < 25; i++) {
        em->place_effects[i].active = 0;
    }
    em->line_clear_count = 0;
    em->place_effect_count = 0;
    em->screen_shake = 0;
    em->shake_time = 0;
}

void update_effects(EffectsManager *em, float dt) {
    int i;
    
    /* Update particles */
    for (i = 0; i < MAX_PARTICLES; i++) {
        if (em->particles[i].active) {
            Particle *p = &em->particles[i];
            p->x += p->vx * dt;
            p->y += p->vy * dt;
            p->vy += 500.0f * dt;  /* Gravity */
            p->life -= dt * 2.0f;
            
            if (p->life <= 0) {
                p->active = 0;
            }
        }
    }
    
    /* Update line clear effects */
    for (i = 0; i < 20; i++) {
        if (em->line_clears[i].active) {
            em->line_clears[i].progress += dt * 3.0f;
            if (em->line_clears[i].progress >= 1.0f) {
                em->line_clears[i].active = 0;
                em->line_clear_count--;
            }
        }
    }
    
    /* Update place effects */
    for (i = 0; i < 25; i++) {
        if (em->place_effects[i].active) {
            PlaceEffect *pe = &em->place_effects[i];
            pe->scale += dt * 8.0f;
            if (pe->scale > 1.0f) pe->scale = 1.0f;
            pe->alpha -= dt * 3.0f;
            
            if (pe->alpha <= 0 || pe->scale >= 1.0f) {
                pe->active = 0;
                em->place_effect_count--;
            }
        }
    }
    
    /* Update screen shake */
    if (em->shake_time > 0) {
        em->shake_time -= dt;
        if (em->shake_time <= 0) {
            em->screen_shake = 0;
        }
    }
}

void spawn_place_effect(EffectsManager *em, int row, int col, int color) {
    int i;
    for (i = 0; i < 25; i++) {
        if (!em->place_effects[i].active) {
            em->place_effects[i].row = row;
            em->place_effects[i].col = col;
            em->place_effects[i].scale = 0.5f;
            em->place_effects[i].alpha = 1.0f;
            em->place_effects[i].color = color;
            em->place_effects[i].active = 1;
            em->place_effect_count++;
            break;
        }
    }
}

void spawn_line_clear_effect(EffectsManager *em, int row, int col, int is_row) {
    int i;
    for (i = 0; i < 20; i++) {
        if (!em->line_clears[i].active) {
            em->line_clears[i].row = is_row ? row : -1;
            em->line_clears[i].col = is_row ? -1 : col;
            em->line_clears[i].progress = 0.0f;
            em->line_clears[i].active = 1;
            em->line_clears[i].color = 0xFFFFFF;
            em->line_clear_count++;
            break;
        }
    }
}

void spawn_particles(EffectsManager *em, int x, int y, int color, int count) {
    int i, spawned = 0;
    
    for (i = 0; i < MAX_PARTICLES && spawned < count; i++) {
        if (!em->particles[i].active) {
            Particle *p = &em->particles[i];
            p->x = (float)x;
            p->y = (float)y;
            /* More dynamic particle spread for better visual effect */
            float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
            float speed = 100.0f + (float)(rand() % 250);
            p->vx = cosf(angle) * speed;
            p->vy = sinf(angle) * speed - 100.0f;  /* Bias upward */
            p->life = 0.6f + (float)(rand() % 40) / 100.0f;
            p->color = color;
            p->active = 1;
            p->size = 4 + (rand() % 6);
            spawned++;
        }
    }
}

/* Spawn burst of particles for celebration effect */
void spawn_celebration_particles(EffectsManager *em, int center_x, int center_y, int count) {
    static const int colors[] = {0xFF5555, 0x55FF55, 0x5555FF, 0xFFFF55, 0xFF55FF, 0x55FFFF, 0xFFAA00};
    int i, spawned = 0;
    
    for (i = 0; i < MAX_PARTICLES && spawned < count; i++) {
        if (!em->particles[i].active) {
            Particle *p = &em->particles[i];
            p->x = (float)center_x;
            p->y = (float)center_y;
            float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
            float speed = 150.0f + (float)(rand() % 300);
            p->vx = cosf(angle) * speed;
            p->vy = sinf(angle) * speed;
            p->life = 0.8f + (float)(rand() % 50) / 100.0f;
            p->color = colors[rand() % 7];
            p->active = 1;
            p->size = 5 + (rand() % 8);
            spawned++;
        }
    }
}

void trigger_screen_shake(EffectsManager *em, float intensity) {
    em->screen_shake = (int)(intensity * 10);
    em->shake_time = 0.2f;
}

void init_game(GameState *gs) {
    /* Initialize empty grid */
    memset(gs->grid, 0, sizeof(gs->grid));
    
    /* Reset score and game over flag */
    gs->score = 0;
    gs->game_over = 0;
    
    /* Initialize cleared line tracking */
    gs->num_cleared_rows = 0;
    gs->num_cleared_cols = 0;
    memset(gs->cleared_rows, 0, sizeof(gs->cleared_rows));
    memset(gs->cleared_cols, 0, sizeof(gs->cleared_cols));
    
    /* Initialize visual effects */
    init_effects(&gs->effects);
    
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
    int lines_cleared = 0;
    
    /* Reset cleared line tracking */
    gs->num_cleared_rows = 0;
    gs->num_cleared_cols = 0;
    
    /* Place piece on grid and spawn placement effects */
    for (i = 0; i < p->h; i++) {
        for (j = 0; j < p->w; j++) {
            if (p->data[i][j]) {
                gs->grid[row + i][col + j] = p->color;
                /* Spawn visual effect for each block placed */
                spawn_place_effect(&gs->effects, row + i, col + j, p->color);
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
            /* Track this row for effects */
            gs->cleared_rows[gs->num_cleared_rows++] = i;
            
            /* Spawn line clear effect */
            spawn_line_clear_effect(&gs->effects, i, -1, 1);
            
            /* Spawn particles for each cell in the row */
            for (j = 0; j < GRID_W; j++) {
                int px = GRID_OFFSET_X + j * BLOCK_SIZE + BLOCK_SIZE / 2;
                int py = GRID_OFFSET_Y + i * BLOCK_SIZE + BLOCK_SIZE / 2;
                spawn_particles(&gs->effects, px, py, gs->grid[i][j], 5);
            }
            
            /* Clear the row */
            for (j = 0; j < GRID_W; j++) {
                gs->grid[i][j] = 0;
            }
            gs->score += 100;
            lines_cleared++;
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
            /* Track this column for effects */
            gs->cleared_cols[gs->num_cleared_cols++] = j;
            
            /* Spawn line clear effect */
            spawn_line_clear_effect(&gs->effects, -1, j, 0);
            
            /* Spawn particles for each cell in the column */
            for (i = 0; i < GRID_H; i++) {
                int px = GRID_OFFSET_X + j * BLOCK_SIZE + BLOCK_SIZE / 2;
                int py = GRID_OFFSET_Y + i * BLOCK_SIZE + BLOCK_SIZE / 2;
                spawn_particles(&gs->effects, px, py, gs->grid[i][j], 5);
            }
            
            /* Clear the column */
            for (i = 0; i < GRID_H; i++) {
                gs->grid[i][j] = 0;
            }
            gs->score += 100;
            lines_cleared++;
        }
    }
    
    /* Trigger screen shake if lines were cleared */
    if (lines_cleared > 0) {
        trigger_screen_shake(&gs->effects, (float)lines_cleared * 0.5f);
        
        /* Spawn celebration particles for multiple lines cleared */
        if (lines_cleared >= 2) {
            int center_x = GRID_OFFSET_X + (GRID_W * BLOCK_SIZE) / 2;
            int center_y = GRID_OFFSET_Y + (GRID_H * BLOCK_SIZE) / 2;
            spawn_celebration_particles(&gs->effects, center_x, center_y, lines_cleared * 15);
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

