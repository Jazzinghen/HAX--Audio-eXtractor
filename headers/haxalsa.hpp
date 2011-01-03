#ifndef HAXALSA_HPP_INCLUDED
#define HAXALSA_HPP_INCLUDED

#include "haxgeneric.hpp"

//general configuration parameters of the device
typedef struct {
    char * device_name;
    snd_pcm_uframes_t buffer_size;      //buffer size in frames
    snd_pcm_uframes_t period_size;      //period size in frames
    unsigned int buffer_time;           //length of the circular buffer in usec
    unsigned int period_time;           //length of one period in usec
    unsigned int n_channels;                     //number of channels
    unsigned int sample_rate;           //frame rate
    snd_pcm_format_t sample_format;     //format of the samples
    snd_pcm_access_t access_type;       //PCM access type
} hax_device_conf_t;

int direct_rw(snd_pcm_t *device, hax_device_conf_t cap_dev_params);
int direct_mmap(snd_pcm_t *device, hax_device_conf_t cap_dev_params, SDL_Surface *surface);

int xrun_recovery(snd_pcm_t *handle, int error);
void help(void);
void hax_alsa_initialiser(int argc, char** argv, snd_pcm_t **device, hax_device_conf_t *hax_device);

#endif // HAXALSA_HPP_INCLUDED
