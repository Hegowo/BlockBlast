#include "audio.h"
#include "globals.h"
#include <stdio.h>

Mix_Chunk *snd_place = NULL;
Mix_Chunk *snd_clear = NULL;
Mix_Chunk *snd_gameover = NULL;
Mix_Chunk *snd_click = NULL;
Mix_Music *music_bg = NULL;

void init_audio(void) {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer init failed: %s\n", Mix_GetError());
        audio_enabled = 0;
        return;
    }
    
    audio_enabled = 1;
    
    snd_place = Mix_LoadWAV("assets/sounds/place.wav");
    if (!snd_place) snd_place = Mix_LoadWAV("../assets/sounds/place.wav");
    if (!snd_place) snd_place = Mix_LoadWAV("assets/place.wav");
    if (!snd_place) printf("WARNING: Could not load place.wav: %s\n", Mix_GetError());
    
    snd_clear = Mix_LoadWAV("assets/sounds/clear.wav");
    if (!snd_clear) snd_clear = Mix_LoadWAV("../assets/sounds/clear.wav");
    if (!snd_clear) snd_clear = Mix_LoadWAV("assets/clear.wav");
    if (!snd_clear) printf("WARNING: Could not load clear.wav: %s\n", Mix_GetError());
    
    snd_gameover = Mix_LoadWAV("assets/sounds/gameover.wav");
    if (!snd_gameover) snd_gameover = Mix_LoadWAV("../assets/sounds/gameover.wav");
    if (!snd_gameover) snd_gameover = Mix_LoadWAV("assets/gameover.wav");
    if (!snd_gameover) printf("WARNING: Could not load gameover.wav: %s\n", Mix_GetError());
    
    snd_click = Mix_LoadWAV("assets/sounds/click.wav");
    if (!snd_click) snd_click = Mix_LoadWAV("../assets/sounds/click.wav");
    if (!snd_click) snd_click = Mix_LoadWAV("assets/click.wav");
    if (!snd_click) printf("WARNING: Could not load click.wav: %s\n", Mix_GetError());
    
    const char *music_paths[] = {
        "assets/sounds/music.wav",
        "assets/sounds/music.ogg",
        "assets/sounds/music.mp3",
        "assets/music.wav",
        "assets/music.ogg",
        "../assets/sounds/music.wav",
        "../assets/sounds/music.ogg",
        NULL
    };
    
    int m;
    for (m = 0; music_paths[m] != NULL && !music_bg; m++) {
        music_bg = Mix_LoadMUS(music_paths[m]);
        if (music_bg) {
            printf("Loaded music from: %s\n", music_paths[m]);
        }
    }
    
    Mix_VolumeMusic(80);
    Mix_Volume(-1, 100);
    
    printf("Audio system initialized.\n");
    if (snd_place) printf("  - place.wav loaded\n");
    if (snd_clear) printf("  - clear.wav loaded\n");
    if (snd_gameover) printf("  - gameover.wav loaded\n");
    if (snd_click) printf("  - click.wav loaded\n");
    if (music_bg) {
        printf("  - background music loaded\n");
    } else {
        printf("  - WARNING: No music file found! Place music.wav in assets/sounds/\n");
    }
}

void cleanup_audio(void) {
    if (snd_place) Mix_FreeChunk(snd_place);
    if (snd_clear) Mix_FreeChunk(snd_clear);
    if (snd_gameover) Mix_FreeChunk(snd_gameover);
    if (snd_click) Mix_FreeChunk(snd_click);
    if (music_bg) Mix_FreeMusic(music_bg);
    
    Mix_CloseAudio();
}

void play_sound(Mix_Chunk *sound) {
    if (audio_enabled && sound) {
        int channel = Mix_PlayChannel(-1, sound, 0);
        if (channel == -1) {
            printf("Error playing sound: %s\n", Mix_GetError());
        }
    }
}

void play_click(void) {
    play_sound(snd_click);
}

void play_place(void) {
    play_sound(snd_place);
}

void play_clear(void) {
    play_sound(snd_clear);
}

void play_gameover(void) {
    Mix_PauseMusic();
    play_sound(snd_gameover);
}

void resume_music(void) {
    if (audio_enabled && music_bg) {
        if (Mix_PausedMusic()) {
            Mix_ResumeMusic();
        } else if (!Mix_PlayingMusic()) {
            Mix_PlayMusic(music_bg, -1);
        }
    }
}

void start_music(void) {
    if (audio_enabled && music_bg) {
        if (Mix_PlayMusic(music_bg, -1) == -1) {
            printf("Error playing music: %s\n", Mix_GetError());
        } else {
            printf("Background music started.\n");
        }
    } else {
        if (!audio_enabled) printf("Music not started: audio disabled\n");
        if (!music_bg) printf("Music not started: no music file loaded\n");
    }
}

void stop_music(void) {
    Mix_HaltMusic();
}

void update_music_volume(void) {
    Mix_VolumeMusic(music_volume);
}

void update_sfx_volume(void) {
    Mix_Volume(-1, sfx_volume);
}
