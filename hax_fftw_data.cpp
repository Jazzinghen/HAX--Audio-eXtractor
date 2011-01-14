#include "headers/hax_fftw_data.h"
#include "headers/hax_generic.hpp"

hax_fftw_data::hax_fftw_data(uint32_t frames) {
    int err;

    pthread_mutexattr_t mutex_attr;

    left_sound_channel = (int16_t *) calloc(frames, sizeof(int16_t));
    right_sound_channel = (int16_t *) calloc(frames, sizeof(int16_t));
    this->frames = frames;

    err = pthread_mutexattr_init(&mutex_attr);
    assert(err == 0);
    err = pthread_mutexattr_setprotocol(&mutex_attr, PTHREAD_PRIO_INHERIT);
    assert(err == 0);
    err = pthread_mutex_init(&protection_mutex, &mutex_attr);
    assert(err == 0);
    err = pthread_mutexattr_destroy(&mutex_attr);

}

hax_fftw_data::~hax_fftw_data() {
    free(left_sound_channel);
    free(right_sound_channel);

    pthread_mutex_destroy(&protection_mutex);
}

void hax_fftw_data::populate_channels(int16_t * data) {
    uint32_t i;

    for (i = 0; i < frames; i++) {
        left_sound_channel[i] = data[i*2];
        right_sound_channel[i] = data[(i*2)+1];
    }
}

int16_t * hax_fftw_data::get_left_channel() {
    return left_sound_channel;
}

int16_t * hax_fftw_data::get_right_channel() {
    return right_sound_channel;
}

int hax_fftw_data::lock_data() {
    printf("[FFTW LOCK]Locking...\n");
    return pthread_mutex_lock(&protection_mutex);
}

int hax_fftw_data::unlock_data() {
    printf("[FFTW LOCK]Unlocking...\n");
    return pthread_mutex_unlock(&protection_mutex);
}
