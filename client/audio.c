#include "audio.h"
#include "globals.h"
#include "embedded_assets.h"
#include <stdio.h>

Mix_Chunk *snd_place = NULL;
Mix_Chunk *snd_clear = NULL;
Mix_Chunk *snd_gameover = NULL;
Mix_Chunk *snd_click = NULL;
Mix_Music *music_bg = NULL;

static Mix_Chunk* load_sound_embedded_or_file(const unsigned char *data, size_t size, const char *path1, const char *path2, const char *path3) {
    Mix_Chunk *chunk = NULL;
    
#ifdef EMBED_ASSETS
    if (data && size > 0) {
        SDL_RWops *rw = SDL_RWFromConstMem(data, (int)size);
        if (rw) {
            chunk = Mix_LoadWAV_RW(rw, 1);
            if (chunk) {
                return chunk;
            }
        }
    }
#else
    (void)data;
    (void)size;
#endif
    
    chunk = Mix_LoadWAV(path1);
    if (chunk) return chunk;
    
    if (path2) {
        chunk = Mix_LoadWAV(path2);
        if (chunk) return chunk;
    }
    
    if (path3) {
        chunk = Mix_LoadWAV(path3);
        if (chunk) return chunk;
    }
    
    return NULL;
}

void init_audio(void) {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer init failed: %s\n", Mix_GetError());
        audio_enabled = 0;
        return;
    }
    
    audio_enabled = 1;
    
    snd_place = load_sound_embedded_or_file(sound_place_data, sound_place_size,
        "assets/sounds/place.wav", "../assets/sounds/place.wav", "assets/place.wav");
    if (!snd_place) printf("WARNING: Could not load place.wav: %s\n", Mix_GetError());
    
    snd_clear = load_sound_embedded_or_file(sound_clear_data, sound_clear_size,
        "assets/sounds/clear.wav", "../assets/sounds/clear.wav", "assets/clear.wav");
    if (!snd_clear) printf("WARNING: Could not load clear.wav: %s\n", Mix_GetError());
    
    snd_gameover = load_sound_embedded_or_file(sound_gameover_data, sound_gameover_size,
        "assets/sounds/gameover.wav", "../assets/sounds/gameover.wav", "assets/gameover.wav");
    if (!snd_gameover) printf("WARNING: Could not load gameover.wav: %s\n", Mix_GetError());
    
    snd_click = load_sound_embedded_or_file(sound_click_data, sound_click_size,
        "assets/sounds/click.wav", "../assets/sounds/click.wav", "assets/click.wav");
    if (!snd_click) printf("WARNING: Could not load click.wav: %s\n", Mix_GetError());
    
#ifdef EMBED_ASSETS
    if (sound_music_data && sound_music_size > 0) {
        SDL_RWops *rw = SDL_RWFromConstMem(sound_music_data, (int)sound_music_size);
        if (rw) {
            music_bg = Mix_LoadMUS_RW(rw);
            if (music_bg) {
                printf("Loaded music from embedded data\n");
            }
        }
    }
#endif
    
    if (!music_bg) {
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
