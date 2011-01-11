#include "headers/hax_sdl.hpp"
#include "headers/hax_threads.hpp"
#include "headers/hax_sdl_data.h"
#include <cmath>

#define PI 3.14159265
#define DAMP_FACTOR 9001

#define SCREENW 854
#define SCREENH 480

/*
 * This is a 32-bit pixel function created with help from this
 * website: http://www.libsdl.org/intro.en/usingvideo.html
 *
 * You will need to make changes if you want it to work with
 * 8-, 16- or 24-bit surfaces.  Consult the above website for
 * more information.
 */
void set_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    Uint8 *target_pixel = (Uint8 *)surface->pixels + y * surface->pitch + x * 4;
    *(Uint32 *)target_pixel = pixel;
}

void sine_graph(SDL_Surface *surface, float freq, float amplitude, float displ, Uint32 pixel)
{
  int i;
  int y;
  int oldy;

  y = floor ( amplitude * sin( (displ - (surface->w / 2)) * PI * freq) );

  for (i = 1; i <= surface->w; i++){
    oldy = y;
    y = floor ( amplitude * sin((i - (surface->w / 2) + displ) * PI * freq) );
    // std::cout << freq << " " << ((i - (surface->w / 2)) * PI * freq) << std::endl;
    aalineColor(surface, i-1, oldy + (surface->h / 2), i,  y + (surface->h / 2), pixel);
  }

  //aalineColor(surface, surface->w / 2, surface->h / 2, surface->w / 2 + amplitude * -cos (displ * PI * freq), surface->h / 2 + amplitude * sin (displ * PI * freq), pixel);
}

void draw_sound(SDL_Surface *surface, int size, int16_t * left_channel, int16_t * right_channel, Uint32 pixel) {
  int i;

  printf("[SDL] Data from L/R channel (SDL Class), frames 24 and 38. L: {%i, %i} R: {%i, %i}\n",
             left_channel[24], left_channel[38],
             right_channel[24], right_channel[38]);

  for (i=1; i<size; i++){
    aalineColor(surface, (surface->w / 2) + ((i-1)*10), ((right_channel[i-1] * (surface->h / 2) / DAMP_FACTOR) + (surface->h / 2)), (surface->w / 2) + (i*10), ((right_channel[i] * (surface->h / 2) / DAMP_FACTOR) + (surface->h / 2)), pixel);
    aalineColor(surface, (surface->w / 2) - ((i-1)*10), ((left_channel[i-1] * (surface->h / 2) / DAMP_FACTOR) + (surface->h / 2)), (surface->w / 2) - (i*10), ((left_channel[i]  * (surface->h / 2) / DAMP_FACTOR) + (surface->h / 2)), pixel);
  }

}

/*
 * This is an implementation of the Midpoint Circle Algorithm
 * found on Wikipedia at the following link:
 *
 *   http://en.wikipedia.org/wiki/Midpoint_circle_algorithm
 *
 * The algorithm elegantly draws a circle quickly, using a
 * set_pixel function for clarity.
 */
void draw_circle(SDL_Surface *surface, int cx, int cy, int radius, Uint32 pixel)
{
    int error = -radius;
    int x = radius;
    int y = 0;

    while (x >= y)
    {
        set_pixel(surface, cx + x, cy + y, pixel);
        set_pixel(surface, cx + y, cy + x, pixel);

        if (x != 0)
        {
            set_pixel(surface, cx - x, cy + y, pixel);
            set_pixel(surface, cx + y, cy - x, pixel);
        }

        if (y != 0)
        {
            set_pixel(surface, cx + x, cy - y, pixel);
            set_pixel(surface, cx - y, cy + x, pixel);
        }

        if (x != 0 && y != 0)
        {
            set_pixel(surface, cx - x, cy - y, pixel);
            set_pixel(surface, cx - y, cy - x, pixel);
        }

        error += y;
        ++y;
        error += y;

        if (error >= 0)
        {
            --x;
            error -= x;
            error -= x;
        }
    }
}

void * hax_sdl_main(void *configuration){

    hax_thread_config_t * hax_configs = (hax_thread_config_t *) configuration;
    hax_general_settings_t * hax_user_settings = &hax_configs->user_settings;
    hax_sdl_data * hax_sound_data = (hax_sdl_data *) hax_configs->data_zone;

    printf("[SDL] Thread Start!\n");

    FPSmanager hax_fps;
    float displ = 0;

    char fpscount[30];

    SDL_initFramerate(&hax_fps);
    SDL_setFramerate(&hax_fps, 30);

    // initialize SDL video
    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "Unable to init SDL: %s\n", SDL_GetError() );
        pthread_exit((void *) 1);
    }

    // make sure SDL cleans up before exit
    atexit(SDL_Quit);

    // create a new window
    SDL_Surface* screen = SDL_SetVideoMode(SCREENW, SCREENH, 32,
                                           SDL_HWSURFACE|SDL_DOUBLEBUF);
    if ( !screen )
    {
        printf("Unable to set 640x480 video: %s\n", SDL_GetError());
        pthread_exit((void *) 1);
    }

    std::cout << "[SDL] " << screen->w/2 << std::endl;
    std::cout << "[SDL] " << screen->h/2 << std::endl;

    // program main loop
    bool done = false;

    printf("[SDL] Waiting to start...\n");
    fflush(stdout);
    std::cout.flush();

    hax_configs->timer->start();

    printf("[SDL] Firing Loop!\n");
    fflush(stdout);
    std::cout.flush();
    while (!done)
    {
        hax_configs->timer->wait_next_activation();

        // message processing loop
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // check for messages
            switch (event.type)
            {
                // exit if the window is closed
            case SDL_QUIT:
                done = true;
                break;

                // check for keypresses
            case SDL_KEYDOWN:
                {
                    // exit if ESCAPE is pressed
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                        done = true;
                    break;
                }
            } // end switch
        } // end of message processing

        // DRAWING STARTS HERE

        // clear screen
        SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0, 0, 0));

        // draw bitmap
        // SDL_BlitSurface(bmp, 0, screen, &dstrect);

        vlineColor(screen, screen->w / 2, 0, screen->h, 0xffffffff);
        hlineColor(screen, 0, screen->w, screen->h / 2, 0xffffffff);

        //sine_graph(screen, 0.01, 60, displ, 0xff00ffff);


        printf("[SDL] Requiring Locks!\n");
        fflush(stdout);
        std::cout.flush();
        hax_sound_data->lock_data();
          printf("[SDL] Plotting Sound Data!\n");
          fflush(stdout);
          std::cout.flush();
          draw_sound(screen, hax_sound_data->get_frames(), hax_sound_data->get_left_sound_channel(), hax_sound_data->get_right_sound_channel(), 0xff00ffff);
          printf("[SDL] Releasing Locks!\n");
          fflush(stdout);
          std::cout.flush();
        hax_sound_data->unlock_data();

        // Draw circle
        draw_circle(screen, (screen->w / 2), (screen->h / 2), 60, 0xffffffff);

        sprintf(fpscount, "%s%i", "Current FPS: ", SDL_getFramerate(&hax_fps));// + SDL_getFramerate(&hax_fps);
        stringColor(screen, 10, 10, fpscount, 0xffffffff);
        // DRAWING ENDS HERE

        // finally, update the screen :)
        SDL_Flip(screen);

        displ ++;
    } // end main loop

    // all is well ;)

    printf("Exited cleanly\n");
    pthread_exit((void *) 0);
}
