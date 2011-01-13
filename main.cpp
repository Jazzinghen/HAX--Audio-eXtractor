#ifdef __cplusplus
    #include <cstdlib>
#else
    #include <stdlib.h>
#endif


#include "headers/hax_generic.hpp"
#include "headers/hax_sdl.hpp"
#include "headers/hax_alsa.hpp"
#include "headers/hax_fftw_data.h"
#include "headers/hax_sdl_data.h"
#include "headers/hax_fftw.h"
#include "headers/hax_threads.hpp"
#include "headers/hax_fftw_cross_data.h"

#define FRAMES 64
#define START_TIEM 2000000

int main ( int argc, char** argv )
{
  hax_general_settings_t hax_settings;
  hax_settings.access = 1;
  void ** res = NULL;
  bool run = true;


  // Hax Settings Initialisation
  hax_settings.sample_rate = 48000;//expected frame rate
  hax_settings.n_channels = 2;//expected number of channels
  hax_settings.device_name = (char *)"front";
  hax_settings.access = 1;
  hax_settings.buffer_size = 2048;//expected buffer size in frames
  hax_settings.period_size = 64;//expected period size in frames
  hax_settings.message = &run;


  struct option long_option[] =
  {

      {"device", 1, NULL, 'd'},
      {"rate", 1, NULL, 'r'},
      {"channels", 1, NULL, 'c'},
      {"method", 1, NULL, 'm'},
      {"buffer", 1, NULL, 'b'},
      {"period", 1, NULL, 'p'},
      {"help", 0, NULL, 'h'},
      {NULL, 0, NULL, 0},
  };//needed for getopt_long

  std::cout << "Parsing Options." << std::endl;
/************************** processing command line parameters ******************************/
  while (1) {
      int c;
      if ((c = getopt_long(argc, argv, "d:r:c:m:b:p:h", long_option, NULL)) < 0)
          break;
      switch (c)
      {
          case 'd':
              hax_settings.device_name = strdup(optarg);
              break;
          case 'r':
              hax_settings.sample_rate = atoi(optarg);
              hax_settings.sample_rate = hax_settings.sample_rate < 4000 ? 4000 : hax_settings.sample_rate;
              hax_settings.sample_rate = hax_settings.sample_rate > 196000 ? 196000 : hax_settings.sample_rate;
              break;
          case 'c':
              hax_settings.n_channels = atoi(optarg);
              hax_settings.n_channels = hax_settings.n_channels < 1 ? 1 : hax_settings.n_channels;
              hax_settings.n_channels = hax_settings.n_channels > 1024 ? 1024 : hax_settings.n_channels;
              break;
          case 'm':
              hax_settings.access = atoi(optarg);
              break;
          case 'b':
              hax_settings.buffer_size = atoi(optarg);
              break;
                      //  buffer_time(us) = 0.001 * buffer_size(frames) / rate(khz)
          case 'p':
              hax_settings.period_size = atoi(optarg);
              break;
          case 'h':
              help();
              exit(1);
              break;
      }
  };

  std::cout << "Creating Data Objects..." << std::endl;

  hax_fftw_data * alsa_data = new hax_fftw_data(hax_settings.period_size);
  hax_sdl_data * sdl_data = new hax_sdl_data(hax_settings.period_size);
  hax_fftw_cross_data_t hax_fftw_data = {sdl_data, alsa_data};

  std::cout << "Creating Threads..." << std::endl;

  hax_thread * sdl_thread = new hax_thread(hax_sdl_main, START_TIEM, 16666666, 1, (void *) sdl_data, hax_settings);
  hax_thread * fftw_thread = new hax_thread(hax_fftw_main, START_TIEM, 8333333, 2, (void *) &hax_fftw_data, hax_settings);
  hax_thread * alsa_thread = new hax_thread(hax_alsa_main, START_TIEM, 210000, 3, (void *) alsa_data, hax_settings);


  std::cout << "Starting Threads..." << std::endl;

  sdl_thread->start();
  fftw_thread->start();
  alsa_thread->start();

  std::cout << "Joining Threads..." << std::endl;

  sdl_thread->join(res);
  fftw_thread->join(res);
  alsa_thread->join(res);

  return 0;
}
