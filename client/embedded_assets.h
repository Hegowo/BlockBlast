#ifndef EMBEDDED_ASSETS_H
#define EMBEDDED_ASSETS_H

#include <stddef.h>

#ifdef EMBED_ASSETS

extern const unsigned char font_orbitron_data[];
extern const size_t font_orbitron_size;

extern const unsigned char font_default_data[];
extern const size_t font_default_size;

extern const unsigned char sound_place_data[];
extern const size_t sound_place_size;

extern const unsigned char sound_clear_data[];
extern const size_t sound_clear_size;

extern const unsigned char sound_gameover_data[];
extern const size_t sound_gameover_size;

extern const unsigned char sound_click_data[];
extern const size_t sound_click_size;

extern const unsigned char sound_victory_data[];
extern const size_t sound_victory_size;

extern const unsigned char sound_music_data[];
extern const size_t sound_music_size;


#else

#define font_orbitron_data NULL
#define font_orbitron_size 0
#define font_default_data NULL
#define font_default_size 0
#define sound_place_data NULL
#define sound_place_size 0
#define sound_clear_data NULL
#define sound_clear_size 0
#define sound_gameover_data NULL
#define sound_gameover_size 0
#define sound_click_data NULL
#define sound_click_size 0
#define sound_victory_data NULL
#define sound_victory_size 0
#define sound_music_data NULL
#define sound_music_size 0

#endif

#endif

