#ifndef HAXGENERIC_HPP_INCLUDED
#define HAXGENERIC_HPP_INCLUDED

//#include <alsa/asoundlib.h>
#ifdef __APPLE__
#include <SDL/SDL.h>
#else
#include <SDL.h>
#endif

#include "SDL_gfxPrimitives.h"
#include "SDL_framerate.h"

#include <alsa/asoundlib.h>

#include <getopt.h>

#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>

#include <iostream>
#include <string>
#include <cassert>

//#include "hax_threads.hpp"


//general configuration parameters of the device
typedef struct {
    char * device_name;
    snd_pcm_uframes_t buffer_size;      //buffer size in frames
    snd_pcm_uframes_t period_size;      //period size in frames
    unsigned int buffer_time;           //length of the circular buffer in usec
    unsigned int period_time;           //length of one period in usec
    unsigned int n_channels;            //number of channels
    unsigned int sample_rate;           //frame rate
    snd_pcm_format_t sample_format;     //format of the samples
    snd_pcm_access_t access_type;       //PCM access type
    int access;                         //Raw Access Mode
    bool * message;

    int window_w;
    int window_h;
} hax_general_settings_t;


#endif // HAXGENERIC_HPP_INCLUDED
