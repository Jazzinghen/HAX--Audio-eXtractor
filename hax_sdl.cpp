#include "headers/hax_sdl.hpp"
#include "headers/hax_threads.hpp"
#include "headers/hax_sdl_data.h"
#include <cmath>

#define PI 3.14159265
#define DAMP_FACTOR  32767

#define SCREENW 854
#define SCREENH 700

/*  The function that plots the sound amplitue */

static void draw_sound(SDL_Surface *surface, int size, int16_t * left_channel,
                int16_t * right_channel, int v_position, int height,
                Uint32 pixel) {
    int i;
    float step;

    /*  Here as in the other plotting functions we need to compute the step for each data, the only
     *  problem was that, if we computed it as an integer then, for higher value of size (That are the
     *  frames for each period) then all the values would have collapsed, since the "step" would have
     *  been 0.
     */
    step = (((float)surface->w / 2) / size);

    stringColor(surface, 10, v_position - height + (height / 10), "Amplitude", 0xffffffff);
    stringColor(surface, (surface->w / 2) - 20, v_position - height + (height / 10), "L", 0xffffffff);
    stringColor(surface, (surface->w / 2) + 10, v_position - height + (height / 10), "R", 0xffffffff);

    hlineColor(surface, 0, surface->w, v_position, 0xffffffff);

    //  We plot directly both channels, by going from left to right.
    for (i=1; i<size; i++) {
        aalineColor(surface, (int)((surface->w / 2) + ((i-1)*step)),
                    ((int)(right_channel[i-1] * height / (float)DAMP_FACTOR) + v_position),
                    (int)((surface->w / 2) + (i*step)),
                    ((int)(right_channel[i] * height / (float)DAMP_FACTOR) + v_position),
                    pixel);

        aalineColor(surface, (int)((surface->w / 2) - ((i-1)*step)),
                    ((int)(left_channel[i-1] * height / (float)DAMP_FACTOR) + v_position),
                    (int)((surface->w / 2) - (i*step)),
                    ((int)(left_channel[i]  * height / (float)DAMP_FACTOR) + v_position),
                    pixel);
    }

}

/*  This function plots only the imaginary part of the spectrum */

static void draw_imaginary_spectrum(SDL_Surface *surface, int size, fftw_complex * spectrum,
                             int v_position, int height, const char * title,
                             Uint32 pixel) {
    int i;
    float step;
    char spectrum_title[255];

    step = (((float)surface->w / 2) / (size));

    sprintf(spectrum_title, "%s Imaginary Spectrum", title);

    stringColor(surface, 10, v_position - height + (height / 10), spectrum_title, 0xffffffff);

    hlineColor(surface, 0, surface->w, v_position, 0xffffffff);

    for (i=1; i < size; i++) {
        aalineColor(surface, (int)((surface->w / 2) + ((i-1)*step)),
                    ((int)(spectrum[1][i-1] * height) + v_position),
                    (int)((surface->w / 2) + (i*step)),
                    ((int)(spectrum[1][i] * height) + v_position), pixel);
        aalineColor(surface, (int)((surface->w / 2) - ((i-1)*step)),
                    ((int)(spectrum[1][i-1] * height) + v_position),
                    (int)((surface->w / 2) - (i*step)),
                    ((int)(spectrum[1][i] * height) + v_position), pixel);
    }

}

/*  This function plots only the real part of the spectrum */

static void draw_real_spectrum(SDL_Surface *surface, int size, fftw_complex * spectrum,
                        int v_position, int height, const char * title,
                        Uint32 pixel) {
    int i;
    float step;
    char spectrum_title[255];

    step = (((float)surface->w / 2) / (size));

    sprintf(spectrum_title, "%s Real Spectrum", title);

    stringColor(surface, 10, v_position - height + (height / 10), spectrum_title, 0xffffffff);

    hlineColor(surface, 0, surface->w, v_position, 0xffffffff);

    for (i=1; i < size; i++) {
        aalineColor(surface, (int)((surface->w / 2) + ((i-1)*step)),
                    ((int)(spectrum[0][i-1] * height) + v_position),
                    (int)((surface->w / 2) + (i*step)),
                    ((int)(spectrum[0][i] * height) + v_position), pixel);
        aalineColor(surface, (int)((surface->w / 2) - ((i-1)*step)),
                    ((int)(spectrum[0][i-1] * height) + v_position),
                    (int)((surface->w / 2) - (i*step)),
                    ((int)(spectrum[0][i] * height) + v_position), pixel);
    }

}

void * hax_sdl_main(void *configuration) {

    hax_thread_config_t * hax_configs = (hax_thread_config_t *) configuration;
    hax_general_settings_t * hax_user_settings = &hax_configs->user_settings;
    hax_sdl_data * hax_sound_data = (hax_sdl_data *) hax_configs->data_zone;

    printf("[SDL] Thread Start!\n");

    FPSmanager hax_fps;
    float displ = 0;

    SDL_initFramerate(&hax_fps);
    SDL_setFramerate(&hax_fps, 60);

    // SDL video initialisation
    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
        printf( "Unable to init SDL: %s\n", SDL_GetError() );
        pthread_exit((void *) 1);
    }

    atexit(SDL_Quit);

    SDL_Surface* screen = SDL_SetVideoMode(hax_user_settings->window_w,
                                           hax_user_settings->window_h, 32,
                                           SDL_HWSURFACE|SDL_DOUBLEBUF);

    if ( !screen ) {
        printf("Unable to set %i x %i video: %s\n", hax_user_settings->window_w,
                hax_user_settings->window_h, SDL_GetError());
        pthread_exit((void *) 1);
    }

    printf("[SDL] Waiting to start...\n");
    fflush(stdout);
    std::cout.flush();

    hax_configs->timer->start();

    printf("[SDL] Firing Loop!\n");
    fflush(stdout);
    std::cout.flush();
    while (*hax_user_settings->message) {
        hax_configs->timer->wait_next_activation();
        hax_configs->timer->stat_execution_start();

        // We catch an SDL signal
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // If there was any signal
            switch (event.type) {
                // We exit if the window is closed
            case SDL_QUIT:
                *hax_user_settings->message = false;
                break;

                // We also check for keypresses
            case SDL_KEYDOWN: {
                // Then we exit if ESCAPE is pressed
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    *hax_user_settings->message = false;
                break;
            }
            }
        } // end of message processing

        // At first we clear the screen
        SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0, 0, 0));


        vlineColor(screen, screen->w / 2, 0, screen->h, 0xffffffff);


        printf("[SDL] Requiring Locks!\n");
        fflush(stdout);
        std::cout.flush();

        // Here we lock the data and then plot amplitude and spectrums
        hax_sound_data->lock_data();
        printf("[SDL] Plotting Sound Data!\n");
        fflush(stdout);
        std::cout.flush();
        draw_sound(screen, hax_sound_data->get_frames(), hax_sound_data->get_left_sound_channel(),
                   hax_sound_data->get_right_sound_channel(), (screen->h)/7, (screen->h)/7, 0xff00ffff);

        draw_real_spectrum(screen, hax_sound_data->get_frames(), hax_sound_data->get_right_spectrum(),
                           (screen->h*7)/14, ((screen->h)/14), (char *)"Right", 0x00ffffff );

        draw_imaginary_spectrum(screen, hax_sound_data->get_frames(),
                                hax_sound_data->get_right_spectrum(), (screen->h*9)/14, ((screen->h)/14),
                                (char *)"Right", 0x00ffffff );

        draw_real_spectrum(screen, hax_sound_data->get_frames(), hax_sound_data->get_left_spectrum(),
                           (screen->h*11)/14, ((screen->h)/14), (char *)"Left", 0x00ffffff);

        draw_imaginary_spectrum(screen, hax_sound_data->get_frames(),
                                hax_sound_data->get_left_spectrum(), (screen->h*13)/14, ((screen->h)/14),
                                (char *)"Left", 0x00ffffff );

        printf("[SDL] Releasing Locks!\n");
        fflush(stdout);
        std::cout.flush();
        hax_sound_data->unlock_data();

        // Finally, we update the screen
        SDL_Flip(screen);


        displ ++;
        hax_configs->timer->stat_update();
    }

    printf("Exited cleanly\n");

    //  Printing Thread Statistics
    hax_configs->timer->stats_print();
    pthread_exit((void *) 0);
}
