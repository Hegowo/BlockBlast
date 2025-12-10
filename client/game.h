#ifndef GAME_H
#define GAME_H

#include "../common/config.h"

typedef struct {
    int data[5][5];
    int w, h;
    int color;
} Piece;

typedef struct {
    float x, y;
    float vx, vy;
    float life;
    int color;
    int active;
    int size;
} Particle;

typedef struct {
    int row;
    int col;
    float progress;
    int active;
    int color;
} LineClearEffect;

typedef struct {
    int row, col;
    float scale;
    float alpha;
    int color;
    int active;
} PlaceEffect;

typedef struct {
    Particle particles[MAX_PARTICLES];
    LineClearEffect line_clears[20];
    PlaceEffect place_effects[25];
    int line_clear_count;
    int place_effect_count;
    int screen_shake;
    float shake_time;
} EffectsManager;

typedef struct {
    int grid[GRID_H][GRID_W];
    int score;
    int game_over;
    Piece current_pieces[3];
    int pieces_available[3];
    EffectsManager effects;
    int cleared_rows[GRID_H];
    int cleared_cols[GRID_W];
    int num_cleared_rows;
    int num_cleared_cols;
} GameState;

void init_game(GameState *gs);

void generate_pieces(GameState *gs);

int can_place(GameState *gs, int row, int col, Piece *p);

void place_piece_logic(GameState *gs, int row, int col, Piece *p);

int check_valid_moves_exist(GameState *gs);

void init_effects(EffectsManager *em);
void update_effects(EffectsManager *em, float dt);
void spawn_place_effect(EffectsManager *em, int row, int col, int color);
void spawn_line_clear_effect(EffectsManager *em, int row, int col, int is_row);
void spawn_particles(EffectsManager *em, int x, int y, int color, int count);
void spawn_celebration_particles(EffectsManager *em, int center_x, int center_y, int count);
void trigger_screen_shake(EffectsManager *em, float intensity);

#endif

