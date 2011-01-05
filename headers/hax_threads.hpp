#ifndef HAX_THREADS_HPP_INCLUDED
#define HAX_THREADS_HPP_INCLUDED

#include "hax_generic.hpp"
#include "hax_rt_timer.h"

typedef struct {
  struct timespec r;
  uint32_t period;
} hax_thread_time_t;

typedef struct {

  hax_rt_timer * timer;
  void * data_zone;
  hax_general_settings_t user_settings;

} hax_thread_config_t;

class hax_thread {

  private:

    hax_thread_config_t settings;
    void *(* thread_routine) (void *);
    struct sched_param thread_parameters;
    pthread_t thread_handler;

  public:

    hax_thread (void *(* routine)(void *), uint32_t period, uint64_t offset, int priority, void * data, hax_general_settings_t settings);
    int start();
    int join(void ** thread_result);
};

#endif // HAX_THREADS_HPP_INCLUDED
