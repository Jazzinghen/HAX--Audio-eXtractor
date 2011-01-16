//Needed libraries
#include "headers/hax_alsa.hpp"
#include "headers/hax_sdl.hpp"
#include "headers/hax_threads.hpp"
#include "headers/hax_fftw_data.h"

#define CIRCULAR_SAMPLES 20

//Enum needed to choose the type of I/O loop
typedef enum {
    METHOD_DIRECT_MMAP, //method with direct use of memory mapping
} enum_io_method;

//struct that defines one I/O method
typedef struct {
    enum_io_method method;   //I/O loop type
    snd_pcm_access_t access; //PCM access type
    int open_mode;           //open function flags
} io_method_t;


int direct_mmap(snd_pcm_t *device, hax_general_settings_t cap_dev_params,
                hax_fftw_data * shared_data, hax_rt_timer * rt_timer) {
    int error, state;
    int i;
    int circular_point = 0;

    //  Period size in frames
    snd_pcm_sframes_t period_size = cap_dev_params.period_size;

    int n_channels = cap_dev_params.n_channels;
    const snd_pcm_channel_area_t *my_areas;
    snd_pcm_uframes_t offset, frames, size;
    snd_pcm_sframes_t avail, commitres;
    int first=1;

    //  Quantity of data that we will get from the sound card
    snd_pcm_sframes_t total_frames = cap_dev_params.period_size * n_channels;
    //  Raw (interleaved) data got from the sound card
    int16_t data[total_frames];
    //  The average amplitude
    float average[total_frames];
    int16_t circular_data[CIRCULAR_SAMPLES][total_frames];

    memset(average, 0, sizeof(float) * total_frames);

    rt_timer->start();

    while(*cap_dev_params.message) {
        rt_timer->wait_next_activation();
        rt_timer->stat_execution_start();

        state = snd_pcm_state(device);
        //  If there was any error with the soundcard we have to fix it
        switch(state) {
        case SND_PCM_STATE_XRUN:  //  In case of Buffer over-run
            if ((error = xrun_recovery(device, -EPIPE)) < 0) {
                fprintf(stderr,"microphone: XRUN recovery failed: %s\n",
                        snd_strerror(error));
                return error;
            }
            //stream is restarted
            first = 1;
            break;

        case SND_PCM_STATE_SUSPENDED: //  In case that PCM is suspended
            if ((error = xrun_recovery(device, -ESTRPIPE)) < 0) {
                fprintf(stderr,"microphone: SUSPEND recovery failed: %s\n",
                        snd_strerror(error));
                return error;
            }
            break;
        }

        //  Checking how many frames we have to compute
        avail = snd_pcm_avail_update(device);
        if (avail < 0) {  // If the result is <0 then there was an error.
            if ((error = xrun_recovery(device, avail)) < 0) {
                fprintf(stderr,"microphone: SUSPEND recovery failed: %s\n",
                        snd_strerror(error));
                return error;
            }
            first = 1;
            continue;
        }
        if (avail < period_size) { // Checks if one period is ready to process
            switch(first) {
            case 1:
                /* If the capture from PCM is started (first=1) and one period is ready to process,
                 * then we have to start the capture stream
                 */
                first = 0;
                if ((error = snd_pcm_start(device)) < 0) {
                    fprintf(stderr,"microphone: Start error: %s\n",
                            snd_strerror(error));
                    exit(EXIT_FAILURE);
                }
                break;

            case 0:
                //  We have to wait until the PCM is ready
                if ((error = snd_pcm_wait(device, -1)) < 0) {
                    if ((error = xrun_recovery(device, error)) < 0) {
                        fprintf(stderr,"microphone: snd_pcm_wait error: %s\n",
                                snd_strerror(error));
                        exit(EXIT_FAILURE);
                    }
                    first = 1;
                }
            }
            continue;
        }
        size = period_size;
        while (size > 0) { //wait until we have period_size frames (in the most cases only one loop is needed)
            frames = size;//expected frames number to be processed
            //frames is a bidirectional variable, this means that the real number of frames processed is written
            //to this variable by the function.
            if ((error = snd_pcm_mmap_begin (device, &my_areas, &offset, &frames)) < 0) {
                if ((error = xrun_recovery(device, error)) < 0) {
                    fprintf(stderr,"microphone: MMAP begin avail error: %s\n",
                            snd_strerror(error));
                    exit(EXIT_FAILURE);
                }
                first = 1;
            }

            //  Copying the data present in the Sound card memory to our local array
            memcpy(circular_data[circular_point],
                   (void *)((uintptr_t)(my_areas[0].addr)+(offset*sizeof(int16_t)*n_channels)),
                   sizeof(int16_t)*total_frames);

            //  Computing the average
            for (i = 0; i < total_frames; i++) {
                average[i] += circular_data[circular_point][i]/(float)CIRCULAR_SAMPLES;
            }
            //  If we have enough samples we will put them into the shared memory
            if (circular_point >= (CIRCULAR_SAMPLES - 1)) {
                for (i = 0; i < total_frames; i++) {
                    data[i] = (int16_t) average[i];
                }
                printf("[ALSA] Locking Data.\n");
                shared_data->lock_data();
                printf("[ALSA] Populating Channels.\n");
                shared_data->populate_channels(data);
                printf("[ALSA] Releasing Locks.\n");
                shared_data->unlock_data();
                circular_point = 0;
                memset(average, 0, sizeof(float) * total_frames);
            } else {
                circular_point ++;
            }
            printf("\n");
            commitres = snd_pcm_mmap_commit(device, offset, frames);
            if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
                if ((error = xrun_recovery(device, commitres >= 0 ? commitres : -EPIPE)) < 0) {
                    fprintf(stderr,"microphone: MMAP commit error: %s\n", snd_strerror(error));
                    exit(EXIT_FAILURE);
                }
                first = 1;
            }
            size -= frames;
        }

        //  Printing Thread Statistics
        rt_timer->stat_update();

    }
    return 0;
}

//recovery callback in case of error
int xrun_recovery(snd_pcm_t *handle, int error) {
    switch(error) {
    case -EPIPE:    // Buffer Over-run
        fprintf(stderr,"microphone: \"Buffer Overrun\" \n");
        if ((error = snd_pcm_recover(handle, error, 1)) < 0)
            fprintf(stderr,"microphone: Buffer overrrun cannot be recovered, snd_pcm_prepare fail: %s\n",
                    snd_strerror(error));
        return 0;
        break;

    case -ESTRPIPE: //suspend event occurred
        fprintf(stderr,"microphone: Error ESTRPIPE\n");
        //EAGAIN means that the request cannot be processed immediately
        while ((error = snd_pcm_resume(handle)) == -EAGAIN)
            sleep(1);// wait until the suspend flag is clear

        if (error < 0) { // error case
            if ((error = snd_pcm_prepare(handle)) < 0)
                fprintf(stderr,"microphone: Suspend cannot be recovered, snd_pcm_prepare fail: %s\n",
                        snd_strerror(error));
        }
        return 0;
        break;

    case -EBADFD://Error PCM descriptor is wrong
        fprintf(stderr,"microphone: Error EBADFD\n");
        break;

    default:
        fprintf(stderr,"microphone: Error unknown, error = %d\n",error);
        break;
    }
    return error;
}


void * hax_alsa_main(void * settings) {

    int error,dir;
    snd_pcm_t *device;//capture device
    snd_pcm_hw_params_t *hw_params;//hardware configuration structure
    hax_thread_config_t * hax_configs = (hax_thread_config_t *) settings;
    hax_general_settings_t * hax_user_settings = &hax_configs->user_settings;

    // Array of the available I/O methods defined for capture, left for extensibility purposes
    io_method_t methods[] = {
        //  Direct Memory Mapping Method, faster than memory read/write and non blocking
        { METHOD_DIRECT_MMAP, SND_PCM_ACCESS_MMAP_INTERLEAVED, 0 },
    };


    printf("[ALSA] Thread Started, Initialising Capture device.\n");
    // Opening the device

    if ((error = snd_pcm_open (&device, hax_user_settings->device_name, SND_PCM_STREAM_CAPTURE,
                               methods[hax_user_settings->access].open_mode)) < 0) {
        fprintf (stderr, "microphone: Device cannot be opened  %s (%s)\n",
                 hax_user_settings->device_name,
                 snd_strerror (error));
        exit (1);
    }
    fprintf (stderr, "microphone: Device: %s open_mode = %d\n", hax_user_settings->device_name,
             methods[hax_user_settings->access].open_mode);

    // Allocating hardware configuration structure
    if ((error = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        fprintf (stderr, "microphone: Hardware configuration structure cannot be allocated (%s)\n",
                 snd_strerror (error));
        exit (1);
    }

    // Assigning the Hardware configuration structure to the device
    if ((error = snd_pcm_hw_params_any (device, hw_params)) < 0) {
        fprintf (stderr, "microphone: Hardware configuration structure cannot be assigned to device (%s)\n",
                 snd_strerror (error));
        exit (1);
    }

    // Showing the Capture Method, in case we implemented more than one

    switch(methods[hax_user_settings->access].method) {
    case METHOD_DIRECT_MMAP:
        fprintf (stderr, "microphone: capture method: METHOD_DIRECT_MMAP (m = 1)\n");
        break;
    }

    //  Configuring access method

    // Setting the configuration method
    fprintf (stderr, "microphone: hax_user_settings->access method: %d\n",
             methods[hax_user_settings->access].access);
    if ((error = snd_pcm_hw_params_set_access (device, hw_params,
                 methods[hax_user_settings->access].access)) < 0) {
        fprintf (stderr, "microphone: Access method cannot be configured (%s)\n",
                 snd_strerror (error));
        exit (1);
    }
    // Checking the access method we just set, to see if it was done right
    if ((error = snd_pcm_hw_params_get_access (hw_params,
                 &hax_user_settings->access_type)) < 0) {
        fprintf (stderr, "microphone: Access method cannot be obtained (%s)\n",
                 snd_strerror (error));
        exit (1);
    }
    // Showing the access method
    switch(hax_user_settings->access_type) {
    case SND_PCM_ACCESS_MMAP_INTERLEAVED:
        fprintf (stderr, "microphone: PCM access method: SND_PCM_ACCESS_MMAP_INTERLEAVED \n");
        break;
    case SND_PCM_ACCESS_MMAP_NONINTERLEAVED:
        fprintf (stderr, "microphone: PCM access method: SND_PCM_ACCESS_MMAP_NONINTERLEAVED \n");
        break;
    case SND_PCM_ACCESS_MMAP_COMPLEX:
        fprintf (stderr, "microphone: PCM access method: SND_PCM_ACCESS_MMAP_COMPLEX \n");
        break;
    case SND_PCM_ACCESS_RW_INTERLEAVED:
        fprintf (stderr, "microphone: PCM access method: SND_PCM_ACCESS_RW_INTERLEAVED \n");
        break;
    case SND_PCM_ACCESS_RW_NONINTERLEAVED:
        fprintf (stderr, "microphone: PCM access method: SND_PCM_ACCESS_RW_NONINTERLEAVED \n");
        break;
    }

    //  Configuring the Capture Format
    //  SND_PCM_FORMAT_S16_LE => 16 bit signed little endian
    if ((error = snd_pcm_hw_params_set_format (device, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
        fprintf (stderr, "microphone: Capture format cannot be configured (%s)\n",
                 snd_strerror (error));
        exit (1);
    }
    //  Checking if we set it right
    if ((error = snd_pcm_hw_params_get_format (hw_params, &hax_user_settings->sample_format)) < 0) {
        fprintf (stderr, "microphone: Capture sample format cannot be obtained (%s)\n",
                 snd_strerror (error));
        exit (1);
    }
    //  Showing it in a Human readible way
    switch(hax_user_settings->sample_format) {
    case SND_PCM_FORMAT_S16_LE:
        fprintf (stderr, "microphone: PCM capture sample format: SND_PCM_FORMAT_S16_LE \n");
        break;
    default:
        fprintf (stderr, "microphone: PCM capture sample format = %d\n",
                 hax_user_settings->sample_format);
    }

    //  Configuring the Sample Rate
    if ((error = snd_pcm_hw_params_set_rate (device, hw_params,
                 hax_user_settings->sample_rate, 0)) < 0) {
        fprintf (stderr, "microphone: Sample rate cannot be configured (%s)\n",
                 snd_strerror (error));
        exit (1);
    }
    if ((error = snd_pcm_hw_params_get_rate (hw_params,
                 &hax_user_settings->sample_rate, 0)) < 0) {
        fprintf (stderr, "microphone: Sample rate cannot be obtained (%s)\n",
                 snd_strerror (error));
        exit (1);
    }
    fprintf (stderr, "microphone: Sample_rate_real = %d\n", hax_user_settings->sample_rate);

    //  Configuring the number of channels
    if ((error = snd_pcm_hw_params_set_channels (device, hw_params,
                 hax_user_settings->n_channels)) < 0) {
        fprintf (stderr, "microphone: Number of channels cannot be configured (%s)\n",
                 snd_strerror (error));
        exit (1);
    }
    if ((error = snd_pcm_hw_params_get_channels (hw_params,
                 &hax_user_settings->n_channels)) < 0) {
        fprintf (stderr, "microphone: Number of channels cannot be obtained (%s)\n",
                 snd_strerror (error));
        exit (1);
    }
    fprintf (stderr, "microphone: real_n_channels = %d\n", hax_user_settings->n_channels);

    //  Configuring the buffer size
    if ((error = snd_pcm_hw_params_set_buffer_size(device, hw_params,
                 hax_user_settings->buffer_size)) < 0) {
        fprintf (stderr, "microphone: Buffer size cannot be configured (%s)\n",
                 snd_strerror (error));
        exit (1);
    }
    if ((error = snd_pcm_hw_params_get_buffer_size(hw_params,
                 &hax_user_settings->buffer_size)) < 0) {
        fprintf (stderr, "microphone: Buffer size cannot be obtained (%s)\n",
                 snd_strerror (error));
        exit (1);
    }
    fprintf (stderr, "microphone: Buffer size = %d [frames]\n", (int)hax_user_settings->buffer_size);

    //  Configuring the period size
    dir=0; /* Necessary since ALSA has this "strange" dir parameter that, if not equal to 0
            * tells the Audio device to chooose up to or at least the size given
            */
    if ((error = snd_pcm_hw_params_set_period_size(device, hw_params,
                 hax_user_settings->period_size, dir)) < 0) {
        fprintf (stderr, "microphone: Period size cannot be configured (%s)\n",
                 snd_strerror (error));
        exit (1);
    }
    if ((error = snd_pcm_hw_params_get_period_size(hw_params,
                 &hax_user_settings->period_size, &dir)) < 0) {
        fprintf (stderr, "microphone: Period size cannot be obtained (%s)\n",
                 snd_strerror (error));
        exit (1);
    }
    fprintf (stderr, "microphone: Period size = %d [frames]\n", (int)hax_user_settings->period_size);

    //  Applying audio configuration
    if ((error = snd_pcm_hw_params (device, hw_params)) < 0) {
        fprintf (stderr, "microphone: Hardware parameters cannot be configured (%s)\n",
                 snd_strerror (error));
        exit (1);
    }
    //  Deallocating the structure of hardware configuration
    snd_pcm_hw_params_free (hw_params);



    /*  Selecting the right way to get the data (In our case is only one, but, in case of
     *  Future implementation of other mewthods, such as RW, this would be necessary
     */
    switch(methods[hax_user_settings->access].method) {
    case METHOD_DIRECT_MMAP:
        direct_mmap(device, *hax_user_settings, (hax_fftw_data *) hax_configs->data_zone,
                    hax_configs->timer);
        break;

    }

    fprintf (stderr, "microphone: BYE BYE\n");
    //  Closing the device
    snd_pcm_close (device);
    // Printing Thread Statistics
    hax_configs->timer->stats_print();
    pthread_exit ((void*) 0);
}
