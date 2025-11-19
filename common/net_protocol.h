#ifndef NET_PROTOCOL_H
#define NET_PROTOCOL_H

#include <stdint.h>
#include "config.h"

#pragma pack(push, 1)

typedef enum {
    MSG_LOGIN = 1,
    MSG_LEADERBOARD_REQ,
    MSG_LEADERBOARD_REP,
    MSG_CREATE_ROOM,
    MSG_JOIN_ROOM,
    MSG_ROOM_UPDATE,
    MSG_KICK_PLAYER,     // Hôte demande exclusion
    MSG_KICKED,          // Serveur dit au joueur "T'es viré"
    MSG_START_GAME,
    MSG_UPDATE_GRID,
    MSG_PLACE_PIECE,
    MSG_GAME_OVER,
    MSG_GAME_CANCELLED,  // Si un joueur part
    MSG_ERROR
} MsgType;

typedef struct {
    char room_code[6];
    char players[4][32];
    int scores[4];
    int is_host;
    int player_count;
    int timer_minutes;
    int game_started;
} LobbyState;

typedef struct {
    char names[5][32];
    int scores[5];
    int count;
} LeaderboardData;

typedef struct {
    int type;
    int client_id;
    char text[64];
    
    int grid_data[GRID_H][GRID_W]; 
    int score;
    char turn_pseudo[32]; // Pseudo du joueur dont c'est le tour
    
    LobbyState lobby;
    LeaderboardData lb;
} Packet;

#pragma pack(pop)

#endif