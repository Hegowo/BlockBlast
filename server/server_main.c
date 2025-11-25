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
    int timer_minutes;          /* Timer (future use) */
    int current_turn;           /* Index of current player (0-3) */
    int grid[GRID_H][GRID_W];   /* Shared game grid */
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
    int i, c;
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
        
        /* Set host flag for this client */
        pkt.lobby.is_host = (c == room->host_id) ? 1 : 0;
        
        /* Copy player names */
        for (int j = 0; j < room->count; j++) {
            strcpy(pkt.lobby.players[j], clients[room->client_ids[j]].pseudo);
        }
        
        send_to_client(c, &pkt);
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
    
    if (room_idx < 0 || room_idx >= MAX_ROOMS) {
        return;
    }
    
    room = &rooms[room_idx];
    
    if (!room->active) {
        clients[client_idx].room_idx = -1;
        return;
    }
    
    /* Find and remove client from room */
    for (i = 0; i < room->count; i++) {
        if (room->client_ids[i] == client_idx) {
            /* Shift remaining players */
            for (j = i; j < room->count - 1; j++) {
                room->client_ids[j] = room->client_ids[j + 1];
            }
            room->count--;
            break;
        }
    }
    
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
            /* Promote new host */
            room->host_id = room->client_ids[0];
            send_room_update(room_idx);
            printf("Room %s: New host is %s\n", room->code, clients[room->host_id].pseudo);
        }
    } else if (room->game_running) {
        /* Non-host left during game */
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
            
            /* Need at least 2 players */
            if (room->count < 2) break;
            
            /* Start game */
            room->game_running = 1;
            room->current_turn = 0;
            memset(room->grid, 0, sizeof(room->grid));
            
            /* Broadcast start */
            memset(&reply, 0, sizeof(reply));
            reply.type = MSG_START_GAME;
            memcpy(reply.grid_data, room->grid, sizeof(room->grid));
            strcpy(reply.turn_pseudo, clients[room->client_ids[0]].pseudo);
            
            broadcast_to_room(room_idx, &reply);
            
            printf("Game started in room %s\n", room->code);
            break;
        
        case MSG_PLACE_PIECE:
            room_idx = clients[client_idx].room_idx;
            if (room_idx < 0) break;
            
            room = &rooms[room_idx];
            
            /* Verify game is running */
            if (!room->game_running) break;
            
            /* Verify it's this client's turn */
            if (room->client_ids[room->current_turn] != client_idx) break;
            
            /* Update grid */
            memcpy(room->grid, pkt->grid_data, sizeof(room->grid));
            
            /* Save score */
            save_score(clients[client_idx].pseudo, pkt->score);
            
            /* Advance turn */
            room->current_turn = (room->current_turn + 1) % room->count;
            
            /* Broadcast update */
            memset(&reply, 0, sizeof(reply));
            reply.type = MSG_UPDATE_GRID;
            memcpy(reply.grid_data, room->grid, sizeof(room->grid));
            strcpy(reply.turn_pseudo, clients[room->client_ids[room->current_turn]].pseudo);
            
            broadcast_to_room(room_idx, &reply);
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
        
        /* Wait for activity */
        if (select((int)(max_sd + 1), &readfds, NULL, NULL, NULL) < 0) {
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

