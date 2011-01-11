#ifndef HAX_RT_TIMER_H
#define HAX_RT_TIMER_H

#include "hax_generic.hpp"

extern int clock_nanosleep(clockid_t __clock_id, int __flags,
                           __const struct timespec *__req,
                           struct timespec *__rem);

class hax_rt_timer
{
  public:
    hax_rt_timer(uint32_t timer_period, uint64_t timer_offset);
    ~hax_rt_timer();
    void wait_next_activation();
    void start();

  protected:
  private:
    inline void add_us(uint64_t time);

    struct timespec m_nanotime;
    uint32_t m_period;
    uint64_t m_offset;
};

#endif // HAX_RT_TIMER_H
