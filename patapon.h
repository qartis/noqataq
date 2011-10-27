#define	FRAMES_PER_SEC	30
#define SCREEN_WIDTH    1024
#define SCREEN_HEIGHT   750

union surface {
    SDL_Surface *image;
    SDL_Surface **images;
};

struct object_state {
    union surface surface;
    int num_images;
    int cur_image;
    int anim_delay;
    int anim_counter;
};

enum state {
    WAITING = 0,
    HIDDEN,
    DEAD,
    RUNNING,
    ATTACKING,
    NUM_STATES
};

typedef struct {
	int alive;
	float x, y;
    struct {
        struct {
            float x, y;
        } vel;
        struct {
            int x, y;
        } finalpos;
        double done_time;
        enum state next_state;
    } anim;
    int fixed_in_place;
    enum state state;
    struct object_state states[NUM_STATES];
    enum {
        HUMAN,
        ANIMAL,
        NUM_ENEMY_TYPES
    } enemy_type;
} object;

enum {
  LEFT,
  UP,
  DOWN,
  RIGHT
};

void setpixel(SDL_Surface *surface, int x, int y, uint32_t pixel);
uint32_t getpixel(SDL_Surface *surface, int x, int y);

int rand_between(int low, int high);
void wait(void);

SDL_Surface *load_bmp(const char *filename);
Mix_Chunk *load_sound(const char *filename);
Mix_Music *load_song(const char *filename);
void toggle_mute(void);
