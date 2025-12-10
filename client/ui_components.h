#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include <SDL.h>

int draw_button(int x, int y, int w, int h, const char *lbl, Uint32 color, int disabled);
void draw_input_field(int x, int y, int w, int h, const char *text, int focused);

void draw_settings_gear(int x, int y);
void draw_eye_icon(int x, int y, Uint32 color);

void render_pause_menu(void);
int handle_pause_menu_click(void);

void render_settings_overlay(void);
int handle_settings_overlay_mousedown(void);
int handle_settings_overlay_click(void);
void handle_settings_overlay_drag(void);

void draw_popup(void);

void handle_input_char(Uint16 unicode);

int perform_connection_test(void);

#endif
