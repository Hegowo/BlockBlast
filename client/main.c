#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "game.h"
#include "net_client.h"
#include "../common/config.h"
#include "../common/net_protocol.h"

#define COLOR_BG 0x1E1E2E 
#define COLOR_BTN 0x4A6FA5
#define COLOR_BTN_HOVER 0x6A8FC5
#define COLOR_BTN_DANGER 0xA54A4A
#define COLOR_INPUT 0x2E2E3E
#define OFFSET_X 70
#define OFFSET_Y 120

enum { ST_MENU, ST_SOLO, ST_LOGIN, ST_MULTI_CHOICE, ST_JOIN_INPUT, ST_LOBBY, ST_MULTI_GAME };

SDL_Surface *screen = NULL;
TTF_Font *font_L = NULL, *font_S = NULL;
GameState game;
int current_state = ST_MENU;
int mouse_x, mouse_y;
int selected_piece_idx = -1; 
char online_ip[32] = "127.0.0.1"; // IP PLUS TARD

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

int draw_button(int x, int y, int w, int h, const char* lbl, Uint32 color) {
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

int is_my_turn() {
    return (strcmp(my_pseudo, current_turn_pseudo) == 0);
}

void handle_input_char(SDL_keysym k) {
    int len = strlen(input_buffer);
    if(k.sym==SDLK_BACKSPACE && len>0) input_buffer[len-1]=0;
    else if(len<12 && ((k.sym>='a'&&k.sym<='z')||(k.sym>='0'&&k.sym<='9')||k.sym==SDLK_SPACE)) { 
        input_buffer[len]=k.sym; input_buffer[len+1]=0; 
    }
}

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO); TTF_Init();
    screen = SDL_SetVideoMode(WINDOW_W, WINDOW_H, 32, SDL_SWSURFACE);
    SDL_WM_SetCaption("BlockBlast V3.2 (UTF8)", NULL);
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
            if(pkt.type == MSG_START_GAME) {
                current_state = ST_MULTI_GAME;
                memcpy(game.grid, pkt.grid_data, sizeof(game.grid));
                strcpy(current_turn_pseudo, pkt.turn_pseudo); 
            }
            if(pkt.type == MSG_UPDATE_GRID) {
                memcpy(game.grid, pkt.grid_data, sizeof(game.grid));
                strcpy(current_turn_pseudo, pkt.turn_pseudo); 
            }
            if(pkt.type == MSG_KICKED) {
                current_state = ST_MENU; net_close();
                sprintf(popup_msg, "L'hôte vous a exclu de la partie.");
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
            if(e.type == SDL_KEYDOWN && (current_state==ST_LOGIN || current_state==ST_JOIN_INPUT)) handle_input_char(e.key.keysym);

            if(e.type == SDL_MOUSEBUTTONUP) {
                if(strlen(popup_msg) > 0) { popup_msg[0] = 0; continue; }

                if(e.button.button == SDL_BUTTON_LEFT) {
                    if(current_state == ST_MENU) {
                        if(mouse_y>400 && mouse_y<460) { current_state=ST_SOLO; init_game(&game); }
                        if(mouse_y>500 && mouse_y<560) { if(net_connect(online_ip)) {current_state=ST_LOGIN; input_buffer[0]=0;} }
                    }
                    else if(current_state == ST_LOGIN && mouse_y>500) {
                         if(strlen(input_buffer)>0) {
                             strcpy(my_pseudo, input_buffer);
                             Packet p; memset(&p,0,sizeof(Packet)); p.type=MSG_LOGIN; strcpy(p.text,my_pseudo); net_send(&p);
                             Packet r; memset(&r,0,sizeof(Packet)); r.type=MSG_LEADERBOARD_REQ; net_send(&r);
                             current_state=ST_MULTI_CHOICE;
                         }
                    }
                    else if(current_state == ST_MULTI_CHOICE) {
                         if(mouse_x<250 && mouse_y>600) { Packet p; memset(&p,0,sizeof(Packet)); p.type=MSG_CREATE_ROOM; net_send(&p); }
                         if(mouse_x>290 && mouse_y>600) { current_state=ST_JOIN_INPUT; input_buffer[0]=0; }
                    }
                    else if(current_state == ST_JOIN_INPUT && mouse_y>500) {
                         if(strlen(input_buffer)==4) {
                             Packet p; memset(&p,0,sizeof(Packet)); p.type=MSG_JOIN_ROOM;
                             for(int i=0;i<4;i++) p.text[i]=(input_buffer[i]>='a'&&input_buffer[i]<='z')?input_buffer[i]-32:input_buffer[i];
                             net_send(&p);
                         }
                    }
                    else if(current_state == ST_LOBBY) {
                        if(mouse_y > 800 && mouse_y < 860) { net_close(); current_state = ST_MENU; }
                        if(current_lobby.is_host && mouse_y > 700 && mouse_y < 760) {
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
                        int center_y = 300 + i*50;
                        if(mouse_y > center_y - 25 && mouse_y < center_y + 25) {
                            if(strcmp(current_lobby.players[i], my_pseudo) != 0) {
                                Packet k; memset(&k,0,sizeof(Packet));
                                k.type = MSG_KICK_PLAYER;
                                strcpy(k.text, current_lobby.players[i]);
                                net_send(&k);
                            }
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
            draw_text(font_L,"BLOCKBLAST V3",WINDOW_W/2,200,0xFFA500);
            draw_button(120,400,300,60,"SOLO",COLOR_BTN); draw_button(120,500,300,60,"MULTI ONLINE",COLOR_BTN);
        }
        else if(current_state == ST_LOGIN) {
            draw_text(font_L,"PSEUDO :",WINDOW_W/2,250,0xFFFFFF); fill_rect(100,300,340,60,COLOR_INPUT);
            draw_text(font_L,input_buffer,WINDOW_W/2,330,0x00FF00); draw_button(150,500,240,50,"VALIDER",COLOR_BTN);
        }
        else if(current_state == ST_MULTI_CHOICE) {
            draw_text(font_S,"- MEILLEURS SCORES -",WINDOW_W/2,180,0xFFFF00);
            for(int i=0;i<leaderboard.count;i++) { char b[64]; sprintf(b,"%d. %s (%d)",i+1,leaderboard.names[i],leaderboard.scores[i]); draw_text(font_S,b,WINDOW_W/2,220+i*30,0xDDDDDD); }
            draw_button(50,600,200,50,"CRÉER",COLOR_BTN); draw_button(290,600,200,50,"REJOINDRE",COLOR_BTN);
        }
        else if(current_state == ST_JOIN_INPUT) {
            draw_text(font_L,"CODE :",WINDOW_W/2,250,0xFFFFFF); fill_rect(100,300,340,60,COLOR_INPUT);
            draw_text(font_L,input_buffer,WINDOW_W/2,330,0x00FFFF); draw_button(150,500,240,50,"ENTRER",COLOR_BTN);
        }
        else if(current_state == ST_LOBBY) {
            draw_text(font_S,"CODE PARTIE :",WINDOW_W/2,100,0xAAAAAA); draw_text(font_L,current_lobby.room_code,WINDOW_W/2,150,0x00FF00);
            
            draw_text(font_S,"JOUEURS :",WINDOW_W/2,250,0xFFFFFF);
            
            for(int i=0;i<current_lobby.player_count;i++) draw_text(font_L,current_lobby.players[i],WINDOW_W/2,300+i*50,0xFFFFFF);
            
            if(current_lobby.is_host) draw_button(120,700,300,60,"LANCER",COLOR_BTN);
            else draw_text(font_S,"En attente...",WINDOW_W/2,700,0x888888);
            draw_button(120,800,300,60,"QUITTER",COLOR_BTN_DANGER);
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