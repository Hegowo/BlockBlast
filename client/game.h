#ifndef GAME_H
#define GAME_H

#include "../common/config.h"

/* Piece structure - represents a game piece */
typedef struct {
    int data[5][5];   /* 5x5 grid for piece shape (1 = filled, 0 = empty) */
    int w, h;         /* Width and height of piece */
    int color;        /* RGB color (0xRRGGBB format) */
} Piece;

/* Visual effect particle */
typedef struct {
    float x, y;       /* Position */
    float vx, vy;     /* Velocity */
    float life;       /* Remaining life (0-1) */
    int color;        /* Particle color */
    int active;       /* Is particle active */
    int size;         /* Particle size */
} Particle;

/* Line clear effect */
typedef struct {
    int row;          /* Row being cleared (-1 if none) */
    int col;          /* Column being cleared (-1 if none) */
    float progress;   /* Animation progress (0-1) */
    int active;       /* Is effect active */
    int color;        /* Flash color */
} LineClearEffect;

/* Block placement effect */
typedef struct {
    int row, col;     /* Grid position */
    float scale;      /* Current scale (for pop effect) */
    float alpha;      /* Current alpha */
    int color;        /* Block color */
    int active;       /* Is effect active */
} PlaceEffect;

/* Visual effects manager */
typedef struct {
    Particle particles[MAX_PARTICLES];
    LineClearEffect line_clears[20];  /* Up to 20 simultaneous line clears */
    PlaceEffect place_effects[25];    /* Up to 25 block placements */
    int line_clear_count;
    int place_effect_count;
    int screen_shake;                 /* Screen shake intensity */
    float shake_time;                 /* Remaining shake time */
} EffectsManager;

/* Game state structure */
typedef struct {
    int grid[GRID_H][GRID_W];    /* Main game grid */
    int score;                    /* Current score */
    int game_over;                /* Game over flag */
    Piece current_pieces[3];      /* Three available pieces */
    int pieces_available[3];      /* Availability flags for each piece */
    EffectsManager effects;       /* Visual effects */
    int cleared_rows[GRID_H];     /* Track cleared rows for effects */
    int cleared_cols[GRID_W];     /* Track cleared columns for effects */
    int num_cleared_rows;         /* Number of rows cleared this turn */
    int num_cleared_cols;         /* Number of columns cleared this turn */
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

/* Visual effects functions */
void init_effects(EffectsManager *em);
void update_effects(EffectsManager *em, float dt);
void spawn_place_effect(EffectsManager *em, int row, int col, int color);
void spawn_line_clear_effect(EffectsManager *em, int row, int col, int is_row);
void spawn_particles(EffectsManager *em, int x, int y, int color, int count);
void spawn_celebration_particles(EffectsManager *em, int center_x, int center_y, int count);
void trigger_screen_shake(EffectsManager *em, float intensity);

#endif

