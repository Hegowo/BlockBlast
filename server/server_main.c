#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <fcntl.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
    typedef int SOCKET;
#endif

#include "../common/config.h"
#include "../common/net_protocol.h"

#define MAX_CLIENTS 20
#define MAX_ROOMS 10
#define LEADERBOARD_FILE "leaderboard.txt"

/* Room structure */
typedef struct {
    char code[6];               /* 4-char room code */
    int host_id;                /* Client ID of host */
    int client_ids[4];          /* Up to 4 players */
    int count;                  /* Current player count */
    int active;                 /* Room is in use */
    int game_running;           /* Game started flag */
    int timer_minutes;          /* Timer for Rush mode */
    int current_turn;           /* Index of current player (0-3) */
    int grid[GRID_H][GRID_W];   /* Shared game grid (Classic mode) */
    /* New fields for extended features */
    int game_mode;              /* GAME_MODE_CLASSIC or GAME_MODE_RUSH */
    int is_public;              /* Is room visible in server browser */
    int is_spectator[4];        /* Spectator flags for each slot */
    int spectator_count;        /* Number of spectators */
    /* Rush mode specific */
    int rush_grids[4][GRID_H][GRID_W];  /* Individual grids for Rush */
    int rush_scores[4];         /* Individual scores for Rush */
    time_t rush_start_time;     /* When Rush game started */
    int rush_duration;          /* Rush duration in seconds */
} Room;

/* Client structure */
typedef struct {
    SOCKET socket;
    char pseudo[32];
    int active;                 /* Connection active */
    int room_idx;               /* Index in rooms array (-1 if not in room) */
} Client;

/* Global arrays */
static Client clients[MAX_CLIENTS];
static Room rooms[MAX_ROOMS];

/* Forward declarations */
static void send_to_client(int client_idx, Packet *pkt);
static void broadcast_to_room(int room_idx, Packet *pkt);
static void save_score(const char *name, int score);
static void send_rush_update(int room_idx);
static void send_room_update(int room_idx);

/* Leaderboard entry for sorting */
typedef struct {
    char name[32];
    int score;
} LeaderboardEntry;

/* Generate random room code */
static void generate_code(char *dest) {
    static const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int i;
    
    for (i = 0; i < 4; i++) {
        dest[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    dest[4] = '\0';
}

/* Save score to leaderboard file */
static void save_score(const char *name, int score) {
    FILE *f;
    LeaderboardEntry entries[100];
    int count = 0;
    int i, found = 0;
    char line[128];
    
    /* Read existing entries */
    f = fopen(LEADERBOARD_FILE, "r");
    if (f) {
        while (fgets(line, sizeof(line), f) && count < 100) {
            if (sscanf(line, "%31s %d", entries[count].name, &entries[count].score) == 2) {
                count++;
            }
        }
        fclose(f);
    }
    
    /* Update or add entry */
    for (i = 0; i < count; i++) {
        if (strcmp(entries[i].name, name) == 0) {
            if (score > entries[i].score) {
                entries[i].score = score;
            }
            found = 1;
            break;
        }
    }
    
    if (!found && count < 100) {
        strcpy(entries[count].name, name);
        entries[count].score = score;
        count++;
    }
    
    /* Write back to file */
    f = fopen(LEADERBOARD_FILE, "w");
    if (f) {
        for (i = 0; i < count; i++) {
            fprintf(f, "%s %d\n", entries[i].name, entries[i].score);
        }
        fclose(f);
    }
}

/* Get leaderboard (sorted top 5) */
static void get_leaderboard(LeaderboardData *lb) {
    FILE *f;
    LeaderboardEntry entries[100];
    int count = 0;
    int i, j;
    char line[128];
    LeaderboardEntry temp;
    
    memset(lb, 0, sizeof(LeaderboardData));
    
    /* Read entries */
    f = fopen(LEADERBOARD_FILE, "r");
    if (f) {
        while (fgets(line, sizeof(line), f) && count < 100) {
            if (sscanf(line, "%31s %d", entries[count].name, &entries[count].score) == 2) {
                count++;
            }
        }
        fclose(f);
    }
    
    /* Bubble sort descending by score */
    for (i = 0; i < count - 1; i++) {
        for (j = 0; j < count - i - 1; j++) {
            if (entries[j].score < entries[j + 1].score) {
                temp = entries[j];
                entries[j] = entries[j + 1];
                entries[j + 1] = temp;
            }
        }
    }
    
    /* Copy top 5 */
    lb->count = count < 5 ? count : 5;
    for (i = 0; i < lb->count; i++) {
        strcpy(lb->names[i], entries[i].name);
        lb->scores[i] = entries[i].score;
    }
}

/* Send packet to a client */
static void send_to_client(int client_idx, Packet *pkt) {
    if (client_idx >= 0 && client_idx < MAX_CLIENTS && clients[client_idx].active) {
        send(clients[client_idx].socket, (const char *)pkt, sizeof(Packet), 0);
    }
}

/* Send room update to all players in room */
static void send_room_update(int room_idx) {
    Packet pkt;
    int i, j, c;
    Room *room;
    
    if (room_idx < 0 || room_idx >= MAX_ROOMS || !rooms[room_idx].active) {
        return;
    }
    
    room = &rooms[room_idx];
    
    for (i = 0; i < room->count; i++) {
        c = room->client_ids[i];
        
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_ROOM_UPDATE;
        
        /* Copy room info */
        strcpy(pkt.lobby.room_code, room->code);
        pkt.lobby.player_count = room->count;
        pkt.lobby.game_started = room->game_running;
        pkt.lobby.timer_minutes = room->timer_minutes;
        pkt.lobby.game_mode = room->game_mode;
        pkt.lobby.is_public = room->is_public;
        pkt.lobby.spectator_count = room->spectator_count;
        
        /* Set host flag for this client */
        pkt.lobby.is_host = (c == room->host_id) ? 1 : 0;
        
        /* Copy player names and spectator flags */
        for (j = 0; j < room->count; j++) {
            strcpy(pkt.lobby.players[j], clients[room->client_ids[j]].pseudo);
            pkt.lobby.is_spectator[j] = room->is_spectator[j];
        }
        
        send_to_client(c, &pkt);
    }
}

/* Build server list for browser */
static void build_server_list(ServerListData *list) {
    int i, count = 0;
    
    memset(list, 0, sizeof(ServerListData));
    
    for (i = 0; i < MAX_ROOMS && count < 10; i++) {
        if (rooms[i].active && rooms[i].is_public) {
            ServerInfo *srv = &list->servers[count];
            strcpy(srv->room_code, rooms[i].code);
            strcpy(srv->host_name, clients[rooms[i].host_id].pseudo);
            srv->player_count = rooms[i].count - rooms[i].spectator_count;
            srv->max_players = 4;
            srv->game_started = rooms[i].game_running;
            srv->game_mode = rooms[i].game_mode;
            srv->is_public = 1;
            count++;
        }
    }
    
    list->count = count;
}

/* Send Rush mode update to all players */
static void send_rush_update(int room_idx) {
    Packet pkt;
    int i, j;
    Room *room;
    
    if (room_idx < 0 || room_idx >= MAX_ROOMS || !rooms[room_idx].active) {
        return;
    }
    
    room = &rooms[room_idx];
    
    /* Calculate remaining time */
    time_t now = time(NULL);
    int elapsed = (int)(now - room->rush_start_time);
    int remaining = room->rush_duration - elapsed;
    if (remaining < 0) remaining = 0;
    
    memset(&pkt, 0, sizeof(pkt));
    pkt.type = MSG_RUSH_UPDATE;
    pkt.time_remaining = remaining;
    pkt.rush_player_count = 0;
    
    /* Copy all player states */
    for (i = 0; i < room->count; i++) {
        if (!room->is_spectator[i]) {
            RushPlayerState *state = &pkt.rush_states[pkt.rush_player_count];
            strcpy(state->pseudo, clients[room->client_ids[i]].pseudo);
            memcpy(state->grid, room->rush_grids[i], sizeof(state->grid));
            state->score = room->rush_scores[i];
            state->is_spectator = 0;
            pkt.rush_player_count++;
        }
    }
    
    /* Broadcast to all in room */
    for (i = 0; i < room->count; i++) {
        send_to_client(room->client_ids[i], &pkt);
    }
    
    /* Check if time is up */
    if (remaining <= 0 && room->game_running) {
        /* End game */
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_GAME_END;
        pkt.time_remaining = 0;
        
        /* Find winner */
        int max_score = -1;
        int winner_idx = 0;
        for (i = 0; i < room->count; i++) {
            if (!room->is_spectator[i] && room->rush_scores[i] > max_score) {
                max_score = room->rush_scores[i];
                winner_idx = i;
            }
        }
        
        strcpy(pkt.turn_pseudo, clients[room->client_ids[winner_idx]].pseudo);
        pkt.score = max_score;
        
        /* Save all scores */
        for (i = 0; i < room->count; i++) {
            if (!room->is_spectator[i]) {
                save_score(clients[room->client_ids[i]].pseudo, room->rush_scores[i]);
            }
        }
        
        broadcast_to_room(room_idx, &pkt);
        room->game_running = 0;
        printf("Rush game ended in room %s. Winner: %s with %d points\n", 
               room->code, clients[room->client_ids[winner_idx]].pseudo, max_score);
    }
}

/* Broadcast packet to all players in room */
static void broadcast_to_room(int room_idx, Packet *pkt) {
    int i, c;
    Room *room;
    
    if (room_idx < 0 || room_idx >= MAX_ROOMS || !rooms[room_idx].active) {
        return;
    }
    
    room = &rooms[room_idx];
    
    for (i = 0; i < room->count; i++) {
        c = room->client_ids[i];
        send_to_client(c, pkt);
    }
}

/* Remove client from their room */
static void remove_client_from_room(int client_idx) {
    int room_idx = clients[client_idx].room_idx;
    Room *room;
    int i, j;
    Packet pkt;
    int was_spectator = 0;
    int slot_idx = -1;
    
    if (room_idx < 0 || room_idx >= MAX_ROOMS) {
        return;
    }
    
    room = &rooms[room_idx];
    
    if (!room->active) {
        clients[client_idx].room_idx = -1;
        return;
    }
    
    /* Find client in room and check if spectator */
    for (i = 0; i < room->count; i++) {
        if (room->client_ids[i] == client_idx) {
            was_spectator = room->is_spectator[i];
            slot_idx = i;
            break;
        }
    }
    
    if (slot_idx < 0) {
        clients[client_idx].room_idx = -1;
        return;
    }
    
    /* Update spectator count */
    if (was_spectator) {
        room->spectator_count--;
    }
    
    /* Shift remaining players and their spectator flags */
    for (j = slot_idx; j < room->count - 1; j++) {
        room->client_ids[j] = room->client_ids[j + 1];
        room->is_spectator[j] = room->is_spectator[j + 1];
    }
    room->count--;
    
    clients[client_idx].room_idx = -1;
    
    /* Handle room state after removal */
    if (room->count == 0) {
        /* Last player left, deactivate room */
        room->active = 0;
        printf("Room %s closed (empty)\n", room->code);
    } else if (client_idx == room->host_id) {
        /* Host left */
        if (room->game_running) {
            /* Cancel game */
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_GAME_CANCELLED;
            strcpy(pkt.text, "L'hote a quitte la partie!");
            broadcast_to_room(room_idx, &pkt);
            room->active = 0;
            
            /* Clear room_idx for remaining clients */
            for (i = 0; i < room->count; i++) {
                clients[room->client_ids[i]].room_idx = -1;
            }
            printf("Room %s closed (host left during game)\n", room->code);
        } else {
            /* Promote new host (skip spectators) */
            for (i = 0; i < room->count; i++) {
                if (!room->is_spectator[i]) {
                    room->host_id = room->client_ids[i];
                    break;
                }
            }
            /* If only spectators remain, just pick first one */
            if (i >= room->count) {
                room->host_id = room->client_ids[0];
            }
            send_room_update(room_idx);
            printf("Room %s: New host is %s\n", room->code, clients[room->host_id].pseudo);
        }
    } else if (room->game_running && !was_spectator) {
        /* Non-host player (not spectator) left during game */
        if (room->game_mode == GAME_MODE_RUSH) {
            /* Rush mode: game can continue, just update everyone */
            send_rush_update(room_idx);
            printf("%s left Rush game in room %s\n", clients[client_idx].pseudo, room->code);
        } else {
            /* Classic mode: cancel game */
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_GAME_CANCELLED;
            strcpy(pkt.text, "Un joueur a quitte la partie!");
            broadcast_to_room(room_idx, &pkt);
            room->active = 0;
            
            /* Clear room_idx for remaining clients */
            for (i = 0; i < room->count; i++) {
                clients[room->client_ids[i]].room_idx = -1;
            }
            printf("Room %s closed (player left during game)\n", room->code);
        }
    } else if (room->game_running && was_spectator) {
        /* Spectator left during game - just notify others */
        send_room_update(room_idx);
        printf("Spectator %s left room %s\n", clients[client_idx].pseudo, room->code);
    } else {
        /* Regular leave in lobby */
        send_room_update(room_idx);
    }
}

/* Process client packet */
static void process_packet(int client_idx, Packet *pkt) {
    Packet reply;
    int i, room_idx;
    Room *room;
    
    switch (pkt->type) {
        case MSG_LOGIN:
            strncpy(clients[client_idx].pseudo, pkt->text, 31);
            clients[client_idx].pseudo[31] = '\0';
            printf("Client %d logged in as: %s\n", client_idx, clients[client_idx].pseudo);
            break;
        
        case MSG_LEADERBOARD_REQ:
            memset(&reply, 0, sizeof(reply));
            reply.type = MSG_LEADERBOARD_REP;
            get_leaderboard(&reply.lb);
            send_to_client(client_idx, &reply);
            break;
        
        case MSG_CREATE_ROOM:
            /* Find empty room slot */
            room_idx = -1;
            for (i = 0; i < MAX_ROOMS; i++) {
                if (!rooms[i].active) {
                    room_idx = i;
                    break;
                }
            }
            
            if (room_idx < 0) {
                memset(&reply, 0, sizeof(reply));
                reply.type = MSG_ERROR;
                strcpy(reply.text, "Pas de salle disponible!");
                send_to_client(client_idx, &reply);
                break;
            }
            
            /* Initialize room */
            room = &rooms[room_idx];
            memset(room, 0, sizeof(Room));
            room->active = 1;
            generate_code(room->code);
            room->host_id = client_idx;
            room->client_ids[0] = client_idx;
            room->count = 1;
            room->game_running = 0;
            room->current_turn = 0;
            
            clients[client_idx].room_idx = room_idx;
            
            printf("Room %s created by %s\n", room->code, clients[client_idx].pseudo);
            send_room_update(room_idx);
            break;
        
        case MSG_JOIN_ROOM:
            /* Find room by code */
            room_idx = -1;
            for (i = 0; i < MAX_ROOMS; i++) {
                if (rooms[i].active && strcmp(rooms[i].code, pkt->text) == 0) {
                    room_idx = i;
                    break;
                }
            }
            
            if (room_idx < 0) {
                memset(&reply, 0, sizeof(reply));
                reply.type = MSG_ERROR;
                strcpy(reply.text, "Salle introuvable!");
                send_to_client(client_idx, &reply);
                break;
            }
            
            room = &rooms[room_idx];
            
            if (room->count >= 4) {
                memset(&reply, 0, sizeof(reply));
                reply.type = MSG_ERROR;
                strcpy(reply.text, "Salle pleine!");
                send_to_client(client_idx, &reply);
                break;
            }
            
            if (room->game_running) {
                memset(&reply, 0, sizeof(reply));
                reply.type = MSG_ERROR;
                strcpy(reply.text, "Partie deja en cours!");
                send_to_client(client_idx, &reply);
                break;
            }
            
            /* Add client to room */
            room->client_ids[room->count] = client_idx;
            room->count++;
            clients[client_idx].room_idx = room_idx;
            
            printf("%s joined room %s\n", clients[client_idx].pseudo, room->code);
            send_room_update(room_idx);
            break;
        
        case MSG_KICK_PLAYER:
            room_idx = clients[client_idx].room_idx;
            if (room_idx < 0) break;
            
            room = &rooms[room_idx];
            
            /* Verify client is host */
            if (client_idx != room->host_id) break;
            
            /* Find target player */
            for (i = 0; i < room->count; i++) {
                int target_idx = room->client_ids[i];
                if (strcmp(clients[target_idx].pseudo, pkt->text) == 0 && target_idx != client_idx) {
                    /* Send kick notification */
                    memset(&reply, 0, sizeof(reply));
                    reply.type = MSG_KICKED;
                    send_to_client(target_idx, &reply);
                    
                    printf("%s kicked from room %s\n", clients[target_idx].pseudo, room->code);
                    
                    /* Remove from room */
                    remove_client_from_room(target_idx);
                    break;
                }
            }
            break;
        
        case MSG_START_GAME:
            room_idx = clients[client_idx].room_idx;
            if (room_idx < 0) break;
            
            room = &rooms[room_idx];
            
            /* Verify client is host */
            if (client_idx != room->host_id) break;
            
            /* Need at least 2 non-spectator players */
            {
                int actual_players = room->count - room->spectator_count;
                if (actual_players < 2) break;
            }
            
            /* Set game mode from packet */
            room->game_mode = pkt->game_mode;
            if (pkt->timer_value > 0) {
                room->rush_duration = pkt->timer_value;
                room->timer_minutes = pkt->timer_value / 60;
            }
            
            /* Start game */
            room->game_running = 1;
            
            if (room->game_mode == GAME_MODE_RUSH) {
                /* Initialize Rush mode */
                room->rush_start_time = time(NULL);
                memset(room->rush_grids, 0, sizeof(room->rush_grids));
                memset(room->rush_scores, 0, sizeof(room->rush_scores));
                
                /* Send start to all players */
                memset(&reply, 0, sizeof(reply));
                reply.type = MSG_START_GAME;
                reply.game_mode = GAME_MODE_RUSH;
                reply.time_remaining = room->rush_duration;
                reply.timer_value = room->rush_duration;
                
                broadcast_to_room(room_idx, &reply);
                
                printf("Rush game started in room %s (duration: %d sec)\n", room->code, room->rush_duration);
                
                /* Send initial rush update */
                send_rush_update(room_idx);
            } else {
                /* Classic mode */
                room->current_turn = 0;
                
                /* Skip spectators for first turn */
                while (room->is_spectator[room->current_turn] && room->current_turn < room->count - 1) {
                    room->current_turn++;
                }
                
                memset(room->grid, 0, sizeof(room->grid));
                
                /* Broadcast start */
                memset(&reply, 0, sizeof(reply));
                reply.type = MSG_START_GAME;
                reply.game_mode = GAME_MODE_CLASSIC;
                memcpy(reply.grid_data, room->grid, sizeof(room->grid));
                strcpy(reply.turn_pseudo, clients[room->client_ids[room->current_turn]].pseudo);
                
                broadcast_to_room(room_idx, &reply);
                
                printf("Classic game started in room %s\n", room->code);
            }
            break;
        
        case MSG_PLACE_PIECE:
            room_idx = clients[client_idx].room_idx;
            if (room_idx < 0) break;
            
            room = &rooms[room_idx];
            
            /* Verify game is running */
            if (!room->game_running) break;
            
            if (room->game_mode == GAME_MODE_RUSH) {
                /* Rush mode - find player index and update their grid */
                int player_idx = -1;
                for (i = 0; i < room->count; i++) {
                    if (room->client_ids[i] == client_idx && !room->is_spectator[i]) {
                        player_idx = i;
                        break;
                    }
                }
                
                if (player_idx >= 0) {
                    memcpy(room->rush_grids[player_idx], pkt->grid_data, sizeof(room->rush_grids[0]));
                    room->rush_scores[player_idx] = pkt->score;
                    
                    /* Send update to all */
                    send_rush_update(room_idx);
                }
            } else {
                /* Classic mode - verify it's this client's turn */
                if (room->client_ids[room->current_turn] != client_idx) break;
                
                /* Update grid */
                memcpy(room->grid, pkt->grid_data, sizeof(room->grid));
                
                /* Save score */
                save_score(clients[client_idx].pseudo, pkt->score);
                
                /* Advance turn (skip spectators) */
                do {
                    room->current_turn = (room->current_turn + 1) % room->count;
                } while (room->is_spectator[room->current_turn] && room->count > 1);
                
                /* Broadcast update */
                memset(&reply, 0, sizeof(reply));
                reply.type = MSG_UPDATE_GRID;
                memcpy(reply.grid_data, room->grid, sizeof(room->grid));
                strcpy(reply.turn_pseudo, clients[room->client_ids[room->current_turn]].pseudo);
                
                broadcast_to_room(room_idx, &reply);
            }
            break;
        
        case MSG_SERVER_LIST_REQ:
            memset(&reply, 0, sizeof(reply));
            reply.type = MSG_SERVER_LIST_REP;
            build_server_list(&reply.server_list);
            send_to_client(client_idx, &reply);
            printf("Sent server list (%d servers) to client %d\n", reply.server_list.count, client_idx);
            break;
        
        case MSG_JOIN_SPECTATE:
            /* Find room by code */
            room_idx = -1;
            for (i = 0; i < MAX_ROOMS; i++) {
                if (rooms[i].active && strcmp(rooms[i].code, pkt->text) == 0) {
                    room_idx = i;
                    break;
                }
            }
            
            if (room_idx < 0) {
                memset(&reply, 0, sizeof(reply));
                reply.type = MSG_ERROR;
                strcpy(reply.text, "Salle introuvable!");
                send_to_client(client_idx, &reply);
                break;
            }
            
            room = &rooms[room_idx];
            
            if (room->count >= 4) {
                memset(&reply, 0, sizeof(reply));
                reply.type = MSG_ERROR;
                strcpy(reply.text, "Salle pleine!");
                send_to_client(client_idx, &reply);
                break;
            }
            
            /* Add client as spectator */
            room->client_ids[room->count] = client_idx;
            room->is_spectator[room->count] = 1;
            room->count++;
            room->spectator_count++;
            clients[client_idx].room_idx = room_idx;
            
            printf("%s joined room %s as spectator\n", clients[client_idx].pseudo, room->code);
            
            /* If game is running, send current game state */
            if (room->game_running) {
                if (room->game_mode == GAME_MODE_RUSH) {
                    send_rush_update(room_idx);
                } else {
                    memset(&reply, 0, sizeof(reply));
                    reply.type = MSG_START_GAME;
                    memcpy(reply.grid_data, room->grid, sizeof(room->grid));
                    strcpy(reply.turn_pseudo, clients[room->client_ids[room->current_turn]].pseudo);
                    reply.game_mode = room->game_mode;
                    send_to_client(client_idx, &reply);
                }
            }
            
            send_room_update(room_idx);
            break;
        
        case MSG_SET_GAME_MODE:
            room_idx = clients[client_idx].room_idx;
            if (room_idx < 0) break;
            
            room = &rooms[room_idx];
            
            /* Verify client is host */
            if (client_idx != room->host_id) break;
            
            /* Don't allow changes during game */
            if (room->game_running) break;
            
            room->game_mode = pkt->game_mode;
            if (pkt->timer_value > 0) {
                room->rush_duration = pkt->timer_value;
                room->timer_minutes = pkt->timer_value / 60;
            }
            
            /* Handle visibility toggle */
            if (pkt->lobby.is_public != room->is_public) {
                room->is_public = pkt->lobby.is_public;
                printf("Room %s visibility: %s\n", room->code, room->is_public ? "public" : "private");
            }
            
            printf("Room %s mode set to: %s, timer: %d min\n", room->code, 
                   room->game_mode == GAME_MODE_RUSH ? "Rush" : "Classic", room->timer_minutes);
            send_room_update(room_idx);
            break;
        
        case MSG_SET_TIMER:
            room_idx = clients[client_idx].room_idx;
            if (room_idx < 0) break;
            
            room = &rooms[room_idx];
            
            /* Verify client is host */
            if (client_idx != room->host_id) break;
            
            room->rush_duration = pkt->timer_value;
            room->timer_minutes = pkt->timer_value / 60;
            
            printf("Room %s timer set to: %d seconds\n", room->code, room->rush_duration);
            send_room_update(room_idx);
            break;
        
        default:
            break;
    }
}

int main(int argc, char *argv[]) {
    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    fd_set readfds;
    SOCKET max_sd;
    int i;
    int port = PORT;
    
    (void)argc;
    (void)argv;
    
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
#endif
    
    /* Initialize random */
    srand((unsigned int)time(NULL));
    
    /* Initialize arrays */
    for (i = 0; i < MAX_CLIENTS; i++) {
        clients[i].active = 0;
        clients[i].room_idx = -1;
        clients[i].socket = INVALID_SOCKET;
    }
    
    for (i = 0; i < MAX_ROOMS; i++) {
        rooms[i].active = 0;
    }
    
    /* Create server socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        return 1;
    }
    
    /* Set socket options */
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
    
    /* Bind */
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons((unsigned short)port);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        printf("Bind failed\n");
        closesocket(server_fd);
        return 1;
    }
    
    /* Listen */
    if (listen(server_fd, 5) < 0) {
        printf("Listen failed\n");
        closesocket(server_fd);
        return 1;
    }
    
    printf("========================================\n");
    printf("  BLOCKBLAST SERVER READY ON PORT %d\n", port);
    printf("========================================\n");
    
    /* Main loop */
    while (1) {
        struct timeval timeout;
        int activity;
        
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;
        
        /* Add client sockets */
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active) {
                FD_SET(clients[i].socket, &readfds);
                if (clients[i].socket > max_sd) {
                    max_sd = clients[i].socket;
                }
            }
        }
        
        /* Set timeout for periodic Rush updates (1 second) */
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        /* Wait for activity with timeout */
        activity = select((int)(max_sd + 1), &readfds, NULL, NULL, &timeout);
        
        /* Check Rush games for time updates */
        for (i = 0; i < MAX_ROOMS; i++) {
            if (rooms[i].active && rooms[i].game_running && rooms[i].game_mode == GAME_MODE_RUSH) {
                send_rush_update(i);
            }
        }
        
        if (activity < 0) {
            continue;
        }
        
        if (activity == 0) {
            /* Timeout - just continue to send periodic updates */
            continue;
        }
        
        /* Check for new connection */
        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            
            if (new_socket != INVALID_SOCKET) {
                /* Find empty client slot */
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (!clients[i].active) {
                        clients[i].socket = new_socket;
                        clients[i].active = 1;
                        clients[i].room_idx = -1;
                        clients[i].pseudo[0] = '\0';
                        
                        printf("New client connected: %d\n", i);
                        break;
                    }
                }
                
                if (i == MAX_CLIENTS) {
                    /* No room for new client */
                    closesocket(new_socket);
                    printf("Rejected connection (server full)\n");
                }
            }
        }
        
        /* Check client sockets */
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active && FD_ISSET(clients[i].socket, &readfds)) {
                Packet pkt;
                int val = recv(clients[i].socket, (char *)&pkt, sizeof(Packet), 0);
                
                if (val <= 0) {
                    /* Client disconnected */
                    printf("Client %d disconnected (%s)\n", i, clients[i].pseudo);
                    
                    /* Handle room cleanup */
                    if (clients[i].room_idx >= 0) {
                        remove_client_from_room(i);
                    }
                    
                    closesocket(clients[i].socket);
                    clients[i].active = 0;
                    clients[i].socket = INVALID_SOCKET;
                } else {
                    process_packet(i, &pkt);
                }
            }
        }
    }
    
    /* Cleanup (never reached) */
    closesocket(server_fd);
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    return 0;
}

