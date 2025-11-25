#ifndef NET_PROTOCOL_H
#define NET_PROTOCOL_H

#include "config.h"

/* Message types for network communication */
typedef enum {
    MSG_LOGIN = 1,
    MSG_LEADERBOARD_REQ,
    MSG_LEADERBOARD_REP,
    MSG_CREATE_ROOM,
    MSG_JOIN_ROOM,
    MSG_ROOM_UPDATE,
    MSG_KICK_PLAYER,
    MSG_KICKED,
    MSG_START_GAME,
    MSG_UPDATE_GRID,
    MSG_PLACE_PIECE,
    MSG_GAME_OVER,
    MSG_GAME_CANCELLED,
    MSG_ERROR
} MsgType;

/* Lobby state structure */
typedef struct {
    char room_code[6];       /* 4-char code + null terminator + padding */
    char players[4][32];     /* Up to 4 player names */
    int scores[4];           /* Player scores */
    int is_host;             /* 1 if this client is host */
    int player_count;        /* Current player count */
    int timer_minutes;       /* Game timer (future use) */
    int game_started;        /* Game running flag */
} LobbyState;

/* Leaderboard data structure */
typedef struct {
    char names[5][32];       /* Top 5 player names */
    int scores[5];           /* Top 5 scores */
    int count;               /* Number of entries */
} LeaderboardData;

/* Network packet structure - packed to ensure no padding */
#pragma pack(push, 1)
typedef struct {
    int type;                           /* MsgType value */
    int client_id;                      /* Client identifier */
    char text[64];                      /* General text field */
    int grid_data[GRID_H][GRID_W];      /* Grid state */
    int score;                          /* Score value */
    char turn_pseudo[32];               /* Current turn player name */
    LobbyState lobby;                   /* Lobby information */
    LeaderboardData lb;                 /* Leaderboard data */
} Packet;
#pragma pack(pop)

#endif

