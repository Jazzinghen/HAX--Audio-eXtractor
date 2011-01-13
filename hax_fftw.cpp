#include "headers/hax_fftw.h"
#include "headers/hax_generic.hpp"
#include "headers/hax_rt_timer.h"
#include "headers/hax_threads.hpp"
#include "headers/hax_fftw_cross_data.h"
#include <fftw3.h>

#define DAMP_FACTOR  32767

static void hax_fftw_normalize(int16_t * int_channel, double * normalized_channel, uint32_t frames) {
  uint32_t i = 0;
  for (i = 0; i < frames; i++){
    normalized_channel[i] = ((double)int_channel[i]) / DAMP_FACTOR;
  }
}

static void hax_print_max(fftw_complex * right_data, fftw_complex * left_data, uint32_t frames){
  int i = 0;

  float lmax, rmax = 0;

  for (i = 0; i < frames; i++) {
    if (left_data[1][i] > lmax) {lmax = left_data[1][i];};
    if (right_data[1][i] > rmax) {rmax = right_data[1][i];};
  }

  printf("[FFTW] Maximum Value For Left Spectrum: %f\n", lmax);
  printf("[FFTW] Maximum Value For Right Spectrum: %f\n", rmax);
};

void * hax_fftw_main(void * configuration) {
  fftw_plan hax_plan_left, hax_plan_right;

  printf("[FFTW] Thread Start!\n");

  struct timespec fftw_tiem;

  hax_thread_config_t * hax_configs = (hax_thread_config_t *) configuration;
  hax_general_settings_t * hax_user_settings = &hax_configs->user_settings;
  hax_fftw_cross_data_t * fftw_data = (hax_fftw_cross_data_t *) hax_configs->data_zone;

  uint32_t frames = fftw_data->sdl->get_frames();

  double right_normalized_channel[frames], left_normalized_channel[frames];

  printf("[FFTW] Initialising FFTW Plans.\n");

  hax_plan_left = fftw_plan_dft_r2c_1d(frames, left_normalized_channel, fftw_data->sdl->left_spectrum, FFTW_MEASURE);
  hax_plan_right = fftw_plan_dft_r2c_1d(frames, right_normalized_channel, fftw_data->sdl->right_spectrum, FFTW_MEASURE);

  printf("[FFTW] Starting FFTW Loop.\n");

  hax_configs->timer->start();

  printf("[FFTW] RT Timer Fired!\n");

  while (*hax_user_settings->message){
    clock_gettime(CLOCK_REALTIME, &fftw_tiem);

    hax_configs->timer->wait_next_activation();

    clock_gettime(CLOCK_REALTIME, &fftw_tiem);


    fftw_data->fftw->lock_data();
    fftw_data->sdl->lock_data();
      printf("[FFTW] Normalizing data and computing FT from the data.\n");
      hax_fftw_normalize(fftw_data->fftw->get_right_channel(), right_normalized_channel, frames);
      hax_fftw_normalize(fftw_data->fftw->get_left_channel(), left_normalized_channel, frames);
      printf("[FFTW] Data from L/R channel (FFTW Class), frames 24 and 38. L: {%i, %i} R: {%i, %i}\n",
             fftw_data->fftw->get_left_channel()[24], fftw_data->fftw->get_left_channel()[38],
             fftw_data->fftw->get_right_channel()[24], fftw_data->fftw->get_right_channel()[38]);
      memcpy(fftw_data->sdl->get_right_sound_channel(),fftw_data->fftw->get_right_channel(),sizeof(int16_t)*frames);
      memcpy(fftw_data->sdl->get_left_sound_channel(),fftw_data->fftw->get_left_channel(),sizeof(int16_t)*frames);
      printf("[FFTW] Data from L/R channel (SDL Class), frames 24 and 38. L: {%i, %i} R: {%i, %i}\n",
             fftw_data->sdl->get_left_sound_channel()[24], fftw_data->sdl->get_left_sound_channel()[38],
             fftw_data->sdl->get_right_sound_channel()[24], fftw_data->sdl->get_right_sound_channel()[38]);
      fftw_execute(hax_plan_left);
      fftw_execute(hax_plan_right);

      hax_print_max(fftw_data->sdl->right_spectrum, fftw_data->sdl->left_spectrum, (frames/2)+1);
    fftw_data->sdl->unlock_data();
    fftw_data->fftw->unlock_data();

  }

  pthread_exit((void*) 0);
};
