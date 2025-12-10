#include "save_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int calculate_checksum(SaveData *data) {
    unsigned char *ptr = (unsigned char *)data;
    unsigned int sum = 0;
    size_t i;
    size_t len = sizeof(SaveData) - sizeof(unsigned int);
    for (i = 0; i < len; i++) {
        sum = ((sum << 5) + sum) + ptr[i];
    }
    return sum ^ 0xDEADBEEF;
}

static unsigned int calculate_game_checksum(GameSaveData *data) {
    unsigned char *ptr = (unsigned char *)data;
    unsigned int sum = 0;
    size_t i;
    size_t len = sizeof(GameSaveData) - sizeof(unsigned int);
    for (i = 0; i < len; i++) {
        sum = ((sum << 5) + sum) + ptr[i];
    }
    return sum ^ 0xCAFEBABE;
}

static void encrypt_data(unsigned char *data, size_t len) {
    const char *key = SAVE_KEY;
    size_t key_len = strlen(key);
    size_t i;
    for (i = 0; i < len; i++) {
        data[i] ^= key[i % key_len];
        data[i] = (unsigned char)((data[i] << 3) | (data[i] >> 5));
        data[i] ^= (unsigned char)(i * 17);
    }
}

static void decrypt_data(unsigned char *data, size_t len) {
    const char *key = SAVE_KEY;
    size_t key_len = strlen(key);
    size_t i;
    for (i = 0; i < len; i++) {
        data[i] ^= (unsigned char)(i * 17);
        data[i] = (unsigned char)((data[i] >> 3) | (data[i] << 5));
        data[i] ^= key[i % key_len];
    }
}

void save_game_data(void) {
    FILE *f;
    SaveData data;
    unsigned char *raw;
    
    memset(&data, 0, sizeof(data));
    data.magic = SAVE_MAGIC;
    data.high_score = solo_high_score;
    data.music_vol = music_volume;
    data.sfx_vol = sfx_volume;
    strncpy(data.server_ip, online_ip, 31);
    data.server_ip[31] = '\0';
    data.server_port = online_port;
    data.checksum = calculate_checksum(&data);
    
    raw = (unsigned char *)&data;
    encrypt_data(raw, sizeof(SaveData));
    
    f = fopen(SAVE_FILE, "wb");
    if (f) {
        fwrite(&data, sizeof(SaveData), 1, f);
        fclose(f);
    }
}

void load_game_data(void) {
    FILE *f;
    SaveData data;
    unsigned char *raw;
    unsigned int expected_checksum;
    
    f = fopen(SAVE_FILE, "rb");
    if (!f) {
        printf("Save file not found, using defaults.\n");
        return;
    }
    
    if (fread(&data, sizeof(SaveData), 1, f) != 1) {
        printf("Save file corrupted (read error), using defaults.\n");
        fclose(f);
        return;
    }
    fclose(f);
    
    raw = (unsigned char *)&data;
    decrypt_data(raw, sizeof(SaveData));
    
    if (data.magic != SAVE_MAGIC) {
        printf("Save file corrupted (bad magic), using defaults.\n");
        return;
    }
    
    expected_checksum = data.checksum;
    data.checksum = 0;
    data.checksum = calculate_checksum(&data);
    
    if (data.checksum != expected_checksum) {
        printf("Save file corrupted (bad checksum), using defaults.\n");
        return;
    }
    
    solo_high_score = data.high_score;
    music_volume = data.music_vol;
    sfx_volume = data.sfx_vol;
    strncpy(online_ip, data.server_ip, 31);
    online_ip[31] = '\0';
    online_port = data.server_port;
    
    if (music_volume < 0) music_volume = 0;
    if (music_volume > 128) music_volume = 128;
    if (sfx_volume < 0) sfx_volume = 0;
    if (sfx_volume > 128) sfx_volume = 128;
    if (online_port <= 0 || online_port > 65535) online_port = PORT;
    
    printf("Save data loaded: score=%d, music=%d, sfx=%d, ip=%s, port=%d\n",
           solo_high_score, music_volume, sfx_volume, online_ip, online_port);
}

void save_current_game(void) {
    FILE *f;
    GameSaveData data;
    unsigned char *raw;
    int i, j, k;
    
    memset(&data, 0, sizeof(data));
    data.magic = GAME_SAVE_MAGIC;
    data.score = game.score;
    
    for (i = 0; i < GRID_H; i++) {
        for (j = 0; j < GRID_W; j++) {
            data.grid[i][j] = game.grid[i][j];
        }
    }
    
    for (i = 0; i < 3; i++) {
        data.piece_w[i] = game.current_pieces[i].w;
        data.piece_h[i] = game.current_pieces[i].h;
        data.piece_color[i] = game.current_pieces[i].color;
        data.pieces_available[i] = game.pieces_available[i];
        for (j = 0; j < 5; j++) {
            for (k = 0; k < 5; k++) {
                data.piece_data[i][j][k] = game.current_pieces[i].data[j][k];
            }
        }
    }
    
    data.checksum = calculate_game_checksum(&data);
    
    raw = (unsigned char *)&data;
    encrypt_data(raw, sizeof(GameSaveData));
    
    f = fopen(GAME_SAVE_FILE, "wb");
    if (f) {
        fwrite(&data, sizeof(GameSaveData), 1, f);
        fclose(f);
        has_saved_game = 1;
    }
}

int load_current_game(void) {
    FILE *f;
    GameSaveData data;
    unsigned char *raw;
    unsigned int expected_checksum;
    int i, j, k;
    
    f = fopen(GAME_SAVE_FILE, "rb");
    if (!f) {
        return 0;
    }
    
    if (fread(&data, sizeof(GameSaveData), 1, f) != 1) {
        fclose(f);
        return 0;
    }
    fclose(f);
    
    raw = (unsigned char *)&data;
    decrypt_data(raw, sizeof(GameSaveData));
    
    if (data.magic != GAME_SAVE_MAGIC) {
        return 0;
    }
    
    expected_checksum = data.checksum;
    data.checksum = 0;
    data.checksum = calculate_game_checksum(&data);
    
    if (data.checksum != expected_checksum) {
        return 0;
    }
    
    game.score = data.score;
    game.game_over = 0;
    init_effects(&game.effects);
    
    for (i = 0; i < GRID_H; i++) {
        for (j = 0; j < GRID_W; j++) {
            game.grid[i][j] = data.grid[i][j];
        }
    }
    
    for (i = 0; i < 3; i++) {
        game.current_pieces[i].w = data.piece_w[i];
        game.current_pieces[i].h = data.piece_h[i];
        game.current_pieces[i].color = data.piece_color[i];
        game.pieces_available[i] = data.pieces_available[i];
        for (j = 0; j < 5; j++) {
            for (k = 0; k < 5; k++) {
                game.current_pieces[i].data[j][k] = data.piece_data[i][j][k];
            }
        }
    }
    
    return 1;
}

void delete_saved_game(void) {
    remove(GAME_SAVE_FILE);
    has_saved_game = 0;
}

void check_saved_game_exists(void) {
    FILE *f = fopen(GAME_SAVE_FILE, "rb");
    if (f) {
        has_saved_game = 1;
        fclose(f);
    } else {
        has_saved_game = 0;
    }
}
