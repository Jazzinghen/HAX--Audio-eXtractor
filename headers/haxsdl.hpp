#ifndef HAXSDL_HPP_INCLUDED
#define HAXSDL_HPP_INCLUDED

#include "haxgeneric.hpp"

void set_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel);

void sine_graph(SDL_Surface *surface, float freq, float amplitude, float displ, Uint32 pixel);

void draw_circle(SDL_Surface *surface, int cx, int cy, int radius, Uint32 pixel);

void draw_sound(SDL_Surface *surface, int size, Uint32 pixel);


#endif // HAXSDL_HPP_INCLUDED
