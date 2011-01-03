#ifndef HAX_SDL_DATA_H
#define HAX_SDL_DATA_H

#include <fftw3.h>

#include "haxgeneric.hpp"

class hax_sdl_data{

  public:

    hax_sdl_data(uint32_t frames);
    ~hax_sdl_data();
    fftw_complex * get_left_spectrum() { return left_spectrum; }
    fftw_complex * get_right_spectrum() { return right_spectrum; }

    void set_spectrums(fftw_complex * left_channel, fftw_complex * right_channel);

    int16_t * get_right_sound_channel() { return right_sound_channel; }
    int16_t * get_left_sound_channel() { return left_sound_channel; }

    void set_sound_channels(int16_t * left_channel, int16_t * right_channel);

    int lock_data();
    int unlock_data();

  protected:
  private:

    fftw_complex * left_spectrum;
    fftw_complex * right_spectrum;
    pthread_mutex_t protection_mutex;
    int16_t * right_sound_channel;
    int16_t * left_sound_channel;
    uint32_t frames;
};

#endif // HAX_SDL_DATA_H
