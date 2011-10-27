#include <time.h>
#include <sys/time.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#ifndef linux
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "patapon.h"

#ifdef linux
double gettime(void){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + ((double)tv.tv_usec / 1000000.0);
}
#else
double gettime(void){
    static __int64 start = 0;
    static __int64 frequency = 0;

    if (start==0)
    {
        QueryPerformanceCounter((LARGE_INTEGER*)&start);
        QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
        return 0.0f;
    }

    __int64 counter = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&counter);
    return (double
    ) ((counter - start) / (double)(frequency));
}
#endif

void wait(void){
    static uint32_t   next_tick = 0;
    uint32_t          this_tick;
    this_tick = SDL_GetTicks();
    if (this_tick < next_tick) SDL_Delay(next_tick - this_tick);
    next_tick = this_tick + (1000 / FRAMES_PER_SEC);
}

#define MAXIMUM_FRAME_RATE 30
#define MINIMUM_FRAME_RATE 5
#define UPDATE_INTERVAL (1.0 / MAXIMUM_FRAME_RATE)
#define MAX_CYCLES_PER_FRAME (MAXIMUM_FRAME_RATE / MINIMUM_FRAME_RATE)

const char *song_file = "data/song2.mp3";
int beat_interval = 577053;
unsigned intro_len = 300000;
/*
const char *song_file   = "data/miracles.ogg";
int beat_interval = 551250;
unsigned intro_len     = 550000;
*/

object player;
object beat_sprite;
object beat_pow;
object background;
object enemies[10];
SDL_Surface *up_img, *left_img, *right_img, *space_img;
SDL_Surface *screen;
Mix_Music *song;
Mix_Music *intro;
Mix_Chunk *up, *left, *right, *space;
TTF_Font *font;
SDL_Color white = {255,255,255,255};
uint32_t red = 0x00FF0000;
int requests_quit;
float viewport_x;

struct object_state load_frames(const char *path, int num_frames){
    int i;
    char buf[64];
    struct object_state state = {{0}};
    state.num_images = num_frames;
    if (num_frames == 1){
        state.surface.image = load_bmp(path);
        return state;
    }
    state.surface.images = malloc(num_frames * sizeof(state.surface.images[0]));
    for (i=0;i<num_frames;i++){
        sprintf(buf,path,i+1);
        state.surface.images[i] = load_bmp(buf);
    }
    return state;
}

void load_data(void){
    player.states[DEAD] = load_frames("data/waiting/1 waiting0001.png", 1);
    player.states[WAITING] = load_frames("data/waiting/1 waiting%04d.png", 28);
    player.states[RUNNING] = load_frames("data/running/1 running%04d.png", 6);
    player.states[ATTACKING] = load_frames("data/attacking/1 running%04d.png", 6);
    player.states[RUNNING].anim_delay = 2;
    enemies[0].states[WAITING] = load_frames("data/enemy_1.png", 1);
    enemies[0].states[ATTACKING] = load_frames("data/attacking/1 running%04d.png", 6);
    enemies[0].states[DEAD] = load_frames("data/crumpled.png", 1);
    enemies[0].enemy_type = HUMAN;
    enemies[1].states[WAITING] = load_frames("data/enemy_1.png", 1);
    enemies[1].states[ATTACKING] = load_frames("data/attacking/1 running%04d.png", 6);
    enemies[1].states[DEAD] = load_frames("data/crumpled.png", 1);
    enemies[1].states[RUNNING] = load_frames("data/running/1 running%04d.png", 6);
    enemies[1].enemy_type = ANIMAL;
    enemies[2].states[WAITING] = load_frames("data/enemy_1.png", 1);
    enemies[3].states[WAITING] = load_frames("data/enemy_1.png", 1);
    enemies[4].states[WAITING] = load_frames("data/enemy_1.png", 1);
    enemies[5].states[WAITING] = load_frames("data/enemy_1.png", 1);
    enemies[6].states[WAITING] = load_frames("data/enemy_1.png", 1);
    enemies[7].states[WAITING] = load_frames("data/enemy_1.png", 1);
    enemies[8].states[WAITING] = load_frames("data/enemy_1.png", 1);
    enemies[9].states[WAITING] = load_frames("data/enemy_1.png", 1);
    beat_sprite.states[WAITING] = load_frames("data/beat.png",1);
    background.states[WAITING] = load_frames("data/background_2.png", 1);
    beat_pow.states[WAITING] = load_frames("data/beat_pow.png", 1);
    song = load_song(song_file);
    up = load_sound("data/up.ogg");
    left = load_sound("data/left.ogg");
    right = load_sound("data/right.ogg");
    space = load_sound("data/space.ogg");
    up_img = load_bmp("data/up.png");
    left_img = load_bmp("data/left.png");
    right_img = load_bmp("data/right.png");
    space_img = load_bmp("data/space.png");
}

void load_early_data(void){
    font = TTF_OpenFont("data/font.ttf",20);
}

void draw_object(object *sprite){
    SDL_Surface *img; 
    SDL_Rect a;
    struct object_state *state = &sprite->states[sprite->state];
    if (state->num_images > 1){
        if (state->cur_image > 2*(state->num_images-1)){
            state->cur_image = 1;
            img = state->surface.images[0];
        } else {
            if (state->cur_image >= state->num_images){
                img = state->surface.images[2*state->num_images - state->cur_image - 2];
            } else {
                img = state->surface.images[state->cur_image];
            }
            if (state->anim_delay > 0){
                state->anim_counter = (state->anim_counter + 1) % state->anim_delay;
            }
            if (state->anim_delay == 0 || state->anim_counter == 0){
                state->cur_image++;
            }
        }
    } else {
        img = state->surface.image;
    }

    if (sprite->alive < 1) return;
    if (sprite->fixed_in_place){
        a.x = sprite->x;
    } else {
        a.x = sprite->x - viewport_x;
    }
    a.y = sprite->y;
    SDL_BlitSurface(img, NULL, screen, &a);
}

SDL_Surface* key_to_img(SDLKey k){
    switch (k){
    case SDLK_UP:
        return up_img;
    case SDLK_LEFT:
        return left_img;
    case SDLK_RIGHT:
        return right_img;
    case SDLK_SPACE:
        return space_img;
    default:
        return NULL;
    }
}

void play_key(SDLKey k){
    Mix_Chunk *c = NULL;
    switch (k){
    case SDLK_UP:
        c = up;
        break;
    case SDLK_LEFT:
        c = left;
        break;
    case SDLK_RIGHT:
        c = right;
        break;
    case SDLK_SPACE:
        c = space;
        break;
    default:
        c = NULL;
    }
    Mix_PlayChannel(0,c,0);
}

#define LEFT  SDLK_LEFT
#define RIGHT SDLK_RIGHT
#define SPACE SDLK_SPACE
#define BLANK SDLK_UNKNOWN
#define CMD_A LEFT,LEFT,LEFT,RIGHT
#define CMD_B LEFT,RIGHT,LEFT,RIGHT
#define CMD_C SPACE,SPACE,SPACE,RIGHT

SDLKey beat_map[] = {
    CMD_A, CMD_A, CMD_B, CMD_C,
    CMD_A, CMD_A, CMD_B, CMD_C,
    CMD_A, CMD_A, CMD_B, CMD_C,
    CMD_A, CMD_A, CMD_B, CMD_C,
    CMD_A, CMD_A, CMD_B, CMD_C,
};

int next_enemy(int my_x){
    unsigned i;
    for(i=0;i<sizeof(enemies)/sizeof(enemies[0]);i++){
        if (enemies[i].x > my_x && enemies[i].alive && enemies[i].state != DEAD){
            return i;
        }
    }
    return -1;
}

void init_animation(object *o, float num_beats, enum state s, int x_delta){
    double anim_time = num_beats * beat_interval / 1000000.0;
    o->anim.done_time = gettime() + anim_time;
    o->anim.next_state = WAITING;
    o->anim.vel.x = (x_delta/anim_time);
    o->anim.vel.y = 0.0;
    o->anim.finalpos.x = o->x + x_delta;
    o->anim.finalpos.y = o->y;
    o->state = s;
    o->states[s].cur_image = 0;
}

void failure(void){
    int i = next_enemy(player.x);
    object *enemy = &enemies[i];
    int dist = enemy->x - player.x;
    if (dist < 350){
        switch (enemy->enemy_type){
        case ANIMAL:
            init_animation(enemy, 2, RUNNING, 400);
            break;
        case HUMAN:
            init_animation(enemy, 2, ATTACKING, 0);
            player.alive -= 34;
            if (player.alive < 0){
                printf("dead!\n");
                exit(0);
            }
            break;
        default:
            printf("unhandled enemy type %d\n", enemy->enemy_type);
        }
    }
}

void success(void){
    int i = next_enemy(player.x);
    object *enemy = &enemies[i];
    int dist = enemy->x - player.x;
    printf("dist: %d\n", dist);
    if (dist < 350){
        init_animation(&player, 1.5, ATTACKING, 0);
        enemy->alive -= 34;
        if (enemy->alive < 0){
            enemy->alive = 1;
            enemy->state = DEAD;
        }
    } else {
        init_animation(&player, 1.5, RUNNING, 400);
    }
}

uint32_t start_time;
int successes[4];

void physics_step(void){
    SDL_Event event;
    unsigned i;
    uint32_t pos;
    float cur_beat_f;
    int cur_beat;
    static int last_displayed_beat = -1;

    object* objects[] = {&player, &enemies[0], &enemies[1], &enemies[2], &enemies[3], &enemies[4]};

    for(i=0;i<sizeof(objects)/sizeof(objects[0]);i++){
        object *o = objects[i];
        if (o->state != WAITING && o->state != DEAD){
            if (gettime() >= o->anim.done_time){
                o->x = o->anim.finalpos.x;
                o->y = o->anim.finalpos.y;
                o->state = o->anim.next_state;
                o->states[o->anim.next_state].cur_image = 0;
            } else {
#define PADDING_RIGHT 700
                o->x += o->anim.vel.x * UPDATE_INTERVAL;
                o->y += o->anim.vel.y * UPDATE_INTERVAL;
                if (o == &player && SCREEN_WIDTH - (player.x - viewport_x) <= PADDING_RIGHT){
                    viewport_x += player.anim.vel.x * UPDATE_INTERVAL;
                }
            }
        }
    }

    pos = SDL_GetTicks() - start_time;
    cur_beat_f = ((float)(pos - intro_len/1000.0)) / (beat_interval / 1001);
    cur_beat = (int)cur_beat_f;
    if (pos > intro_len/1000){
        if (beat_sprite.alive > 0){
            beat_sprite.alive--;
        }
        if (beat_pow.alive > 0){
            beat_pow.alive--;
        }
        if (cur_beat > last_displayed_beat){
            printf("beat %d\n",cur_beat);
            last_displayed_beat = cur_beat;
            beat_sprite.alive = 4;
            if (beat_map[cur_beat] != SDLK_UNKNOWN){
                beat_pow.alive = 4;
            }
            if (cur_beat != 0 && cur_beat % 4 == 0){
                printf("checking for operation: ");
                if ((beat_map[cur_beat-4] == BLANK || successes[0]) &&
                    (beat_map[cur_beat-3] == BLANK || successes[1]) &&
                    (beat_map[cur_beat-2] == BLANK || successes[2]) &&
                    (beat_map[cur_beat-1] == BLANK || successes[3])){
                    if ((beat_map[cur_beat-4] != BLANK ||
                        beat_map[cur_beat-3] != BLANK ||
                        beat_map[cur_beat-2] != BLANK ||
                        beat_map[cur_beat-1] != BLANK)){
                        success();
                        printf("SUCCESS\n");
                    }
                } else {
                    printf("FAILURE\n");
                    failure();
                }
                successes[0] = 0;
                successes[1] = 0;
                successes[2] = 0;
                successes[3] = 0;
            }
        }
    }

    while (SDL_PollEvent(&event)){
        if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_q))){
            requests_quit = 1;
            return;
        } else if (event.type == SDL_KEYDOWN && (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_SPACE)){
            float offset_f;
            cur_beat_f -= 0.25;
#define TOLERANCE 0.3
            offset_f = cur_beat_f - (int)cur_beat_f;
            if (offset_f < 0.0){
                offset_f = 0.0f;
                cur_beat = 0;
            }
            if (offset_f > 1.0 - TOLERANCE || offset_f < TOLERANCE){
                if (event.key.keysym.sym == beat_map[cur_beat]){
                    play_key(event.key.keysym.sym);
                    successes[cur_beat % 4] = 1;
                }
            } else {
                printf("missed note: early would have been offset_f %f > %f, normal would have been %f < %f\n",offset_f, 1.0 - TOLERANCE, offset_f, TOLERANCE);
            }
        }
    }
}

void render(void){
    int length_into_song;
    double offset_in_s;
    int x;
    int idx;
    int y;
    SDL_Rect r;
    unsigned i;
    draw_object(&background);
    for(i=0;i<sizeof(enemies)/sizeof(enemies[0]);i++){
        draw_object(&enemies[i]);
    }
    draw_object(&player);
    draw_object(&beat_sprite);
    draw_object(&beat_pow);

#define SPACING 35
#define BEAT_BAR_PADDING_LEFT 130
    length_into_song = SDL_GetTicks() - start_time;
    offset_in_s = (intro_len/1000000.0 - length_into_song/1000.0) / (beat_interval / 1000001.0);
    x = offset_in_s * (up_img->w + SPACING);
    for(i=0;i<sizeof(beat_map)/sizeof(beat_map[0]);i++){
        SDL_Surface *img;
        r.x = x + i*(up_img->w + SPACING) + BEAT_BAR_PADDING_LEFT;
        if (r.x > SCREEN_WIDTH) break;
        r.y = 20;
        img = key_to_img(beat_map[i]);
        if (img) SDL_BlitSurface(img,NULL,screen,&r);
    }

    idx = next_enemy(player.x);
    x = enemies[idx].x;
    y = enemies[idx].y;

    if (idx > -1 && enemies[idx].state != DEAD){
        r.x = x - viewport_x + 30;
        r.y = y;
        r.w = enemies[idx].alive;
        r.h = 10;

        SDL_FillRect(screen, &r, red);
    }

    if (idx > -1 && enemies[idx].x - player.x < 350 && enemies[idx].state != DEAD){
        r.x = player.x - viewport_x + 30;
        r.y = player.y - 10;
        r.w = player.alive;
        r.h = 10;
        SDL_FillRect(screen, &r, red);
    }


    SDL_UpdateRect(screen,0,0,0,0);
}


void game(void){
  static double lastFrameTime = 0.0;
  static double cyclesLeftOver = 0.0;
  double currentTime;
  double updateIterations;

    wait();
  
  currentTime = gettime();
  updateIterations = ((currentTime - lastFrameTime) + cyclesLeftOver);
  
  if (updateIterations > (MAX_CYCLES_PER_FRAME * UPDATE_INTERVAL)) {
    updateIterations = (MAX_CYCLES_PER_FRAME * UPDATE_INTERVAL);
  }
  
  while (updateIterations > UPDATE_INTERVAL) {
    updateIterations -= UPDATE_INTERVAL;
    
    physics_step(); /* Update game state a variable number of times */
  }
  
  cyclesLeftOver = updateIterations;
  lastFrameTime = currentTime;
  
  render(); /* Draw the scene only once */
}

int main(int argc, char **argv){
    SDL_Surface *text;
    SDL_Rect r;
    unsigned i;
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        printf("Failed to initialize sdl: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    atexit(SDL_Quit);

    Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 4096);
    screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_HWSURFACE);
    TTF_Init();

    SDL_WM_SetCaption("patapon", "");

    load_early_data();

    text = TTF_RenderText_Blended(font,"loading",white);
    TTF_SizeText(font,"loading",(int*)&r.x,(int*)&r.y);
    r.x = (SCREEN_WIDTH - r.x) / 2;
    r.y = (SCREEN_HEIGHT - r.y) / 2;
    SDL_BlitSurface(text,NULL,screen,&r);
    SDL_UpdateRect(screen,0,0,0,0);
    free(text);

    load_data();


    srand(time(NULL));
    requests_quit = 0;
    player.x = 150;
    player.y = 350;
    player.alive = 100;
    for(i=0;i<sizeof(enemies)/sizeof(enemies[0]);i++){
        enemies[i].alive = 100;
        enemies[i].y = 450;
        enemies[i].x = 200 * (i*4+6);
    }
    beat_sprite.x = 0;
    beat_sprite.y = 0;
    beat_sprite.alive = 0;
    beat_sprite.fixed_in_place = 1;
    beat_pow.x = BEAT_BAR_PADDING_LEFT - 20;
    beat_pow.y = 20;
    beat_pow.alive = 0;
    beat_pow.fixed_in_place = 1;
    background.alive = 1;

    start_time = SDL_GetTicks();
    Mix_PlayMusic(song,-1);
    while (!requests_quit) {
        game();
    }
    Mix_HaltMusic();
    Mix_FreeMusic(song);
    Mix_CloseAudio();
    return 0;
}
