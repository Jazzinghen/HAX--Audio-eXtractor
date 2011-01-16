#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include <sys/mman.h>

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
#define NSEC_PER_SEC 1000000000ULL

/*  Function that prints the help text in case the -h parameter is given
 *  to the application
 */
void help(void) {
    printf(
        "Usage: HAX_Audio_eXtractor [OPTIONS]\n"
        "-h,--help      show this usage help\n"
        "-d,--device    device of capture\n"
        "-r,--rate      sample rate in hz\n"
        "-p,--period    period size in samples\n"
        "-b,--buffer    circular buffer size in samples\n"
        "-w,--width     plotting window width in pixels\n"
        "-a,--height    plotting window height in pixels\n"
        "\n");
}

int main ( int argc, char** argv ) {
    hax_general_settings_t hax_settings;
    hax_settings.access = 1;
    void ** res = NULL;
    bool run = true;
    uint32_t alsa_period;


    // Hax Settings Initialisation
    hax_settings.sample_rate = 48000; // Default frame rate
    hax_settings.n_channels = 2;      // Default number of channels
    hax_settings.device_name = (char *)"front"; // Default Capture Device
    hax_settings.access = 0;          // Default access method.
    hax_settings.buffer_size = 2048;  // Default buffer size in frames
    hax_settings.period_size = 256;   // Default period size in frames
    hax_settings.message = &run;      // Pointer to the run flag.
    hax_settings.window_w = 800;      // Default Plotting Window Width
    hax_settings.window_h = 600;      // Default Plotting Window Height


    struct option long_option[] = {

        {"device", 1, NULL, 'd'},
        {"rate", 1, NULL, 'r'},
        {"buffer", 1, NULL, 'b'},
        {"period", 1, NULL, 'p'},
        {"width", 1, NULL, 'w'},
        {"height", 1, NULL, 'a'},
        {"help", 0, NULL, 'h'},
        {NULL, 0, NULL, 0},
    };

    std::cout << "Parsing Options." << std::endl;

    while (1) {
        int c;
        if ((c = getopt_long(argc, argv, "d:r:b:p:a:w:h", long_option, NULL)) < 0)
            break;
        switch (c) {
        case 'd':
            hax_settings.device_name = strdup(optarg);
            break;
        case 'r':
            hax_settings.sample_rate = atoi(optarg);
            if (hax_settings.sample_rate < 4000) {
              hax_settings.sample_rate = 4000;
            } else if (hax_settings.sample_rate > 196000) {
              hax_settings.sample_rate = 196000;
            }
            break;
        case 'b':
            hax_settings.buffer_size = atoi(optarg);
            break;
        case 'p':
            hax_settings.period_size = atoi(optarg);
            break;
        case 'w':
            hax_settings.window_w = atoi(optarg);
            if (hax_settings.window_w < 150) {
              hax_settings.window_w = 150;
            }
            break;
        case 'a':
            hax_settings.window_h = atoi(optarg);
            if (hax_settings.window_h < 150) {
              hax_settings.window_h = 150;
            }
            break;
        case 'h':
            help();
            exit(1);
            break;
        }
    };


    // Formula to compute the period for the ALSA Thread

    alsa_period = (uint32_t)((NSEC_PER_SEC * (float)hax_settings.buffer_size) / (hax_settings.sample_rate * 10));

    printf("[MAIN] Creating Data Objects...\n");

    // Creating all the shared memory spaces for the threads
    hax_fftw_data * alsa_data = new hax_fftw_data(hax_settings.period_size);
    hax_sdl_data * sdl_data = new hax_sdl_data(hax_settings.period_size);
    hax_fftw_cross_data_t hax_fftw_data = {sdl_data, alsa_data};

    // Locking memory to avoid Major Page Faults
    mlockall( MCL_CURRENT | MCL_FUTURE );

    printf("[MAIN] Creating Threads...\n");

    // Creating Threads
    hax_thread * sdl_thread = new hax_thread(hax_sdl_main, 16666666, START_TIEM,
                                             1, (void *) sdl_data, hax_settings, "SDL");
    hax_thread * fftw_thread = new hax_thread(hax_fftw_main, 8333333, START_TIEM,
                                              2, (void *) &hax_fftw_data, hax_settings, "FFTW");
    hax_thread * alsa_thread = new hax_thread(hax_alsa_main, alsa_period, START_TIEM,
                                              3, (void *) alsa_data, hax_settings, "ALSA");

    printf("Starting Threads...\n");

    // Starting Threads
    sdl_thread->start();
    fftw_thread->start();
    alsa_thread->start();

    printf("Joining Threads...\n");

    // Waiting for them to terminate
    sdl_thread->join(res);
    fftw_thread->join(res);
    alsa_thread->join(res);

    printf("Cleaning Memory...\n");

    // Clearing memory from all the objects we allocated
    delete alsa_data;
    delete sdl_data;
    delete sdl_thread;
    delete fftw_thread;
    delete alsa_thread;

    munlockall();

    printf("All Done!\n");

    return 0;
}
