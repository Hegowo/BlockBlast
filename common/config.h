#ifndef CONFIG_H
#define CONFIG_H

#define GRID_W 10
#define GRID_H 10

#define WINDOW_W 540
#define WINDOW_H 960

#define BLOCK_SIZE 40

#define PORT 5000

#define COLOR_BG           0x0D0221    
#define COLOR_BG_DARK      0x080115    
#define COLOR_BG_LIGHTER   0x1A0A3E    

#define COLOR_PANEL        0x12082A    
#define COLOR_GRID         0x1A1A3E    
#define COLOR_GRID_BRIGHT  0x2A2A5E    
#define COLOR_GRID_CELL    0x0F0520    

#define COLOR_NEON_CYAN    0x00FFFF    
#define COLOR_NEON_MAGENTA 0xFF00FF    
#define COLOR_NEON_GREEN   0x39FF14    
#define COLOR_NEON_ORANGE  0xFF6600    
#define COLOR_NEON_YELLOW  0xFFFF00    
#define COLOR_NEON_BLUE    0x00AAFF    
#define COLOR_NEON_RED     0xFF3366    
#define COLOR_NEON_PURPLE  0xBB00FF    

#define COLOR_GLOW_CYAN    0x004455    
#define COLOR_GLOW_MAGENTA 0x550055    
#define COLOR_GLOW_GREEN   0x115500    
#define COLOR_GLOW_ORANGE  0x552200    

#define COLOR_BUTTON       0x1A1A4A    
#define COLOR_BTN_HOVER    0x2A2A6A    
#define COLOR_BTN_DISABLED 0x333344    
#define COLOR_BTN_BORDER   0x00FFFF    

#define COLOR_SUCCESS      0x39FF14    
#define COLOR_DANGER       0xFF3366    
#define COLOR_WARNING      0xFF6600    

#define COLOR_INPUT        0x12082A    
#define COLOR_INPUT_FOCUS  0x1A0A3E    
#define COLOR_INPUT_BORDER 0x00FFFF    

#define COLOR_WHITE        0xFFFFFF
#define COLOR_GREY         0x888899
#define COLOR_DARK_GREY    0x444455

#define COLOR_CYAN         0x00FFFF    
#define COLOR_ORANGE       0xFF6600    
#define COLOR_GOLD         0xFFD700    
#define COLOR_PURPLE       0xBB00FF    

#define BLOCK_COLOR_RED     0xFF3366    
#define BLOCK_COLOR_GREEN   0x39FF14    
#define BLOCK_COLOR_BLUE    0x00AAFF    
#define BLOCK_COLOR_YELLOW  0xFFFF00    
#define BLOCK_COLOR_MAGENTA 0xFF00FF    
#define BLOCK_COLOR_CYAN    0x00FFFF    
#define BLOCK_COLOR_ORANGE  0xFF6600    
#define BLOCK_COLOR_PURPLE  0xBB00FF    
#define BLOCK_COLOR_LIME    0x66FF33    

#define GRID_OFFSET_X ((WINDOW_W - GRID_W * BLOCK_SIZE) / 2)
#define GRID_OFFSET_Y 100  

#define GAME_MODE_CLASSIC 0
#define GAME_MODE_RUSH    1

#define EFFECT_NONE       0
#define EFFECT_PLACE      1
#define EFFECT_LINE_CLEAR 2

#define MAX_PARTICLES 100

#define GLOW_PULSE_SPEED   3.0f    
#define BUTTON_PULSE_SPEED 2.0f   

#endif
