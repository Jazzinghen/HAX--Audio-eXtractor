#ifndef HAX_RT_TIMER_H
#define HAX_RT_TIMER_H

#include "hax_generic.hpp"

extern int clock_nanosleep(clockid_t __clock_id, int __flags,
                           __const struct timespec *__req,
                           struct timespec *__rem);

typedef struct {
    uint64_t execution_times;
    uint64_t executions;
    uint64_t wcet;
    uint64_t deadlines;

    struct timespec execution_start;
} hax_rt_timer_stats_t;

class hax_rt_timer {
public:
    hax_rt_timer(uint32_t timer_period, uint64_t timer_offset, const char * name);
    ~hax_rt_timer();
    void wait_next_activation();
    void start();
    void stat_execution_start();
    void stat_update();
    void stats_print();

protected:
private:
    inline void add_us(uint64_t time);

    hax_rt_timer_stats_t stats;

    struct timespec m_nanotime;
    uint32_t m_period;
    uint64_t m_offset;
    char * thread_name;
};

#endif // HAX_RT_TIMER_H
