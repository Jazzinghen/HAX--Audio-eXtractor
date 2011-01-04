#ifndef HAX_FFTW_DATA_H
#define HAX_FFTW_DATA_H

#include "hax_generic.hpp"

class hax_fftw_data{

  public:
    hax_fftw_data(uint32_t frames);
    ~hax_fftw_data();
    void populate_channels(int16_t * data);
    int16_t * get_left_channel();
    int16_t * get_right_channel();
    int lock_data();
    int unlock_data();

  protected:
  private:

    int16_t * left_sound_channel;
    int16_t * right_sound_channel;
    pthread_mutex_t protection_mutex;
    uint32_t frames;
};

#endif // HAX_FFTW_DATA_H
