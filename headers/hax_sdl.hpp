#ifndef HAXSDL_HPP_INCLUDED
#define HAXSDL_HPP_INCLUDED

#include "hax_generic.hpp"
#include <fftw3.h>

void * hax_sdl_main(void *configuration);

void set_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel);

void sine_graph(SDL_Surface *surface, float freq, float amplitude, float displ, Uint32 pixel);

void draw_circle(SDL_Surface *surface, int cx, int cy, int radius, Uint32 pixel);

void draw_sound(SDL_Surface *surface, int size, int16_t * left_channel, int16_t * right_channel, int v_position, int height, Uint32 pixel);

void draw_imaginary_spectrum(SDL_Surface *surface, int size, fftw_complex * spectrum, int v_position, int height, const char * title, Uint32 pixel);

void draw_real_spectrum(SDL_Surface *surface, int size, fftw_complex * spectrum, int v_position, int height, const char * title, Uint32 pixel);

#endif // HAXSDL_HPP_INCLUDED
