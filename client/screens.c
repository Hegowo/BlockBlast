#include "screens.h"
#include "globals.h"
#include "graphics.h"
#include "ui_components.h"
#include "audio.h"
#include "save_system.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

void render_menu(void) {
    float title_pulse = (sinf(glow_pulse * 2.0f) + 1.0f) * 0.5f;
    int title_y = 120;
    int center_x = window_w / 2;
    
    {
        int glow_size = (int)(8 + title_pulse * 4);
        int i;
        for (i = glow_size; i > 0; i--) {
            float alpha = (float)(glow_size - i + 1) / (float)(glow_size + 1) * 0.15f;
            (void)alpha;
        }
    }
    
    draw_text(font_L, "BLOCKBLAST", center_x + 2, title_y + 2, darken_color(COLOR_NEON_CYAN, 0.3f));
    draw_text(font_L, "BLOCKBLAST", center_x, title_y, COLOR_NEON_CYAN);
    
    draw_text(font_S, "NEON EDITION", center_x, title_y + 50, COLOR_NEON_MAGENTA);
    
    int line_y = title_y + 85;
    fill_rect(center_x - 150, line_y, 100, 2, COLOR_NEON_CYAN);
    fill_rect(center_x + 50, line_y, 100, 2, COLOR_NEON_MAGENTA);
    
    {
        int cx = center_x;
        int cy = line_y;
        int size = 6;
        fill_rect(cx - size/2, cy - 1, size, 3, COLOR_NEON_GREEN);
        fill_rect(cx - 1, cy - size/2, 3, size, COLOR_NEON_GREEN);
    }
    
    int btn_y = 250;
    
    if (has_saved_game) {
        draw_button(center_x - 150, btn_y, 300, 55, "CONTINUER", COLOR_NEON_ORANGE, 0);
        btn_y += 70;
    }
    
    draw_button(center_x - 150, btn_y, 300, 55, "NOUVELLE PARTIE", COLOR_SUCCESS, 0);
    btn_y += 70;
    draw_button(center_x - 150, btn_y, 300, 55, "MULTI ONLINE", COLOR_BUTTON, 0);
    btn_y += 70;
    draw_button(center_x - 150, btn_y, 300, 55, "PARAMETRES", COLOR_PURPLE, 0);
    
    draw_settings_gear(window_w - 50, 20);
    
    draw_text(font_XS ? font_XS : font_S, "v4.0 NEON", center_x, window_h - 30, COLOR_GREY);
}

void render_options(void) {
    draw_text(font_L, "CONFIGURATION", WINDOW_W / 2 + 2, 80 + 2, darken_color(COLOR_NEON_MAGENTA, 0.3f));
    draw_text(font_L, "CONFIGURATION", WINDOW_W / 2, 80, COLOR_NEON_MAGENTA);
    
    fill_rect(WINDOW_W / 2 - 120, 120, 240, 2, COLOR_NEON_MAGENTA);
    
    draw_text(font_S, "Adresse IP:", WINDOW_W / 2, 170, COLOR_NEON_CYAN);
    draw_input_field(70, 195, 400, 50, edit_ip, active_input == 1);
    
    draw_text(font_S, "Port:", WINDOW_W / 2, 280, COLOR_NEON_CYAN);
    draw_input_field(70, 305, 400, 50, edit_port, active_input == 2);
    
    if (test_result == 1) {
        draw_text(font_S, "Connexion OK!", WINDOW_W / 2 + 1, 401, darken_color(COLOR_NEON_GREEN, 0.4f));
        draw_text(font_S, "Connexion OK!", WINDOW_W / 2, 400, COLOR_NEON_GREEN);
    } else if (test_result == -1) {
        draw_text(font_S, "Echec connexion!", WINDOW_W / 2 + 1, 401, darken_color(COLOR_NEON_RED, 0.4f));
        draw_text(font_S, "Echec connexion!", WINDOW_W / 2, 400, COLOR_NEON_RED);
    }
    
    draw_button(70, 460, 180, 50, "TESTER", COLOR_BUTTON, 0);
    draw_button(290, 460, 180, 50, "SAUVEGARDER", COLOR_SUCCESS, test_result != 1);
    draw_button(WINDOW_W / 2 - 90, 560, 180, 50, "RETOUR", COLOR_DANGER, 0);
}

void render_login(void) {
    draw_settings_gear(window_w - 50, 20);
    
    draw_text(font_L, "CONNEXION", WINDOW_W / 2 + 2, 120 + 2, darken_color(COLOR_NEON_CYAN, 0.3f));
    draw_text(font_L, "CONNEXION", WINDOW_W / 2, 120, COLOR_NEON_CYAN);
    
    fill_rect(WINDOW_W / 2 - 100, 165, 200, 2, COLOR_NEON_CYAN);
    fill_rect(WINDOW_W / 2 - 80, 170, 160, 1, darken_color(COLOR_NEON_CYAN, 0.5f));
    
    {
        int icon_x = WINDOW_W / 2;
        int icon_y = 220;
        
        fill_rect(icon_x - 15, icon_y, 30, 30, COLOR_NEON_MAGENTA);
        fill_rect(icon_x - 10, icon_y + 5, 20, 20, COLOR_PANEL);
        fill_rect(icon_x - 5, icon_y + 8, 10, 10, COLOR_NEON_MAGENTA);
    }
    
    draw_text(font_S, "PSEUDO :", WINDOW_W / 2, 280, COLOR_NEON_CYAN);
    draw_input_field(70, 310, 400, 50, input_buffer, 1);
    
    draw_button(WINDOW_W / 2 - 90, 420, 180, 50, "VALIDER", COLOR_SUCCESS, strlen(input_buffer) == 0);
    draw_button(WINDOW_W / 2 - 90, 500, 180, 50, "RETOUR", COLOR_DANGER, 0);
}

void render_multi_choice(void) {
    int i;
    char line[64];
    
    draw_settings_gear(window_w - 50, 20);
    
    draw_text(font_L, "MULTIJOUEUR", WINDOW_W / 2 + 2, 80 + 2, darken_color(COLOR_NEON_PURPLE, 0.3f));
    draw_text(font_L, "MULTIJOUEUR", WINDOW_W / 2, 80, COLOR_NEON_PURPLE);
    
    {
        int panel_x = 50;
        int panel_y = 130;
        int panel_w = WINDOW_W - 100;
        int panel_h = 230;
        
        for (i = 4; i > 0; i--) {
            float alpha = (float)(4 - i + 1) / 5.0f * 0.1f;
            Uint32 glow_color = darken_color(COLOR_NEON_CYAN, alpha);
            fill_rect(panel_x - i, panel_y - i, panel_w + i * 2, panel_h + i * 2, glow_color);
        }
        
        fill_rect(panel_x, panel_y, panel_w, panel_h, COLOR_PANEL);
        
        fill_rect(panel_x, panel_y, panel_w, 2, COLOR_NEON_CYAN);
        fill_rect(panel_x, panel_y + panel_h - 2, panel_w, 2, COLOR_NEON_CYAN);
        fill_rect(panel_x, panel_y, 2, panel_h, COLOR_NEON_CYAN);
        fill_rect(panel_x + panel_w - 2, panel_y, 2, panel_h, COLOR_NEON_CYAN);
    }
    
    draw_text(font_S, "MEILLEURS SCORES", WINDOW_W / 2, 155, COLOR_NEON_CYAN);
    fill_rect(WINDOW_W / 2 - 80, 175, 160, 1, COLOR_NEON_CYAN);
    
    for (i = 0; i < leaderboard.count && i < 5; i++) {
        Uint32 entry_color;
        
        if (i == 0) entry_color = COLOR_GOLD;
        else if (i == 1) entry_color = 0xC0C0C0;
        else if (i == 2) entry_color = 0xCD7F32;
        else entry_color = COLOR_WHITE;
        
        snprintf(line, sizeof(line), "%d. %s", i + 1, leaderboard.names[i]);
        draw_text_left(font_S, line, 80, 195 + i * 32, entry_color);
        
        snprintf(line, sizeof(line), "%d pts", leaderboard.scores[i]);
        draw_text_left(font_S, line, WINDOW_W - 180, 195 + i * 32, COLOR_NEON_GREEN);
    }
    
    if (leaderboard.count == 0) {
        draw_text(font_S, "Aucun score enregistre", WINDOW_W / 2, 250, COLOR_GREY);
    }
    
    draw_button(WINDOW_W / 2 - 150, 400, 300, 55, "CREER UNE PARTIE", COLOR_SUCCESS, 0);
    draw_button(WINDOW_W / 2 - 150, 475, 300, 55, "REJOINDRE (CODE)", COLOR_BUTTON, 0);
    draw_button(WINDOW_W / 2 - 150, 550, 300, 55, "SERVEURS OUVERTS", COLOR_PURPLE, 0);
    draw_button(WINDOW_W / 2 - 90, 640, 180, 50, "RETOUR", COLOR_DANGER, 0);
}

void render_join_input(void) {
    draw_settings_gear(window_w - 50, 20);
    
    draw_text(font_L, "REJOINDRE", WINDOW_W / 2 + 2, 120 + 2, darken_color(COLOR_NEON_GREEN, 0.3f));
    draw_text(font_L, "REJOINDRE", WINDOW_W / 2, 120, COLOR_NEON_GREEN);
    
    fill_rect(WINDOW_W / 2 - 80, 160, 160, 2, COLOR_NEON_GREEN);
    
    {
        int box_x = 110;
        int box_y = 250;
        int box_w = 320;
        int box_h = 120;
        int i;
        
        for (i = 5; i > 0; i--) {
            float alpha = (float)(5 - i + 1) / 6.0f * 0.15f;
            Uint32 glow_color = darken_color(COLOR_NEON_GREEN, alpha);
            fill_rect(box_x - i, box_y - i, box_w + i * 2, box_h + i * 2, glow_color);
        }
        
        fill_rect(box_x, box_y, box_w, box_h, COLOR_PANEL);
        fill_rect(box_x, box_y, box_w, 2, COLOR_NEON_GREEN);
        fill_rect(box_x, box_y + box_h - 2, box_w, 2, COLOR_NEON_GREEN);
        fill_rect(box_x, box_y, 2, box_h, COLOR_NEON_GREEN);
        fill_rect(box_x + box_w - 2, box_y, 2, box_h, COLOR_NEON_GREEN);
    }
    
    draw_text(font_S, "CODE PARTIE :", WINDOW_W / 2, 275, COLOR_NEON_CYAN);
    draw_input_field(140, 300, 260, 50, input_buffer, 1);
    
    draw_button(WINDOW_W / 2 - 90, 420, 180, 50, "ENTRER", COLOR_SUCCESS, strlen(input_buffer) != 4);
    draw_button(WINDOW_W / 2 - 90, 500, 180, 50, "RETOUR", COLOR_DANGER, 0);
}

void render_server_browser(void) {
    int i, j;
    char line[128];
    int y_start = 150;
    int entry_height = 105;
    
    draw_settings_gear(window_w - 50, 20);
    
    draw_text(font_L, "SERVEURS", WINDOW_W / 2 + 2, 50 + 2, darken_color(COLOR_NEON_PURPLE, 0.3f));
    draw_text(font_L, "SERVEURS", WINDOW_W / 2, 50, COLOR_NEON_PURPLE);
    
    draw_text(font_S, "Parties Publiques", WINDOW_W / 2, 90, COLOR_NEON_CYAN);
    fill_rect(WINDOW_W / 2 - 80, 108, 160, 1, COLOR_NEON_CYAN);
    
    if (server_list.count == 0) {
        draw_neon_rect(WINDOW_W / 2 - 150, 300, 300, 120, COLOR_PANEL, COLOR_NEON_CYAN, 4);
        draw_text(font_S, "Aucun serveur", WINDOW_W / 2, 340, COLOR_GREY);
        draw_text(font_S, "disponible", WINDOW_W / 2, 370, COLOR_GREY);
    } else {
        for (j = 4; j > 0; j--) {
            float alpha = (float)(4 - j + 1) / 5.0f * 0.1f;
            Uint32 glow_color = darken_color(COLOR_NEON_CYAN, alpha);
            fill_rect(20 - j, 120 - j, WINDOW_W - 40 + j * 2, 520 + j * 2, glow_color);
        }
        fill_rect(20, 120, WINDOW_W - 40, 520, COLOR_PANEL);
        fill_rect(20, 120, WINDOW_W - 40, 2, COLOR_NEON_CYAN);
        fill_rect(20, 638, WINDOW_W - 40, 2, COLOR_NEON_MAGENTA);
        
        for (i = 0; i < server_list.count && i < 5; i++) {
            int idx = i + browser_scroll_offset;
            if (idx >= server_list.count) break;
            
            ServerInfo *srv = &server_list.servers[idx];
            int y = y_start + i * entry_height;
            
            fill_rect(30, y, WINDOW_W - 60, entry_height - 10, darken_color(COLOR_BG_LIGHTER, 0.8f));
            
            Uint32 status_color = srv->game_started ? COLOR_NEON_ORANGE : COLOR_NEON_GREEN;
            fill_rect(30, y, 4, entry_height - 10, status_color);
            
            snprintf(line, sizeof(line), "Code: %s", srv->room_code);
            draw_text_left(font_S, line, 45, y + 8, COLOR_NEON_CYAN);
            
            snprintf(line, sizeof(line), "Hote: %s", srv->host_name);
            draw_text_left(font_S, line, 180, y + 8, COLOR_WHITE);
            
            snprintf(line, sizeof(line), "Joueurs: %d/4", srv->player_count);
            draw_text_left(font_S, line, 45, y + 35, COLOR_GREY);
            
            const char *mode_str = srv->game_mode == GAME_MODE_RUSH ? "Rush" : "Classic";
            Uint32 mode_color = srv->game_mode == GAME_MODE_RUSH ? COLOR_NEON_ORANGE : COLOR_NEON_CYAN;
            snprintf(line, sizeof(line), "Mode: %s", mode_str);
            draw_text_left(font_S, line, 180, y + 35, mode_color);
            
            if (srv->game_started) {
                draw_text_left(font_S, "En cours", 45, y + 62, COLOR_NEON_ORANGE);
                draw_button(WINDOW_W - 140, y + 8, 100, 35, "Rejoindre", COLOR_BTN_DISABLED, 1);
            } else {
                draw_text_left(font_S, "En attente", 45, y + 62, COLOR_NEON_GREEN);
                draw_button(WINDOW_W - 140, y + 8, 100, 35, "Rejoindre", COLOR_SUCCESS, 0);
            }
            draw_button(WINDOW_W - 140, y + 50, 100, 35, "Observer", COLOR_PURPLE, 0);
        }
        
        if (browser_scroll_offset > 0) {
            draw_text(font_S, "^ Defiler ^", WINDOW_W / 2, 128, COLOR_NEON_CYAN);
        }
        if (browser_scroll_offset + 5 < server_list.count) {
            draw_text(font_S, "v Defiler v", WINDOW_W / 2, 648, COLOR_NEON_MAGENTA);
        }
    }
    
    draw_button(WINDOW_W / 2 - 90, 680, 180, 45, "Actualiser", COLOR_BUTTON, 0);
    draw_button(WINDOW_W / 2 - 90, 735, 180, 45, "RETOUR", COLOR_DANGER, 0);
}

void render_lobby(void) {
    int i;
    char title[64];
    char mode_str[32];
    
    draw_settings_gear(window_w - 50, 20);
    
    snprintf(title, sizeof(title), "CODE: %s", current_lobby.room_code);
    draw_text(font_L, title, WINDOW_W / 2 + 2, 50 + 2, darken_color(COLOR_NEON_GREEN, 0.3f));
    draw_text(font_L, title, WINDOW_W / 2, 50, COLOR_NEON_GREEN);
    
    if (current_lobby.is_public) {
        draw_text(font_S, "Partie Publique", WINDOW_W / 2, 90, COLOR_NEON_GREEN);
    } else {
        draw_text(font_S, "Partie Privee", WINDOW_W / 2, 90, COLOR_GREY);
    }
    
    {
        int panel_x = 50;
        int panel_y = 115;
        int panel_w = WINDOW_W - 100;
        int panel_h = 170;
        
        for (i = 4; i > 0; i--) {
            float alpha = (float)(4 - i + 1) / 5.0f * 0.1f;
            Uint32 glow_color = darken_color(COLOR_NEON_CYAN, alpha);
            fill_rect(panel_x - i, panel_y - i, panel_w + i * 2, panel_h + i * 2, glow_color);
        }
        fill_rect(panel_x, panel_y, panel_w, panel_h, COLOR_PANEL);
        fill_rect(panel_x, panel_y, panel_w, 2, COLOR_NEON_CYAN);
        fill_rect(panel_x, panel_y + panel_h - 2, panel_w, 2, COLOR_NEON_CYAN);
    }
    
    draw_text(font_S, "JOUEURS", WINDOW_W / 2, 135, COLOR_NEON_CYAN);
    fill_rect(WINDOW_W / 2 - 50, 152, 100, 1, COLOR_NEON_CYAN);
    
    for (i = 0; i < current_lobby.player_count; i++) {
        Uint32 color = COLOR_WHITE;
        int name_x = WINDOW_W / 2;
        int name_y = 175 + i * 32;
        
        if (strcmp(current_lobby.players[i], my_pseudo) == 0) {
            color = COLOR_NEON_CYAN;
            fill_rect(70, name_y - 12, WINDOW_W - 140, 24, darken_color(COLOR_NEON_CYAN, 0.15f));
        }
        
        if (current_lobby.is_spectator[i]) {
            draw_eye_icon(name_x - 100, name_y - 5, COLOR_NEON_PURPLE);
            draw_text(font_S, current_lobby.players[i], name_x, name_y, COLOR_NEON_PURPLE);
        } else {
            draw_text(font_S, current_lobby.players[i], name_x, name_y, color);
        }
        
        if (i == 0) {
            draw_text(font_S, "[HOST]", name_x + 100, name_y, COLOR_GOLD);
        }
    }
    
    if (current_lobby.is_host && !current_lobby.game_started) {
        int mode_y = 320;
        
        draw_text(font_S, "MODE DE JEU :", WINDOW_W / 2, mode_y, COLOR_WHITE);
        
        int classic_selected = (selected_game_mode == GAME_MODE_CLASSIC);
        Uint32 classic_color = classic_selected ? COLOR_CYAN : COLOR_BUTTON;
        draw_button(WINDOW_W / 2 - 160, mode_y + 25, 150, 35, "Classic", classic_color, 0);
        
        int rush_selected = (selected_game_mode == GAME_MODE_RUSH);
        Uint32 rush_color = rush_selected ? COLOR_CYAN : COLOR_BUTTON;
        draw_button(WINDOW_W / 2 + 10, mode_y + 25, 150, 35, "Rush", rush_color, 0);
        
        if (selected_game_mode == GAME_MODE_RUSH) {
            draw_text(font_S, "DUREE :", WINDOW_W / 2, mode_y + 80, COLOR_WHITE);
            
            draw_button(WINDOW_W / 2 - 200, mode_y + 100, 55, 30, "1m", 
                       selected_timer_minutes == 1 ? COLOR_CYAN : COLOR_BUTTON, 0);
            draw_button(WINDOW_W / 2 - 130, mode_y + 100, 55, 30, "2m", 
                       selected_timer_minutes == 2 ? COLOR_CYAN : COLOR_BUTTON, 0);
            draw_button(WINDOW_W / 2 - 60, mode_y + 100, 55, 30, "3m", 
                       selected_timer_minutes == 3 ? COLOR_CYAN : COLOR_BUTTON, 0);
            draw_button(WINDOW_W / 2 + 10, mode_y + 100, 55, 30, "5m", 
                       selected_timer_minutes == 5 ? COLOR_CYAN : COLOR_BUTTON, 0);
            draw_button(WINDOW_W / 2 + 80, mode_y + 100, 55, 30, "10m", 
                       selected_timer_minutes == 10 ? COLOR_CYAN : COLOR_BUTTON, 0);
        }
        
        int toggle_y = selected_game_mode == GAME_MODE_RUSH ? mode_y + 150 : mode_y + 80;
        const char *visibility_text = current_lobby.is_public ? "Rendre Privee" : "Rendre Publique";
        draw_button(WINDOW_W / 2 - 90, toggle_y, 180, 35, visibility_text, COLOR_PURPLE, 0);
        
        int start_y = selected_game_mode == GAME_MODE_RUSH ? 530 : 470;
        int need_players = current_lobby.player_count < 2 - current_lobby.spectator_count;
        
        if (need_players) {
            draw_text(font_S, "En attente d'autres joueurs...", WINDOW_W / 2, start_y, COLOR_GREY);
            start_y += 40;
        }
        
        draw_button(WINDOW_W / 2 - 150, start_y, 300, 55, "LANCER LA PARTIE", COLOR_SUCCESS, need_players);
    } else if (!current_lobby.is_host) {
        snprintf(mode_str, sizeof(mode_str), "Mode: %s", 
                 current_lobby.game_mode == GAME_MODE_RUSH ? "Rush" : "Classic");
        draw_text(font_S, mode_str, WINDOW_W / 2, 350, COLOR_GOLD);
        
        if (current_lobby.game_mode == GAME_MODE_RUSH) {
            snprintf(mode_str, sizeof(mode_str), "Duree: %d minutes", current_lobby.timer_minutes);
            draw_text(font_S, mode_str, WINDOW_W / 2, 380, COLOR_CYAN);
        }
        
        draw_text(font_S, "En attente du lancement...", WINDOW_W / 2, 450, COLOR_GREY);
    }
    
    draw_button(WINDOW_W / 2 - 90, 660, 180, 50, "QUITTER", COLOR_DANGER, 0);
    
    if (current_lobby.is_host && current_lobby.player_count > 1) {
        draw_text(font_S, "(Clic droit pour expulser)", WINDOW_W / 2, 720, COLOR_GREY);
    }
}

void render_game_grid(void) {
    render_game_grid_ex(&game, 0, 0);
}

void render_game_grid_ex(GameState *gs, int offset_x, int offset_y) {
    int i, j;
    int shake_x = 0, shake_y = 0;
    
    if (gs->effects.screen_shake > 0 && gs->effects.shake_time > 0) {
        shake_x = (rand() % (gs->effects.screen_shake * 2 + 1)) - gs->effects.screen_shake;
        shake_y = (rand() % (gs->effects.screen_shake * 2 + 1)) - gs->effects.screen_shake;
    }
    
    int base_x = grid_offset_x + offset_x + shake_x;
    int base_y = grid_offset_y + offset_y + shake_y;
    int grid_w = GRID_W * block_size;
    int grid_h = GRID_H * block_size;
    
    {
        int glow_layers = 6;
        for (i = glow_layers; i > 0; i--) {
            float alpha = (float)(glow_layers - i + 1) / (float)(glow_layers + 1) * 0.15f;
            Uint32 glow_color = darken_color(COLOR_NEON_CYAN, alpha);
            fill_rect(base_x - 8 - i, base_y - 8 - i, 
                      grid_w + 16 + i * 2, grid_h + 16 + i * 2, glow_color);
        }
    }
    
    fill_rect(base_x - 8, base_y - 8, grid_w + 16, grid_h + 16, COLOR_PANEL);
    
    fill_rect(base_x - 8, base_y - 8, grid_w + 16, 3, COLOR_NEON_CYAN);
    fill_rect(base_x - 8, base_y + grid_h + 5, grid_w + 16, 3, COLOR_NEON_MAGENTA);
    fill_rect(base_x - 8, base_y - 8, 3, grid_h + 16, COLOR_NEON_CYAN);
    fill_rect(base_x + grid_w + 5, base_y - 8, 3, grid_h + 16, COLOR_NEON_MAGENTA);
    
    fill_rect(base_x - 8, base_y - 8, 15, 3, 0xFFFFFF);
    fill_rect(base_x - 8, base_y - 8, 3, 15, 0xFFFFFF);
    fill_rect(base_x + grid_w - 7, base_y + grid_h + 5, 15, 3, 0xFFFFFF);
    fill_rect(base_x + grid_w + 5, base_y + grid_h - 7, 3, 15, 0xFFFFFF);
    
    fill_rect(base_x, base_y, grid_w, grid_h, COLOR_GRID_CELL);
    
    for (i = 0; i < GRID_H; i++) {
        for (j = 0; j < GRID_W; j++) {
            int cell_x = base_x + j * block_size;
            int cell_y = base_y + i * block_size;
            
            if ((i + j) % 2 == 0) {
                fill_rect(cell_x + 1, cell_y + 1, block_size - 2, block_size - 2, 
                          darken_color(COLOR_GRID_CELL, 0.85f));
            }
        }
    }
    
    for (i = 0; i <= GRID_H; i++) {
        fill_rect(base_x, base_y + i * block_size - 1, grid_w, 1, 
                  darken_color(COLOR_NEON_CYAN, 0.1f));
        fill_rect(base_x, base_y + i * block_size + 1, grid_w, 1, 
                  darken_color(COLOR_NEON_CYAN, 0.1f));
        fill_rect(base_x, base_y + i * block_size, grid_w, 1, COLOR_GRID);
    }
    for (j = 0; j <= GRID_W; j++) {
        fill_rect(base_x + j * block_size - 1, base_y, 1, grid_h, 
                  darken_color(COLOR_NEON_MAGENTA, 0.1f));
        fill_rect(base_x + j * block_size + 1, base_y, 1, grid_h, 
                  darken_color(COLOR_NEON_MAGENTA, 0.1f));
        fill_rect(base_x + j * block_size, base_y, 1, grid_h, COLOR_GRID);
    }
    
    for (i = 0; i < GRID_H; i++) {
        for (j = 0; j < GRID_W; j++) {
            if (gs->grid[i][j] != 0) {
                draw_styled_block(base_x + j * block_size + 2,
                                  base_y + i * block_size + 2,
                                  block_size - 4, gs->grid[i][j]);
            }
        }
    }
    
    render_effects(&gs->effects, shake_x + offset_x, shake_y + offset_y);
}

void render_mini_grid(int grid[GRID_H][GRID_W], int x, int y, int bs, const char *label, int score, int is_selected) {
    int i, j;
    int grid_w = GRID_W * bs;
    int grid_h = GRID_H * bs;
    char score_str[32];
    
    if (is_selected) {
        fill_rect(x - 4, y - 25, grid_w + 8, grid_h + 55, COLOR_CYAN);
    }
    
    fill_rect(x - 2, y - 2, grid_w + 4, grid_h + 4, 0x151520);
    fill_rect(x, y, grid_w, grid_h, 0x202030);
    
    for (i = 0; i < GRID_H; i++) {
        for (j = 0; j < GRID_W; j++) {
            if (grid[i][j] != 0) {
                fill_rect(x + j * bs, y + i * bs, bs - 1, bs - 1, grid[i][j]);
            }
        }
    }
    
    if (font_XS) {
        draw_text(font_XS, label, x + grid_w / 2, y - 12, COLOR_WHITE);
    }
    
    snprintf(score_str, sizeof(score_str), "%d pts", score);
    if (font_XS) {
        draw_text(font_XS, score_str, x + grid_w / 2, y + grid_h + 12, COLOR_GOLD);
    }
}

void render_pieces(int greyed) {
    int i, j, k;
    int slot_x, slot_y;
    int piece_block_size = 28;
    int slot_w, slot_h;
    
    if (layout_horizontal) {
        slot_w = (window_w / 2 - 80);
        slot_h = (window_h - 280) / 3;
        
        for (i = 0; i < 3; i++) {
            if (!game.pieces_available[i]) continue;
            if (selected_piece_idx == i) continue;
            
            Piece *p = &game.current_pieces[i];
            slot_x = window_w / 2 + 40;
            slot_y = 200 + i * slot_h;
            
            int piece_px = slot_x + (slot_w - p->w * piece_block_size) / 2;
            int piece_py = slot_y + (slot_h - p->h * piece_block_size) / 2;
            
            for (j = 0; j < p->h; j++) {
                for (k = 0; k < p->w; k++) {
                    if (p->data[j][k]) {
                        Uint32 color = p->color;
                        if (greyed) {
                            int r = ((color >> 16) & 0xFF) / 3;
                            int g = ((color >> 8) & 0xFF) / 3;
                            int b = (color & 0xFF) / 3;
                            color = (r << 16) | (g << 8) | b;
                        }
                        draw_styled_block(piece_px + k * piece_block_size, piece_py + j * piece_block_size, piece_block_size - 2, color);
                    }
                }
            }
        }
    } else {
        slot_w = (window_w - 60) / 3;
        slot_h = window_h - piece_area_y - 20;
        
        for (i = 0; i < 3; i++) {
            if (!game.pieces_available[i]) continue;
            if (selected_piece_idx == i) continue;
            
            Piece *p = &game.current_pieces[i];
            slot_x = 30 + i * slot_w;
            slot_y = piece_area_y;
            
            int piece_px = slot_x + (slot_w - p->w * piece_block_size) / 2;
            int piece_py = slot_y + (slot_h - p->h * piece_block_size) / 2;
            
            for (j = 0; j < p->h; j++) {
                for (k = 0; k < p->w; k++) {
                    if (p->data[j][k]) {
                        Uint32 color = p->color;
                        if (greyed) {
                            int r = ((color >> 16) & 0xFF) / 3;
                            int g = ((color >> 8) & 0xFF) / 3;
                            int b = (color & 0xFF) / 3;
                            color = (r << 16) | (g << 8) | b;
                        }
                        draw_styled_block(piece_px + k * piece_block_size, piece_py + j * piece_block_size, piece_block_size - 2, color);
                    }
                }
            }
        }
    }
}

void render_dragged_piece(void) {
    int j, k;
    
    if (selected_piece_idx < 0 || selected_piece_idx >= 3) return;
    
    Piece *p = &game.current_pieces[selected_piece_idx];
    
    int base_x = mouse_x - (p->w * block_size) / 2;
    int base_y = mouse_y - (p->h * block_size) / 2;
    
    for (j = 0; j < p->h; j++) {
        for (k = 0; k < p->w; k++) {
            if (p->data[j][k]) {
                draw_styled_block(base_x + k * block_size, 
                                  base_y + j * block_size, 
                                  block_size - 2, p->color);
            }
        }
    }
}

void render_solo(void) {
    char score_text[64];
    int i;
    
    draw_settings_gear(window_w - 50, 20);
    
    update_effects(&game.effects, delta_time);
    
    if (game.score > solo_high_score) {
        solo_high_score = game.score;
        save_game_data();
    }
    
    render_game_grid_ex(&game, 0, 0);
    
    if (layout_horizontal) {
        int right_x = window_w / 2 + 30;
        
        draw_text(font_S, "SCORE", right_x + 80, 40, COLOR_WHITE);
        snprintf(score_text, sizeof(score_text), "%d", game.score);
        draw_text(font_L, score_text, right_x + 80 + 2, 80 + 2, darken_color(COLOR_NEON_GREEN, 0.4f));
        draw_text(font_L, score_text, right_x + 80, 80, COLOR_NEON_GREEN);
        
        snprintf(score_text, sizeof(score_text), "BEST: %d", solo_high_score);
        draw_text(font_S, score_text, right_x + 80, 130, COLOR_GOLD);
        
        {
            int panel_x = right_x;
            int panel_y = 180;
            int panel_w = window_w / 2 - 50;
            int panel_h = window_h - 220;
            
            for (i = 3; i > 0; i--) {
                float alpha = (float)(3 - i + 1) / 4.0f * 0.1f;
                Uint32 glow_color = darken_color(COLOR_NEON_MAGENTA, alpha);
                fill_rect(panel_x - i, panel_y - i, panel_w + i * 2, panel_h + i * 2, glow_color);
            }
            fill_rect(panel_x, panel_y, panel_w, panel_h, COLOR_PANEL);
            fill_rect(panel_x, panel_y, panel_w, 2, COLOR_NEON_MAGENTA);
        }
    } else {
        snprintf(score_text, sizeof(score_text), "%d", game.score);
        draw_text(font_L, score_text, window_w / 2 + 2, 32, darken_color(COLOR_NEON_GREEN, 0.4f));
        draw_text(font_L, score_text, window_w / 2, 30, COLOR_NEON_GREEN);
        
        snprintf(score_text, sizeof(score_text), "BEST: %d", solo_high_score);
        draw_text(font_XS ? font_XS : font_S, score_text, window_w - 60, 25, COLOR_GOLD);
        
        {
            int panel_x = 20;
            int panel_y = piece_area_y - 15;
            int panel_w = window_w - 40;
            int panel_h = window_h - piece_area_y + 5;
            
            for (i = 3; i > 0; i--) {
                float alpha = (float)(3 - i + 1) / 4.0f * 0.1f;
                Uint32 glow_color = darken_color(COLOR_NEON_MAGENTA, alpha);
                fill_rect(panel_x - i, panel_y - i, panel_w + i * 2, panel_h + i * 2, glow_color);
            }
            fill_rect(panel_x, panel_y, panel_w, panel_h, COLOR_PANEL);
            fill_rect(panel_x, panel_y, panel_w, 2, COLOR_NEON_MAGENTA);
        }
    }
    
    render_pieces(0);
    render_dragged_piece();
    
    if (game.game_over) {
        fill_rect(0, 0, window_w, window_h, darken_color(COLOR_BG, 0.6f));
        
        int panel_x = 40;
        int panel_y = window_h / 2 - 100;
        int panel_w = window_w - 80;
        int panel_h = 200;
        
        for (i = 8; i > 0; i--) {
            float alpha = (float)(8 - i + 1) / 9.0f * 0.2f;
            Uint32 glow_color = darken_color(COLOR_NEON_RED, alpha);
            fill_rect(panel_x - i, panel_y - i, panel_w + i * 2, panel_h + i * 2, glow_color);
        }
        
        fill_rect(panel_x, panel_y, panel_w, panel_h, COLOR_PANEL);
        fill_rect(panel_x, panel_y, panel_w, 3, COLOR_NEON_RED);
        fill_rect(panel_x, panel_y + panel_h - 3, panel_w, 3, COLOR_NEON_RED);
        fill_rect(panel_x, panel_y, 3, panel_h, COLOR_NEON_RED);
        fill_rect(panel_x + panel_w - 3, panel_y, 3, panel_h, COLOR_NEON_RED);
        
        draw_text(font_L, "GAME OVER", window_w / 2 + 2, window_h / 2 - 50 + 2, darken_color(COLOR_NEON_RED, 0.4f));
        draw_text(font_L, "GAME OVER", window_w / 2, window_h / 2 - 50, COLOR_NEON_RED);
        
        snprintf(score_text, sizeof(score_text), "Score Final: %d", game.score);
        draw_text(font_S, score_text, window_w / 2, window_h / 2, COLOR_WHITE);
        
        if (game.score >= solo_high_score && solo_high_score > 0) {
            draw_text(font_S, "NOUVEAU RECORD!", window_w / 2, window_h / 2 + 35, COLOR_GOLD);
        }
        
        draw_text(font_S, "Cliquez pour continuer", window_w / 2, window_h / 2 + 70, COLOR_NEON_CYAN);
    }
}

void render_multi_game(void) {
    char score_text[32];
    char turn_text[64];
    int i;
    
    draw_settings_gear(window_w - 50, 20);
    
    update_effects(&game.effects, delta_time);
    
    if (current_lobby.game_mode == GAME_MODE_RUSH) {
        render_rush_game();
        return;
    }
    
    if (is_spectator) {
        draw_text(font_S, "MODE SPECTATEUR", WINDOW_W / 2, 25, COLOR_NEON_PURPLE);
        snprintf(turn_text, sizeof(turn_text), "Tour de %s", current_turn_pseudo);
        draw_text(font_S, turn_text, WINDOW_W / 2, 50, COLOR_GREY);
    } else if (is_my_turn()) {
        float pulse = (sinf(glow_pulse * 4.0f) + 1.0f) * 0.5f;
        Uint32 turn_color = blend_colors(COLOR_NEON_GREEN, COLOR_NEON_CYAN, pulse);
        draw_text(font_L, "A TOI!", WINDOW_W / 2 + 2, 35 + 2, darken_color(turn_color, 0.4f));
        draw_text(font_L, "A TOI!", WINDOW_W / 2, 35, turn_color);
    } else {
        snprintf(turn_text, sizeof(turn_text), "Tour de %s...", current_turn_pseudo);
        draw_text(font_S, turn_text, WINDOW_W / 2, 35, COLOR_GREY);
    }
    
    snprintf(score_text, sizeof(score_text), "Score: %d", game.score);
    draw_text(font_S, score_text, WINDOW_W / 2, 65, COLOR_NEON_GREEN);
    
    render_game_grid_ex(&game, 0, 0);
    
    {
        int panel_x = 20;
        int panel_y = piece_area_y - 15;
        int panel_w = WINDOW_W - 40;
        int panel_h = PIECE_SLOT_H + 30;
        
        for (i = 3; i > 0; i--) {
            float alpha = (float)(3 - i + 1) / 4.0f * 0.1f;
            Uint32 glow_color = darken_color(COLOR_NEON_MAGENTA, alpha);
            fill_rect(panel_x - i, panel_y - i, panel_w + i * 2, panel_h + i * 2, glow_color);
        }
        fill_rect(panel_x, panel_y, panel_w, panel_h, COLOR_PANEL);
        fill_rect(panel_x, panel_y, panel_w, 2, COLOR_NEON_MAGENTA);
    }
    
    render_pieces(!is_my_turn() || is_spectator);
    
    if (is_my_turn() && !is_spectator) {
        render_dragged_piece();
    }
    
    if (multi_game_over == 1) {
        fill_rect(0, 0, window_w, window_h, darken_color(COLOR_BG, 0.6f));
        
        int panel_x = 40;
        int panel_y = window_h / 2 - 100;
        int panel_w = window_w - 80;
        int panel_h = 200;
        
        for (i = 8; i > 0; i--) {
            float alpha = (float)(8 - i + 1) / 9.0f * 0.2f;
            Uint32 glow_color = darken_color(COLOR_NEON_RED, alpha);
            fill_rect(panel_x - i, panel_y - i, panel_w + i * 2, panel_h + i * 2, glow_color);
        }
        
        fill_rect(panel_x, panel_y, panel_w, panel_h, COLOR_PANEL);
        fill_rect(panel_x, panel_y, panel_w, 3, COLOR_NEON_RED);
        fill_rect(panel_x, panel_y + panel_h - 3, panel_w, 3, COLOR_NEON_RED);
        fill_rect(panel_x, panel_y, 3, panel_h, COLOR_NEON_RED);
        fill_rect(panel_x + panel_w - 3, panel_y, 3, panel_h, COLOR_NEON_RED);
        
        draw_text(font_L, "DEFAITE", window_w / 2 + 2, window_h / 2 - 50 + 2, darken_color(COLOR_NEON_RED, 0.4f));
        draw_text(font_L, "DEFAITE", window_w / 2, window_h / 2 - 50, COLOR_NEON_RED);
        
        snprintf(score_text, sizeof(score_text), "Score: %d", game.score);
        draw_text(font_S, score_text, window_w / 2, window_h / 2, COLOR_WHITE);
        
        snprintf(turn_text, sizeof(turn_text), "Gagnant: %s", multi_winner_name);
        draw_text(font_S, turn_text, window_w / 2, window_h / 2 + 35, COLOR_NEON_CYAN);
        
        draw_text(font_S, "Cliquez pour continuer", window_w / 2, window_h / 2 + 70, COLOR_GREY);
    }
    else if (multi_game_over == 2) {
        fill_rect(0, 0, window_w, window_h, darken_color(COLOR_BG, 0.6f));
        
        int panel_x = 40;
        int panel_y = window_h / 2 - 120;
        int panel_w = window_w - 80;
        int panel_h = 240;
        
        for (i = 8; i > 0; i--) {
            float alpha = (float)(8 - i + 1) / 9.0f * 0.2f;
            Uint32 glow_color = darken_color(COLOR_GOLD, alpha);
            fill_rect(panel_x - i, panel_y - i, panel_w + i * 2, panel_h + i * 2, glow_color);
        }
        
        fill_rect(panel_x, panel_y, panel_w, panel_h, COLOR_PANEL);
        fill_rect(panel_x, panel_y, panel_w, 3, COLOR_GOLD);
        fill_rect(panel_x, panel_y + panel_h - 3, panel_w, 3, COLOR_GOLD);
        fill_rect(panel_x, panel_y, 3, panel_h, COLOR_GOLD);
        fill_rect(panel_x + panel_w - 3, panel_y, 3, panel_h, COLOR_GOLD);
        
        {
            int cx = window_w / 2;
            int cy = window_h / 2 - 70;
            float pulse = (sinf(glow_pulse * 3.0f) + 1.0f) * 0.5f;
            Uint32 crown_color = blend_colors(COLOR_GOLD, 0xFFFFFF, pulse * 0.3f);
            
            fill_rect(cx - 40, cy, 80, 20, crown_color);
            fill_rect(cx - 50, cy - 25, 15, 25, crown_color);
            fill_rect(cx - 8, cy - 35, 16, 35, crown_color);
            fill_rect(cx + 35, cy - 25, 15, 25, crown_color);
            
            fill_rect(cx - 50, cy - 30, 15, 8, darken_color(crown_color, 0.8f));
            fill_rect(cx - 8, cy - 42, 16, 10, darken_color(crown_color, 0.8f));
            fill_rect(cx + 35, cy - 30, 15, 8, darken_color(crown_color, 0.8f));
        }
        
        draw_text(font_L, "VICTOIRE!", window_w / 2 + 2, window_h / 2 - 10 + 2, darken_color(COLOR_GOLD, 0.4f));
        draw_text(font_L, "VICTOIRE!", window_w / 2, window_h / 2 - 10, COLOR_GOLD);
        
        snprintf(score_text, sizeof(score_text), "Score: %d", game.score);
        draw_text(font_S, score_text, window_w / 2, window_h / 2 + 40, COLOR_WHITE);
        
        draw_text(font_S, "Cliquez pour continuer", window_w / 2, window_h / 2 + 80, COLOR_GREY);
    }
}

void render_rush_game(void) {
    char timer_text[32];
    char score_text[64];
    int i;
    
    int mins = rush_time_remaining / 60;
    int secs = rush_time_remaining % 60;
    snprintf(timer_text, sizeof(timer_text), "%d:%02d", mins, secs);
    
    Uint32 timer_color = COLOR_NEON_CYAN;
    if (rush_time_remaining <= 30) {
        float pulse = (sinf(glow_pulse * 6.0f) + 1.0f) * 0.5f;
        timer_color = blend_colors(COLOR_NEON_RED, 0xFFFFFF, pulse * 0.3f);
    } else if (rush_time_remaining <= 60) {
        timer_color = COLOR_NEON_ORANGE;
    }
    
    draw_text(font_L, timer_text, WINDOW_W / 2 + 2, 35 + 2, darken_color(timer_color, 0.4f));
    draw_text(font_L, timer_text, WINDOW_W / 2, 35, timer_color);
    
    if (is_spectator) {
        draw_text(font_L, "SPECTATEUR", WINDOW_W / 2, 70, COLOR_PURPLE);
        
        int mini_block = 15;
        int spacing = 20;
        int total_width = rush_player_count * (GRID_W * mini_block) + (rush_player_count - 1) * spacing;
        int start_x = (WINDOW_W - total_width) / 2;
        
        for (i = 0; i < rush_player_count; i++) {
            int grid_x = start_x + i * (GRID_W * mini_block + spacing);
            int is_selected = (spectate_view_idx == i);
            render_mini_grid(rush_states[i].grid, grid_x, 100, mini_block, 
                           rush_states[i].pseudo, rush_states[i].score, is_selected);
        }
        
        if (spectate_view_idx >= 0 && spectate_view_idx < rush_player_count) {
            memcpy(game.grid, rush_states[spectate_view_idx].grid, sizeof(game.grid));
            game.score = rush_states[spectate_view_idx].score;
            
            draw_text(font_S, rush_states[spectate_view_idx].pseudo, WINDOW_W / 2, 310, COLOR_CYAN);
            snprintf(score_text, sizeof(score_text), "Score: %d", rush_states[spectate_view_idx].score);
            draw_text(font_S, score_text, WINDOW_W / 2, 335, COLOR_GOLD);
            
            render_game_grid_ex(&game, 0, 280);
        }
        
        draw_text(font_S, "< > pour changer de joueur", WINDOW_W / 2, WINDOW_H - 30, COLOR_GREY);
    } else {
        render_game_grid_ex(&game, 0, 0);
        
        snprintf(score_text, sizeof(score_text), "%d", game.score);
        draw_text(font_S, "Score", 35, 100, COLOR_WHITE);
        draw_text(font_L, score_text, 35, 135, COLOR_GOLD);
        
        draw_text(font_XS ? font_XS : font_S, "Classement", 35, 180, COLOR_WHITE);
        for (i = 0; i < rush_player_count && i < 4; i++) {
            char entry[48];
            int entry_y = 205 + i * 40;
            Uint32 entry_color = (strcmp(rush_states[i].pseudo, my_pseudo) == 0) ? COLOR_CYAN : COLOR_WHITE;
            
            snprintf(entry, sizeof(entry), "%d. %s", i + 1, rush_states[i].pseudo);
            draw_text_left(font_XS ? font_XS : font_S, entry, 10, entry_y, entry_color);
            
            snprintf(entry, sizeof(entry), "   %d pts", rush_states[i].score);
            draw_text_left(font_XS ? font_XS : font_S, entry, 10, entry_y + 16, entry_color);
        }
        
        render_pieces(0);
        render_dragged_piece();
    }
    
    if (rush_time_remaining <= 0 && !is_spectator) {
        fill_rect(0, WINDOW_H / 2 - 80, WINDOW_W, 160, 0x000000DD);
        draw_text(font_L, "TEMPS ECOULE !", WINDOW_W / 2, WINDOW_H / 2 - 40, COLOR_ORANGE);
        
        int max_score = -1;
        int winner_idx = 0;
        for (i = 0; i < rush_player_count; i++) {
            if (rush_states[i].score > max_score) {
                max_score = rush_states[i].score;
                winner_idx = i;
            }
        }
        
        char winner_text[64];
        snprintf(winner_text, sizeof(winner_text), "Gagnant: %s (%d pts)", 
                 rush_states[winner_idx].pseudo, max_score);
        draw_text(font_S, winner_text, WINDOW_W / 2, WINDOW_H / 2 + 10, COLOR_GOLD);
        draw_text(font_S, "Cliquez pour quitter", WINDOW_W / 2, WINDOW_H / 2 + 50, COLOR_WHITE);
    }
}

void render_spectate(void) {
    draw_settings_gear(window_w - 50, 20);
    
    if (current_lobby.game_mode == GAME_MODE_RUSH) {
        render_rush_game();
    } else {
        draw_text(font_L, "SPECTATEUR", WINDOW_W / 2 + 2, 35 + 2, darken_color(COLOR_NEON_PURPLE, 0.3f));
        draw_text(font_L, "SPECTATEUR", WINDOW_W / 2, 35, COLOR_NEON_PURPLE);
        
        char info[64];
        snprintf(info, sizeof(info), "Tour de %s", current_turn_pseudo);
        draw_text(font_S, info, WINDOW_W / 2, 70, COLOR_GREY);
        
        render_game_grid_ex(&game, 0, 10);
        
        char score_text[32];
        snprintf(score_text, sizeof(score_text), "Score: %d", game.score);
        draw_text(font_S, score_text, WINDOW_W / 2, WINDOW_H - 80, COLOR_NEON_GREEN);
    }
    
    draw_button(WINDOW_W / 2 - 90, WINDOW_H - 50, 180, 40, "QUITTER", COLOR_DANGER, 0);
}
