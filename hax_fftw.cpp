#include "headers/hax_fftw.h"
#include "headers/hax_generic.hpp"
#include "headers/hax_rt_timer.h"
#include "headers/hax_threads.hpp"
#include "headers/hax_fftw_cross_data.h"
#include <fftw3.h>

#define DAMP_FACTOR  32767

/*  Normalisation of the data got from the soundcard.
 *
 *  This is useful to get consistent data from the FFTW library. It simply divides each frame
 *  by INT16_MAX (Which I cannot manage to use for some strange reason).
 */
static void hax_fftw_normalize(int16_t * int_channel, double * normalized_channel, uint32_t frames) {
    uint32_t i = 0;
    for (i = 0; i < frames; i++) {
        normalized_channel[i] = ((double)int_channel[i]) / DAMP_FACTOR;
    }
}

void * hax_fftw_main(void * configuration) {
    fftw_plan hax_plan_left, hax_plan_right;

    printf("[FFTW] Thread Start!\n");

    hax_thread_config_t * hax_configs = (hax_thread_config_t *) configuration;
    hax_general_settings_t * hax_user_settings = &hax_configs->user_settings;
    hax_fftw_cross_data_t * fftw_data = (hax_fftw_cross_data_t *) hax_configs->data_zone;

    uint32_t frames = fftw_data->sdl->get_frames();

    double right_normalized_channel[frames], left_normalized_channel[frames];

    printf("[FFTW] Initialising FFTW Plans.\n");

    //  Initialising the necessary fftw plans.
    hax_plan_left = fftw_plan_dft_r2c_1d(frames, left_normalized_channel,
                      fftw_data->sdl->left_spectrum, FFTW_MEASURE);
    hax_plan_right = fftw_plan_dft_r2c_1d(frames, right_normalized_channel,
                      fftw_data->sdl->right_spectrum, FFTW_MEASURE);

    printf("[FFTW] Starting FFTW Loop.\n");

    hax_configs->timer->start();

    printf("[FFTW] RT Timer Fired!\n");

    while (*hax_user_settings->message) {
        hax_configs->timer->wait_next_activation();
        hax_configs->timer->stat_execution_start();

        //  Locking data
        fftw_data->fftw->lock_data();
        fftw_data->sdl->lock_data();
        printf("[FFTW] Normalizing data and computing FT from the data.\n");
        hax_fftw_normalize(fftw_data->fftw->get_right_channel(), right_normalized_channel, frames);
        hax_fftw_normalize(fftw_data->fftw->get_left_channel(), left_normalized_channel, frames);

        memcpy(fftw_data->sdl->get_right_sound_channel(),fftw_data->fftw->get_right_channel(),sizeof(int16_t)*frames);
        memcpy(fftw_data->sdl->get_left_sound_channel(),fftw_data->fftw->get_left_channel(),sizeof(int16_t)*frames);

        //  Computing the two spectrums
        fftw_execute(hax_plan_left);
        fftw_execute(hax_plan_right);

        fftw_data->sdl->unlock_data();
        fftw_data->fftw->unlock_data();

        hax_configs->timer->stat_update();
    }

    // Printing Thread Statistics
    hax_configs->timer->stats_print();

    pthread_exit((void*) 0);
};
