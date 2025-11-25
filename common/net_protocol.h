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
    MSG_ERROR,
    /* New message types for extended features */
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

/* Server info for server browser */
typedef struct {
    char room_code[6];       /* Room code */
    char host_name[32];      /* Host player name */
    int player_count;        /* Current players */
    int max_players;         /* Maximum players (4) */
    int game_started;        /* Game already started? */
    int game_mode;           /* GAME_MODE_CLASSIC or GAME_MODE_RUSH */
    int is_public;           /* Is room public/listed */
} ServerInfo;

/* Server list data structure */
typedef struct {
    ServerInfo servers[10];  /* Up to 10 servers */
    int count;               /* Number of servers */
} ServerListData;

/* Rush mode player state (for spectating) */
typedef struct {
    char pseudo[32];         /* Player name */
    int grid[GRID_H][GRID_W]; /* Player's grid */
    int score;               /* Player's score */
    int is_spectator;        /* Is this a spectator? */
} RushPlayerState;

/* Lobby state structure */
typedef struct {
    char room_code[6];       /* 4-char code + null terminator + padding */
    char players[4][32];     /* Up to 4 player names */
    int scores[4];           /* Player scores */
    int is_host;             /* 1 if this client is host */
    int player_count;        /* Current player count */
    int timer_minutes;       /* Game timer for Rush mode */
    int game_started;        /* Game running flag */
    int game_mode;           /* GAME_MODE_CLASSIC or GAME_MODE_RUSH */
    int is_public;           /* Is room public/listed */
    int spectator_count;     /* Number of spectators */
    int is_spectator[4];     /* Spectator flags for each slot */
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
    /* Extended fields for new features */
    ServerListData server_list;         /* Server browser data */
    int game_mode;                       /* Game mode */
    int timer_value;                     /* Timer in seconds */
    int time_remaining;                  /* Remaining time for Rush */
    int viewing_player_idx;              /* Index of player being viewed (spectator) */
    RushPlayerState rush_states[4];      /* Rush mode player states */
    int rush_player_count;               /* Number of players in Rush */
} Packet;
#pragma pack(pop)

#endif

