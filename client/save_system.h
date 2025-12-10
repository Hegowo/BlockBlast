#ifndef SAVE_SYSTEM_H
#define SAVE_SYSTEM_H

#include "globals.h"

typedef struct {
    unsigned int magic;
    int high_score;
    int music_vol;
    int sfx_vol;
    char server_ip[32];
    int server_port;
    unsigned int checksum;
} SaveData;

typedef struct {
    unsigned int magic;
    int grid[GRID_H][GRID_W];
    int score;
    int piece_data[3][5][5];
    int piece_w[3];
    int piece_h[3];
    int piece_color[3];
    int pieces_available[3];
    unsigned int checksum;
} GameSaveData;

void save_game_data(void);
void load_game_data(void);

void save_current_game(void);
int load_current_game(void);
void delete_saved_game(void);
void check_saved_game_exists(void);

#endif
