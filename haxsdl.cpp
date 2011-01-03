#include "headers/haxsdl.hpp"
#include <cmath>
#include <iostream>

#define PI 3.14159265
#define DAMP_FACTOR 9001

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
}

void draw_sound(SDL_Surface *surface, int size, Uint32 pixel) {
  int i;

  short right[size];
  short left[size];

  for (i=0; i < size; i++){
    scanf("%hi ", &left[i]);
  }
  for (i=0; i < size; i++){
    scanf("%hi ", &right[i]);
  }
  scanf("\n");

  for (i=1; i<size; i++){
    aalineColor(surface, (surface->w / 2) + ((i-1)*10), ((right[i-1] * (surface->h / 2) / DAMP_FACTOR) + (surface->h / 2)), (surface->w / 2) + (i*10), ((right[i] * (surface->h / 2) / DAMP_FACTOR) + (surface->h / 2)), pixel);
    aalineColor(surface, (surface->w / 2) - ((i-1)*10), ((left[i-1] * (surface->h / 2) / DAMP_FACTOR) + (surface->h / 2)), (surface->w / 2) - (i*10), ((left[i]  * (surface->h / 2) / DAMP_FACTOR) + (surface->h / 2)), pixel);
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
