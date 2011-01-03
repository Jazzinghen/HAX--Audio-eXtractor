#ifndef HAX_THREADS_HPP_INCLUDED
#define HAX_THREADS_HPP_INCLUDED

#include <pthread.h>
#include "haxgeneric.hpp"

typedef struct {
  struct timespec r;
  uint32_t period;
} hax_thread_time_t;

typedef struct {

  uint32_t period;
  uint64_t offset;
  void * data_zone;

} hax_thread_config_t;

class hax_thread {

  private:

    hax_thread_config_t settings;
    void *(* thread_routine) (void *);
    pthread_t thread_handler;

  public:

    hax_thread (void *(* routine)(void *), uint32_t period, uint64_t offset, void * data);
    int start();
    int join(void ** thread_result);
};

#endif // HAX_THREADS_HPP_INCLUDED
