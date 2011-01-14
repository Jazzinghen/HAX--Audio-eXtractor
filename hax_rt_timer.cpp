#include "headers/hax_rt_timer.h"

#define NSEC_PER_SEC 1000000000ULL

hax_rt_timer::hax_rt_timer(uint32_t timer_period,
                           uint64_t timer_offset,
                           const char * name) {
    m_offset = timer_offset;
    m_period = timer_period;
    thread_name = strdup(name);
}

hax_rt_timer::~hax_rt_timer() {
    //dtor
}

inline void hax_rt_timer::add_us(uint64_t time) {
    //time *= 1000;
    time += m_nanotime.tv_nsec;
    while (time >= NSEC_PER_SEC) {
        time -= NSEC_PER_SEC;
        m_nanotime.tv_sec++;
    }
    m_nanotime.tv_nsec = time;
}

void hax_rt_timer::wait_next_activation() {

    struct timespec debug_time;

    clock_gettime(CLOCK_REALTIME, &debug_time);

    printf("[DE_TIMER_%s] Requested Nanosleep.\n"
           "[DE_TIMER_%s] \tStarting time: %li, %li\n"
           "[DE_TIMER_%s] \tTarget: %li, %li\n",
           thread_name,
           thread_name,debug_time.tv_sec, debug_time.tv_nsec,
           thread_name,m_nanotime.tv_sec, m_nanotime.tv_nsec);

    int err = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME,
                              &m_nanotime, NULL);
    assert(err == 0);

    clock_gettime(CLOCK_REALTIME, &debug_time);
    printf("[DE_TIMER_%s] Nanosleep terminated.\n"
           "[DE_TIMER_%s] \tArrival time: %li, %li\n"
           "[DE_TIMER_%s] \tError in Arrival time: %li, %li\n",
           thread_name, thread_name, debug_time.tv_sec, debug_time.tv_nsec,
           thread_name, debug_time.tv_sec-m_nanotime.tv_sec, debug_time.tv_nsec-m_nanotime.tv_nsec);


    add_us(m_period);
}

void hax_rt_timer::start() {
    clock_gettime(CLOCK_REALTIME, &m_nanotime);
    add_us(m_offset);
    wait_next_activation();
}
