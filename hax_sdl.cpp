#include "headers/hax_sdl.hpp"
#include "headers/hax_threads.hpp"
#include "headers/hax_sdl_data.h"
#include <cmath>

#define PI 3.14159265
#define DAMP_FACTOR  32767

#define SCREENW 854
#define SCREENH 700

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

void draw_sound(SDL_Surface *surface, int size, int16_t * left_channel, int16_t * right_channel, int v_position, int height, Uint32 pixel) {
  int i;
  float step;

  step = (((float)surface->w / 2) / size);

  stringColor(surface, 10, v_position - height + (height / 10), "Amplitude", 0xffffffff);
  stringColor(surface, (surface->w / 2) - 20, v_position - height + (height / 10), "L", 0xffffffff);
  stringColor(surface, (surface->w / 2) + 10, v_position - height + (height / 10), "R", 0xffffffff);

  hlineColor(surface, 0, surface->w, v_position, 0xffffffff);

  for (i=1; i<size; i++){
    aalineColor(surface, (int)((surface->w / 2) + ((i-1)*step)), ((int)(right_channel[i-1] * height / (float)DAMP_FACTOR) + v_position), (int)((surface->w / 2) + (i*step)), ((int)(right_channel[i] * height / (float)DAMP_FACTOR) + v_position), pixel);
    aalineColor(surface, (int)((surface->w / 2) - ((i-1)*step)), ((int)(left_channel[i-1] * height / (float)DAMP_FACTOR) + v_position), (int)((surface->w / 2) - (i*step)), ((int)(left_channel[i]  * height / (float)DAMP_FACTOR) + v_position), pixel);
  }

}

void draw_imaginary_spectrum(SDL_Surface *surface, int size, fftw_complex * spectrum, int v_position, int height, const char * title, Uint32 pixel) {
  int i;
  float step;
  char spectrum_title[255];

  step = (((float)surface->w / 2) / (size));

  sprintf(spectrum_title, "%s Imaginary Spectrum", title);

  stringColor(surface, 10, v_position - height + (height / 10), spectrum_title, 0xffffffff);

  hlineColor(surface, 0, surface->w, v_position, 0xffffffff);

  for (i=1; i < size; i++){
    aalineColor(surface, (int)((surface->w / 2) + ((i-1)*step)), ((int)(spectrum[1][i-1] * height) + v_position), (int)((surface->w / 2) + (i*step)), ((int)(spectrum[1][i] * height) + v_position), pixel);
    aalineColor(surface, (int)((surface->w / 2) - ((i-1)*step)), ((int)(spectrum[1][i-1] * height) + v_position), (int)((surface->w / 2) - (i*step)), ((int)(spectrum[1][i] * height) + v_position), pixel);
  }

}

void draw_real_spectrum(SDL_Surface *surface, int size, fftw_complex * spectrum, int v_position, int height, const char * title, Uint32 pixel) {
  int i;
  float step;
   char spectrum_title[255];

  step = (((float)surface->w / 2) / (size));

  sprintf(spectrum_title, "%s Real Spectrum", title);

  stringColor(surface, 10, v_position - height + (height / 10), spectrum_title, 0xffffffff);

  hlineColor(surface, 0, surface->w, v_position, 0xffffffff);

  for (i=1; i < size; i++){
    aalineColor(surface, (int)((surface->w / 2) + ((i-1)*step)), ((int)(spectrum[0][i-1] * height) + v_position), (int)((surface->w / 2) + (i*step)), ((int)(spectrum[0][i] * height) + v_position), pixel);
    aalineColor(surface, (int)((surface->w / 2) - ((i-1)*step)), ((int)(spectrum[0][i-1] * height) + v_position), (int)((surface->w / 2) - (i*step)), ((int)(spectrum[0][i] * height) + v_position), pixel);
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

    SDL_initFramerate(&hax_fps);
    SDL_setFramerate(&hax_fps, 60);

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
        printf("Unable to set %i x %i video: %s\n", SCREENW, SCREENH, SDL_GetError());
        pthread_exit((void *) 1);
    }

    std::cout << "[SDL] " << screen->w/2 << std::endl;
    std::cout << "[SDL] " << screen->h/2 << std::endl;

    // program main loop
    // bool done = false;

    printf("[SDL] Waiting to start...\n");
    fflush(stdout);
    std::cout.flush();

    hax_configs->timer->start();

    printf("[SDL] Firing Loop!\n");
    fflush(stdout);
    std::cout.flush();
    while (*hax_user_settings->message)
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
                *hax_user_settings->message = false;
                break;

                // check for keypresses
            case SDL_KEYDOWN:
                {
                    // exit if ESCAPE is pressed
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                        *hax_user_settings->message = false;
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

        //sine_graph(screen, 0.01, 60, displ, 0xff00ffff);


        printf("[SDL] Requiring Locks!\n");
        fflush(stdout);
        std::cout.flush();
        hax_sound_data->lock_data();
          printf("[SDL] Plotting Sound Data!\n");
          fflush(stdout);
          std::cout.flush();
          draw_sound(screen, hax_sound_data->get_frames(), hax_sound_data->get_left_sound_channel(), hax_sound_data->get_right_sound_channel(), (screen->h)/7, (screen->h)/7, 0xff00ffff);
          draw_real_spectrum(screen, hax_sound_data->get_frames(), hax_sound_data->get_right_spectrum(), (screen->h*7)/14, ((screen->h)/14), (char *)"Right", 0x00ffffff );
          draw_imaginary_spectrum(screen, hax_sound_data->get_frames(), hax_sound_data->get_right_spectrum(), (screen->h*9)/14, ((screen->h)/14), (char *)"Right", 0x00ffffff );

          draw_real_spectrum(screen, hax_sound_data->get_frames(), hax_sound_data->get_left_spectrum(), (screen->h*11)/14, ((screen->h)/14), (char *)"Left", 0x00ffffff);
          draw_imaginary_spectrum(screen, hax_sound_data->get_frames(), hax_sound_data->get_left_spectrum(), (screen->h*13)/14, ((screen->h)/14), (char *)"Left", 0x00ffffff );
          printf("[SDL] Releasing Locks!\n");
          fflush(stdout);
          std::cout.flush();
        hax_sound_data->unlock_data();

        // Draw circle
        //draw_circle(screen, (screen->w / 2), (screen->h / 2), 60, 0xffffffff);

        //sprintf(fpscount, "%s%i", "Current FPS: ", SDL_getFramerate(&hax_fps));// + SDL_getFramerate(&hax_fps);
        //stringColor(screen, 10, 10, fpscount, 0xffffffff);
        // DRAWING ENDS HERE

        // finally, update the screen :)
        SDL_Flip(screen);


        displ ++;
    } // end main loop

    // all is well ;)

    printf("Exited cleanly\n");
    pthread_exit((void *) 0);
}
