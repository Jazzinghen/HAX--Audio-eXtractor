#include "headers/hax_threads.hpp"

hax_thread::hax_thread (void *(* routine)(void *), uint32_t period, uint64_t offset, int priority, void * data, hax_general_settings_t user_settings){

  thread_parameters.sched_priority = priority;
  thread_routine = routine;
  settings.timer = new hax_rt_timer(period, offset);
  settings.data_zone = data;
  settings.user_settings = user_settings;
}

int hax_thread::join (void ** thread_result){
  return pthread_join(thread_handler, thread_result);
}

int hax_thread::start(){

  int err;
  pthread_attr_t thread_attributes;

  err = pthread_attr_init(&thread_attributes);

  err = pthread_attr_setschedpolicy(&thread_attributes, SCHED_FIFO);
  err = pthread_attr_setschedparam(&thread_attributes, &thread_parameters);
  err = pthread_attr_setinheritsched(&thread_attributes, PTHREAD_EXPLICIT_SCHED);

  err = pthread_create(&thread_handler, NULL, thread_routine, (void *) &settings);

  pthread_attr_destroy(&thread_attributes);

  return err;
}
