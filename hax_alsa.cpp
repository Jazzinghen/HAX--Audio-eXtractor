//Needed libraries
#include "headers/hax_alsa.hpp"
#include "headers/hax_sdl.hpp"
#include "headers/hax_threads.hpp"
#include "headers/hax_fftw_data.h"

#define CIRCULAR_SAMPLES 5

//Enum needed to choose the type of I/O loop
typedef enum {
    METHOD_DIRECT_RW,   //method with direct use of read/write functions
    METHOD_DIRECT_MMAP, //method with direct use of memory mapping
} enum_io_method;

//struct that defines one I/O method
typedef struct{
    enum_io_method method;   //I/O loop type
    snd_pcm_access_t access; //PCM access type
    int open_mode;           //open function flags
} io_method_t;

/*******************************************************************************/
/********************* case: direct with r/w functions *************************/
/*******************************************************************************/
//This case only uses a main loop
int direct_rw(snd_pcm_t *device, hax_general_settings_t cap_dev_params)

{
    int error;
    snd_pcm_sframes_t period_size = cap_dev_params.period_size;
    int n_channels = cap_dev_params.n_channels;
    short buf[n_channels*period_size];//audio samples buffer
    signed short *ptr;
    int cptr;

    while(1)
    {
        ptr = buf; //aux pointer in buff
        cptr = period_size; //aux ptr needed to calculate remained frames
        //in the most cases readi only uses one bucle
        while(cptr > 0) {//wait until buf is full with period_size frames
            error = snd_pcm_readi (device, ptr, cptr);
            if (error < 0)
            {
                if (xrun_recovery(device, error) < 0) {
                    fprintf(stderr,"microphone: Write error: %s\n", snd_strerror(error));
                    exit(EXIT_FAILURE);
                }
                break; //discard current period
            }
            ptr += error * n_channels;
            cptr -= error;
        }
        //write to standard output
        write(STDOUT_FILENO, buf, sizeof(short)*period_size*n_channels);
    }
}


/*******************************************************************************/
/********************** case: direct with memory mapping ***********************/
/*******************************************************************************/
//this case also uses one main loop
int direct_mmap(snd_pcm_t *device, hax_general_settings_t cap_dev_params, hax_fftw_data * shared_data, hax_rt_timer * rt_timer)
{
    int error, state;

    snd_pcm_sframes_t period_size = cap_dev_params.period_size;//period size in frames
    int n_channels = cap_dev_params.n_channels;//number of channels
    const snd_pcm_channel_area_t *my_areas;//mapped memory area info
    snd_pcm_uframes_t offset, frames, size;//aux for frames count
    snd_pcm_sframes_t avail, commitres;//aux for frames count

    int first = 1; //first == 1  => first period of the stream is processed now
    int circular_step = 0;
    int i, j;

    int16_t average[cap_dev_params.period_size*n_channels];
    int16_t circular_data[CIRCULAR_SAMPLES][cap_dev_params.period_size*n_channels];

    std::cout << "Inizio la cattura audio" << std::endl;

    while(1) //main loop
    {
      rt_timer->wait_next_activation();
      state = snd_pcm_state(device); //needed for descriptor check
      std::cout << "WAT" << std::endl;
      switch(state)
      {
          case SND_PCM_STATE_XRUN://buffer over-run
              //fprintf(stderr,"microphone: SND_PCM_STATE_XRUN\n");
              if ((error = xrun_recovery(device, -EPIPE)) < 0)
              {
                  fprintf(stderr,"microphone: XRUN recovery failed: %s\n", snd_strerror(error));
                  return error;
              }
      //stream is restarted
              first = 1;
              break;

          case SND_PCM_STATE_SUSPENDED://PCM is suspended
          //fprintf(stderr,"microphone: SND_PCM_STATE_SUSPENDED\n");
              if ((error = xrun_recovery(device, -ESTRPIPE)) < 0)
              {
                  fprintf(stderr,"microphone: SUSPEND recovery failed: %s\n", snd_strerror(error));
                  return error;
              }
              break;
      }

  //checks how many frames are ready to read or write
      std::cout << "Update available blocks" << std::endl;
      avail = snd_pcm_avail_update(device);
      while (avail < 0)
      {
          if ((error = xrun_recovery(device, avail)) < 0) {
              fprintf(stderr,"microphone: SUSPEND recovery failed: %s\n", snd_strerror(error));
              return error;
          }
          first = 1;
      }
      while (avail < period_size)//checks if one period is ready to process
      {
          switch(first)
          {
              case 1:
        //if the capture from PCM is started (first=1) and one period is ready to process,
        //the stream must start
                  first = 0;
                  if ((error = snd_pcm_start(device)) < 0) {
                      fprintf(stderr,"microphone: Start error: %s\n", snd_strerror(error));
                      exit(EXIT_FAILURE);
                  }
                  break;

              case 0:
                  //wait for pcm to become ready
                  if ((error = snd_pcm_wait(device, -1)) < 0) {
                      if ((error = xrun_recovery(device, error)) < 0) {
                          fprintf(stderr,"microphone: snd_pcm_wait error: %s\n", snd_strerror(error));
                          exit(EXIT_FAILURE);
                      }
                      first = 1;
                  }
          }
          continue;
      }
      while (size > 0) //wait until we have period_size frames (in the most cases only one loop is needed)
      {
          frames = size;//expected frames number to be processed
    //frames is a bidirectional variable, this means that the real number of frames processed is written
    //to this variable by the function.
          if ((error = snd_pcm_mmap_begin (device, &my_areas, &offset, &frames)) < 0) {
              if ((error = xrun_recovery(device, error)) < 0) {
                  fprintf(stderr,"microphone: MMAP begin avail error: %s\n", snd_strerror(error));
                  exit(EXIT_FAILURE);
              }
              first = 1;
          }
          //write to standard output

          memcpy(circular_data[circular_step], (void *)((uintptr_t)(my_areas[0].addr)+(offset*sizeof(short)*n_channels)), frames*sizeof(short)*n_channels);

          memset(average, 0, sizeof(int16_t) * frames*sizeof(short)*n_channels);

          for (j = 0; j < CIRCULAR_SAMPLES; j++){
            for (i = 0; i < (int)cap_dev_params.period_size*2; i++){
              average[i] += circular_data[j][i] / CIRCULAR_SAMPLES;
            }
          }

          shared_data->lock_data();
          shared_data->populate_channels(average);
          shared_data->unlock_data();

          commitres = snd_pcm_mmap_commit(device, offset, frames);
          if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
              if ((error = xrun_recovery(device, commitres >= 0 ? commitres : -EPIPE)) < 0) {
                  fprintf(stderr,"microphone: MMAP commit error: %s\n", snd_strerror(error));
                  exit(EXIT_FAILURE);
              }
              first = 1;
          }
          size -= frames;//needed in the condition of the while loop to check if period is filled
      }
    }

    return 0;
}

void help(void)
{
    printf(
      "Usage: microphone [OPTIONS]\n"
      "-h,--help      show this usage help\n"
      "-d,--device    device of capture\n"
      "-r,--rate      sample rate in hz\n"
      "-c,--channels  channels number\n"
      "-m,--method    I/O method\n"
      "-p,--period    period size in samples\n"
      "-b,--buffer    circular buffer size in samples\n"
      "\n");
    printf("The I/O methods are:\n");
    printf("(0) DIRECT_RW\n");
    printf("(1) DIRECT_MMAP\n");
}

//recovery callback in case of error
int xrun_recovery(snd_pcm_t *handle, int error)
{
    switch(error)
    {
        case -EPIPE:    // Buffer Over-run
            fprintf(stderr,"microphone: \"Buffer Overrun\" \n");
            if ((error = snd_pcm_prepare(handle)) < 0)
                fprintf(stderr,"microphone: Buffer overrrun cannot be recovered, snd_pcm_prepare fail: %s\n", snd_strerror(error));
            return 0;
            break;

        case -ESTRPIPE: //suspend event occurred
            fprintf(stderr,"microphone: Error ESTRPIPE\n");
			//EAGAIN means that the request cannot be processed immediately
            while ((error = snd_pcm_resume(handle)) == -EAGAIN)
                sleep(1);// wait until the suspend flag is clear

            if (error < 0) // error case
            {
                if ((error = snd_pcm_prepare(handle)) < 0)
                    fprintf(stderr,"microphone: Suspend cannot be recovered, snd_pcm_prepare fail: %s\n", snd_strerror(error));
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


void * hax_alsa_main(void * settings)
{

    int error,dir;
    //unsigned int sample_rate = 48000;//expected frame rate
    //unsigned int real_sample_rate;//real frame rate
    //int n_channels = 2;//expected number of channels
    //unsigned int real_n_channels;//real number of channels
    //char * device_name = "front";
    snd_pcm_t *device;//capture device
    snd_pcm_hw_params_t *hw_params;//hardware configuration structure
    hax_thread_config_t * hax_configs = (hax_thread_config_t *) settings;
    hax_general_settings_t * hax_user_settings = &hax_configs->user_settings;

    /*snd_pcm_sframes_t buffer_size = 2048;//expected buffer size in frames
    snd_pcm_uframes_t period_size = 8;//expected period size in frames
    snd_pcm_sframes_t real_buffer_size;//real buffer size in frames
    snd_pcm_uframes_t real_period_size;//real period size in frames*/
    //unsigned int buffer_time;//length of the circular buffer in us
    //unsigned int period_time;//length of one period in us
    //snd_pcm_format_t real_sample_format;
    //snd_pcm_access_t real_access;

    //array of the available I/O methods defined for capture
    io_method_t methods[] = {
        { METHOD_DIRECT_RW, SND_PCM_ACCESS_RW_INTERLEAVED, 0 },
        { METHOD_DIRECT_MMAP, SND_PCM_ACCESS_MMAP_INTERLEAVED, 0 },
    };

/************************************** opens the device *****************************************/

    if ((error = snd_pcm_open (&device, hax_user_settings->device_name, SND_PCM_STREAM_CAPTURE, methods[hax_user_settings->access].open_mode)) < 0) {
        fprintf (stderr, "microphone: Device cannot be opened  %s (%s)\n",
             hax_user_settings->device_name,
             snd_strerror (error));
        exit (1);
    }
    fprintf (stderr, "microphone: Device: %s open_mode = %d\n", hax_user_settings->device_name, methods[hax_user_settings->access].open_mode);

    //allocating the hardware configuration structure
    if ((error = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        fprintf (stderr, "microphone: Hardware configuration structure cannot be allocated (%s)\n",
             snd_strerror (error));
        exit (1);
    }

    //assigning the hw configuration structure to the device0
    if ((error = snd_pcm_hw_params_any (device, hw_params)) < 0) {
        fprintf (stderr, "microphone: Hardware configuration structure cannot be assigned to device (%s)\n",
             snd_strerror (error));
        exit (1);
    }

/*********************************** shows the audio capture method ****************************/

    switch(methods[hax_user_settings->access].method)
    {
        case METHOD_DIRECT_RW:
            fprintf (stderr, "microphone: capture method: METHOD_DIRECT_RW (m = 0) \n");
            break;
        case METHOD_DIRECT_MMAP:
            fprintf (stderr, "microphone: capture method: METHOD_DIRECT_MMAP (m = 1)\n");
            break;
    }

/*************************** configures access method ******************************************/
    //sets the configuration method
    fprintf (stderr, "microphone: hax_user_settings->access method: %d\n",methods[hax_user_settings->access].access);
    if ((error = snd_pcm_hw_params_set_access (device, hw_params, methods[hax_user_settings->access].access)) < 0) {
        fprintf (stderr, "microphone: Access method cannot be configured (%s)\n",
             snd_strerror (error));
        exit (1);
    }
    //checks the access method
    if ((error = snd_pcm_hw_params_get_access (hw_params, &hax_user_settings->access_type)) < 0) {
        fprintf (stderr, "microphone: Access method cannot be obtained (%s)\n",
             snd_strerror (error));
        exit (1);
    }
    //shows the access method
    switch(hax_user_settings->access_type)
    {
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
/****************************  configures the capture format *******************************/
    //SND_PCM_FORMAT_S16_LE => 16 bit signed little endian
    if ((error = snd_pcm_hw_params_set_format (device, hw_params, SND_PCM_FORMAT_S16_LE)) < 0)
    {
        fprintf (stderr, "microphone: Capture format cannot be configured (%s)\n",
             snd_strerror (error));
        exit (1);
    }
    //checks capture format
    if ((error = snd_pcm_hw_params_get_format (hw_params, &hax_user_settings->sample_format)) < 0)
    {
        fprintf (stderr, "microphone: Capture sample format cannot be obtained (%s)\n",
             snd_strerror (error));
        exit (1);
    }
    //just shows the capture format in a human readable way
    switch(hax_user_settings->sample_format)
    {
    case SND_PCM_FORMAT_S16_LE:
        fprintf (stderr, "microphone: PCM capture sample format: SND_PCM_FORMAT_S16_LE \n");
        break;
    default:
        fprintf (stderr, "microphone: PCM capture sample format = %d\n", hax_user_settings->sample_format);
    }
/*************************** configures the sample rate  ***************************/
	//sets the sample rate
    if ((error = snd_pcm_hw_params_set_rate (device, hw_params, hax_user_settings->sample_rate, 0)) < 0) {
        fprintf (stderr, "microphone: Sample rate cannot be configured (%s)\n",
             snd_strerror (error));
        exit (1);
    }
    //checks sample rate
    if ((error = snd_pcm_hw_params_get_rate (hw_params, &hax_user_settings->sample_rate, 0)) < 0) {
        fprintf (stderr, "microphone: Sample rate cannot be obtained (%s)\n",
             snd_strerror (error));
        exit (1);
    }
    fprintf (stderr, "microphone: Sample_rate_real = %d\n", hax_user_settings->sample_rate);

/**************************** configures the number of channels ********************************/
    //sets the number of channels
    if ((error = snd_pcm_hw_params_set_channels (device, hw_params, hax_user_settings->n_channels)) < 0) {
        fprintf (stderr, "microphone: Number of channels cannot be configured (%s)\n",
             snd_strerror (error));
        exit (1);
    }
    //checks the number of channels
    if ((error = snd_pcm_hw_params_get_channels (hw_params, &hax_user_settings->n_channels)) < 0) {
        fprintf (stderr, "microphone: Number of channels cannot be obtained (%s)\n",
             snd_strerror (error));
        exit (1);
    }
    fprintf (stderr, "microphone: real_n_channels = %d\n", hax_user_settings->n_channels);

/***************************** configures the buffer size *************************************/
    //sets the buffer size
    if ((error = snd_pcm_hw_params_set_buffer_size(device, hw_params, hax_user_settings->buffer_size)) < 0) {
        fprintf (stderr, "microphone: Buffer size cannot be configured (%s)\n",
             snd_strerror (error));
        exit (1);
    }
    //checks the value of the buffer size
    if ((error = snd_pcm_hw_params_get_buffer_size(hw_params, &hax_user_settings->buffer_size)) < 0) {
    fprintf (stderr, "microphone: Buffer size cannot be obtained (%s)\n",
             snd_strerror (error));
        exit (1);
    }
    fprintf (stderr, "microphone: Buffer size = %d [frames]\n", (int)hax_user_settings->buffer_size);
/***************************** configures period size *************************************/
    dir=0; //dir=0  =>  period size must be equal to period_size
    //sets the period size
    if ((error = snd_pcm_hw_params_set_period_size(device, hw_params, hax_user_settings->period_size, dir)) < 0) {
        fprintf (stderr, "microphone: Period size cannot be configured (%s)\n",
             snd_strerror (error));
        exit (1);
    }
    //checks the value of period size
    if ((error = snd_pcm_hw_params_get_period_size(hw_params, &hax_user_settings->period_size, &dir)) < 0) {
    fprintf (stderr, "microphone: Period size cannot be obtained (%s)\n",
             snd_strerror (error));
        exit (1);
    }
    fprintf (stderr, "microphone: Period size = %d [frames]\n", (int)hax_user_settings->period_size);
/************************* applies the hardware configuration  ******************************/

    if ((error = snd_pcm_hw_params (device, hw_params)) < 0) {
        fprintf (stderr, "microphone: Hardware parameters cannot be configured (%s)\n",
             snd_strerror (error));
        exit (1);
    }
    //frees the structure of hardware configuration
    snd_pcm_hw_params_free (hw_params);

/*********************************** filling capture_device_params *************************************/

    /*capture_device_params.access_type = real_access;         //real access method
    capture_device_params.buffer_size = real_buffer_size;      //real buffer size
    capture_device_params.period_size = real_period_size;      //real period size
    capture_device_params.buffer_time = buffer_time;           //real buffer time
    capture_device_params.period_time = period_time;           //real period time
    capture_device_params.sample_format = real_sample_format;  //real samples format
    capture_device_params.sample_rate = real_sample_rate;      //real sample rate
    capture_device_params.n_channels = n_channels;*/           //real number of channels

/********************** selects the appropriate access method for audio capture *******************/

	switch(methods[hax_user_settings->access].method)
	{
		case METHOD_DIRECT_RW:
			direct_rw(device, *hax_user_settings);
		break;

		case METHOD_DIRECT_MMAP:
			direct_mmap(device, *hax_user_settings, (hax_fftw_data *) hax_configs->data_zone, hax_configs->timer);
		break;

	}

    fprintf (stderr, "microphone: BYE BYE\n");
    //closes the device
    snd_pcm_close (device);
    pthread_exit ((void*) 0);
}
