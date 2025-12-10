#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#include "globals.h"
#include "save_system.h"
#include "audio.h"
#include "graphics.h"
#include "ui_components.h"
#include "screens.h"
#include "input_handlers.h"
#include "game.h"
#include "net_client.h"
#include "embedded_assets.h"

static TTF_Font* load_font_embedded_or_file(const unsigned char *data, size_t size, int ptsize, const char **file_paths) {
    TTF_Font *font = NULL;
    
#ifdef EMBED_ASSETS
    if (data && size > 0) {
        SDL_RWops *rw = SDL_RWFromConstMem(data, (int)size);
        if (rw) {
            font = TTF_OpenFontRW(rw, 1, ptsize);
            if (font) {
                return font;
            }
        }
    }
#else
    (void)data;
    (void)size;
#endif
    
    if (file_paths) {
        int i;
        for (i = 0; file_paths[i] != NULL; i++) {
            font = TTF_OpenFont(file_paths[i], ptsize);
            if (font) {
                printf("Loaded font from: %s\n", file_paths[i]);
                return font;
            }
        }
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {
    SDL_Event e;
    int running = 1;
    
    (void)argc;
    (void)argv;
    
    srand((unsigned int)time(NULL));
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL init failed: %s\n", SDL_GetError());
        return 1;
    }
    
    if (TTF_Init() < 0) {
        printf("TTF init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    
    init_audio();
    
    load_game_data();
    check_saved_game_exists();
    
    update_music_volume();
    update_sfx_volume();
    
    SDL_EnableUNICODE(1);
    
    screen = SDL_SetVideoMode(WINDOW_W, WINDOW_H, 32, SDL_SWSURFACE | SDL_RESIZABLE);
    if (!screen) {
        printf("Video mode failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    window_w = WINDOW_W;
    window_h = WINDOW_H;
    recalculate_layout();
    
    SDL_WM_SetCaption("BlockBlast V4.0", NULL);
    
    {
        const char *font_paths[] = {
            "assets/orbitron.ttf",
            "../assets/orbitron.ttf",
            "assets/Orbitron.ttf",
            "../assets/Orbitron.ttf",
            "assets/font.ttf",
            "../assets/font.ttf",
            "font.ttf",
            NULL
        };
        
#ifdef EMBED_ASSETS
        if (font_orbitron_size > 0) {
            SDL_RWops *rw_L = SDL_RWFromConstMem(font_orbitron_data, (int)font_orbitron_size);
            SDL_RWops *rw_S = SDL_RWFromConstMem(font_orbitron_data, (int)font_orbitron_size);
            SDL_RWops *rw_XS = SDL_RWFromConstMem(font_orbitron_data, (int)font_orbitron_size);
            
            if (rw_L && rw_S && rw_XS) {
                font_L = TTF_OpenFontRW(rw_L, 1, 40);
                font_S = TTF_OpenFontRW(rw_S, 1, 22);
                font_XS = TTF_OpenFontRW(rw_XS, 1, 14);
                
                if (font_L && font_S && font_XS) {
                    printf("Loaded font from embedded data (orbitron)\n");
                } else {
                    if (font_L) { TTF_CloseFont(font_L); font_L = NULL; }
                    if (font_S) { TTF_CloseFont(font_S); font_S = NULL; }
                    if (font_XS) { TTF_CloseFont(font_XS); font_XS = NULL; }
                }
            }
        }
        
        if (!font_L && font_default_size > 0) {
            SDL_RWops *rw_L = SDL_RWFromConstMem(font_default_data, (int)font_default_size);
            SDL_RWops *rw_S = SDL_RWFromConstMem(font_default_data, (int)font_default_size);
            SDL_RWops *rw_XS = SDL_RWFromConstMem(font_default_data, (int)font_default_size);
            
            if (rw_L && rw_S && rw_XS) {
                font_L = TTF_OpenFontRW(rw_L, 1, 40);
                font_S = TTF_OpenFontRW(rw_S, 1, 22);
                font_XS = TTF_OpenFontRW(rw_XS, 1, 14);
                
                if (font_L && font_S && font_XS) {
                    printf("Loaded font from embedded data (default)\n");
                } else {
                    if (font_L) { TTF_CloseFont(font_L); font_L = NULL; }
                    if (font_S) { TTF_CloseFont(font_S); font_S = NULL; }
                    if (font_XS) { TTF_CloseFont(font_XS); font_XS = NULL; }
                }
            }
        }
#endif
        
        if (!font_L) {
            int i;
            for (i = 0; font_paths[i] != NULL; i++) {
                font_L = TTF_OpenFont(font_paths[i], 40);
                if (font_L) {
                    font_S = TTF_OpenFont(font_paths[i], 22);
                    font_XS = TTF_OpenFont(font_paths[i], 14);
                    if (font_S && font_XS) {
                        printf("Loaded font from: %s\n", font_paths[i]);
                        break;
                    }
                    TTF_CloseFont(font_L);
                    font_L = NULL;
                    if (font_S) { TTF_CloseFont(font_S); font_S = NULL; }
                    if (font_XS) { TTF_CloseFont(font_XS); font_XS = NULL; }
                }
            }
        }
    }
    
    if (!font_L || !font_S) {
        printf("Failed to load font: %s\n", TTF_GetError());
        printf("Tried: assets/font.ttf, ../assets/font.ttf, font.ttf\n");
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    init_game(&game);
    memset(&current_lobby, 0, sizeof(current_lobby));
    memset(&leaderboard, 0, sizeof(leaderboard));
    
    start_music();
    
    while (running) {
        process_network();
        
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    running = 0;
                    break;
                
                case SDL_VIDEORESIZE:
                    window_w = e.resize.w;
                    window_h = e.resize.h;
                    if (window_w < 400) window_w = 400;
                    if (window_h < 400) window_h = 400;
                    screen = SDL_SetVideoMode(window_w, window_h, 32, SDL_SWSURFACE | SDL_RESIZABLE);
                    recalculate_layout();
                    break;
                
                case SDL_KEYDOWN:
                    if (e.key.keysym.unicode) {
                        handle_input_char(e.key.keysym.unicode);
                    }
                    
                    if (e.key.keysym.sym == SDLK_p) {
                        show_settings_overlay = !show_settings_overlay;
                        play_click();
                    }
                    
                    if (e.key.keysym.sym == SDLK_ESCAPE) {
                        if (show_settings_overlay) {
                            show_settings_overlay = 0;
                            play_click();
                        } else if (show_pause_menu) {
                            show_pause_menu = 0;
                            play_click();
                        } else if (current_state == ST_SOLO && !game.game_over) {
                            show_pause_menu = 1;
                            play_click();
                        } else if (current_state == ST_SOLO && game.game_over) {
                            delete_saved_game();
                            current_state = ST_MENU;
                            resume_music();
                        } else if (current_state == ST_SPECTATE || current_state == ST_MULTI_GAME) {
                            net_close();
                            current_state = ST_MENU;
                            is_spectator = 0;
                        } else if (current_state != ST_MENU) {
                            net_close();
                            current_state = ST_MENU;
                        }
                    }
                    
                    if ((current_state == ST_MULTI_GAME || current_state == ST_SPECTATE) && 
                        is_spectator && current_lobby.game_mode == GAME_MODE_RUSH) {
                        if (e.key.keysym.sym == SDLK_LEFT) {
                            spectate_view_idx--;
                            if (spectate_view_idx < 0) spectate_view_idx = rush_player_count - 1;
                            if (spectate_view_idx < 0) spectate_view_idx = 0;
                        } else if (e.key.keysym.sym == SDLK_RIGHT) {
                            spectate_view_idx++;
                            if (spectate_view_idx >= rush_player_count) spectate_view_idx = 0;
                        }
                    }
                    break;
                
                case SDL_MOUSEMOTION:
                    SDL_GetMouseState(&mouse_x, &mouse_y);
                    if (show_settings_overlay) {
                        handle_settings_overlay_drag();
                    }
                    break;
                
                case SDL_MOUSEBUTTONDOWN:
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        if (show_settings_overlay) {
                            handle_settings_overlay_mousedown();
                            break;
                        }
                        if (current_state == ST_SOLO) {
                            handle_game_mousedown(0);
                        } else if (current_state == ST_MULTI_GAME && !is_spectator) {
                            handle_game_mousedown(1);
                        }
                    }
                    else if (e.button.button == SDL_BUTTON_WHEELUP) {
                        if (current_state == ST_SERVER_BROWSER) {
                            handle_server_browser_scroll(-1);
                        }
                    }
                    else if (e.button.button == SDL_BUTTON_WHEELDOWN) {
                        if (current_state == ST_SERVER_BROWSER) {
                            handle_server_browser_scroll(1);
                        }
                    }
                    break;
                
                case SDL_MOUSEBUTTONUP:
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        if (show_pause_menu) {
                            handle_pause_menu_click();
                            break;
                        }
                        
                        if (show_settings_overlay) {
                            handle_settings_overlay_click();
                            break;
                        }
                        
                        if (popup_msg[0]) {
                            popup_msg[0] = '\0';
                            break;
                        }
                        
                        switch (current_state) {
                            case ST_MENU:
                                handle_menu_click();
                                break;
                            case ST_OPTIONS:
                                handle_options_click();
                                break;
                            case ST_LOGIN:
                                handle_login_click();
                                break;
                            case ST_MULTI_CHOICE:
                                handle_multi_choice_click();
                                break;
                            case ST_JOIN_INPUT:
                                handle_join_input_click();
                                break;
                            case ST_LOBBY:
                                handle_lobby_click();
                                break;
                            case ST_SOLO:
                                if (point_in_rect(mouse_x, mouse_y, window_w - 50, 20, 28, 28)) {
                                    play_click();
                                    show_settings_overlay = 1;
                                    settings_tab = 0;
                                } else if (game.game_over) {
                                    play_click();
                                    delete_saved_game();
                                    current_state = ST_MENU;
                                    resume_music();
                                } else {
                                    handle_game_click(0);
                                }
                                break;
                            case ST_MULTI_GAME:
                                if (point_in_rect(mouse_x, mouse_y, window_w - 50, 20, 28, 28)) {
                                    play_click();
                                    show_settings_overlay = 1;
                                    settings_tab = 0;
                                } else if (is_spectator) {
                                    if (current_lobby.game_mode == GAME_MODE_RUSH && rush_player_count > 0) {
                                        spectate_view_idx = (spectate_view_idx + 1) % rush_player_count;
                                    }
                                } else if (current_lobby.game_mode == GAME_MODE_RUSH) {
                                    if (rush_time_remaining <= 0) {
                                        play_click();
                                        net_close();
                                        current_state = ST_MENU;
                                        is_spectator = 0;
                                    } else {
                                        handle_game_click(1);
                                    }
                                } else {
                                    handle_game_click(1);
                                }
                                break;
                            case ST_SERVER_BROWSER:
                                play_click();
                                handle_server_browser_click();
                                break;
                            case ST_SPECTATE:
                                if (point_in_rect(mouse_x, mouse_y, WINDOW_W / 2 - 90, WINDOW_H - 50, 180, 40)) {
                                    net_close();
                                    current_state = ST_MENU;
                                    is_spectator = 0;
                                }
                                else if (current_lobby.game_mode == GAME_MODE_RUSH && rush_player_count > 0) {
                                    spectate_view_idx = (spectate_view_idx + 1) % rush_player_count;
                                }
                                break;
                        }
                    } else if (e.button.button == SDL_BUTTON_RIGHT) {
                        if (current_state == ST_LOBBY) {
                            handle_lobby_right_click();
                        }
                    }
                    break;
            }
        }
        
        {
            Uint32 current_time = SDL_GetTicks();
            if (last_frame_time > 0) {
                delta_time = (current_time - last_frame_time) / 1000.0f;
                if (delta_time > 0.1f) delta_time = 0.1f;
            }
            last_frame_time = current_time;
        }
        
        if ((current_state == ST_MULTI_GAME || current_state == ST_SPECTATE) && 
            current_lobby.game_mode == GAME_MODE_RUSH && rush_time_remaining > 0) {
            Uint32 now = SDL_GetTicks();
            int elapsed = (now - last_time_update) / 1000;
            if (elapsed > 0) {
                rush_time_remaining -= elapsed;
                if (rush_time_remaining < 0) rush_time_remaining = 0;
                last_time_update = now;
            }
        }
        
        global_time += delta_time;
        glow_pulse += delta_time;
        if (glow_pulse > 6.28318f) glow_pulse -= 6.28318f;
        
        draw_cyberpunk_background_responsive();
        
        switch (current_state) {
            case ST_MENU:
                render_menu();
                break;
            case ST_OPTIONS:
                render_options();
                break;
            case ST_LOGIN:
                render_login();
                break;
            case ST_MULTI_CHOICE:
                render_multi_choice();
                break;
            case ST_JOIN_INPUT:
                render_join_input();
                break;
            case ST_LOBBY:
                render_lobby();
                break;
            case ST_SOLO:
                render_solo();
                break;
            case ST_MULTI_GAME:
                render_multi_game();
                break;
            case ST_SERVER_BROWSER:
                render_server_browser();
                break;
            case ST_SPECTATE:
                render_spectate();
                break;
        }
        
        draw_popup();
        
        if (show_pause_menu && current_state == ST_SOLO) {
            render_pause_menu();
        }
        
        if (show_settings_overlay) {
            render_settings_overlay();
        }
        
        SDL_Flip(screen);
        
        SDL_Delay(16);
    }
    
    net_close();
    cleanup_audio();
    if (font_L) TTF_CloseFont(font_L);
    if (font_S) TTF_CloseFont(font_S);
    if (font_XS) TTF_CloseFont(font_XS);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}
