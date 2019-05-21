#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "mpi.h"
#include "SDL.h"
#include "SDL_image.h"

#define SCREEN_WIDTH 320 
#define SCREEN_HEIGHT 240

SDL_Window* win = NULL;  //yeah, yeah...we hate globals
int rank;
int size;

void mp_init(char*);
int sdl_init(char*);
void main_loop(char*);

typedef enum {false, true} bool;

typedef struct {
    int x, y, w, h, r, g, b;
} Rect;

int main(int argc, char* argv[])
{
    char lbl[25];
    
    mp_init(lbl); //initialize MPI
    if(sdl_init(lbl) != 0) return 1; //initialize SDL
    main_loop(lbl);

    return 0;
}

//Get MPI up and running and get processor info
void mp_init(char *s) {
    int world_size;
    int world_rank;

    MPI_Init(NULL,NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    size = world_size;
    rank = world_rank;

    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;

    MPI_Get_processor_name(processor_name, &name_len);
    sprintf(s, "Processor %s, rank %d", processor_name, world_rank);
    fprintf(stdout, "World size: %d\n", world_size);
}

int sdl_init(char* lbl) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
        return 1;
    }
    fprintf(stdout, "%s sdl_init successful\n", lbl);
    
    win = SDL_CreateWindow(lbl,
				SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
				SCREEN_WIDTH, SCREEN_HEIGHT,
				SDL_WINDOW_SHOWN);

    if(win == NULL) {
	    fprintf(stderr, "could not create window: %s\n", SDL_GetError());
        return 1;
    }
    fprintf(stdout, "%s window created\n", lbl);

    //Initialize PNG loading 
    int imgFlags = IMG_INIT_PNG; 
    if(!(IMG_Init(imgFlags) & imgFlags)) {
	    fprintf(stderr, "SDL_image could not initialize! SDL_image Error: %s\n",IMG_GetError()); 
	    return 1; 
    }
    fprintf(stdout, "%s SDL_image initialized successfully\n", lbl);
    return 0; 
}

void main_loop(char* lbl) {
    bool quit = false;
    SDL_Event e;
    SDL_Surface* screenSurface = NULL;
    SDL_Rect r;
    Rect rct;
    //int side_len = (int) sqrt(size);
    //fprintf(stdout, "Side length: %d\n", side_len);
    int WORLD_WIDTH = SCREEN_WIDTH * size;
    //int WORLD_HEIGHT = SCREEN_HEIGHT * side_len;

    MPI_Datatype dt_rect;
    MPI_Type_contiguous(7, MPI_INT, &dt_rect);
    MPI_Type_commit(&dt_rect);

    srand(time(NULL));
    fprintf(stdout, "%s all good up 'til now\n", lbl);
    
    screenSurface = SDL_GetWindowSurface(win);
    if(screenSurface != NULL) {
        fprintf(stdout, "%s got surface\n", lbl);
        while(!quit) {
            while(SDL_PollEvent(&e) != 0) {
                if(e.type == SDL_QUIT) {
                    quit = true;
                }
            }

            if(rank == 0) {
                rct.x = rand() % WORLD_WIDTH + 1;
                rct.y = rand() % SCREEN_HEIGHT + 1;
                rct.w = (rand() % WORLD_WIDTH + 1) - rct.x;
                rct.h = (rand() % SCREEN_HEIGHT + 1) - rct.y;
                rct.r = rand() % 255;
                rct.g = rand() % 255;
                rct.b = rand() % 255;
            }

            //broadcast rectangle information to other processors
            MPI_Bcast(&rct, 1, dt_rect, 0, MPI_COMM_WORLD);
            r.x = rct.x - (SCREEN_WIDTH * rank);
            //r.y = rct.y - (SCREEN_HEIGHT * (rank % side_len));
            r.y = rct.y;
            r.w = rct.w;
            r.h = rct.h;

            MPI_Barrier(MPI_COMM_WORLD);
            SDL_FillRect(screenSurface, &r, SDL_MapRGB(screenSurface->format,rct.r,rct.g,rct.b));
            SDL_UpdateWindowSurface(win);
            SDL_Delay(10);
        }
    }
    else {
        fprintf(stderr, "%s unable to get surface\n", lbl);
    }

    SDL_DestroyWindow(win);
    SDL_Quit();
}
