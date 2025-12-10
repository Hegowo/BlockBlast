#include "graphics.h"
#include "globals.h"

Uint32 get_pixel(int r, int g, int b) {
    return SDL_MapRGB(screen->format, r, g, b);
}

Uint32 color_to_pixel(Uint32 color) {
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;
    return get_pixel(r, g, b);
}

Uint32 blend_colors(Uint32 c1, Uint32 c2, float t) {
    int r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
    int r2 = (c2 >> 16) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = c2 & 0xFF;
    int r = (int)(r1 + (r2 - r1) * t);
    int g = (int)(g1 + (g2 - g1) * t);
    int b = (int)(b1 + (b2 - b1) * t);
    return (r << 16) | (g << 8) | b;
}

Uint32 darken_color(Uint32 color, float factor) {
    int r = (int)(((color >> 16) & 0xFF) * factor);
    int g = (int)(((color >> 8) & 0xFF) * factor);
    int b = (int)((color & 0xFF) * factor);
    return (r << 16) | (g << 8) | b;
}

Uint32 lighten_color(Uint32 color, float factor) {
    int r = (int)(((color >> 16) & 0xFF) + (255 - ((color >> 16) & 0xFF)) * factor);
    int g = (int)(((color >> 8) & 0xFF) + (255 - ((color >> 8) & 0xFF)) * factor);
    int b = (int)((color & 0xFF) + (255 - (color & 0xFF)) * factor);
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return (r << 16) | (g << 8) | b;
}

void fill_rect(int x, int y, int w, int h, Uint32 color) {
    SDL_Rect rect = {x, y, w, h};
    SDL_FillRect(screen, &rect, color_to_pixel(color));
}

void draw_gradient_v(int x, int y, int w, int h, Uint32 color_top, Uint32 color_bottom) {
    int i;
    for (i = 0; i < h; i++) {
        float t = (float)i / (float)h;
        Uint32 color = blend_colors(color_top, color_bottom, t);
        fill_rect(x, y + i, w, 1, color);
    }
}

void draw_gradient_h(int x, int y, int w, int h, Uint32 color_left, Uint32 color_right) {
    int i;
    for (i = 0; i < w; i++) {
        float t = (float)i / (float)w;
        Uint32 color = blend_colors(color_left, color_right, t);
        fill_rect(x + i, y, 1, h, color);
    }
}

void draw_neon_border(int x, int y, int w, int h, Uint32 color, int glow_size) {
    int i;
    
    for (i = glow_size; i > 0; i--) {
        float alpha = (float)(glow_size - i + 1) / (float)(glow_size + 1) * 0.3f;
        Uint32 glow_color = darken_color(color, alpha);
        
        fill_rect(x - i, y - i, w + i * 2, 1, glow_color);
        fill_rect(x - i, y + h + i - 1, w + i * 2, 1, glow_color);
        fill_rect(x - i, y - i, 1, h + i * 2, glow_color);
        fill_rect(x + w + i - 1, y - i, 1, h + i * 2, glow_color);
    }
    
    fill_rect(x, y, w, 2, color);
    fill_rect(x, y + h - 2, w, 2, color);
    fill_rect(x, y, 2, h, color);
    fill_rect(x + w - 2, y, 2, h, color);
}

void draw_neon_rect(int x, int y, int w, int h, Uint32 bg_color, Uint32 border_color, int glow_size) {
    int i;
    for (i = glow_size; i > 0; i--) {
        float alpha = (float)(glow_size - i + 1) / (float)(glow_size + 1) * 0.2f;
        Uint32 glow_color = darken_color(border_color, alpha);
        fill_rect(x - i, y - i, w + i * 2, h + i * 2, glow_color);
    }
    
    fill_rect(x, y, w, h, bg_color);
    
    fill_rect(x, y, w, 2, border_color);
    fill_rect(x, y + h - 2, w, 2, border_color);
    fill_rect(x, y, 2, h, border_color);
    fill_rect(x + w - 2, y, 2, h, border_color);
}

void draw_neon_block(int x, int y, int size, Uint32 color) {
    int glow_size = 3;
    int i;
    Uint32 inner_color = darken_color(color, 0.4f);
    Uint32 highlight = lighten_color(color, 0.5f);
    
    for (i = glow_size; i > 0; i--) {
        float alpha = (float)(glow_size - i + 1) / (float)(glow_size + 1) * 0.25f;
        Uint32 glow_color = darken_color(color, alpha);
        fill_rect(x - i, y - i, size + i * 2, size + i * 2, glow_color);
    }
    
    fill_rect(x, y, size, size, color);
    fill_rect(x + 4, y + 4, size - 8, size - 8, inner_color);
    fill_rect(x + 6, y + 6, size - 12, size - 12, darken_color(color, 0.6f));
    fill_rect(x + 2, y + 2, size - 4, 2, highlight);
    fill_rect(x + 2, y + 2, 2, size - 4, lighten_color(color, 0.3f));
    fill_rect(x + 2, y + size - 4, size - 4, 2, darken_color(color, 0.3f));
    fill_rect(x + size - 4, y + 2, 2, size - 4, darken_color(color, 0.3f));
    fill_rect(x + 3, y + 3, 4, 2, 0xFFFFFF);
    fill_rect(x + 3, y + 3, 2, 4, 0xFFFFFF);
}

void draw_styled_block(int x, int y, int size, Uint32 color) {
    draw_neon_block(x, y, size, color);
}

void draw_styled_block_scaled(int x, int y, int size, Uint32 color, float scale) {
    int scaled_size = (int)(size * scale);
    int offset = (size - scaled_size) / 2;
    int glow_size = (int)(2 * scale);
    int i;
    Uint32 inner_color = darken_color(color, 0.4f);
    Uint32 highlight = lighten_color(color, 0.4f);
    
    int bx = x + offset;
    int by = y + offset;
    
    for (i = glow_size; i > 0; i--) {
        float alpha = (float)(glow_size - i + 1) / (float)(glow_size + 1) * 0.2f;
        Uint32 glow_color = darken_color(color, alpha);
        fill_rect(bx - i, by - i, scaled_size + i * 2, scaled_size + i * 2, glow_color);
    }
    
    fill_rect(bx, by, scaled_size, scaled_size, color);
    
    int inner_margin = (int)(3 * scale);
    if (inner_margin < 1) inner_margin = 1;
    fill_rect(bx + inner_margin, by + inner_margin, 
              scaled_size - inner_margin * 2, scaled_size - inner_margin * 2, inner_color);
    
    fill_rect(bx + 1, by + 1, scaled_size - 2, 1, highlight);
    
    if (scale > 0.5f) {
        fill_rect(bx + 2, by + 2, 2, 1, 0xFFFFFF);
    }
}

void draw_text(TTF_Font *f, const char *txt, int cx, int cy, Uint32 col_val) {
    SDL_Color col;
    SDL_Surface *surface;
    SDL_Rect dest;
    
    if (!txt || !txt[0]) return;
    
    col.r = (col_val >> 16) & 0xFF;
    col.g = (col_val >> 8) & 0xFF;
    col.b = col_val & 0xFF;
    
    surface = TTF_RenderUTF8_Blended(f, txt, col);
    if (!surface) return;
    
    dest.x = cx - surface->w / 2;
    dest.y = cy - surface->h / 2;
    dest.w = surface->w;
    dest.h = surface->h;
    
    SDL_BlitSurface(surface, NULL, screen, &dest);
    SDL_FreeSurface(surface);
}

void draw_text_left(TTF_Font *f, const char *txt, int x, int y, Uint32 col_val) {
    SDL_Color col;
    SDL_Surface *surface;
    SDL_Rect dest;
    
    if (!txt || !txt[0]) return;
    
    col.r = (col_val >> 16) & 0xFF;
    col.g = (col_val >> 8) & 0xFF;
    col.b = col_val & 0xFF;
    
    surface = TTF_RenderUTF8_Blended(f, txt, col);
    if (!surface) return;
    
    dest.x = x;
    dest.y = y;
    dest.w = surface->w;
    dest.h = surface->h;
    
    SDL_BlitSurface(surface, NULL, screen, &dest);
    SDL_FreeSurface(surface);
}

void draw_cyberpunk_background_responsive(void) {
    int i, j;
    int grid_spacing = 60;
    
    draw_gradient_v(0, 0, window_w, window_h, COLOR_BG, COLOR_BG_DARK);
    
    for (i = 0; i < window_h; i += 3) {
        fill_rect(0, i, window_w, 1, darken_color(COLOR_BG, 0.8f));
    }
    
    for (i = 0; i < window_w; i += grid_spacing) {
        fill_rect(i - 1, 0, 1, window_h, darken_color(COLOR_NEON_CYAN, 0.05f));
        fill_rect(i + 1, 0, 1, window_h, darken_color(COLOR_NEON_CYAN, 0.05f));
        fill_rect(i, 0, 1, window_h, darken_color(COLOR_NEON_CYAN, 0.15f));
    }
    
    for (j = 0; j < window_h; j += grid_spacing) {
        fill_rect(0, j - 1, window_w, 1, darken_color(COLOR_NEON_MAGENTA, 0.05f));
        fill_rect(0, j + 1, window_w, 1, darken_color(COLOR_NEON_MAGENTA, 0.05f));
        fill_rect(0, j, window_w, 1, darken_color(COLOR_NEON_MAGENTA, 0.15f));
    }
    
    fill_rect(10, 10, 50, 2, COLOR_NEON_CYAN);
    fill_rect(10, 10, 2, 30, COLOR_NEON_CYAN);
    
    fill_rect(window_w - 60, 10, 50, 2, COLOR_NEON_CYAN);
    fill_rect(window_w - 12, 10, 2, 30, COLOR_NEON_CYAN);
    
    fill_rect(10, window_h - 12, 50, 2, COLOR_NEON_MAGENTA);
    fill_rect(10, window_h - 40, 2, 30, COLOR_NEON_MAGENTA);
    
    fill_rect(window_w - 60, window_h - 12, 50, 2, COLOR_NEON_MAGENTA);
    fill_rect(window_w - 12, window_h - 40, 2, 30, COLOR_NEON_MAGENTA);
}

void render_effects(EffectsManager *em, int offset_x, int offset_y) {
    int i;
    
    for (i = 0; i < MAX_PARTICLES; i++) {
        if (em->particles[i].active) {
            Particle *p = &em->particles[i];
            float alpha = p->life;
            if (alpha > 1.0f) alpha = 1.0f;
            if (alpha < 0.0f) alpha = 0.0f;
            
            Uint32 color = p->color;
            int px = (int)p->x + offset_x;
            int py = (int)p->y + offset_y;
            
            Uint32 glow_color = darken_color(color, alpha * 0.3f);
            fill_rect(px - p->size, py - p->size, p->size * 2, p->size * 2, glow_color);
            
            int r = (int)(((color >> 16) & 0xFF) * alpha);
            int g = (int)(((color >> 8) & 0xFF) * alpha);
            int b = (int)((color & 0xFF) * alpha);
            Uint32 core_color = (r << 16) | (g << 8) | b;
            
            fill_rect(px - p->size/2, py - p->size/2, p->size, p->size, core_color);
            
            if (alpha > 0.5f) {
                int center_size = p->size / 3;
                if (center_size < 1) center_size = 1;
                fill_rect(px - center_size/2, py - center_size/2, center_size, center_size, 0xFFFFFF);
            }
        }
    }
    
    for (i = 0; i < 20; i++) {
        if (em->line_clears[i].active) {
            LineClearEffect *lc = &em->line_clears[i];
            float progress = lc->progress;
            float intensity = (1.0f - progress);
            if (intensity < 0) intensity = 0;
            
            Uint32 flash_colors[] = {COLOR_NEON_CYAN, COLOR_NEON_MAGENTA, COLOR_NEON_GREEN};
            Uint32 flash_color = flash_colors[i % 3];
            
            if (lc->row >= 0) {
                int sweep_pos = (int)(progress * GRID_W * BLOCK_SIZE);
                int flash_width = (int)(GRID_W * BLOCK_SIZE * (1.0f - progress * 0.5f));
                
                Uint32 glow = darken_color(flash_color, intensity * 0.4f);
                fill_rect(GRID_OFFSET_X + offset_x - 5, 
                          GRID_OFFSET_Y + lc->row * BLOCK_SIZE + offset_y - 5,
                          GRID_W * BLOCK_SIZE + 10, BLOCK_SIZE + 10, glow);
                
                Uint32 main_flash = darken_color(flash_color, intensity * 0.8f);
                fill_rect(GRID_OFFSET_X + offset_x, 
                          GRID_OFFSET_Y + lc->row * BLOCK_SIZE + offset_y,
                          flash_width, BLOCK_SIZE, main_flash);
                
                if (progress < 0.8f) {
                    fill_rect(GRID_OFFSET_X + offset_x + sweep_pos - 5, 
                              GRID_OFFSET_Y + lc->row * BLOCK_SIZE + offset_y,
                              10, BLOCK_SIZE, 0xFFFFFF);
                }
            }
            if (lc->col >= 0) {
                int sweep_pos = (int)(progress * GRID_H * BLOCK_SIZE);
                int flash_height = (int)(GRID_H * BLOCK_SIZE * (1.0f - progress * 0.5f));
                
                Uint32 glow = darken_color(flash_color, intensity * 0.4f);
                fill_rect(GRID_OFFSET_X + lc->col * BLOCK_SIZE + offset_x - 5,
                          GRID_OFFSET_Y + offset_y - 5,
                          BLOCK_SIZE + 10, GRID_H * BLOCK_SIZE + 10, glow);
                
                Uint32 main_flash = darken_color(flash_color, intensity * 0.8f);
                fill_rect(GRID_OFFSET_X + lc->col * BLOCK_SIZE + offset_x,
                          GRID_OFFSET_Y + offset_y,
                          BLOCK_SIZE, flash_height, main_flash);
                
                if (progress < 0.8f) {
                    fill_rect(GRID_OFFSET_X + lc->col * BLOCK_SIZE + offset_x,
                              GRID_OFFSET_Y + offset_y + sweep_pos - 5,
                              BLOCK_SIZE, 10, 0xFFFFFF);
                }
            }
        }
    }
    
    for (i = 0; i < 25; i++) {
        if (em->place_effects[i].active) {
            PlaceEffect *pe = &em->place_effects[i];
            int px = GRID_OFFSET_X + pe->col * BLOCK_SIZE + BLOCK_SIZE/2 + offset_x;
            int py = GRID_OFFSET_Y + pe->row * BLOCK_SIZE + BLOCK_SIZE/2 + offset_y;
            
            float ring_progress = 1.0f - pe->scale;
            int ring_size = (int)(ring_progress * 25);
            float ring_alpha = pe->alpha;
            
            if (ring_size > 0 && ring_alpha > 0) {
                Uint32 ring_color = darken_color(pe->color, ring_alpha * 0.6f);
                
                int half = ring_size;
                fill_rect(px - half, py - half, ring_size * 2, 2, ring_color);
                fill_rect(px - half, py + half - 2, ring_size * 2, 2, ring_color);
                fill_rect(px - half, py - half, 2, ring_size * 2, ring_color);
                fill_rect(px + half - 2, py - half, 2, ring_size * 2, ring_color);
            }
        }
    }
}

int point_in_rect(int px, int py, int x, int y, int w, int h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}
