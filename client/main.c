#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h> 
#include "game.h"
#include "net_client.h"
#include "../common/config.h"
#include "../common/net_protocol.h"

#define COLOR_BG 0x1E1E2E 
#define COLOR_BTN 0x4A6FA5
#define COLOR_BTN_HOVER 0x6A8FC5
#define COLOR_BTN_DISABLED 0x555555
#define COLOR_BTN_SUCCESS 0x28A745
#define COLOR_BTN_DANGER 0xA54A4A
#define COLOR_INPUT 0x2E2E3E
#define COLOR_INPUT_FOCUS 0x3E3E5E
#define COLOR_CYAN 0x00FFFF
#define OFFSET_X 70
#define OFFSET_Y 120

enum { ST_MENU, ST_OPTIONS, ST_SOLO, ST_LOGIN, ST_MULTI_CHOICE, ST_JOIN_INPUT, ST_LOBBY, ST_MULTI_GAME };

SDL_Surface *screen = NULL;
TTF_Font *font_L = NULL, *font_S = NULL;
GameState game;
int current_state = ST_MENU;
int mouse_x, mouse_y;
int selected_piece_idx = -1; 

char online_ip[32] = "127.0.0.1";
int online_port = 5000;

char edit_ip[32] = "127.0.0.1";
char edit_port[16] = "5000";
int active_input = 0; 
int test_result = 0;

char my_pseudo[32] = "";
char current_turn_pseudo[32] = ""; 
char input_buffer[32] = ""; 
char popup_msg[128] = ""; 

LobbyState current_lobby;
LeaderboardData leaderboard;

Uint32 get_pixel(int r, int g, int b) { return SDL_MapRGB(screen->format, r, g, b); }
void fill_rect(int x, int y, int w, int h, Uint32 color) { SDL_Rect r={x,y,w,h}; SDL_FillRect(screen,&r,color); }

void draw_text(TTF_Font *f, const char* txt, int cx, int cy, Uint32 col_val) {
    if(!txt||!txt[0]) return;
    SDL_Color c={(Uint8)(col_val>>16),(Uint8)(col_val>>8),(Uint8)col_val};
    SDL_Surface *s = TTF_RenderUTF8_Blended(f, txt, c);
    if(s){
        SDL_Rect p={cx - s->w/2, cy - s->h/2, 0, 0}; 
        SDL_BlitSurface(s, NULL, screen, &p); 
        SDL_FreeSurface(s);
    }
}

int draw_button(int x, int y, int w, int h, const char* lbl, Uint32 color, int disabled) {
    if (disabled) {
        fill_rect(x, y, w, h, COLOR_BTN_DISABLED);
        draw_text(font_S, lbl, x+w/2, y+h/2, 0x888888);
        return 0;
    }
    
    int over = (mouse_x>=x && mouse_x<=x+w && mouse_y>=y && mouse_y<=y+h);
    fill_rect(x+4, y+4, w, h, get_pixel(30,30,40));
    fill_rect(x, y, w, h, over ? get_pixel((color>>16)+20,(color>>8)+20,(color&255)+20) : color);
    draw_text(font_S, lbl, x+w/2, y+h/2, 0xFFFFFF);
    return over;
}

void draw_styled_block(int x, int y, int size, Uint32 color) {
    fill_rect(x, y, size, size, color);
    fill_rect(x+2, y+2, size-4, size-4, get_pixel((color>>16&0xFF)*0.8, (color>>8&0xFF)*0.8, (color&0xFF)*0.8));
}

void draw_popup() {
    if(strlen(popup_msg) == 0) return;
    fill_rect(50, WINDOW_H/2 - 100, WINDOW_W-100, 200, 0x000000);
    fill_rect(52, WINDOW_H/2 - 98, WINDOW_W-104, 196, 0x333333);
    draw_text(font_S, popup_msg, WINDOW_W/2, WINDOW_H/2 - 30, 0xFFFFFF);
    draw_text(font_S, "(Cliquez pour fermer)", WINDOW_W/2, WINDOW_H/2 + 50, 0xAAAAAA);
}

void handle_input_char(Uint16 unicode) {
    char c = (char)unicode;
    
    char *target_buffer = NULL;
    int max_len = 12;

    if (current_state == ST_OPTIONS) {
        if (active_input == 1) { target_buffer = edit_ip; max_len = 30; }
        if (active_input == 2) { target_buffer = edit_port; max_len = 6; }
    } 
    else {
        target_buffer = input_buffer;
    }

    if (!target_buffer) return;

    int len = strlen(target_buffer);

    if (c == 8) {
        if (len > 0) {
            target_buffer[len-1] = 0;
            if(current_state == ST_OPTIONS) test_result = 0; 
        }
        return;
    }

    if (len < max_len) {
        int valid = 0;
        if (current_state == ST_OPTIONS && active_input == 1) {
            if ((c >= '0' && c <= '9') || c == '.') valid = 1;
        } 
        else if (current_state == ST_OPTIONS && active_input == 2) {
            if (c >= '0' && c <= '9') valid = 1;
        }
        else {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ') valid = 1;
        }

        if (valid) {
            if(current_state == ST_JOIN_INPUT && c >= 'a' && c <= 'z') c -= 32;
            
            target_buffer[len] = c;
            target_buffer[len+1] = 0;
            if(current_state == ST_OPTIONS) test_result = 0;
        }
    }
}

int is_my_turn() {
    return (strcmp(my_pseudo, current_turn_pseudo) == 0);
}

int perform_connection_test() {
    int p = atoi(edit_port);
    if (p <= 0) return 0;
    
    if (net_connect(edit_ip, p)) {
        net_close(); 
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO); TTF_Init();
    SDL_EnableUNICODE(1); 

    screen = SDL_SetVideoMode(WINDOW_W, WINDOW_H, 32, SDL_SWSURFACE);
    SDL_WM_SetCaption("BlockBlast V4.0 Options", NULL);
    font_L=TTF_OpenFont("assets/font.ttf", 40); font_S=TTF_OpenFont("assets/font.ttf", 22);
    if(!font_L) return 1;

    init_game(&game);
    int running=1;

    while(running) {
        SDL_Event e;
        Packet pkt; memset(&pkt,0,sizeof(Packet));

        while(net_receive(&pkt)) {
            if(pkt.type == MSG_LEADERBOARD_REP) leaderboard = pkt.lb;
            if(pkt.type == MSG_ROOM_UPDATE) {
                if(current_state == ST_MULTI_CHOICE || current_state == ST_JOIN_INPUT || current_state == ST_LOBBY) {
                    current_lobby = pkt.lobby; current_state = ST_LOBBY;
                }
            }
            if(pkt.type == MSG_START_GAME || pkt.type == MSG_UPDATE_GRID) {
                if(pkt.type == MSG_START_GAME) current_state = ST_MULTI_GAME;
                memcpy(game.grid, pkt.grid_data, sizeof(game.grid));
                strcpy(current_turn_pseudo, pkt.turn_pseudo); 
            }
            if(pkt.type == MSG_KICKED) {
                current_state = ST_MENU; net_close();
                sprintf(popup_msg, "L'hôte vous a exclu.");
            }
            if(pkt.type == MSG_GAME_CANCELLED) {
                current_state = ST_LOBBY; 
                sprintf(popup_msg, "%s Partie annulée.", pkt.text);
            }
            if(pkt.type == MSG_ERROR) sprintf(popup_msg, "Erreur : %s", pkt.text);
        }

        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) running=0;
            if(e.type == SDL_MOUSEMOTION) { mouse_x=e.motion.x; mouse_y=e.motion.y; }
            
            if(e.type == SDL_KEYDOWN && (current_state==ST_LOGIN || current_state==ST_JOIN_INPUT || current_state==ST_OPTIONS)) {
                handle_input_char(e.key.keysym.unicode);
            }

            if(e.type == SDL_MOUSEBUTTONUP) {
                if(strlen(popup_msg) > 0) { popup_msg[0] = 0; continue; }

                if(e.button.button == SDL_BUTTON_LEFT) {
                    if(current_state == ST_MENU) {
                        if(draw_button(120,350,300,60,"SOLO",COLOR_BTN,0)) { 
                            current_state=ST_SOLO; init_game(&game); 
                        }
                        if(draw_button(120,450,300,60,"MULTI ONLINE",COLOR_BTN,0)) { 
                            if(net_connect(online_ip, online_port)) {
                                current_state=ST_LOGIN; input_buffer[0]=0;
                            } else {
                                sprintf(popup_msg, "Serveur inaccessible !");
                            }
                        }
                        if(draw_button(120,550,300,60,"OPTIONS",COLOR_BTN,0)) {
                            current_state = ST_OPTIONS;
                            active_input = 0;
                            test_result = 0;
                            strcpy(edit_ip, online_ip);
                            sprintf(edit_port, "%d", online_port);
                        }
                    }

                    else if(current_state == ST_OPTIONS) {
                        if(mouse_y >= 250 && mouse_y <= 300 && mouse_x >= 100 && mouse_x <= 440) active_input = 1;
                        else if(mouse_y >= 350 && mouse_y <= 400 && mouse_x >= 100 && mouse_x <= 440) active_input = 2;
                        else active_input = 0;

                        if(draw_button(120, 500, 300, 60, "TESTER", COLOR_BTN, 0)) {
                            if(perform_connection_test()) test_result = 1;
                            else test_result = -1;
                        }
                        
                        int save_disabled = (test_result != 1);
                        if(draw_button(120, 600, 300, 60, "SAUVEGARDER", COLOR_BTN_SUCCESS, save_disabled)) {
                            strcpy(online_ip, edit_ip);
                            online_port = atoi(edit_port);
                            current_state = ST_MENU;
                            sprintf(popup_msg, "Configuration sauvegardée !");
                        }

                        if(draw_button(120, 700, 300, 60, "RETOUR", COLOR_BTN_DANGER, 0)) {
                            current_state = ST_MENU;
                        }
                    }

                    else if(current_state == ST_LOGIN) {
                        if(draw_button(150,500,240,50,"VALIDER",COLOR_BTN,0) && strlen(input_buffer)>0) {
                             strcpy(my_pseudo, input_buffer);
                             Packet p; memset(&p,0,sizeof(Packet)); p.type=MSG_LOGIN; strcpy(p.text,my_pseudo); net_send(&p);
                             Packet r; memset(&r,0,sizeof(Packet)); r.type=MSG_LEADERBOARD_REQ; net_send(&r);
                             current_state=ST_MULTI_CHOICE;
                        }
                    }
                    else if(current_state == ST_MULTI_CHOICE) {
                         if(draw_button(50,600,200,50,"CRÉER",COLOR_BTN,0)) { Packet p; memset(&p,0,sizeof(Packet)); p.type=MSG_CREATE_ROOM; net_send(&p); }
                         if(draw_button(290,600,200,50,"REJOINDRE",COLOR_BTN,0)) { current_state=ST_JOIN_INPUT; input_buffer[0]=0; }
                    }
                    else if(current_state == ST_JOIN_INPUT) {
                         if(draw_button(150,500,240,50,"ENTRER",COLOR_BTN,0) && strlen(input_buffer)==4) {
                             Packet p; memset(&p,0,sizeof(Packet)); p.type=MSG_JOIN_ROOM; strcpy(p.text, input_buffer); net_send(&p);
                         }
                    }
                    else if(current_state == ST_LOBBY) {
                        if(draw_button(120,800,300,60,"QUITTER",COLOR_BTN_DANGER,0)) { net_close(); current_state = ST_MENU; }
                        if(current_lobby.is_host && draw_button(120,700,300,60,"LANCER",COLOR_BTN,0)) {
                            Packet p; memset(&p,0,sizeof(Packet)); p.type=MSG_START_GAME; net_send(&p);
                        }
                    }
                    else if(current_state == ST_SOLO || current_state == ST_MULTI_GAME) {
                        if(current_state == ST_MULTI_GAME && !is_my_turn()) { } 
                        else if(selected_piece_idx != -1) {
                            int gx = (mouse_x-OFFSET_X-30)/BLOCK_SIZE, gy = (mouse_y-OFFSET_Y-30)/BLOCK_SIZE;
                            if(can_place(&game, gy, gx, &game.current_pieces[selected_piece_idx])) {
                                place_piece_logic(&game, gy, gx, &game.current_pieces[selected_piece_idx]);
                                game.pieces_available[selected_piece_idx]=0;
                                if(current_state == ST_MULTI_GAME) {
                                    Packet p; memset(&p,0,sizeof(Packet)); p.type=MSG_PLACE_PIECE; p.score=game.score;
                                    memcpy(p.grid_data, game.grid, sizeof(game.grid)); net_send(&p);
                                }
                                int u=1; for(int k=0;k<3;k++) if(game.pieces_available[k]) u=0;
                                if(u) generate_pieces(&game);
                            }
                            selected_piece_idx = -1;
                        }
                    }
                }
                
                if(e.button.button == SDL_BUTTON_RIGHT && current_state == ST_LOBBY && current_lobby.is_host) {
                    for(int i=0; i<current_lobby.player_count; i++) {
                        int cy = 300 + i*50;
                        if(mouse_y > cy - 25 && mouse_y < cy + 25 && strcmp(current_lobby.players[i], my_pseudo) != 0) {
                            Packet k; memset(&k,0,sizeof(Packet)); k.type = MSG_KICK_PLAYER; strcpy(k.text, current_lobby.players[i]); net_send(&k);
                        }
                    }
                }
            }
            
            if(e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                 if(current_state == ST_SOLO || (current_state == ST_MULTI_GAME && is_my_turn())) {
                    for(int i=0;i<3;i++) if(game.pieces_available[i]) {
                         int px = 50 + i * 150;
                         if(mouse_x >= px && mouse_x <= px+100 && mouse_y >= 680) selected_piece_idx = i;
                    }
                }
            }
        }

        fill_rect(0,0,WINDOW_W,WINDOW_H, COLOR_BG);
        
        if(current_state == ST_MENU) {
            draw_text(font_L,"BLOCKBLAST V4",WINDOW_W/2,150,0xFFA500);
            draw_button(120,350,300,60,"SOLO",COLOR_BTN,0); 
            draw_button(120,450,300,60,"MULTI ONLINE",COLOR_BTN,0);
            draw_button(120,550,300,60,"OPTIONS",COLOR_BTN,0);
        }
        else if(current_state == ST_OPTIONS) {
            draw_text(font_L, "CONFIGURATION", WINDOW_W/2, 100, 0xFFFFFF);
            
            draw_text(font_S, "ADRESSE IP :", WINDOW_W/2, 220, 0xAAAAAA);
            Uint32 col_ip = (active_input == 1) ? COLOR_INPUT_FOCUS : COLOR_INPUT;
            fill_rect(100, 250, 340, 50, col_ip);
            draw_text(font_L, edit_ip, WINDOW_W/2, 275, 0x00FFFF);

            draw_text(font_S, "PORT :", WINDOW_W/2, 320, 0xAAAAAA);
            Uint32 col_port = (active_input == 2) ? COLOR_INPUT_FOCUS : COLOR_INPUT;
            fill_rect(100, 350, 340, 50, col_port);
            draw_text(font_L, edit_port, WINDOW_W/2, 375, 0x00FFFF);

            if(test_result == 1) draw_text(font_S, "CONNEXION RÉUSSIE !", WINDOW_W/2, 450, 0x00FF00);
            else if(test_result == -1) draw_text(font_S, "ÉCHEC DE LA CONNEXION", WINDOW_W/2, 450, 0xFF0000);

            draw_button(120, 500, 300, 60, "TESTER", COLOR_BTN, 0);
            int save_disabled = (test_result != 1);
            draw_button(120, 600, 300, 60, "SAUVEGARDER", COLOR_BTN_SUCCESS, save_disabled);
            draw_button(120, 700, 300, 60, "RETOUR", COLOR_BTN_DANGER, 0);
        }
        else if(current_state == ST_LOGIN) {
            draw_text(font_L,"PSEUDO :",WINDOW_W/2,250,0xFFFFFF); fill_rect(100,300,340,60,COLOR_INPUT);
            draw_text(font_L,input_buffer,WINDOW_W/2,330,0x00FF00); draw_button(150,500,240,50,"VALIDER",COLOR_BTN,0);
        }
        else if(current_state == ST_MULTI_CHOICE) {
            draw_text(font_S,"- MEILLEURS SCORES -",WINDOW_W/2,180,0xFFFF00);
            for(int i=0;i<leaderboard.count;i++) { char b[64]; sprintf(b,"%d. %s (%d)",i+1,leaderboard.names[i],leaderboard.scores[i]); draw_text(font_S,b,WINDOW_W/2,220+i*30,0xDDDDDD); }
            draw_button(50,600,200,50,"CRÉER",COLOR_BTN,0); draw_button(290,600,200,50,"REJOINDRE",COLOR_BTN,0);
        }
        else if(current_state == ST_JOIN_INPUT) {
            draw_text(font_L,"CODE :",WINDOW_W/2,250,0xFFFFFF); fill_rect(100,300,340,60,COLOR_INPUT);
            draw_text(font_L,input_buffer,WINDOW_W/2,330,0x00FFFF); draw_button(150,500,240,50,"ENTRER",COLOR_BTN,0);
        }
        else if(current_state == ST_LOBBY) {
            draw_text(font_S,"CODE PARTIE :",WINDOW_W/2,100,0xAAAAAA); draw_text(font_L,current_lobby.room_code,WINDOW_W/2,150,0x00FF00);
            draw_text(font_S,"JOUEURS :",WINDOW_W/2,250,0xFFFFFF);
            for(int i=0;i<current_lobby.player_count;i++) {
                Uint32 col = (strcmp(current_lobby.players[i], my_pseudo) == 0) ? COLOR_CYAN : 0xFFFFFF;
                draw_text(font_L, current_lobby.players[i], WINDOW_W/2, 300 + i*50, col);
            }
            if(current_lobby.is_host) draw_button(120,700,300,60,"LANCER",COLOR_BTN,0);
            else draw_text(font_S,"En attente...",WINDOW_W/2,700,0x888888);
            draw_button(120,800,300,60,"QUITTER",COLOR_BTN_DANGER,0);
        }
        else if(current_state == ST_SOLO || current_state == ST_MULTI_GAME) {
            if(current_state == ST_MULTI_GAME) {
                char buf[64];
                if(is_my_turn()) { sprintf(buf, "À TOI DE JOUER !"); draw_text(font_L,buf,WINDOW_W/2, 80, 0x00FF00); }
                else { sprintf(buf, "Tour de %s...", current_turn_pseudo); draw_text(font_L,buf,WINDOW_W/2, 80, 0xAAAAAA); }
            }
            fill_rect(OFFSET_X-5,OFFSET_Y-5,GRID_W*BLOCK_SIZE+10,GRID_H*BLOCK_SIZE+10,0x444455);
            fill_rect(OFFSET_X,OFFSET_Y,GRID_W*BLOCK_SIZE,GRID_H*BLOCK_SIZE,0x202025);
            for(int i=0;i<GRID_H;i++) for(int j=0;j<GRID_W;j++) if(game.grid[i][j]) draw_styled_block(OFFSET_X+j*BLOCK_SIZE+1,OFFSET_Y+i*BLOCK_SIZE+1,BLOCK_SIZE-2,game.grid[i][j]);
            for(int i=0;i<3;i++) if(game.pieces_available[i] && i!=selected_piece_idx) {
                Piece *p=&game.current_pieces[i]; 
                Uint32 col = (current_state==ST_MULTI_GAME && !is_my_turn()) ? 0x555555 : p->color;
                for(int r=0;r<p->h;r++) for(int c=0;c<p->w;c++) if(p->data[r][c]) draw_styled_block(50+i*150+c*30,680+r*30,28,col);
            }
            if(selected_piece_idx!=-1) {
                Piece *p=&game.current_pieces[selected_piece_idx];
                for(int r=0;r<p->h;r++) for(int c=0;c<p->w;c++) if(p->data[r][c]) draw_styled_block(mouse_x-BLOCK_SIZE*1.5+c*BLOCK_SIZE,mouse_y-BLOCK_SIZE*1.5+r*BLOCK_SIZE,BLOCK_SIZE,p->color);
            }
        }

        draw_popup();
        SDL_Flip(screen); SDL_Delay(16);
    }
    TTF_CloseFont(font_L); TTF_CloseFont(font_S); TTF_Quit(); SDL_Quit();
    return 0;
}