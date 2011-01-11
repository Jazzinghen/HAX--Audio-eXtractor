#include "headers/hax_rt_timer.h"

#define NSEC_PER_SEC 1000000000ULL

hax_rt_timer::hax_rt_timer(uint32_t timer_period, uint64_t timer_offset) {
  m_offset = timer_offset;
  m_period = timer_period;
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
  printf("[TIMER] Waiting...\n");
  clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &m_nanotime, NULL);

  add_us(m_period);
}

void hax_rt_timer::start() {
    clock_gettime(CLOCK_REALTIME, &m_nanotime);
    add_us(m_offset);
    wait_next_activation();
}
