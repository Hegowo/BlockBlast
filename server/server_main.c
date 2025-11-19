#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "../common/net_protocol.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define close closesocket
    typedef int socklen_t;
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <netinet/in.h>
    #define INVALID_SOCKET -1
    typedef int SOCKET;
#endif

#define MAX_CLIENTS 20
#define MAX_ROOMS 10

typedef struct {
    char code[6];
    int host_id;
    int client_ids[4];
    int count;
    int active;
    int game_running;
    int timer_minutes;
    int current_turn; // Index (0-3) du joueur qui doit jouer
    int grid[GRID_H][GRID_W];
} Room;

typedef struct {
    SOCKET socket;
    char pseudo[32];
    int active;
    int room_idx;
} Client;

Client clients[MAX_CLIENTS];
Room rooms[MAX_ROOMS];

// --- LEADERBOARD INTELLIGENT (Gardez le meilleur score) ---
typedef struct { char n[32]; int s; } LEntry;

void save_score(const char* name, int score) {
    LEntry entries[100];
    int count = 0;
    int found = 0;

    // 1. Lire tout le fichier
    FILE *f = fopen("leaderboard.txt", "r");
    if(f) {
        while(count < 100 && fscanf(f, "%s %d", entries[count].n, &entries[count].s) == 2) {
            if(strcmp(entries[count].n, name) == 0) {
                // Si joueur existe déjà, on garde le max
                if(score > entries[count].s) entries[count].s = score;
                found = 1;
            }
            count++;
        }
        fclose(f);
    }

    // 2. Si nouveau joueur, ajouter
    if(!found && count < 100) {
        strncpy(entries[count].n, name, 31);
        entries[count].s = score;
        count++;
    }

    // 3. Réécrire tout le fichier proprement
    f = fopen("leaderboard.txt", "w");
    if(f) {
        for(int i=0; i<count; i++) fprintf(f, "%s %d\n", entries[i].n, entries[i].s);
        fclose(f);
    }
}

// (Lecture leaderboard inchangée, je la remets pour compilation)
void get_leaderboard(LeaderboardData *lb) {
    lb->count = 0;
    LEntry entries[100];
    int count = 0;
    FILE *f = fopen("leaderboard.txt", "r");
    if(f) {
        while(count < 100 && fscanf(f, "%s %d", entries[count].n, &entries[count].s) == 2) count++;
        fclose(f);
    }
    // Tri simple
    for(int i=0; i<count-1; i++) for(int j=0; j<count-i-1; j++) {
        if(entries[j].s < entries[j+1].s) { LEntry t = entries[j]; entries[j]=entries[j+1]; entries[j+1]=t; }
    }
    lb->count = (count > 5) ? 5 : count;
    for(int i=0; i<lb->count; i++) {
        strncpy(lb->names[i], entries[i].n, 31); lb->scores[i] = entries[i].s;
    }
}

// --- OUTILS ---
void generate_code(char *dest) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for(int i=0; i<4; i++) dest[i] = charset[rand() % 36];
    dest[4] = '\0';
}

void send_room_update(int room_idx) {
    if(room_idx < 0 || !rooms[room_idx].active) return;
    Packet pkt; memset(&pkt, 0, sizeof(Packet));
    pkt.type = MSG_ROOM_UPDATE;
    strncpy(pkt.lobby.room_code, rooms[room_idx].code, 6);
    pkt.lobby.player_count = rooms[room_idx].count;
    pkt.lobby.game_started = rooms[room_idx].game_running;
    
    for(int i=0; i<rooms[room_idx].count; i++) {
        int cid = rooms[room_idx].client_ids[i];
        strncpy(pkt.lobby.players[i], clients[cid].pseudo, 31);
    }
    for(int i=0; i<rooms[room_idx].count; i++) {
        int cid = rooms[room_idx].client_ids[i];
        pkt.lobby.is_host = (cid == rooms[room_idx].host_id);
        send(clients[cid].socket, (char*)&pkt, sizeof(Packet), 0);
    }
}

void broadcast_game(int room_idx, Packet *p) {
    if(room_idx < 0 || !rooms[room_idx].active) return;
    for(int i=0; i<rooms[room_idx].count; i++) {
        int cid = rooms[room_idx].client_ids[i];
        send(clients[cid].socket, (char*)p, sizeof(Packet), 0);
    }
}

// --- MAIN ---
int main() {
    #ifdef _WIN32
    WSADATA wsaData; WSAStartup(MAKEWORD(2, 2), &wsaData);
    #endif
    srand(time(NULL));

    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    fd_set readfds;

    for(int i=0; i<MAX_CLIENTS; i++) { clients[i].active=0; clients[i].room_idx=-1; }
    for(int i=0; i<MAX_ROOMS; i++) rooms[i].active=0;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt=1; setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    address.sin_family=AF_INET; address.sin_addr.s_addr=INADDR_ANY; address.sin_port=htons(PORT);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    printf("SERVER V3 READY PORT %d\n", PORT);

    while(1) {
        FD_ZERO(&readfds); FD_SET(server_fd, &readfds);
        SOCKET max_sd = server_fd;
        for(int i=0; i<MAX_CLIENTS; i++) if(clients[i].active) { FD_SET(clients[i].socket, &readfds); if(clients[i].socket>max_sd) max_sd=clients[i].socket; }

        if(select((int)max_sd+1, &readfds, NULL, NULL, NULL) < 0) continue;

        // Nouvelle connexion
        if(FD_ISSET(server_fd, &readfds)) {
            int addrlen = sizeof(address);
            if((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) != INVALID_SOCKET) {
                for(int i=0; i<MAX_CLIENTS; i++) if(!clients[i].active) {
                    clients[i].socket=new_socket; clients[i].active=1; clients[i].room_idx=-1;
                    strcpy(clients[i].pseudo, "Guest"); printf("Client connected ID %d\n", i); break;
                }
            }
        }

        // Gestion Clients
        for(int i=0; i<MAX_CLIENTS; i++) {
            if(clients[i].active && FD_ISSET(clients[i].socket, &readfds)) {
                Packet pkt;
                int val = recv(clients[i].socket, (char*)&pkt, sizeof(Packet), 0);
                
                if(val <= 0) {
                    // -- DÉCONNEXION --
                    printf("Client %d disconnected\n", i);
                    int rid = clients[i].room_idx;
                    if(rid != -1 && rooms[rid].active) {
                        // Retirer le joueur de la liste des IDs
                        int pos = -1;
                        for(int k=0; k<rooms[rid].count; k++) if(rooms[rid].client_ids[k] == i) pos = k;
                        
                        if(pos != -1) {
                            // Décaler le tableau
                            for(int k=pos; k<rooms[rid].count-1; k++) rooms[rid].client_ids[k] = rooms[rid].client_ids[k+1];
                            rooms[rid].count--;
                        }

                        // Si le jeu était en cours et qu'il reste moins de 2 joueurs (ou 1 seul en solo ?)
                        // On annule la partie pour les survivants
                        if(rooms[rid].game_running && rooms[rid].count > 0) {
                            Packet cancel; memset(&cancel,0,sizeof(Packet));
                            cancel.type = MSG_GAME_CANCELLED;
                            sprintf(cancel.text, "%s s'est deconnecte.", clients[i].pseudo);
                            broadcast_game(rid, &cancel);
                            
                            // Reset la salle
                            rooms[rid].game_running = 0;
                            // On ne détruit pas la salle, on les renvoie au lobby (géré par le client)
                        }
                        
                        // Si la salle est vide ou si c'était l'hôte, on ferme
                        if(rooms[rid].count == 0 || rooms[rid].host_id == i) {
                            rooms[rid].active = 0;
                        } else {
                            send_room_update(rid);
                        }
                    }
                    close(clients[i].socket); clients[i].active = 0;
                } 
                else {
                    if(pkt.type == 0) continue;

                    if(pkt.type == MSG_LOGIN) strncpy(clients[i].pseudo, pkt.text, 31);
                    
                    else if(pkt.type == MSG_LEADERBOARD_REQ) {
                        Packet resp; memset(&resp,0,sizeof(Packet)); resp.type=MSG_LEADERBOARD_REP;
                        get_leaderboard(&resp.lb); send(clients[i].socket, (char*)&resp, sizeof(Packet), 0);
                    }
                    
                    else if(pkt.type == MSG_CREATE_ROOM) {
                        int rid=-1; for(int r=0;r<MAX_ROOMS;r++) if(!rooms[r].active) {rid=r;break;}
                        if(rid!=-1) {
                            rooms[rid].active=1; generate_code(rooms[rid].code);
                            rooms[rid].host_id=i; rooms[rid].count=1; rooms[rid].client_ids[0]=i;
                            rooms[rid].game_running=0; rooms[rid].current_turn=0;
                            memset(rooms[rid].grid, 0, sizeof(rooms[rid].grid));
                            clients[i].room_idx=rid; send_room_update(rid);
                        }
                    }
                    
                    else if(pkt.type == MSG_JOIN_ROOM) {
                        int rid=-1; for(int r=0;r<MAX_ROOMS;r++) if(rooms[r].active && strcmp(rooms[r].code,pkt.text)==0) {rid=r;break;}
                        if(rid!=-1 && rooms[rid].count<4 && !rooms[rid].game_running) {
                            rooms[rid].client_ids[rooms[rid].count++]=i; clients[i].room_idx=rid; send_room_update(rid);
                        } else {
                            Packet err; memset(&err,0,sizeof(Packet)); err.type=MSG_ERROR; strcpy(err.text,"Erreur Room");
                            send(clients[i].socket, (char*)&err, sizeof(Packet), 0);
                        }
                    }
                    
                    else if(pkt.type == MSG_KICK_PLAYER) {
                        // L'hôte veut virer quelqu'un (pseudo dans pkt.text)
                        int rid = clients[i].room_idx;
                        if(rid != -1 && rooms[rid].host_id == i) {
                            int target_id = -1;
                            int target_pos = -1;
                            for(int k=0; k<rooms[rid].count; k++) {
                                int cid = rooms[rid].client_ids[k];
                                if(strcmp(clients[cid].pseudo, pkt.text) == 0 && cid != i) {
                                    target_id = cid; target_pos = k; break;
                                }
                            }
                            
                            if(target_id != -1) {
                                // Notifier l'expulsé
                                Packet k; memset(&k,0,sizeof(Packet)); k.type=MSG_KICKED;
                                send(clients[target_id].socket, (char*)&k, sizeof(Packet), 0);
                                
                                // Retirer de la salle
                                clients[target_id].room_idx = -1; // Retour menu
                                for(int k=target_pos; k<rooms[rid].count-1; k++) rooms[rid].client_ids[k] = rooms[rid].client_ids[k+1];
                                rooms[rid].count--;
                                
                                // Notifier les autres
                                send_room_update(rid);
                            }
                        }
                    }

                    else if(pkt.type == MSG_START_GAME) {
                        int rid = clients[i].room_idx;
                        if(rid != -1 && rooms[rid].host_id == i) {
                            rooms[rid].game_running=1;
                            rooms[rid].current_turn = 0; // Le joueur 0 (hôte) commence
                            Packet p; memset(&p,0,sizeof(Packet)); p.type=MSG_START_GAME;
                            memcpy(p.grid_data, rooms[rid].grid, sizeof(rooms[rid].grid));
                            // On dit à tout le monde que c'est à Host de jouer
                            strcpy(p.turn_pseudo, clients[rooms[rid].client_ids[0]].pseudo);
                            broadcast_game(rid, &p);
                        }
                    }
                    
                    else if(pkt.type == MSG_PLACE_PIECE) {
                        int rid = clients[i].room_idx;
                        if(rid != -1 && rooms[rid].game_running) {
                            // Vérifier si c'est bien son tour
                            int current_player_id = rooms[rid].client_ids[rooms[rid].current_turn];
                            
                            if(current_player_id == i) {
                                // Coup valide
                                memcpy(rooms[rid].grid, pkt.grid_data, sizeof(rooms[rid].grid));
                                save_score(clients[i].pseudo, pkt.score);
                                
                                // Passer au tour suivant
                                rooms[rid].current_turn = (rooms[rid].current_turn + 1) % rooms[rid].count;
                                int next_player_id = rooms[rid].client_ids[rooms[rid].current_turn];
                                
                                Packet p; memset(&p,0,sizeof(Packet)); p.type=MSG_UPDATE_GRID;
                                memcpy(p.grid_data, rooms[rid].grid, sizeof(rooms[rid].grid));
                                strncpy(p.turn_pseudo, clients[next_player_id].pseudo, 31);
                                
                                broadcast_game(rid, &p);
                            }
                        }
                    }
                }
            }
        }
    }
    #ifdef _WIN32
    WSACleanup();
    #endif
    return 0;
}