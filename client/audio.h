#ifndef AUDIO_H
#define AUDIO_H

#include <SDL_mixer.h>

extern Mix_Chunk *snd_place;
extern Mix_Chunk *snd_clear;
extern Mix_Chunk *snd_gameover;
extern Mix_Chunk *snd_click;
extern Mix_Music *music_bg;

void init_audio(void);
void cleanup_audio(void);

void play_sound(Mix_Chunk *sound);
void play_click(void);
void play_place(void);
void play_clear(void);
void play_gameover(void);

void start_music(void);
void stop_music(void);
void resume_music(void);

void update_music_volume(void);
void update_sfx_volume(void);

#endif
