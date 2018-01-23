/**
 * \file
 * \author Krzysztof Stasch
 * \date 2014-01-22
 */
#ifdef ALSA
#include <alsa/asoundlib.h>
#endif
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <utility>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <linux/spi/spidev.h>
#include <sched.h>
#include <semaphore.h>

#define WRITE_INT_TO_BUF(buf, off, i)	{ buf[off + 3] = i >> 24; buf[off + 2] = (i >> 16) & 0xFF; buf[off + 1] = (i >> 8) & 0xFF; buf[off] = i & 0xFF; }
#define BUF_TO_INT(buf, off)		((buf[off + 3] << 24) | (buf[off + 2] << 16) | (buf[off + 1] << 8) | buf[off])
#define BUF_TO_INT64(buf, off)      (((int64_t)BUF_TO_INT(buf, off + 4) << 32) | BUF_TO_INT(buf, off))
#define WRITE_INT64_TO_BUF(buf, off, i64)   { WRITE_INT_TO_BUF(buf, off, i64 & 0xFFFFFFFF); WRITE_INT_TO_BUF(buf, off + 4, i64 >> 32); }
#define TCP_RCVBUF	32768

#define FORMATS_NB	7
const char *formats[FORMATS_NB] = { "a.mp3", "a.aac", "a.wav", "a.mp4", "a.wma", "a.ogg", "a.flac" };

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif
extern "C" {
#include <libavutil/samplefmt.h>
#include <libavformat/avformat.h>
#include "libavutil/opt.h"
#include "libavutil/mathematics.h"
#include <libavresample/avresample.h>
#include <libswscale/swscale.h>
}

#define ALSA_PCM_NEW_HW_PARAMS_API

enum STATES { PLAYING = 0, PLAYER_PAUSE, PLAYER_STOP, PLAYER_SEEK_REQ, WAITING_FOR_CONF };
enum { INFO_DURATION = 0 };


enum PLAYER_MODE { LOCAL_FILE_MODE = 0, REMOTE_FILE_MODE = 1, REM_CTRL_LOCAL_FILE = 2 };
enum AUDIO_DEVICE { ALSA, STM };

volatile int state[2];
struct timeval last_net_access[2];

int64_t remote_file_size[2] {0, 0}, remote_f_pos[2] {0, 0};
int file_mode[2] {0, 0}, curr_song_pos[2] {0, 0}, total_song_time[2] {0, 0};
int fd1[2], fd2[2];

static uint64_t getNow(void)
{
  struct timeval now;
  gettimeofday(&now, NULL);
  return (uint64_t)now.tv_usec + 1000000ULL*(uint64_t)now.tv_sec;
}

int net_init(bool reconnect, int portno, int &net_fd1, int &net_fd2)
{
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  int a, reuse;
  if (!reconnect)
    {
      net_fd2 = socket(AF_INET, SOCK_STREAM, 0);
      if (net_fd2 < 0)
        {
          std::cout << "ERROR opening socket";
          return -1;
        }
      bzero((char *) &serv_addr, sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr = INADDR_ANY;
      serv_addr.sin_port = htons(portno);
      reuse = 1;
      if (setsockopt(net_fd2, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)))
        {
          close(net_fd2);
          std::cout << "ERROR on setsockoption, SO_REUSEADDR\n";
          return -2;
        }
      if (setsockopt(net_fd2, SOL_TCP, TCP_NODELAY, &reuse, sizeof(int)))
        {
          close(net_fd2);
          std::cout << "ERROR on setsockoption, SO_REUSEADDR\n";
          return -3;
        }
      reuse = TCP_RCVBUF;
      if (a = setsockopt(net_fd2, SOL_SOCKET, SO_RCVBUF, &reuse, sizeof(int)))
        {
          close(net_fd2);
          std::cout << "ERROR on setsockoption, SO_SNDBUF\n";
          return -4;
        }
      if (bind(net_fd2, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        {
          close(net_fd2);
          std::cout << "ERROR on binding";
          return -5;
        }
    }
  else
    {
      close(net_fd1);
    }
  std::cout << "Started listening on port " << portno << std::endl;
  listen(net_fd2, 5);
  clilen = sizeof(cli_addr);
  net_fd1 = accept(net_fd2, (struct sockaddr *) &cli_addr, &clilen);
  if (net_fd1 < 0)
    {
      std::cout << "ERROR on accept\n";
      return -6;
    }
  char tmp[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &(cli_addr.sin_addr), tmp, sizeof(tmp)) == nullptr) return 0;
  std::cout << "Accepted connection from " << tmp << ":" << ntohs(cli_addr.sin_port) << std::endl;
  return 0;
}

int net_get_file_size(int net_fd, int &file_type, int64_t &file_size, std::string& fname)
{
  unsigned char buffer[1024] = { 0x01, 0x00 };
  int n = write(net_fd, buffer, 2);
  if (n != 2) return -1;
  n = read(net_fd, buffer, sizeof(buffer));
//	std::cout << "Read: " << n << std::endl;
//	for(int i=0; i<n; ++i) std::cout << std::hex << (int)buffer[i] << ", ";
  int file_name_length = BUF_TO_INT(buffer, 6);
  if ((n != (file_name_length + 10)) || (buffer[0] != 0x81)) return -1;
  file_type = buffer[1];
  file_size = BUF_TO_INT(buffer, 2);
  if (file_size == 0)
    {
      std::string f_name((const char*)buffer + 10, file_name_length);
      fname = f_name;
//    	std::cout << "Local file: '" << fname << "'\n";
    }
  else
    {
      fname.clear();
    }
  return 0;
}

int net_finish_connection(int net_fd, int param)
{
  unsigned char buffer[32] = { 0x03, 0x00 };
  int n = write(net_fd, buffer, 2);
  if (n != 2) return -1;
  n = read(net_fd, buffer, 5);
  if ((n != 5) || (buffer[0] != 0x83)) return -1;
  return 0;
}

int net_get_status(int net_fd, int param, int thread_id)
{
  unsigned char buffer[16] = { 0x04 };
  WRITE_INT_TO_BUF(buffer, 1, param);
  WRITE_INT_TO_BUF(buffer, 5, curr_song_pos[thread_id]);
  int n = write(net_fd, buffer, 9);
  if (n != 9) return -1;
  n = read(net_fd, buffer, 5);
  if ((n != 5) || (buffer[0] != 0x84)) return -1;
  state[thread_id] = buffer[1];
  // setup timer that will assure that the communication with client will occur at least every 300ms
  struct timeval time_add, time_act;
  time_add.tv_sec = 0;
  time_add.tv_usec = 300000;
  gettimeofday(&time_act, NULL);
  timeradd(&time_act, &time_add, &last_net_access[thread_id]);

  return 0;
}

int net_get_seek_req_pos(int net_fd)
{
  unsigned char buffer[32] = { 0x05, 0x00 };
  int n = write(net_fd, buffer, 2);
  if (n != 2) return -1;
  n = read(net_fd, buffer, 10);
  if ((n != 10) || (buffer[0] != 0x85)) return -1;
  return BUF_TO_INT(buffer, 2);
}

int net_set_info(int net_fd, int info_type, int info)
{
  unsigned char buffer[32] = { 0x06, 0x00 };
  WRITE_INT_TO_BUF(buffer, 2, info_type);
  WRITE_INT_TO_BUF(buffer, 6, info);
  int n = write(net_fd, buffer, 10);
  if (n != 10) return -1;
  n = read(net_fd, buffer, 5);
  if ((n != 5) || (buffer[0] != 0x86)) return -1;
  return BUF_TO_INT(buffer, 2);
}

int net_read_file(int net_fd, int64_t adr, int bytes, unsigned char *dest, int thread_id)
{
  unsigned char buffer[bytes + 24];
  struct pollfd my_fds;
  int off = 0, c, a = 14, n;
  my_fds.fd = net_fd;
  my_fds.events = POLLIN;
  my_fds.revents = 0;
  buffer[0] = 0x02;
  buffer[1] = 0;
  WRITE_INT_TO_BUF(buffer, 2, bytes);
  WRITE_INT_TO_BUF(buffer, 6, adr);
  WRITE_INT_TO_BUF(buffer, 14, curr_song_pos[thread_id]);
  n = write(net_fd, buffer, 18);
  if (n != 18) return -1;
  bytes += 14;
  while (bytes > 0)
    {
      my_fds.revents = 0;
      while (poll(&my_fds, 1, 500) == 0) usleep(3000);
      n = read(net_fd, buffer, bytes);
      if (n < 0) return -1;
      if (a)
        {
          if (buffer[0] != 0x82) return -1;
          state[thread_id] = buffer[1];
          c = BUF_TO_INT(buffer, 2);
          bytes = c + 14;
        }
      memcpy(dest + off, buffer + a, n - a);
      bytes -= n;
      off += n - a;
      a = 0;
    }
  // setup timer that will assure that the communication with client will occur at least every 300ms
  struct timeval time_add, time_act;
  time_add.tv_sec = 0;
  time_add.tv_usec = 300000;
  gettimeofday(&time_act, NULL);
  timeradd(&time_act, &time_add, &last_net_access[thread_id]);

  return c;
}

void net_close(int fd1, int fd2)
{
  close(fd1);
  close(fd2);
}

int alsa_restart {0};
int audio_output;

#define AUDIO_BUFFER_SIZE  2*1024*1024
#define STM_AUDIO_BUFFER_SIZE   1024            // nb of words (2 bytes)
#define STM_AUDIO_BUFFER_SIZE_BYTES   (2 * STM_AUDIO_BUFFER_SIZE)   // nb of bytes
#define STM_AUDIO_HEADER_SIZE   16

int audio_buf[2][AUDIO_BUFFER_SIZE];
volatile int audio_buf_rd_ptr[2] {0, 0}, audio_buf_wr_ptr[2] {0, 0};
volatile bool audio_buf_filled[2] {false, false};
volatile bool finish_thread {false};
pthread_mutex_t mutex[2] { PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER };
static sem_t refillSignal[2];
static int useSem = 0;
volatile AVSampleFormat _av_sample_format[2];
int _av_sample_rate[2] {0, 0}, _av_channels_nb[2] {0, 0};

#ifdef ALSA

snd_pcm_t *pcm_handle;
snd_pcm_uframes_t frames;
snd_pcm_hw_params_t *hw_params;
snd_pcm_uframes_t alsa_buffer_size {100 * 1024};
snd_pcm_uframes_t alsa_period_size {1 * 1024};


int alsa_play_sound(unsigned char *buffer, int size)
{
  int rc;
  static int error = 0;
  static int started = false;
  rc = snd_pcm_writei(pcm_handle, buffer, size);
  if (rc == -EPIPE)
    {
      std::cout <<  "underrun occurred\n";
      snd_pcm_prepare(pcm_handle);
    }
  else if (rc < 0)
    {
      if (error != 1) std::cout << "error from writei: " << snd_strerror(rc) << std::endl;
      error = 1;
    }
  else if (rc != size)
    {
      if (error != 2) std::cout << "short write, write frames" << rc << std::endl;
      error = 2;
    }
  return 0;
}

int alsa_play_sound_init(unsigned int sample_rate, int channels, int sample_format)
{
  int dir, rc;
  rc = snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
  if (rc < 0)
    {
      std::cout << "unable to open pcm device: " << snd_strerror(rc) << std::endl;
      return -1;
    }
  snd_pcm_hw_params_alloca(&hw_params);
  snd_pcm_hw_params_any(pcm_handle, hw_params);
  snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(pcm_handle, hw_params, sample_format ? SND_PCM_FORMAT_S32_LE : SND_PCM_FORMAT_S16_LE);
  snd_pcm_hw_params_set_channels(pcm_handle, hw_params, channels);
  dir = 0;
  snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &sample_rate, &dir);

  snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hw_params, &alsa_buffer_size);
  snd_pcm_hw_params_set_period_size_near(pcm_handle, hw_params, &alsa_period_size, NULL);

  rc = snd_pcm_hw_params(pcm_handle, hw_params);
  if (rc < 0)
    {
      std::cout << "unable to set hw parameters: " << snd_strerror(rc) << std::endl;
      return -1;
    }

  snd_pcm_sw_params_t *sw_params;
  snd_pcm_sw_params_malloc (&sw_params);
  snd_pcm_sw_params_current (pcm_handle, sw_params);
  snd_pcm_sw_params_set_start_threshold(pcm_handle, sw_params, alsa_buffer_size / 4);
  snd_pcm_sw_params(pcm_handle, sw_params);
  snd_pcm_sw_params_free (sw_params);

  snd_pcm_prepare(pcm_handle);
  snd_pcm_start(pcm_handle);
  return 0;
}

int alsa_play_sound_finish(bool drop)
{
  if (drop) snd_pcm_drop(pcm_handle);
  else snd_pcm_drain(pcm_handle);
  snd_pcm_close(pcm_handle);
  return 0;
}

void* alsa_play_thread(void *data)
{
  int rc;
  int *params = (int*)data;
  if (alsa_play_sound_init(params[0], params[1], 0)) return NULL;
  do
    {
      int pr = audio_buf_rd_ptr.load(std::memory_order_relaxed);
      int pw = audio_buf_wr_ptr.load(std::memory_order_relaxed);
      bool buf_filled = audio_buf_filled.load(std::memory_order_relaxed);
      int r1 {0}, r2 {0};
      if (pw > pr)
        {
          r1 = pw - pr;
          r2 = 0;
        }
      else if (buf_filled)
        {
          r1 = AUDIO_BUFFER_SIZE - pr;
          r2 = pw;
        }
      if (r1 + r2 > alsa_period_size)
        {
          if (r1 >= alsa_period_size)
            {
              r1 = alsa_period_size;
              r2 = 0;
            }
          if (r1 < alsa_period_size) r2 = alsa_period_size - r1;
          rc = snd_pcm_writei(pcm_handle, &audio_buf[pr], r1 / 2);
          pr += r1;
          if (pr >= AUDIO_BUFFER_SIZE) pr = 0;
          if (rc <= 0) snd_pcm_prepare(pcm_handle);
          if (r2 > 0)
            {
              rc = snd_pcm_writei(pcm_handle, audio_buf, r2 / 2);
              pr = r2;
              if (rc <= 0) snd_pcm_prepare(pcm_handle);
            }
          pthread_mutex_lock(&mutex);      // lock during updating the pointers in the circular buffer in case that in the meantime spi_audio_buf_wr_ptr changed
          pw = audio_buf_wr_ptr.load(std::memory_order_relaxed);
          if (pr == pw) audio_buf_filled.store(false, std::memory_order_relaxed);
          audio_buf_rd_ptr.store(pr, std::memory_order_relaxed);
          pthread_mutex_unlock(&mutex);
        }
      if (state.load(std::memory_order_relaxed) == PLAYER_PAUSE)
        {
          // mute during pause
          snd_pcm_pause(pcm_handle, true);
          while (state.load(std::memory_order_relaxed) == PLAYER_PAUSE) usleep(300000);
          snd_pcm_pause(pcm_handle, false);
        }
    }
  while (!finish_thread.load(std::memory_order_relaxed));
  std::cout << "Finished alsa thread " << std::endl;
  alsa_play_sound_finish(state.load(std::memory_order_relaxed) != PLAYING);
  return NULL;
}

#endif

int spi_player_init_params(const std::string& device, int speed)
{
  int ret = 0, fd, mode;
  fd = open(device.c_str(), O_RDWR | O_NONBLOCK);
  if (fd < 0)	return -1;
  mode = SPI_MODE_0;
  ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
  if (ret == -1) return -1;
  ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
  if (ret == -1) return -1;

  mode = 0;
  ret = ioctl(fd, SPI_IOC_RD_LSB_FIRST, &mode);
  if (ret == -1) return -1;
  mode = 16;
  ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &mode);
  if (ret == -1) return -1;
  std::cout << "SPI device initialized\n";
  return fd;
}

void invalidate_audio_buffer(int thread_id)
{
  pthread_mutex_lock(&mutex[thread_id]);      /// lock during updating the pointers to the circular buffer
  audio_buf_filled[thread_id] = false;
  audio_buf_wr_ptr[thread_id] = 0;
  audio_buf_rd_ptr[thread_id] = 0;
  pthread_mutex_unlock(&mutex[thread_id]);
}

int fill_audio_buffer(unsigned char *src, int count, int pts_sec, int thread_id)
{
  int res, st, sample_size { _av_sample_format[thread_id] == AV_SAMPLE_FMT_S32 ? 4 : 2 };
  int pw = audio_buf_wr_ptr[thread_id];
  bool buf_filled = audio_buf_filled[thread_id];
  int pr = audio_buf_rd_ptr[thread_id];
  int r1, r2;
  if ((pw > pr) || ((pw == pr) && !buf_filled))
    {
      r1 = AUDIO_BUFFER_SIZE - pw;
      r2 = pr;
    }
  else
    {
      r1 = pr - pw;
      r2 = 0;
    }
  st = state[thread_id];
  if (st == PLAYING) curr_song_pos[thread_id] = pts_sec - (AUDIO_BUFFER_SIZE - r1 - r2) / (sample_size * _av_sample_rate[thread_id]);
  if ((r1 + r2) > count)
    {
      if (r1 >= count)
        {
          r1 = count;
          r2 = 0;
        }
      if (r1 < count) r2 = count - r1;
      memcpy(((unsigned char*)audio_buf[thread_id]) + sample_size * pw, src, sample_size * r1);
      pw += r1;
      if (pw >= AUDIO_BUFFER_SIZE) pw = 0;
      if (r2 > 0)
        {
          memcpy(((unsigned char*)audio_buf[thread_id]), src + sample_size * r1, sample_size * r2);
          pw = r2;
        }
      pthread_mutex_lock(&mutex[thread_id]);      /// lock during updating the pointers to the circular buffer
      audio_buf_filled[thread_id] = true;
      audio_buf_wr_ptr[thread_id] = pw;
      pthread_mutex_unlock(&mutex[thread_id]);
      res = 0;
    }
  else
    {
      res = -1;
    }

  struct timeval act_time;
  gettimeofday(&act_time, NULL);
  if ((timercmp(&last_net_access[thread_id], &act_time, <)) && (file_mode[thread_id] != LOCAL_FILE_MODE))
    {
      if (net_get_status(fd1[thread_id], 0, thread_id))
        return -11;
      else
        {
          st = state[thread_id];
        }
    }
  if (st == PLAYER_SEEK_REQ) res = -2;
  else if ((st != PLAYING) && (st != PLAYER_PAUSE)) res = -10;

  return res;
}

bool stop_th {false};

void spi_play(int fd_spi, int fd_cs, int buf[], int count)
{
  int v;
  v = '0';
  write(fd_cs, &v, 1);
  write(fd_spi, buf, count);
  v = '1';
  write(fd_cs, &v, 1);
}

void* spi_player_thread(void *ptr)
{
  unsigned char head[] {0x11, 0x22, 0x33, 40, 44, 1}, tail[] { 1, 2, 3, 4, 5, 6, 7, 8};
  int buf[2][STM_AUDIO_HEADER_SIZE + STM_AUDIO_BUFFER_SIZE + STM_AUDIO_HEADER_SIZE];
  int fd_spi, fd_cs, fd_flow_ctrl0, fd_flow_ctrl1, threshold[2] { 10 * STM_AUDIO_BUFFER_SIZE, 10 * STM_AUDIO_BUFFER_SIZE };
  int *params = (int*)ptr;
  head[4] = params[0] / 1000;		// set sample rate
  memcpy(buf[0], head, sizeof(head));
  memcpy(buf[1], head, sizeof(head));
  memcpy(&buf[1][16 + STM_AUDIO_BUFFER_SIZE], tail, sizeof(tail));
  fd_spi = spi_player_init_params(std::string("spi"), 16000000);
  if (fd_spi < 0)
    {
      std::cout << "Error cannot open SPI device\n";
      return NULL;
    }
  fd_cs = open("gpioCS", O_RDWR);	// Variscite pin
  if (fd_cs < 0)
    {
      std::cout << "Cannot open gpioCS";
      return NULL;
    }
  fd_flow_ctrl0 = open("gpioReq1", O_RDWR);	// Variscite pin
  if (fd_flow_ctrl0 < 0)
    {
      std::cout << "Cannot open gpioReq1";
      return NULL;
    }
  fd_flow_ctrl1 = open("gpioReq2", O_RDWR);			/// 2nd channel
  if (fd_flow_ctrl1 < 0)
    {
      std::cout << "Cannot open gpioReq2";
      return NULL;
    }
  // Increase priority for this thread
  pthread_t this_thread = pthread_self();
  struct sched_param sched_params;
  int a;
  pthread_getschedparam(this_thread, &a, &sched_params);
  sched_params.sched_priority = sched_get_priority_max(SCHED_FIFO);
  if (pthread_setschedparam(this_thread, SCHED_FIFO, &sched_params) != 0) std::cout << "Unsuccessful in setting thread priority\n";

  int thread_id, ch_req[2] {0, 0};
  bool buf_ready[2] {false, false};
  do
    {
      thread_id = -1;
      lseek(fd_flow_ctrl0, 0, SEEK_SET);
      read(fd_flow_ctrl0, &ch_req[0], 1);
      lseek(fd_flow_ctrl1, 0, SEEK_SET);			/// 2nd channel
      read(fd_flow_ctrl1, &ch_req[1], 1);			/// 2nd channel
      if (ch_req[0] == '0')
        thread_id = 0;
      else if (ch_req[1] == '0')					/// 2nd channel
        thread_id = 1;								/// 2nd channel
      if (thread_id < 0)
        {
          if (!buf_ready[0])
            thread_id = 0;
          else if (!buf_ready[1])						/// 2nd channel
            thread_id = 1;							/// 2nd channel
          else
            {
              usleep(300);
              continue;
            }
        }

      const int sample_size { _av_sample_format[thread_id] == AV_SAMPLE_FMT_S32 ? 4 : 2 };
      if (!buf_ready[thread_id])
        {
          int pr = audio_buf_rd_ptr[thread_id] * sample_size;
          int pw = audio_buf_wr_ptr[thread_id] * sample_size;
          bool buf_filled = audio_buf_filled[thread_id];
          int r1 {0}, r2 {0};
          if (pw > pr)
            {
              r1 = pw - pr;
              r2 = 0;
            }
          else if (buf_filled)
            {
              r1 = AUDIO_BUFFER_SIZE * sample_size - pr;
              r2 = pw;
            }
          if (((r1 + r2) > threshold[thread_id]) && (state[thread_id] == PLAYING))
            {
              threshold[thread_id] = STM_AUDIO_BUFFER_SIZE_BYTES;
              if (r1 >= STM_AUDIO_BUFFER_SIZE_BYTES)
                {
                  r1 = STM_AUDIO_BUFFER_SIZE_BYTES;
                  r2 = 0;
                }
              if (r1 < STM_AUDIO_BUFFER_SIZE_BYTES) r2 = STM_AUDIO_BUFFER_SIZE_BYTES - r1;
              memcpy(((unsigned char*)buf[thread_id]) + 32, ((unsigned char*)audio_buf[thread_id]) + pr, r1);
              pr += r1;
              if (pr >= (AUDIO_BUFFER_SIZE * sample_size)) pr = 0;
              if (r2 > 0)
                {
                  memcpy(((unsigned char*)buf[thread_id]) + 32 + r1, ((unsigned char*)audio_buf[thread_id]), r2);
                  pr = r2;
                }
              pthread_mutex_lock(&mutex[thread_id]);      // lock during updating the pointers in the circular buffer in case that in the meantime spi_audio_buf_wr_ptr was change
              pw = audio_buf_wr_ptr[thread_id] * sample_size;
              if (pr == pw) audio_buf_filled[thread_id] = false;
              audio_buf_rd_ptr[thread_id] = pr / sample_size;
              pthread_mutex_unlock(&mutex[thread_id]);
            }
          else
            {
              memset(((unsigned char*)buf[thread_id]) + 32, 0, STM_AUDIO_BUFFER_SIZE_BYTES);
              threshold[thread_id] = 10 * STM_AUDIO_BUFFER_SIZE_BYTES;
            }
          if(useSem)
            {
              sem_post(&refillSignal[thread_id]);
            }
          ((unsigned char*)buf[thread_id])[4] = (unsigned int)_av_sample_rate[thread_id] / 1000;
          ((unsigned char*)buf[thread_id])[5] = _av_sample_format[thread_id] == AV_SAMPLE_FMT_S32 ? 3 : 1;
          memcpy(((unsigned char*)buf[0]) + 32 + STM_AUDIO_BUFFER_SIZE_BYTES, tail, sizeof(tail));
          buf_ready[thread_id] = true;
        }
      if (ch_req[thread_id] == '0')
        {
          spi_play(fd_spi, fd_cs, buf[thread_id], 32 + 32 + STM_AUDIO_BUFFER_SIZE_BYTES);
          buf_ready[thread_id] = false;
        }
      stop_th = finish_thread;
    }
  while (!stop_th);
  close(fd_spi);
  close(fd_cs);
  close(fd_flow_ctrl0);
  close(fd_flow_ctrl1);
}

int do_seek(AVFormatContext *container, int stream_id, double time_base, int thread_id)
{
  state[thread_id] = PLAYER_PAUSE;
  invalidate_audio_buffer(thread_id);
  int pos = net_get_seek_req_pos(fd1[thread_id]);
  std::cout << "Seek req: " << pos << std::endl;
//    avio_seek(container->pb, 15000000, SEEK_SET);
  av_seek_frame(container, stream_id, pos / time_base, AVSEEK_FLAG_FRAME);
//    snd_pcm_drop(pcm_handle);
//    snd_pcm_prepare(pcm_handle);
//    snd_pcm_start(pcm_handle);
  state[thread_id] = PLAYING;
  return 0;
}

void flush_audio_buffer(int thread_id)
{
  int r1, r2;
  do
    {
      int pr = audio_buf_rd_ptr[thread_id];
      int pw = audio_buf_wr_ptr[thread_id];
      bool buf_filled = audio_buf_filled[thread_id];
      if (!buf_filled || (state[thread_id] != PLAYING)) return;
      if (pw > pr)
        {
          r1 = pw - pr;
          r2 = 0;
        }
      else
        {
          r1 = AUDIO_BUFFER_SIZE - pr;
          r2 = pw;
        }
      usleep(300000);
      curr_song_pos[thread_id] = total_song_time[thread_id] - (r1 + r2) / (2 * _av_sample_rate[thread_id]);
      if (file_mode[thread_id] != LOCAL_FILE_MODE)
        {
          if (net_get_status(fd1[thread_id], 0, thread_id))
            return;
          else if (state[thread_id] != PLAYING) return;
        }
    }
  while ((r1 + r2) > STM_AUDIO_BUFFER_SIZE);
}

int remote_readFunction(void* opaque, uint8_t* buf, int buf_size)
{
  int thread_id { *(int*)opaque }, a;
//    std::cout << "ReadF " << buf_size << std::endl;
  a = net_read_file(fd1[thread_id], remote_f_pos[thread_id], buf_size, buf, thread_id);
  if (a < 0)
    std::cout << "Error while reading file, thread: " << thread_id << std::endl;
  else
    remote_f_pos[thread_id] += a;
  return a;
}

int64_t remote_seekFunction(void* opaque, int64_t offset, int whence)
{
  int thread_id { *(int*)opaque };
//    std::cout << "SeekF opaque: "  << *(int*)opaque << std::endl;
  if (whence == AVSEEK_SIZE) return remote_file_size[thread_id];
  else if (whence == SEEK_SET) remote_f_pos[thread_id] = offset;
  else if (whence == SEEK_CUR) remote_f_pos[thread_id] += offset;
  else if (whence == SEEK_END)
    {
      remote_f_pos[thread_id] = remote_file_size[thread_id];
      remote_f_pos[thread_id] += offset;
    }
  if (remote_f_pos[thread_id] > remote_file_size[thread_id])
    {
      remote_f_pos[thread_id] = remote_file_size[thread_id];
      return -EINVAL;
    }
  else if (remote_f_pos[thread_id] < 0)
    {
      remote_f_pos[thread_id] = 0;
      return -EINVAL;
    }
  return remote_f_pos[thread_id];
}

int send_dgram_to_controller(std::vector<unsigned char>& dgram, std::string&& ip_addr, int port)
{
  int dgram_fd;
  struct sockaddr_in saddr;
  int ret;
  dgram_fd = socket(AF_INET, SOCK_DGRAM, 0);
  bzero(&saddr, sizeof(saddr));
  saddr.sin_family = AF_INET;
  inet_pton(AF_INET, ip_addr.c_str(), &saddr.sin_addr);
  saddr.sin_port = htons(port);
  int adr_len = sizeof(saddr);
  std::vector<unsigned char> hd { 0xF0, 0x02, dgram.size() & 0xFF, (dgram.size() >> 8) & 0xFF };
  hd.insert(hd.end(), dgram.begin(), dgram.end());
  if (sendto(dgram_fd, hd.data(), hd.size(), 0, (struct sockaddr *)&saddr, adr_len) < 0) ret = -1;
  ret = 0;
  close(dgram_fd);
  return ret;
}

int decode_audio(const char* input_filename, int thread_id)
{
  AVIOContext *avioContext;
  unsigned char *ioBuffer;
  int a;
  // initialize libav
  av_register_all();
  AVFormatContext *container = avformat_alloc_context();
  if (file_mode[thread_id] == REMOTE_FILE_MODE)
    {
      ioBuffer = (unsigned char *)av_malloc(65536 + FF_INPUT_BUFFER_PADDING_SIZE);
      avioContext = avio_alloc_context(ioBuffer, 65536, 0, &thread_id, &remote_readFunction, NULL, &remote_seekFunction);
      container->pb = avioContext;
      container->flags = AVFMT_FLAG_CUSTOM_IO;
      remote_f_pos[thread_id] = 0;
    }
  // initialize pointers in circular buffer for spi data
  audio_buf_rd_ptr[thread_id] = 0;
  audio_buf_wr_ptr[thread_id] = 0;
  audio_buf_filled[thread_id] = false;
  state[thread_id] = PLAYING;

  if (avformat_open_input(&container, input_filename, NULL, NULL) < 0)
    {
      std::cout << "Cannot open file\n";
      return -1;
    }
  if (avformat_find_stream_info(container, NULL) < 0)
    {
      std::cout << "Cannot find file info\n";
    }
  av_dump_format(container, 0, input_filename, false);
  int stream_id=-1;
  int i;
  for(i=0; i<container->nb_streams; i++)
    {
      if (container->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
          stream_id=i;
          break;
        }
    }
  if (stream_id == -1)
    {
      std::cout << "Cannot not find audio stream in the file\n";
    }
//    AVDictionary *metadata = container->metadata;
  AVCodecContext *ctx = container->streams[stream_id]->codec;
  AVCodec *codec = avcodec_find_decoder(ctx->codec_id);
  if (codec == NULL)
    {
      std::cout << "Cannot find codec!\n";
      return -1;
    }
  if (avcodec_open2(ctx, codec, NULL) < 0)
    {
      std::cout << "Cannot open codec\n";
      return -1;
    }

  switch(ctx->sample_fmt)
    {
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P:
      _av_sample_format[thread_id] = AV_SAMPLE_FMT_S32;
      break;
    default:
      _av_sample_format[thread_id] = AV_SAMPLE_FMT_S16;
    }
  _av_sample_rate[thread_id] = ctx->sample_rate;	// > 96000 ? 96000 : ctx->sample_rate;

  /// send 0x92 cmd to STM through bb_controller - not used
  /*    unsigned char fs = 0xFF;
      switch (_av_sample_rate[thread_id]) {
      	case 44100 : fs = 0x01; break;
      	case 48000 : fs = 0x02; break;
      	case 88200 : fs = 0x03; break;
      	case 96000 : fs = 0x04; break;
      	case 176400 : fs = 0x05; break;
      	case 192000 : fs = 0x06; break;
      }
      std::vector<unsigned char> _0x92_cmd { 0x92, 0x00, fs, thread_id, 0x00, 0x00 };
      if (send_dgram_to_controller(_0x92_cmd, "127.0.0.1", 45554) < 0) std::cout << "Cannot send 0x92 cmd to controller\n";
  */
  _av_channels_nb[thread_id] = 2;
  double time_base = av_q2d(container->streams[stream_id]->time_base);
  total_song_time[thread_id] = container->streams[stream_id]->duration * time_base;
  if (file_mode[thread_id] != LOCAL_FILE_MODE)
    {
      if (net_set_info(fd1[thread_id], INFO_DURATION, total_song_time[thread_id]) < 0)
        {
          std::cout << "Cannot send info to client\n";
          return -1;
        }
    }
  /*    pthread_t thread1;
      int th_data[3] {ctx->sample_rate, ctx->channels, ctx->sample_fmt};
  #ifdef ALSA
      if (audio_output == ALSA)
          pthread_create (&thread1, NULL, alsa_play_thread, (void*)th_data);
      else
  #endif
          pthread_create (&thread1, NULL, spi_player_thread, (void*)th_data);
  */

  AVAudioResampleContext *avr = avresample_alloc_context();
  av_opt_set_int(avr, "in_channel_layout", ctx->channels == 2 ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO, 0);
  av_opt_set_int(avr, "out_channel_layout", AV_CH_LAYOUT_STEREO,  0);
  av_opt_set_int(avr, "in_sample_rate", ctx->sample_rate, 0);
  av_opt_set_int(avr, "out_sample_rate", _av_sample_rate[thread_id], 0);
  av_opt_set_int(avr, "in_sample_fmt", ctx->sample_fmt, 0);
  av_opt_set_int(avr, "out_sample_fmt", _av_sample_format[thread_id],  0);
  if (avresample_open(avr)) std::cout << "AVresample open error\n";

  AVPacket packet;
  bool planar_format;
  if (planar_format = av_sample_fmt_is_planar(ctx->sample_fmt)) std::cout << "Sample format is planar\n";
  std::cout << "Sample format: " << ctx->sample_fmt << " " << ctx->channel_layout << " " << ctx->channels << std::endl;
  av_init_packet(&packet);
  AVFrame *frame = avcodec_alloc_frame();
  int buffer_size = 192000 + FF_INPUT_BUFFER_PADDING_SIZE;
  unsigned char buf0[buffer_size];
  packet.data = buf0;
  packet.size = buffer_size;
  int len, size, av_read_res, frameFinished = 0;
  do
    {
      av_read_res = av_read_frame(container, &packet);
      if (av_read_res >= 0)
        {
          if (packet.stream_index == stream_id)
            {
              size = packet.size;
              while (size > 0)
                {
                  len = avcodec_decode_audio4(ctx, frame, &frameFinished, &packet);
                  if (frameFinished)
                    {
                      uint8_t *dst[2];
                      int out_linesize;
                      int out_samples = avresample_available(avr) + av_rescale_rnd(avresample_get_delay(avr) + frame->nb_samples, _av_sample_rate[thread_id], ctx->sample_rate, AV_ROUND_UP);
                      av_samples_alloc(dst, &out_linesize, _av_channels_nb[thread_id], out_samples, _av_sample_format[thread_id], 0);
                      out_samples = avresample_convert(avr, dst, out_linesize, out_samples, frame->extended_data, frame->linesize[0], frame->nb_samples);
                      while (a = fill_audio_buffer(dst[0], _av_channels_nb[thread_id] * out_samples, frame->pkt_dts * time_base, thread_id))
                        {
                          if (a > -10)
                            {
                              if (a == -2)
                                {
                                  do_seek(container, stream_id, time_base, thread_id);
                                  size = 0;
                                  break;
                                }
                              else
                                {
                                  if(useSem)
                                    {
                                      sem_wait(&refillSignal[thread_id]);
                                    }
                                  else
                                    {
                                      usleep(10);
                                    }
                                }
                            }
                          else
                            {
                              break;
                            }
                        }
                      av_freep(&dst);
                    }
                  size -= len;
                }
            }
          av_free_packet(&packet);
        }
      else
        {
          flush_audio_buffer(thread_id);
          if (state[thread_id] == PLAYER_SEEK_REQ)
            {
              do_seek(container, stream_id, time_base, thread_id);
              av_read_res = 0;
            }
        }
      if (state[thread_id] == PLAYER_STOP) break;
    }
  while (av_read_res >= 0);
  avresample_free(&avr);
  av_freep(&frame);
  avcodec_close(ctx);
  avformat_close_input(&container);
  if (file_mode[thread_id] == REMOTE_FILE_MODE)
    {
      av_freep(&avioContext);
    }
  return 0;
}

void* player_thread_2(void* arg)
{
  int a {0};
  std::string loc_file;

  // Increase priority for this thread
  pthread_t this_thread = pthread_self();
  struct sched_param sched_params;
  pthread_getschedparam(this_thread, &a, &sched_params);
  sched_params.sched_priority = sched_get_priority_min(a) + (4 * (sched_get_priority_max(a) - sched_get_priority_min(a))) / 5;
  if (pthread_setschedparam(this_thread, a, &sched_params) != 0) std::cout << "Unsuccessful in setting thread priority\n";

  bool reconnect {false};
  do
    {
      file_mode[1] = REMOTE_FILE_MODE;
      if (net_init(reconnect, 45556, fd1[1], fd2[1]))
        {
          std::cout << "Open connection failed\n";
          return NULL;
        }
      if (net_get_file_size(fd1[1], a, remote_file_size[1], loc_file))
        {
          std::cout << "Read file size failed\n";
          return NULL;
        }
      if (loc_file.size() < 1)
        {
          file_mode[0] = REMOTE_FILE_MODE;
          decode_audio("", 1);
        }
      else
        {
          file_mode[1] = REM_CTRL_LOCAL_FILE;
          decode_audio(loc_file.c_str(), 1);
        }
      net_finish_connection(fd1[1], 0);
      reconnect = true;
    }
  while (true);
  net_close(fd1[1], fd2[1]);
  return NULL;
}

int main(int argc, char **argv)
{
  int a = '0';
  setpriority(PRIO_PROCESS, 0, -20);
  // Increase priority for this thread
  pthread_t this_thread = pthread_self();
  struct sched_param sched_params;

  sem_init(&refillSignal[0], 0, 1);
  sem_init(&refillSignal[1], 0, 1);

  pthread_getschedparam(this_thread, &a, &sched_params);
  sched_params.sched_priority = sched_get_priority_min(a) + (4 * (sched_get_priority_max(a) - sched_get_priority_min(a))) / 5;
  if (pthread_setschedparam(this_thread, a, &sched_params) != 0) std::cout << "Unsuccessful in setting thread priority\n";
  if (argc > 1) a = argv[1][0];
  if (argc > 3) if(memcmp(argv[3], "useSem", 6) == 0) useSem = 1;

  switch(a)
    {
    case '3':
      if (argc < 3)
        std::cout << "Not enough parameters\n";
      else
        {
          file_mode[0] = LOCAL_FILE_MODE;
          audio_output = STM;
          decode_audio(argv[2], 0);
        }
      break;
    case '4':


      pthread_t thread1;
      int th_data[3] {96000, 2, AV_SAMPLE_FMT_S16P};
      pthread_create (&thread1, NULL, spi_player_thread, (void*)th_data);

      /// start 2nd player thread
      pthread_t player_th2;
      int pl_th_data[3] {0, 0, 0};
      pthread_create (&player_th2, NULL, player_thread_2, (void*)pl_th_data);

      bool reconnect {false};
      std::string loc_file;
      do
        {
          file_mode[0] = REMOTE_FILE_MODE;
          if (net_init(reconnect, 45555, fd1[0], fd2[0]))
            {
              std::cout << "Open connection failed\n";
              return -1;
            }
          if (net_get_file_size(fd1[0], a, remote_file_size[0], loc_file))
            {
              std::cout << "Read file size failed\n";
              return -1;
            }
          audio_output = STM;
          if (loc_file.size() < 1)
            {
              file_mode[0] = REMOTE_FILE_MODE;
              decode_audio("", 0);
            }
          else
            {
              file_mode[0] = REM_CTRL_LOCAL_FILE;
              decode_audio(loc_file.c_str(), 0);
            }
          net_finish_connection(fd1[0], 0);
          reconnect = true;
        }
      while (true);
      net_close(fd1[0], fd2[0]);
      finish_thread = true;
      pthread_join(thread1, NULL);
      break;
    }
  return 0;
}
