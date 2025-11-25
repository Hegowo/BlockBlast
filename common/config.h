#ifndef CONFIG_H
#define CONFIG_H

/* Grid dimensions */
#define GRID_W 10
#define GRID_H 10

/* Window dimensions */
#define WINDOW_W 540
#define WINDOW_H 960

/* Block size in pixels */
#define BLOCK_SIZE 40

/* Default server port */
#define PORT 5000

/* Color scheme */
#define COLOR_BG        0x1E1E2E
#define COLOR_GRID      0x303030
#define COLOR_BUTTON    0x4A6FA5
#define COLOR_BTN_HOVER 0x6A8FC5
#define COLOR_BTN_DISABLED 0x555555
#define COLOR_SUCCESS   0x28A745
#define COLOR_DANGER    0xA54A4A
#define COLOR_INPUT     0x2E2E3E
#define COLOR_INPUT_FOCUS 0x3E3E5E
#define COLOR_CYAN      0x00FFFF
#define COLOR_ORANGE    0xFF8C00
#define COLOR_WHITE     0xFFFFFF
#define COLOR_GREY      0x888888

/* Grid offset for centering */
#define GRID_OFFSET_X ((WINDOW_W - GRID_W * BLOCK_SIZE) / 2)
#define GRID_OFFSET_Y 80

#endif

