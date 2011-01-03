#include "headers/hax_threads.hpp"

hax_thread::hax_thread (void *(* routine)(void *), uint32_t period, uint64_t offset, void * data){
  thread_routine = routine;
  settings.period = period;
  settings.offset = offset;
  settings.data_zone = data;
}

int hax_thread::join (void ** thread_result){
  return pthread_join(thread_handler, thread_result);
}

int hax_thread::start(){
  return pthread_create(&thread_handler, NULL, thread_routine, (void *) &settings);
}
