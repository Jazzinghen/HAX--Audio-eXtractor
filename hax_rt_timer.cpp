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

/*  Function to add a certain quantity of nanoseconds
 *
 *  To our next target activation time
 */
inline void hax_rt_timer::add_us(uint64_t time) {

    time += m_nanotime.tv_nsec;
    while (time >= NSEC_PER_SEC) {
        time -= NSEC_PER_SEC;
        m_nanotime.tv_sec++;
    }
    m_nanotime.tv_nsec = time;
}

/*  Function used to wait until the next arrival of the Thread
 *
 */
void hax_rt_timer::wait_next_activation() {

    int err = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME,
                              &m_nanotime, NULL);
    assert(err == 0);


    add_us(m_period);
}

/*  Function to fire the RT Timer
 *
 */
void hax_rt_timer::start() {
    clock_gettime(CLOCK_REALTIME, &m_nanotime);
    add_us(m_offset);
    wait_next_activation();
}

/*  Function used to get the time of the begin of a thread "cycle"
 *
 */
void hax_rt_timer::stat_execution_start(){
  clock_gettime(CLOCK_REALTIME, &stats.execution_start);
  stats.executions++;
}

/*  Function used to update a thread time statistics
 *
 *  Should be called at the end of each thread "cycle"
 */
void hax_rt_timer::stat_update(){
  struct timespec arrival_time;
  uint64_t difference;
  uint64_t sec_difference;

  clock_gettime(CLOCK_REALTIME, &arrival_time);

  difference = arrival_time.tv_nsec - stats.execution_start.tv_nsec;

  sec_difference = arrival_time.tv_sec - stats.execution_start.tv_sec;
  difference += sec_difference * NSEC_PER_SEC;

  stats.execution_times += difference;

  if (difference > stats.wcet){
    stats.wcet = difference;
  }

  if (difference > m_period){
    stats.deadlines++;
    fprintf (stderr, "[%s TIMER STATS] Deadline Miss!\n", thread_name);
  }
};

/*  Function to print a thread statistics
 *
 */
void hax_rt_timer::stats_print(){
  uint64_t execution_avg;
  float deadlines_ratio = 0;


  if (stats.deadlines > 0){
    deadlines_ratio = ((float)stats.deadlines / stats.executions) * 100;
  }

  execution_avg = stats.execution_times / stats.executions;

  fprintf (stderr, "\n[%s STATS] Stats for %s Thread:\n\n", thread_name, thread_name);
  fprintf (stderr, "[%s STATS] \tNumber of Executions: %li\n", thread_name, stats.executions);
  fprintf (stderr, "[%s STATS] \tAverage response time: %li\n", thread_name, execution_avg);
  fprintf (stderr, "[%s STATS] \tWorst Case Execution Time: %li\n", thread_name, stats.wcet);
  fprintf (stderr, "[%s STATS] \tNumber of Missed Deadlines: %li\n", thread_name, stats.deadlines);
  fprintf (stderr, "[%s STATS] \tDeadlines to Executions Ratio: %f%%\n", thread_name, deadlines_ratio);
}
