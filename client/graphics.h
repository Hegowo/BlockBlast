#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <SDL.h>
#include <SDL_ttf.h>
#include "game.h"

Uint32 get_pixel(int r, int g, int b);
Uint32 color_to_pixel(Uint32 color);
Uint32 blend_colors(Uint32 c1, Uint32 c2, float t);
Uint32 darken_color(Uint32 color, float factor);
Uint32 lighten_color(Uint32 color, float factor);

void fill_rect(int x, int y, int w, int h, Uint32 color);
void draw_gradient_v(int x, int y, int w, int h, Uint32 color_top, Uint32 color_bottom);
void draw_gradient_h(int x, int y, int w, int h, Uint32 color_left, Uint32 color_right);

void draw_neon_border(int x, int y, int w, int h, Uint32 color, int glow_size);
void draw_neon_rect(int x, int y, int w, int h, Uint32 bg_color, Uint32 border_color, int glow_size);
void draw_neon_block(int x, int y, int size, Uint32 color);
void draw_styled_block(int x, int y, int size, Uint32 color);
void draw_styled_block_scaled(int x, int y, int size, Uint32 color, float scale);

void draw_text(TTF_Font *f, const char *txt, int cx, int cy, Uint32 col_val);
void draw_text_left(TTF_Font *f, const char *txt, int x, int y, Uint32 col_val);

void draw_cyberpunk_background_responsive(void);

void render_effects(EffectsManager *em, int offset_x, int offset_y);

int point_in_rect(int px, int py, int x, int y, int w, int h);

#endif
