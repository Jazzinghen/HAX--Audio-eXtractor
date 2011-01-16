#include "headers/hax_threads.hpp"
#include "headers/hax_generic.hpp"

/*  Initialisation function for the hax_thread object.
 *
 *  It simply sets all the correct settings for the Thread, such as
 *  its name, its priority, the callback function, the period...
 */

hax_thread::hax_thread (void *(* routine)(void *),
                        uint32_t period,
                        uint64_t offset,
                        int priority,
                        void * data,
                        hax_general_settings_t user_settings,
                        const char * name) {

    thread_parameters.sched_priority = priority;
    thread_routine = routine;
    settings.timer = new hax_rt_timer(period, offset, name);
    settings.data_zone = data;
    settings.user_settings = user_settings;
}

hax_thread::~hax_thread(){
  delete settings.timer;
}

int hax_thread::join (void ** thread_result) {
    return pthread_join(thread_handler, thread_result);
}

/*  Function that fires the thread
 *
 *  In here we set all the parameters for the thread and then we call the
 *  pthread_create function.
 */
int hax_thread::start() {

    int err;
    pthread_attr_t thread_attributes;

    err = pthread_attr_init(&thread_attributes);

    assert(err == 0);

    err = pthread_attr_setschedpolicy(&thread_attributes, SCHED_FIFO);
    assert(err == 0);
    printf("[MAIN] Initializing Thread with priority %i\n",
           thread_parameters.sched_priority);
    err = pthread_attr_setschedparam(&thread_attributes, &thread_parameters);
    assert(err == 0);
    err = pthread_attr_setinheritsched(&thread_attributes, PTHREAD_EXPLICIT_SCHED);
    assert(err == 0);

    err = pthread_create(&thread_handler, &thread_attributes, thread_routine,
          (void *) &settings);

    assert(err == 0);

    pthread_attr_destroy(&thread_attributes);

    return err;
}
