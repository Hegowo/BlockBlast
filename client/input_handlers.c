#include "input_handlers.h"
#include "globals.h"
#include "graphics.h"
#include "audio.h"
#include "save_system.h"
#include "ui_components.h"
#include "net_client.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void handle_menu_click(void) {
    int center_x = window_w / 2;
    int btn_y = 250;
    
    if (point_in_rect(mouse_x, mouse_y, window_w - 50, 20, 28, 28)) {
        play_click();
        show_settings_overlay = 1;
        settings_tab = 0;
        return;
    }
    
    if (has_saved_game) {
        if (point_in_rect(mouse_x, mouse_y, center_x - 150, btn_y, 300, 55)) {
            play_click();
            if (load_current_game()) {
                current_state = ST_SOLO;
            } else {
                delete_saved_game();
                init_game(&game);
                current_state = ST_SOLO;
            }
            return;
        }
        btn_y += 70;
    }
    
    if (point_in_rect(mouse_x, mouse_y, center_x - 150, btn_y, 300, 55)) {
        play_click();
        delete_saved_game();
        init_game(&game);
        current_state = ST_SOLO;
        return;
    }
    btn_y += 70;
    
    if (point_in_rect(mouse_x, mouse_y, center_x - 150, btn_y, 300, 55)) {
        play_click();
        if (net_connect(online_ip, online_port)) {
            memset(input_buffer, 0, sizeof(input_buffer));
            current_state = ST_LOGIN;
        } else {
            strcpy(popup_msg, "Impossible de se connecter au serveur!");
        }
        return;
    }
    btn_y += 70;
    
    if (point_in_rect(mouse_x, mouse_y, center_x - 150, btn_y, 300, 55)) {
        play_click();
        show_settings_overlay = 1;
        settings_tab = 0;
        strcpy(edit_ip, online_ip);
        snprintf(edit_port, sizeof(edit_port), "%d", online_port);
        active_input = 0;
    }
}

void handle_options_click(void) {
    if (point_in_rect(mouse_x, mouse_y, 70, 210, 400, 50)) {
        play_click();
        active_input = 1;
    }
    else if (point_in_rect(mouse_x, mouse_y, 70, 330, 400, 50)) {
        play_click();
        active_input = 2;
    }
    else if (point_in_rect(mouse_x, mouse_y, 70, 480, 180, 50)) {
        play_click();
        test_result = perform_connection_test() ? 1 : -1;
    }
    else if (point_in_rect(mouse_x, mouse_y, 290, 480, 180, 50) && test_result == 1) {
        play_click();
        strcpy(online_ip, edit_ip);
        online_port = atoi(edit_port);
        strcpy(popup_msg, "Configuration sauvegardee!");
    }
    else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 580, 180, 50)) {
        play_click();
        current_state = ST_MENU;
    }
}

void handle_login_click(void) {
    Packet pkt;
    
    if (point_in_rect(mouse_x, mouse_y, window_w - 50, 20, 28, 28)) {
        play_click();
        show_settings_overlay = 1;
        settings_tab = 0;
        return;
    }
    
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 420, 180, 50) && strlen(input_buffer) > 0) {
        play_click();
        strcpy(my_pseudo, input_buffer);
        
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_LOGIN;
        strcpy(pkt.text, my_pseudo);
        net_send(&pkt);
        
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_LEADERBOARD_REQ;
        net_send(&pkt);
        
        memset(input_buffer, 0, sizeof(input_buffer));
        current_state = ST_MULTI_CHOICE;
    }
    else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 500, 180, 50)) {
        play_click();
        net_close();
        current_state = ST_MENU;
    }
}

void handle_multi_choice_click(void) {
    Packet pkt;
    
    if (point_in_rect(mouse_x, mouse_y, window_w - 50, 20, 28, 28)) {
        play_click();
        show_settings_overlay = 1;
        settings_tab = 0;
        return;
    }
    
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 150, 400, 300, 55)) {
        play_click();
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_CREATE_ROOM;
        net_send(&pkt);
        
        selected_game_mode = GAME_MODE_CLASSIC;
        selected_timer_minutes = 3;
    }
    else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 150, 475, 300, 55)) {
        play_click();
        memset(input_buffer, 0, sizeof(input_buffer));
        current_state = ST_JOIN_INPUT;
    }
    else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 150, 550, 300, 55)) {
        play_click();
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_SERVER_LIST_REQ;
        net_send(&pkt);
        browser_scroll_offset = 0;
        current_state = ST_SERVER_BROWSER;
    }
    else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 640, 180, 50)) {
        play_click();
        net_close();
        current_state = ST_MENU;
    }
}

void handle_join_input_click(void) {
    Packet pkt;
    
    if (point_in_rect(mouse_x, mouse_y, window_w - 50, 20, 28, 28)) {
        play_click();
        show_settings_overlay = 1;
        settings_tab = 0;
        return;
    }
    
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 420, 180, 50) && strlen(input_buffer) == 4) {
        play_click();
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_JOIN_ROOM;
        strcpy(pkt.text, input_buffer);
        net_send(&pkt);
    }
    else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 500, 180, 50)) {
        play_click();
        current_state = ST_MULTI_CHOICE;
    }
}

void handle_lobby_click(void) {
    Packet pkt;
    int mode_y = 320;
    int toggle_y = selected_game_mode == GAME_MODE_RUSH ? mode_y + 150 : mode_y + 80;
    int need_players = current_lobby.player_count < 2 - current_lobby.spectator_count;
    int start_y = selected_game_mode == GAME_MODE_RUSH ? 530 : 470;
    if (need_players) start_y += 40;
    
    if (point_in_rect(mouse_x, mouse_y, window_w - 50, 20, 28, 28)) {
        play_click();
        show_settings_overlay = 1;
        settings_tab = 0;
        return;
    }
    
    if (current_lobby.is_host && !current_lobby.game_started) {
        if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 160, mode_y + 25, 150, 35)) {
            play_click();
            selected_game_mode = GAME_MODE_CLASSIC;
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_SET_GAME_MODE;
            pkt.game_mode = GAME_MODE_CLASSIC;
            net_send(&pkt);
        }
        else if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 + 10, mode_y + 25, 150, 35)) {
            play_click();
            selected_game_mode = GAME_MODE_RUSH;
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_SET_GAME_MODE;
            pkt.game_mode = GAME_MODE_RUSH;
            pkt.timer_value = selected_timer_minutes * 60;
            net_send(&pkt);
        }
        
        if (selected_game_mode == GAME_MODE_RUSH) {
            int timer_buttons[] = {1, 2, 3, 5, 10};
            int timer_x[] = {WINDOW_W / 2 - 200, WINDOW_W / 2 - 130, WINDOW_W / 2 - 60, 
                            WINDOW_W / 2 + 10, WINDOW_W / 2 + 80};
            int i;
            for (i = 0; i < 5; i++) {
                if (point_in_rect(mouse_x, mouse_y, timer_x[i], mode_y + 100, 55, 30)) {
                    play_click();
                    selected_timer_minutes = timer_buttons[i];
                    memset(&pkt, 0, sizeof(pkt));
                    pkt.type = MSG_SET_TIMER;
                    pkt.timer_value = selected_timer_minutes * 60;
                    net_send(&pkt);
                    break;
                }
            }
        }
        
        if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, toggle_y, 180, 35)) {
            play_click();
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_SET_GAME_MODE;
            pkt.game_mode = selected_game_mode;
            pkt.lobby.is_public = !current_lobby.is_public;
            net_send(&pkt);
        }
        
        if (!need_players && point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 150, start_y, 300, 55)) {
            play_click();
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_START_GAME;
            pkt.game_mode = selected_game_mode;
            pkt.timer_value = selected_timer_minutes * 60;
            net_send(&pkt);
        }
    }
    
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 660, 180, 50)) {
        play_click();
        net_close();
        current_state = ST_MENU;
        is_spectator = 0;
    }
}

void handle_lobby_right_click(void) {
    int i;
    Packet pkt;
    
    if (!current_lobby.is_host) return;
    
    for (i = 1; i < current_lobby.player_count; i++) {
        if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 100, 190 + i * 40, 200, 30)) {
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_KICK_PLAYER;
            strcpy(pkt.text, current_lobby.players[i]);
            net_send(&pkt);
            break;
        }
    }
}

void handle_server_browser_click(void) {
    Packet pkt;
    int i;
    int y_start = 160;
    int entry_height = 110;
    
    if (point_in_rect(mouse_x, mouse_y, window_w - 50, 20, 28, 28)) {
        play_click();
        show_settings_overlay = 1;
        settings_tab = 0;
        return;
    }
    
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 680, 180, 45)) {
        play_click();
        memset(&pkt, 0, sizeof(pkt));
        pkt.type = MSG_SERVER_LIST_REQ;
        net_send(&pkt);
        return;
    }
    
    for (i = 0; i < server_list.count && i < 5; i++) {
        int idx = i + browser_scroll_offset;
        if (idx >= server_list.count) break;
        
        ServerInfo *srv = &server_list.servers[idx];
        int y = y_start + i * entry_height;
        
        if (!srv->game_started && point_in_rect(mouse_x, mouse_y, WINDOW_W - 140, y + 10, 100, 35)) {
            play_click();
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_JOIN_ROOM;
            strcpy(pkt.text, srv->room_code);
            is_spectator = 0;
            net_send(&pkt);
            return;
        }
        
        if (point_in_rect(mouse_x, mouse_y, WINDOW_W - 140, y + 52, 100, 35)) {
            play_click();
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = MSG_JOIN_SPECTATE;
            strcpy(pkt.text, srv->room_code);
            is_spectator = 1;
            spectate_view_idx = 0;
            net_send(&pkt);
            return;
        }
    }
    
    if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, 735, 180, 45)) {
        play_click();
        current_state = ST_MULTI_CHOICE;
    }
}

void handle_server_browser_scroll(int direction) {
    if (direction < 0 && browser_scroll_offset > 0) {
        browser_scroll_offset--;
    } else if (direction > 0 && browser_scroll_offset + 5 < server_list.count) {
        browser_scroll_offset++;
    }
}

void handle_game_click(int is_multi) {
    Packet pkt;
    int grid_x, grid_y;
    int all_placed, i;
    
    if (is_multi && current_lobby.game_mode == GAME_MODE_CLASSIC && !is_my_turn()) return;
    
    if (selected_piece_idx >= 0) {
        Piece *p = &game.current_pieces[selected_piece_idx];
        
        grid_x = (mouse_x - grid_offset_x - (p->w * block_size) / 2 + block_size / 2) / block_size;
        grid_y = (mouse_y - grid_offset_y - (p->h * block_size) / 2 + block_size / 2) / block_size;
        
        if (can_place(&game, grid_y, grid_x, p)) {
            place_piece_logic(&game, grid_y, grid_x, p);
            game.pieces_available[selected_piece_idx] = 0;
            
            if (game.num_cleared_rows > 0 || game.num_cleared_cols > 0) {
                play_clear();
            } else {
                play_place();
            }
            
            all_placed = 1;
            for (i = 0; i < 3; i++) {
                if (game.pieces_available[i]) {
                    all_placed = 0;
                    break;
                }
            }
            
            if (all_placed) {
                generate_pieces(&game);
            }
            
            if (is_multi) {
                memset(&pkt, 0, sizeof(pkt));
                pkt.type = MSG_PLACE_PIECE;
                memcpy(pkt.grid_data, game.grid, sizeof(game.grid));
                pkt.score = game.score;
                net_send(&pkt);
                
                if (current_lobby.game_mode == GAME_MODE_CLASSIC && !check_valid_moves_exist(&game)) {
                    memset(&pkt, 0, sizeof(pkt));
                    pkt.type = MSG_GAME_OVER;
                    strcpy(pkt.text, my_pseudo);
                    pkt.score = game.score;
                    net_send(&pkt);
                }
            } else {
                if (!check_valid_moves_exist(&game)) {
                    game.game_over = 1;
                    play_gameover();
                }
            }
        }
        
        selected_piece_idx = -1;
    }
}

void handle_game_mousedown(int is_multi) {
    int i;
    
    if (is_multi && current_lobby.game_mode == GAME_MODE_CLASSIC && !is_my_turn()) return;
    
    if (layout_horizontal) {
        int slot_w = (window_w / 2 - 80);
        int slot_h = (window_h - 280) / 3;
        int slot_x = window_w / 2 + 40;
        
        if (mouse_x >= slot_x) {
            for (i = 0; i < 3; i++) {
                if (!game.pieces_available[i]) continue;
                
                int slot_y = 200 + i * slot_h;
                if (mouse_y >= slot_y && mouse_y < slot_y + slot_h) {
                    selected_piece_idx = i;
                    break;
                }
            }
        }
    } else {
        if (mouse_y >= piece_area_y) {
            int slot_w = (window_w - 60) / 3;
            
            for (i = 0; i < 3; i++) {
                if (!game.pieces_available[i]) continue;
                
                int slot_x = 30 + i * slot_w;
                if (mouse_x >= slot_x && mouse_x < slot_x + slot_w) {
                    selected_piece_idx = i;
                    break;
                }
            }
        }
    }
}

void process_network(void) {
    Packet pkt;
    int i;
    
    while (net_receive(&pkt)) {
        switch (pkt.type) {
            case MSG_LEADERBOARD_REP:
                leaderboard = pkt.lb;
                break;
            
            case MSG_ROOM_UPDATE:
                current_lobby = pkt.lobby;
                selected_game_mode = current_lobby.game_mode;
                selected_timer_minutes = current_lobby.timer_minutes;
                if (current_state != ST_MULTI_GAME && current_state != ST_SPECTATE) {
                    current_state = ST_LOBBY;
                }
                break;
            
            case MSG_KICKED:
                strcpy(popup_msg, "Vous avez ete expulse!");
                net_close();
                current_state = ST_MENU;
                is_spectator = 0;
                break;
            
            case MSG_START_GAME:
                current_lobby.game_mode = pkt.game_mode;
                
                if (pkt.game_mode == GAME_MODE_RUSH) {
                    memset(game.grid, 0, sizeof(game.grid));
                    game.score = 0;
                    generate_pieces(&game);
                    init_effects(&game.effects);
                    
                    rush_time_remaining = pkt.time_remaining;
                    last_time_update = SDL_GetTicks();
                    rush_player_count = 0;
                    
                    current_state = is_spectator ? ST_SPECTATE : ST_MULTI_GAME;
                } else {
                    memcpy(game.grid, pkt.grid_data, sizeof(game.grid));
                    strcpy(current_turn_pseudo, pkt.turn_pseudo);
                    game.score = 0;
                    generate_pieces(&game);
                    init_effects(&game.effects);
                    
                    current_state = is_spectator ? ST_SPECTATE : ST_MULTI_GAME;
                }
                break;
            
            case MSG_UPDATE_GRID:
                memcpy(game.grid, pkt.grid_data, sizeof(game.grid));
                strcpy(current_turn_pseudo, pkt.turn_pseudo);
                
                if (current_lobby.game_mode == GAME_MODE_CLASSIC && is_my_turn() && !is_spectator) {
                    if (!check_valid_moves_exist(&game)) {
                        Packet game_over_pkt;
                        memset(&game_over_pkt, 0, sizeof(game_over_pkt));
                        game_over_pkt.type = MSG_GAME_OVER;
                        strcpy(game_over_pkt.text, my_pseudo);
                        game_over_pkt.score = game.score;
                        net_send(&game_over_pkt);
                    }
                }
                break;
            
            case MSG_RUSH_UPDATE:
                rush_time_remaining = pkt.time_remaining;
                rush_player_count = pkt.rush_player_count;
                
                for (i = 0; i < pkt.rush_player_count && i < 4; i++) {
                    rush_states[i] = pkt.rush_states[i];
                }
                
                last_time_update = SDL_GetTicks();
                break;
            
            case MSG_TIME_SYNC:
                rush_time_remaining = pkt.time_remaining;
                last_time_update = SDL_GetTicks();
                break;
            
            case MSG_GAME_END:
                if (current_lobby.game_mode == GAME_MODE_RUSH) {
                    rush_time_remaining = 0;
                } else {
                    strcpy(multi_winner_name, pkt.turn_pseudo);
                    if (strcmp(my_pseudo, pkt.turn_pseudo) == 0) {
                        multi_game_over = 2;
                        play_victory();
                    } else {
                        multi_game_over = 1;
                        play_gameover();
                    }
                }
                break;
            
            case MSG_SERVER_LIST_REP:
                server_list = pkt.server_list;
                break;
            
            case MSG_GAME_CANCELLED:
                strcpy(popup_msg, pkt.text);
                net_close();
                current_state = ST_MENU;
                is_spectator = 0;
                break;
            
            case MSG_ERROR:
                strcpy(popup_msg, pkt.text);
                break;
            
            default:
                break;
        }
    }
}
