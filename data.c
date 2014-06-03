#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>

#include "patapon.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

void __debug(const char *function, int lineno, const char *fmt, ...){
#ifdef debug
    va_list ap;

    fprintf(stderr, "%s():%-3d  ", function, lineno);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr,"\n");
#endif
}

void setpixel(SDL_Surface *surface, int x, int y, uint32_t pixel){
    uint8_t *p = (uint8_t *) surface->pixels + y * surface->pitch + x * 4;
    *(uint32_t *) p = pixel;
}

uint32_t getpixel(SDL_Surface *surface, int x, int y){
    uint8_t *p = (uint8_t *) surface->pixels + y * surface->pitch + x * 4;
    return *(uint32_t *) p;
}


Mix_Chunk *load_sound(const char *filename){
    Mix_Chunk *sound = Mix_LoadWAV(filename);
    if (!sound) printf("error %s",Mix_GetError());
    return sound;
}

Mix_Music *load_song(const char *filename){
    Mix_Music *song = Mix_LoadMUS(filename);
    if (!song) printf("Mix_LoadMUS_RW error: %s",Mix_GetError());
    return song;
}


SDL_Surface *load_bmp(const char *filename) {
    SDL_Surface *temp = IMG_Load(filename);
    SDL_Surface *image;
    if (!temp) printf("%s: %s",filename,IMG_GetError());
    image = SDL_DisplayFormatAlpha(temp);
    SDL_FreeSurface(temp);
    if (!image) printf("%s: %s",filename,IMG_GetError());
    return image;
}
