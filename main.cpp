#ifdef __cplusplus
    #include <cstdlib>
#else
    #include <stdlib.h>
#endif


#include "headers/haxgeneric.hpp"
#include "headers/haxsdl.hpp"
//#include "headers/haxalsa.hpp"
#include "headers/hax_fftw_data.h"
#include "headers/hax_sdl_data.h"

#define SCREENW 854
#define SCREENH 480

int main ( int argc, char** argv )
{

    hax_fftw_data * fftw_data = new hax_fftw_data(64);
    hax_sdl_data * sdl_data = new hax_sdl_data(64);

    FPSmanager hax_fps;
    float displ = 0;

    char fpscount[30];

    SDL_initFramerate(&hax_fps);
    SDL_setFramerate(&hax_fps, 30);

    // initialize SDL video
    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "Unable to init SDL: %s\n", SDL_GetError() );
        return 1;
    }

    // make sure SDL cleans up before exit
    atexit(SDL_Quit);

    // create a new window
    SDL_Surface* screen = SDL_SetVideoMode(SCREENW, SCREENH, 32,
                                           SDL_HWSURFACE|SDL_DOUBLEBUF);
    if ( !screen )
    {
        printf("Unable to set 640x480 video: %s\n", SDL_GetError());
        return 1;
    }

    std::cout << screen->w/2 << std::endl;
    std::cout << screen->h/2 << std::endl;

    // program main loop
    bool done = false;
    while (!done)
    {
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

        draw_sound(screen, 64, 0xff00ffff);

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


    fprintf (stderr, "microphone: BYE BYE\n");
    //closes the device
    //snd_pcm_close (*hax_device);
    printf("Exited cleanly\n");
    return 0;
}
