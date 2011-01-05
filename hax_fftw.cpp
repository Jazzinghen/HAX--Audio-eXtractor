#include "headers/hax_fftw.h"
#include "headers/hax_generic.hpp"
#include "headers/hax_rt_timer.h"
#include "headers/hax_threads.hpp"
#include "headers/hax_fftw_cross_data.h"
#include <fftw3.h>


void * hax_fftw_main(void * configuration) {
  fftw_plan hax_plan_left, hax_plan_right;

  hax_thread_config_t * hax_configs = (hax_thread_config_t *) configuration;
  hax_general_settings_t * hax_user_settings = &hax_configs->user_settings;
  hax_fftw_cross_data_t * fftw_data = (hax_fftw_cross_data_t *) hax_configs->data_zone;

  uint32_t frames = fftw_data->sdl->get_frames();

  doublex right_normalized_channel[frames], left_normalized_channel[frames];

  hax_plan_left = fftw_plan_dft_r2c_1d(frames, left_normalized_channel, fftw_data->sdl->left_spectrum, FFTW_MEASURE);
  hax_plan_right = fftw_plan_dft_r2c_1d(frames, right_normalized_channel, fftw_data->sdl->right_spectrum, FFTW_MEASURE);

  pthread_exit((void*) 0);
};
