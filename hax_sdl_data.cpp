#include "headers/hax_sdl_data.h"

hax_sdl_data::hax_sdl_data(uint32_t frames) {
    int err = 0;
    pthread_mutexattr_t mutex_attr;

    left_sound_channel = (int16_t *) calloc(frames, sizeof(int16_t));
    right_sound_channel = (int16_t *) calloc(frames, sizeof(int16_t));

    left_spectrum = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * ( (frames / 2) + 1));
    right_spectrum = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * ( (frames / 2) + 1));

    this->frames = frames;

    err = pthread_mutexattr_init(&mutex_attr);
    assert(err == 0);
    err = pthread_mutexattr_setprotocol(&mutex_attr, PTHREAD_PRIO_INHERIT);
    assert(err == 0);
    err = pthread_mutex_init(&protection_mutex, &mutex_attr);
    assert(err == 0);
    err = pthread_mutexattr_destroy(&mutex_attr);
}

hax_sdl_data::~hax_sdl_data() {
    free(left_sound_channel);
    free(right_sound_channel);

    fftw_free(left_spectrum);
    fftw_free(right_spectrum);

    pthread_mutex_destroy(&protection_mutex);
}

void hax_sdl_data::set_sound_channels(int16_t * left_channel, int16_t * right_channel) {
    memcpy(left_sound_channel, left_channel, frames * sizeof(int16_t));
    memcpy(right_sound_channel, right_channel, frames * sizeof(int16_t));
}

void hax_sdl_data::set_spectrums(fftw_complex * left_channel, fftw_complex * right_channel) {
    memcpy(left_spectrum, left_channel, frames * sizeof(fftw_complex));
    memcpy(right_spectrum, right_channel, frames * sizeof(fftw_complex));
}

int hax_sdl_data::lock_data() {
    printf("[SDL LOCK]Locking...\n");
    return pthread_mutex_lock(&protection_mutex);
}

int hax_sdl_data::unlock_data() {
    printf("[SDL LOCK]Unlocking...\n");
    return pthread_mutex_unlock(&protection_mutex);
}
