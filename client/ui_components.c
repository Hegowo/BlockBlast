#include "ui_components.h"
#include "globals.h"
#include "graphics.h"
#include "audio.h"
#include "save_system.h"
#include "net_client.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

int draw_button(int x, int y, int w, int h, const char *lbl, Uint32 color, int disabled) {
    int hover;
    Uint32 border_color;
    int glow_intensity;
    float pulse;
    
    if (disabled) {
        fill_rect(x, y, w, h, COLOR_BTN_DISABLED);
        fill_rect(x, y, w, 1, 0x444455);
        fill_rect(x, y + h - 1, w, 1, 0x444455);
        fill_rect(x, y, 1, h, 0x444455);
        fill_rect(x + w - 1, y, 1, h, 0x444455);
        draw_text(font_S, lbl, x + w/2, y + h/2, COLOR_GREY);
        return 0;
    }
    
    hover = point_in_rect(mouse_x, mouse_y, x, y, w, h);
    
    if (color == COLOR_SUCCESS) {
        border_color = COLOR_NEON_GREEN;
    } else if (color == COLOR_DANGER) {
        border_color = COLOR_NEON_RED;
    } else if (color == COLOR_PURPLE) {
        border_color = COLOR_NEON_PURPLE;
    } else {
        border_color = COLOR_NEON_CYAN;
    }
    
    pulse = (sinf(glow_pulse * BUTTON_PULSE_SPEED) + 1.0f) * 0.5f;
    glow_intensity = hover ? (int)(4 + pulse * 3) : 2;
    
    {
        int i;
        for (i = glow_intensity; i > 0; i--) {
            float alpha = (float)(glow_intensity - i + 1) / (float)(glow_intensity + 1) * (hover ? 0.4f : 0.15f);
            Uint32 glow_color = darken_color(border_color, alpha);
            fill_rect(x - i, y - i, w + i * 2, h + i * 2, glow_color);
        }
    }
    
    if (hover) {
        draw_gradient_v(x, y, w, h, lighten_color(COLOR_BUTTON, 0.3f), COLOR_BUTTON);
    } else {
        draw_gradient_v(x, y, w, h, COLOR_BUTTON, darken_color(COLOR_BUTTON, 0.7f));
    }
    
    {
        Uint32 actual_border = hover ? lighten_color(border_color, 0.3f) : border_color;
        fill_rect(x, y, w, 2, actual_border);
        fill_rect(x, y + h - 2, w, 2, actual_border);
        fill_rect(x, y, 2, h, actual_border);
        fill_rect(x + w - 2, y, 2, h, actual_border);
        fill_rect(x + 4, y + 4, w - 8, 1, darken_color(actual_border, 0.5f));
    }
    
    if (hover) {
        draw_text(font_S, lbl, x + w/2 + 1, y + h/2 + 1, darken_color(border_color, 0.3f));
    }
    draw_text(font_S, lbl, x + w/2, y + h/2, COLOR_WHITE);
    
    return hover;
}

void draw_input_field(int x, int y, int w, int h, const char *text, int focused) {
    Uint32 bg_color = focused ? COLOR_INPUT_FOCUS : COLOR_INPUT;
    Uint32 border_color = focused ? COLOR_NEON_CYAN : darken_color(COLOR_NEON_CYAN, 0.4f);
    int glow_size = focused ? 4 : 0;
    int i;
    float pulse = (sinf(glow_pulse * 3.0f) + 1.0f) * 0.5f;
    
    if (focused) {
        for (i = glow_size; i > 0; i--) {
            float alpha = (float)(glow_size - i + 1) / (float)(glow_size + 1) * (0.2f + pulse * 0.1f);
            Uint32 glow_color = darken_color(COLOR_NEON_CYAN, alpha);
            fill_rect(x - i, y - i, w + i * 2, h + i * 2, glow_color);
        }
    }
    
    draw_gradient_v(x, y, w, h, bg_color, darken_color(bg_color, 0.7f));
    
    fill_rect(x, y, w, 2, border_color);
    fill_rect(x, y + h - 2, w, 2, border_color);
    fill_rect(x, y, 2, h, border_color);
    fill_rect(x + w - 2, y, 2, h, border_color);
    fill_rect(x + 2, y + 2, w - 4, 2, darken_color(bg_color, 0.5f));
    
    if (text && text[0]) {
        draw_text_left(font_S, text, x + 12, y + (h - 22) / 2, COLOR_WHITE);
    }
    
    if (focused) {
        int text_w = 0;
        if (text && text[0]) {
            TTF_SizeUTF8(font_S, text, &text_w, NULL);
        }
        
        if (((int)(glow_pulse * 2) % 2) == 0) {
            fill_rect(x + 12 + text_w + 2, y + 8, 2, h - 16, COLOR_NEON_CYAN);
        }
    }
}

void draw_settings_gear(int x, int y) {
    int size = 28;
    int cx = x + size / 2;
    int cy = y + size / 2;
    int hover = point_in_rect(mouse_x, mouse_y, x, y, size, size);
    Uint32 color = hover ? COLOR_NEON_CYAN : COLOR_GREY;
    
    fill_rect(cx - 10, cy - 2, 20, 4, color);
    fill_rect(cx - 2, cy - 10, 4, 20, color);
    
    fill_rect(cx - 8, cy - 8, 4, 4, color);
    fill_rect(cx + 4, cy - 8, 4, 4, color);
    fill_rect(cx - 8, cy + 4, 4, 4, color);
    fill_rect(cx + 4, cy + 4, 4, 4, color);
    
    fill_rect(cx - 4, cy - 4, 8, 8, COLOR_PANEL);
    fill_rect(cx - 3, cy - 3, 6, 6, color);
    fill_rect(cx - 2, cy - 2, 4, 4, COLOR_PANEL);
}

void draw_eye_icon(int x, int y, Uint32 color) {
    fill_rect(x + 2, y + 6, 12, 4, color);
    fill_rect(x + 6, y + 4, 4, 8, color);
    fill_rect(x + 7, y + 7, 2, 2, 0x000000);
}

void render_pause_menu(void) {
    int i;
    int overlay_w = 350;
    int overlay_h = 280;
    int overlay_x = (window_w - overlay_w) / 2;
    int overlay_y = (window_h - overlay_h) / 2;
    
    fill_rect(0, 0, window_w, window_h, darken_color(COLOR_BG, 0.7f));
    
    for (i = 6; i > 0; i--) {
        float alpha = (float)(6 - i + 1) / 7.0f * 0.2f;
        Uint32 glow_color = darken_color(COLOR_NEON_ORANGE, alpha);
        fill_rect(overlay_x - i, overlay_y - i, overlay_w + i * 2, overlay_h + i * 2, glow_color);
    }
    
    fill_rect(overlay_x, overlay_y, overlay_w, overlay_h, COLOR_PANEL);
    
    fill_rect(overlay_x, overlay_y, overlay_w, 3, COLOR_NEON_ORANGE);
    fill_rect(overlay_x, overlay_y + overlay_h - 3, overlay_w, 3, COLOR_NEON_ORANGE);
    fill_rect(overlay_x, overlay_y, 3, overlay_h, COLOR_NEON_ORANGE);
    fill_rect(overlay_x + overlay_w - 3, overlay_y, 3, overlay_h, COLOR_NEON_ORANGE);
    
    draw_text(font_L, "PAUSE", overlay_x + overlay_w / 2, overlay_y + 45, COLOR_NEON_ORANGE);
    fill_rect(overlay_x + 50, overlay_y + 70, overlay_w - 100, 2, COLOR_NEON_ORANGE);
    
    draw_button(overlay_x + overlay_w / 2 - 120, overlay_y + 95, 240, 50, "REPRENDRE", COLOR_SUCCESS, 0);
    draw_button(overlay_x + overlay_w / 2 - 120, overlay_y + 155, 240, 50, "RECOMMENCER", COLOR_NEON_CYAN, 0);
    draw_button(overlay_x + overlay_w / 2 - 120, overlay_y + 215, 240, 50, "QUITTER", COLOR_DANGER, 0);
}

int handle_pause_menu_click(void) {
    int overlay_w = 350;
    int overlay_h = 280;
    int overlay_x = (window_w - overlay_w) / 2;
    int overlay_y = (window_h - overlay_h) / 2;
    
    if (point_in_rect(mouse_x, mouse_y, overlay_x + overlay_w / 2 - 120, overlay_y + 95, 240, 50)) {
        play_click();
        show_pause_menu = 0;
        return 1;
    }
    
    if (point_in_rect(mouse_x, mouse_y, overlay_x + overlay_w / 2 - 120, overlay_y + 155, 240, 50)) {
        play_click();
        show_pause_menu = 0;
        delete_saved_game();
        init_game(&game);
        return 1;
    }
    
    if (point_in_rect(mouse_x, mouse_y, overlay_x + overlay_w / 2 - 120, overlay_y + 215, 240, 50)) {
        play_click();
        show_pause_menu = 0;
        save_current_game();
        current_state = ST_MENU;
        resume_music();
        return 1;
    }
    
    return 0;
}

void render_settings_overlay(void) {
    int i;
    int overlay_w = 450;
    int overlay_h = 480;
    int overlay_x = (window_w - overlay_w) / 2;
    int overlay_y = (window_h - overlay_h) / 2;
    
    fill_rect(0, 0, window_w, window_h, darken_color(COLOR_BG, 0.7f));
    
    for (i = 6; i > 0; i--) {
        float alpha = (float)(6 - i + 1) / 7.0f * 0.2f;
        Uint32 glow_color = darken_color(COLOR_NEON_CYAN, alpha);
        fill_rect(overlay_x - i, overlay_y - i, overlay_w + i * 2, overlay_h + i * 2, glow_color);
    }
    
    fill_rect(overlay_x, overlay_y, overlay_w, overlay_h, COLOR_PANEL);
    
    fill_rect(overlay_x, overlay_y, overlay_w, 3, COLOR_NEON_CYAN);
    fill_rect(overlay_x, overlay_y + overlay_h - 3, overlay_w, 3, COLOR_NEON_MAGENTA);
    fill_rect(overlay_x, overlay_y, 3, overlay_h, COLOR_NEON_CYAN);
    fill_rect(overlay_x + overlay_w - 3, overlay_y, 3, overlay_h, COLOR_NEON_MAGENTA);
    
    draw_text(font_L, "PARAMETRES", overlay_x + overlay_w / 2, overlay_y + 35, COLOR_NEON_CYAN);
    
    int tab_y = overlay_y + 65;
    int tab_w = (overlay_w - 40) / 2;
    
    Uint32 audio_tab_color = (settings_tab == 0) ? COLOR_NEON_CYAN : COLOR_GREY;
    fill_rect(overlay_x + 15, tab_y, tab_w, 35, (settings_tab == 0) ? darken_color(COLOR_NEON_CYAN, 0.2f) : COLOR_PANEL);
    fill_rect(overlay_x + 15, tab_y, tab_w, 2, audio_tab_color);
    draw_text(font_S, "AUDIO", overlay_x + 15 + tab_w / 2, tab_y + 18, audio_tab_color);
    
    Uint32 net_tab_color = (settings_tab == 1) ? COLOR_NEON_MAGENTA : COLOR_GREY;
    fill_rect(overlay_x + 25 + tab_w, tab_y, tab_w, 35, (settings_tab == 1) ? darken_color(COLOR_NEON_MAGENTA, 0.2f) : COLOR_PANEL);
    fill_rect(overlay_x + 25 + tab_w, tab_y, tab_w, 2, net_tab_color);
    draw_text(font_S, "RESEAU", overlay_x + 25 + tab_w + tab_w / 2, tab_y + 18, net_tab_color);
    
    fill_rect(overlay_x + 15, tab_y + 40, overlay_w - 30, 2, COLOR_GREY);
    
    int content_y = tab_y + 55;
    int slider_x = overlay_x + 40;
    int slider_w = overlay_w - 80;
    char vol_text[16];
    
    if (settings_tab == 0) {
        int music_slider_y = content_y + 15;
        draw_text(font_S, "Volume Musique", overlay_x + overlay_w / 2, music_slider_y, COLOR_WHITE);
        
        fill_rect(slider_x, music_slider_y + 30, slider_w, 24, darken_color(COLOR_PANEL, 0.5f));
        fill_rect(slider_x, music_slider_y + 30, slider_w, 2, COLOR_GREY);
        fill_rect(slider_x, music_slider_y + 52, slider_w, 2, COLOR_GREY);
        
        int music_fill_w = (music_volume * slider_w) / 128;
        fill_rect(slider_x, music_slider_y + 30, music_fill_w, 24, COLOR_NEON_CYAN);
        
        int music_handle_x = slider_x + music_fill_w - 6;
        fill_rect(music_handle_x, music_slider_y + 26, 12, 32, COLOR_WHITE);
        
        snprintf(vol_text, sizeof(vol_text), "%d%%", (music_volume * 100) / 128);
        draw_text(font_S, vol_text, overlay_x + overlay_w / 2, music_slider_y + 75, COLOR_NEON_CYAN);
        
        int sfx_slider_y = content_y + 120;
        draw_text(font_S, "Volume Effets Sonores", overlay_x + overlay_w / 2, sfx_slider_y, COLOR_WHITE);
        
        fill_rect(slider_x, sfx_slider_y + 30, slider_w, 24, darken_color(COLOR_PANEL, 0.5f));
        fill_rect(slider_x, sfx_slider_y + 30, slider_w, 2, COLOR_GREY);
        fill_rect(slider_x, sfx_slider_y + 52, slider_w, 2, COLOR_GREY);
        
        int sfx_fill_w = (sfx_volume * slider_w) / 128;
        fill_rect(slider_x, sfx_slider_y + 30, sfx_fill_w, 24, COLOR_NEON_MAGENTA);
        
        int sfx_handle_x = slider_x + sfx_fill_w - 6;
        fill_rect(sfx_handle_x, sfx_slider_y + 26, 12, 32, COLOR_WHITE);
        
        snprintf(vol_text, sizeof(vol_text), "%d%%", (sfx_volume * 100) / 128);
        draw_text(font_S, vol_text, overlay_x + overlay_w / 2, sfx_slider_y + 75, COLOR_NEON_MAGENTA);
        
    } else {
        int ip_y = content_y + 15;
        draw_text(font_S, "Adresse IP du serveur", overlay_x + overlay_w / 2, ip_y, COLOR_WHITE);
        draw_input_field(overlay_x + 30, ip_y + 25, overlay_w - 60, 45, edit_ip, active_input == 1);
        
        int port_y = content_y + 110;
        draw_text(font_S, "Port", overlay_x + overlay_w / 2, port_y, COLOR_WHITE);
        draw_input_field(overlay_x + 30, port_y + 25, overlay_w - 60, 45, edit_port, active_input == 2);
        
        draw_text(font_XS ? font_XS : font_S, "Cliquez sur un champ pour le modifier", 
                  overlay_x + overlay_w / 2, content_y + 200, COLOR_GREY);
    }
    
    int close_btn_y = overlay_y + overlay_h - 65;
    draw_button(overlay_x + overlay_w / 2 - 90, close_btn_y, 180, 45, "FERMER", COLOR_DANGER, 0);
    
    draw_text(font_XS ? font_XS : font_S, "Appuyez sur ESC pour fermer", 
              overlay_x + overlay_w / 2, overlay_y + overlay_h - 15, COLOR_GREY);
}

int handle_settings_overlay_mousedown(void) {
    int overlay_w = 450;
    int overlay_h = 480;
    int overlay_x = (window_w - overlay_w) / 2;
    int overlay_y = (window_h - overlay_h) / 2;
    int slider_x = overlay_x + 40;
    int slider_w = overlay_w - 80;
    int tab_y = overlay_y + 65;
    int content_y = tab_y + 55;
    
    if (settings_tab == 0) {
        int music_slider_y = content_y + 15 + 30;
        if (mouse_y >= music_slider_y - 5 && mouse_y <= music_slider_y + 30 &&
            mouse_x >= slider_x - 10 && mouse_x <= slider_x + slider_w + 10) {
            dragging_music_slider = 1;
            int new_vol = ((mouse_x - slider_x) * 128) / slider_w;
            if (new_vol < 0) new_vol = 0;
            if (new_vol > 128) new_vol = 128;
            music_volume = new_vol;
            update_music_volume();
            return 1;
        }
        
        int sfx_slider_y = content_y + 120 + 30;
        if (mouse_y >= sfx_slider_y - 5 && mouse_y <= sfx_slider_y + 30 &&
            mouse_x >= slider_x - 10 && mouse_x <= slider_x + slider_w + 10) {
            dragging_sfx_slider = 1;
            int new_vol = ((mouse_x - slider_x) * 128) / slider_w;
            if (new_vol < 0) new_vol = 0;
            if (new_vol > 128) new_vol = 128;
            sfx_volume = new_vol;
            update_sfx_volume();
            return 1;
        }
    }
    
    return 0;
}

int handle_settings_overlay_click(void) {
    int overlay_w = 450;
    int overlay_h = 480;
    int overlay_x = (window_w - overlay_w) / 2;
    int overlay_y = (window_h - overlay_h) / 2;
    int tab_y = overlay_y + 65;
    int tab_w = (overlay_w - 40) / 2;
    int content_y = tab_y + 55;
    
    if (dragging_sfx_slider) {
        play_click();
    }
    dragging_music_slider = 0;
    dragging_sfx_slider = 0;
    
    if (mouse_y >= tab_y && mouse_y <= tab_y + 35) {
        if (mouse_x >= overlay_x + 15 && mouse_x <= overlay_x + 15 + tab_w) {
            if (settings_tab != 0) {
                settings_tab = 0;
                active_input = 0;
                play_click();
            }
            return 1;
        }
        if (mouse_x >= overlay_x + 25 + tab_w && mouse_x <= overlay_x + 25 + tab_w + tab_w) {
            if (settings_tab != 1) {
                settings_tab = 1;
                strcpy(edit_ip, online_ip);
                snprintf(edit_port, sizeof(edit_port), "%d", online_port);
                active_input = 0;
                play_click();
            }
            return 1;
        }
    }
    
    if (settings_tab == 1) {
        int ip_y = content_y + 15 + 25;
        int port_y = content_y + 110 + 25;
        
        if (point_in_rect(mouse_x, mouse_y, overlay_x + 30, ip_y, overlay_w - 60, 45)) {
            active_input = 1;
            play_click();
            return 1;
        }
        if (point_in_rect(mouse_x, mouse_y, overlay_x + 30, port_y, overlay_w - 60, 45)) {
            active_input = 2;
            play_click();
            return 1;
        }
    }
    
    int close_btn_y = overlay_y + overlay_h - 65;
    if (point_in_rect(mouse_x, mouse_y, overlay_x + overlay_w / 2 - 90, close_btn_y, 180, 45)) {
        if (settings_tab == 1) {
            strcpy(online_ip, edit_ip);
            online_port = atoi(edit_port);
            if (online_port <= 0) online_port = 12345;
        }
        save_game_data();
        show_settings_overlay = 0;
        play_click();
        return 1;
    }
    
    return 0;
}

void handle_settings_overlay_drag(void) {
    if (!dragging_music_slider && !dragging_sfx_slider) return;
    
    int overlay_w = 450;
    int overlay_x = (window_w - overlay_w) / 2;
    int slider_x = overlay_x + 40;
    int slider_w = overlay_w - 80;
    
    if (dragging_music_slider) {
        int new_vol = ((mouse_x - slider_x) * 128) / slider_w;
        if (new_vol < 0) new_vol = 0;
        if (new_vol > 128) new_vol = 128;
        if (new_vol != music_volume) {
            music_volume = new_vol;
            update_music_volume();
        }
    }
    
    if (dragging_sfx_slider) {
        int new_vol = ((mouse_x - slider_x) * 128) / slider_w;
        if (new_vol < 0) new_vol = 0;
        if (new_vol > 128) new_vol = 128;
        if (new_vol != sfx_volume) {
            sfx_volume = new_vol;
            update_sfx_volume();
        }
    }
}

void draw_popup(void) {
    int i;
    int popup_x = 40;
    int popup_y = WINDOW_H / 2 - 80;
    int popup_w = WINDOW_W - 80;
    int popup_h = 160;
    
    if (!popup_msg[0]) return;
    
    fill_rect(0, 0, WINDOW_W, WINDOW_H, darken_color(COLOR_BG, 0.5f));
    
    for (i = 8; i > 0; i--) {
        float alpha = (float)(8 - i + 1) / 9.0f * 0.25f;
        Uint32 glow_color = darken_color(COLOR_NEON_CYAN, alpha);
        fill_rect(popup_x - i, popup_y - i, popup_w + i * 2, popup_h + i * 2, glow_color);
    }
    
    draw_gradient_v(popup_x, popup_y, popup_w, popup_h, COLOR_PANEL, darken_color(COLOR_PANEL, 0.6f));
    
    fill_rect(popup_x, popup_y, popup_w, 3, COLOR_NEON_CYAN);
    fill_rect(popup_x, popup_y + popup_h - 3, popup_w, 3, COLOR_NEON_MAGENTA);
    fill_rect(popup_x, popup_y, 3, popup_h, COLOR_NEON_CYAN);
    fill_rect(popup_x + popup_w - 3, popup_y, 3, popup_h, COLOR_NEON_MAGENTA);
    
    fill_rect(popup_x, popup_y, 20, 3, 0xFFFFFF);
    fill_rect(popup_x, popup_y, 3, 20, 0xFFFFFF);
    fill_rect(popup_x + popup_w - 20, popup_y + popup_h - 3, 20, 3, 0xFFFFFF);
    fill_rect(popup_x + popup_w - 3, popup_y + popup_h - 20, 3, 20, 0xFFFFFF);
    
    draw_text(font_S, popup_msg, WINDOW_W / 2 + 1, WINDOW_H / 2 - 20 + 1, darken_color(COLOR_NEON_CYAN, 0.3f));
    draw_text(font_S, popup_msg, WINDOW_W / 2, WINDOW_H / 2 - 20, COLOR_WHITE);
    
    draw_text(font_S, "(Cliquez pour fermer)", WINDOW_W / 2, WINDOW_H / 2 + 30, COLOR_NEON_MAGENTA);
}

void handle_input_char(Uint16 unicode) {
    char c = (char)unicode;
    int len;
    
    if (current_state == ST_OPTIONS) {
        if (active_input == 1) {
            len = strlen(edit_ip);
            if (unicode == 8 && len > 0) {
                edit_ip[len - 1] = '\0';
            } else if ((c >= '0' && c <= '9') || c == '.') {
                if (len < 30) {
                    edit_ip[len] = c;
                    edit_ip[len + 1] = '\0';
                }
            }
            test_result = 0;
        } else if (active_input == 2) {
            len = strlen(edit_port);
            if (unicode == 8 && len > 0) {
                edit_port[len - 1] = '\0';
            } else if (c >= '0' && c <= '9') {
                if (len < 6) {
                    edit_port[len] = c;
                    edit_port[len + 1] = '\0';
                }
            }
            test_result = 0;
        }
    } else if (show_settings_overlay && settings_tab == 1) {
        if (active_input == 1) {
            len = strlen(edit_ip);
            if (unicode == 8 && len > 0) {
                edit_ip[len - 1] = '\0';
            } else if ((c >= '0' && c <= '9') || c == '.') {
                if (len < 30) {
                    edit_ip[len] = c;
                    edit_ip[len + 1] = '\0';
                }
            }
        } else if (active_input == 2) {
            len = strlen(edit_port);
            if (unicode == 8 && len > 0) {
                edit_port[len - 1] = '\0';
            } else if (c >= '0' && c <= '9') {
                if (len < 6) {
                    edit_port[len] = c;
                    edit_port[len + 1] = '\0';
                }
            }
        }
    } else {
        len = strlen(input_buffer);
        
        if (unicode == 8 && len > 0) {
            input_buffer[len - 1] = '\0';
        } else if (len < 12) {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') || c == ' ') {
                
                if (current_state == ST_JOIN_INPUT && c >= 'a' && c <= 'z') {
                    c = c - 'a' + 'A';
                }
                
                input_buffer[len] = c;
                input_buffer[len + 1] = '\0';
            }
        }
    }
}

int perform_connection_test(void) {
    int port = atoi(edit_port);
    
    if (net_connect(edit_ip, port)) {
        net_close();
        return 1;
    }
    return 0;
}
