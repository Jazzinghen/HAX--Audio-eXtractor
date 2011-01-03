typedef struct {
  struct timespec r;
  uint32_t period;
} hax_thread_time_t;

typedef struct {

  uint32_t period;
  uint64_t offset;
  pthread_mutex_t * data_mutex;
  void * data_zone;

} hax_thread_config_t;

class hax_thread {

  hax_thread_config_t settings;
  void * thread_routine;
  pthread_t thread_handler;

  public:

    hax_thread (void * routine, uint32_t period, uint64_t offset, pthread_mutex_t mutex, void * data);
    int start();
    void join(void ** thread_result);
}


hax_thread::hax_thread (void * routine, uint32_t period, uint64_t offset, pthread_mutex_t * mutex, void * data){
  thread_routine = routine;
  settings.period = period;
  settings.offset = offset;
  settings.data_mutex = mutex;
  settings.data_zone = data;
}

void hax_thread::join (void ** thread_result){
  pthread_join(thread_handler, thread_result);
}
