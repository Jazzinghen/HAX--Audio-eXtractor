#ifndef HAXALSA_HPP_INCLUDED
#define HAXALSA_HPP_INCLUDED

#include "hax_generic.hpp"

int direct_rw(snd_pcm_t *device, hax_general_settings_t cap_dev_params);
int direct_mmap(snd_pcm_t *device, hax_general_settings_t cap_dev_params);

int xrun_recovery(snd_pcm_t *handle, int error);
void help(void);
void hax_alsa_main(void * settings);

#endif // HAXALSA_HPP_INCLUDED
