#ifndef NET_PROTOCOL_H
#define NET_PROTOCOL_H

#include "config.h"

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
    MSG_ERROR,
    MSG_SERVER_LIST_REQ,
    MSG_SERVER_LIST_REP,
    MSG_JOIN_SPECTATE,
    MSG_SPECTATE_UPDATE,
    MSG_SET_GAME_MODE,
    MSG_SET_TIMER,
    MSG_RUSH_UPDATE,
    MSG_SWITCH_VIEW,
    MSG_TIME_SYNC,
    MSG_GAME_END
} MsgType;

typedef struct {
    char room_code[6];
    char host_name[32];
    int player_count;
    int max_players;
    int game_started;
    int game_mode;
    int is_public;
} ServerInfo;

typedef struct {
    ServerInfo servers[10];
    int count;
} ServerListData;

typedef struct {
    char pseudo[32];
    int grid[GRID_H][GRID_W];
    int score;
    int is_spectator;
} RushPlayerState;

typedef struct {
    char room_code[6];
    char players[4][32];
    int scores[4];
    int is_host;
    int player_count;
    int timer_minutes;
    int game_started;
    int game_mode;
    int is_public;
    int spectator_count;
    int is_spectator[4];
} LobbyState;

typedef struct {
    char names[5][32];
    int scores[5];
    int count;
} LeaderboardData;

#pragma pack(push, 1)
typedef struct {
    int type;
    int client_id;
    char text[64];
    int grid_data[GRID_H][GRID_W];
    int score;
    char turn_pseudo[32];
    LobbyState lobby;
    LeaderboardData lb;
    ServerListData server_list;
    int game_mode;
    int timer_value;
    int time_remaining;
    int viewing_player_idx;
    RushPlayerState rush_states[4];
    int rush_player_count;
} Packet;
#pragma pack(pop)

#endif
