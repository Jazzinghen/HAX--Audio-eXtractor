#ifndef HAX_FFTW_CROSS_DATA_H_INCLUDED
#define HAX_FFTW_CROSS_DATA_H_INCLUDED

#include "hax_sdl_data.h"
#include "hax_fftw_data.h"

typedef struct {
    hax_sdl_data * sdl;
    hax_fftw_data * fftw;
} hax_fftw_cross_data_t;

#endif // HAX_FFTW_CROSS_DATA_H_INCLUDED
