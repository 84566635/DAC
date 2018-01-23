#include <iostream>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <termios.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <stdlib.h>
#include <tuple>
#include <deque>
#include <list>
#include <ctime>
#include <dirent.h>
#include <taglib/tag.h>
#include <taglib/fileref.h>

#define SER_BUF_SIZE    1024
#define UDP_BUF_SIZE    2048

//#define HAVE_LEDS

unsigned char rec_buffer_A[SER_BUF_SIZE], send_buffer_A[SER_BUF_SIZE], rec_buffer_B[SER_BUF_SIZE], send_buffer_B[SER_BUF_SIZE];
unsigned char rec_buffer_udp[UDP_BUF_SIZE], send_buffer_udp[UDP_BUF_SIZE];

int ptr_A {0}, ptr_B {0};

int fd_serial_A, fd_serial_B, fd_udp, fd_watchdog;

#ifdef HAVE_LEDS
unsigned char leds_action[16];
int leds_action_count {0}, leds_cmd {0}, fd_leds[4];
#endif

struct pollfd poll_serial_A, poll_serial_B, poll_udp;
struct timeval timer_serial_A, timer_serial_B, leds_timer;
bool timer_A_active, timer_B_active, processing_command {false}, host_addr_set {false}, connected_player_addr_set {false};
unsigned char processed_command {0};

struct sockaddr_in serv_addr, remaddr, connected_player_addr;
socklen_t adr_len;

#ifdef HAVE_LEDS
void set_leds_cmd(int serial_cmd);
#endif

enum { PORT_A, PORT_B };

/// type of Message
enum class MessageType_
{
  /// command with timeout
  CONF_TIMEOUT,
  /// negative confirmation of associated command
  NEG_CONFIRMATION,
  /// ?
  PER_GEN
};

/// information about a Message
struct MessageInformation_
{
  /// command (first byte)
  uint8_t command;
  /// length, bytes
  size_t length;
  /// type of Message
  MessageType_ type;
  /// associated command
  uint8_t associatedCommand;
  /// ?
  uint8_t speakerFlag;
  /// ?
  size_t outLength;
  /// timeout, milliseconds
  uint32_t timeout;
};

/// information about all handled Messages
const MessageInformation_ messageInformation_[] =
{
  {0x28, 6, MessageType_::CONF_TIMEOUT, 0x00, 0x00, 3, 300},
  {0x20, 6, MessageType_::CONF_TIMEOUT, 0x00, 0x00, 3, 300},
  {0x48, 6, MessageType_::CONF_TIMEOUT, 0x49, 0x00, 6, 300},
  {0x49, 6, MessageType_::NEG_CONFIRMATION, 0x48, 0, 6, 0},
  {0x50, 6, MessageType_::CONF_TIMEOUT, 0x51, 0x03, 6, 1000},
  {0x51, 6, MessageType_::NEG_CONFIRMATION, 0x50, 0, 6, 0},
  {0x52, 6, MessageType_::CONF_TIMEOUT, 0x53, 0x03, 6, 300},
  {0x53, 6, MessageType_::NEG_CONFIRMATION, 0x52, 0, 6, 0},
  {0x55, 15, MessageType_::PER_GEN, 0x55, 0, 15, 0},
  {0x58, 6, MessageType_::CONF_TIMEOUT, 0x59, 0x03, 6, 300},
  {0x59, 6, MessageType_::NEG_CONFIRMATION, 0x58, 0, 6, 0},
  {0x5A, 6, MessageType_::CONF_TIMEOUT, 0x5B, 0x02, 6, 300},
  {0x5B, 6, MessageType_::NEG_CONFIRMATION, 0x5A, 0, 6, 0},
  {0x60, 7, MessageType_::PER_GEN, 0, 0, 6, 0},
  {0x70, 7, MessageType_::PER_GEN, 0, 0, 6, 0},
  {0x80, 6, MessageType_::CONF_TIMEOUT, 0x81, 0x03, 6, 300},
  {0x81, 6, MessageType_::NEG_CONFIRMATION, 0x80, 0, 6, 0},
  {0x90, 8, MessageType_::CONF_TIMEOUT, 0x00, 0x03, 1, 300},
  {0x92, 10, MessageType_::PER_GEN, 0, 0x0, 0, 800},
  {0x94, 6, MessageType_::CONF_TIMEOUT, 0x95, 0x03, 6, 600},
  {0x95, 6, MessageType_::NEG_CONFIRMATION, 0x94, 0, 6, 0},
  {0x96, 10, MessageType_::CONF_TIMEOUT, 0x97, 0x00, 2, 300},
  {0x97, 10, MessageType_::NEG_CONFIRMATION, 0x96, 0, 0, 0},
  {0x98, 6, MessageType_::CONF_TIMEOUT, 0x99, 0x00, 6, 300},
  {0x99, 6, MessageType_::NEG_CONFIRMATION, 0x98, 0, 0, 0},
  {0x9C, 7, MessageType_::CONF_TIMEOUT, 0x9D, 0x00, 6, 300},
  {0x9D, 7, MessageType_::NEG_CONFIRMATION, 0x9C, 0, 0, 0},
  {0xA0, 6, MessageType_::PER_GEN, 0, 0, 0, 0},
  {0xB0, 7, MessageType_::PER_GEN, 0, 0, 0, 0},
  {0xB1, 9, MessageType_::PER_GEN, 0, 0, 0, 0},
  {0xF0, 0, MessageType_::CONF_TIMEOUT, 0xF1, 0, 0, 500},
  {0xF1, 6, MessageType_::NEG_CONFIRMATION, 0xF0, 0, 0, 0},
  {0xF2, 3, MessageType_::CONF_TIMEOUT, 0xF3, 0, 0, 100},
  {0xF3, 3, MessageType_::NEG_CONFIRMATION, 0xF2, 0, 0, 0},
};

///							ip address, port, id, struct sockaddr, last access time
std::list<std::tuple<std::string, int, int, struct sockaddr_in, time_t>> list_of_ids;
static unsigned char id {1};

int receive_serial_data_A()
{
  int l;
  bool cmd_valid {false};
  unsigned int send_buffer_udp_size { 0 };
  bool finish_loop { false };
  l = read(fd_serial_A, rec_buffer_A + ptr_A, SER_BUF_SIZE - ptr_A);
  if (l < 0)
    {
//        std::cout << "A serial error" << std::endl;
      timer_A_active = false;
      processing_command = false;
      ptr_A = 0;
      return -1;
    }
  if ((ptr_A + l) < SER_BUF_SIZE) ptr_A += l;
  while (ptr_A > 0 && !finish_loop)
    {
      cmd_valid = false;
for(auto& msi : messageInformation_)
        {
          if (rec_buffer_A[0] == msi.command)
            {
              if (ptr_A >= msi.length)
                {
                  if (processed_command == (rec_buffer_A[0] & 0xFE))
                    {
                      timer_A_active = false;
                      processing_command = false;
                    }
                  if ((send_buffer_udp_size + msi.length) <= UDP_BUF_SIZE)  				/// leave loop if we would overflow udp_buffer
                    {
                      memcpy(send_buffer_udp + send_buffer_udp_size, rec_buffer_A, msi.length);
                      send_buffer_udp_size += msi.length;
                      if (host_addr_set)
                        {
                          if (msi.command == 0x48)
                            {
for(auto &host : list_of_ids)
                                {
                                  struct sockaddr_in l_remaddr = static_cast<struct sockaddr_in>(std::get<3>(host));
                                  if (l_remaddr.sin_family == remaddr.sin_family && l_remaddr.sin_addr.s_addr == remaddr.sin_addr.s_addr && l_remaddr.sin_port == remaddr.sin_port) continue;
                                  adr_len = sizeof(l_remaddr);
                                  rec_buffer_A[msi.length - 1] = std::get<2>(host);
                                  int udp_succes, try_cnt { 3 };
                                  do
                                    {
                                      udp_succes = sendto(fd_udp, rec_buffer_A, msi.length, 0, (struct sockaddr *)&l_remaddr, adr_len);
                                    }
                                  while (udp_succes < 0 && try_cnt-- > 0);
                                }
                            }
                        }
                      ptr_A -= msi.length;
                      for(int i=0; i<ptr_A; ++i) rec_buffer_A[i] = rec_buffer_A[i + msi.length];

#ifdef HAVE_LEDS
                      set_leds_cmd(msi.command);
#endif
                    }
                  else
                    {
                      finish_loop = true;
                    }
                }
              else
                {
                  finish_loop = true;
                }
              cmd_valid = true;
              break;
            }
        }
      if (!cmd_valid)
        {
//            std::cout << "A cmd not valid" << std::endl;
          timer_A_active = false;
          processing_command = false;
          ptr_A = 0;
          finish_loop = true;
        }
    }

  if (host_addr_set && send_buffer_udp_size > 0)
    {
      int udp_succes, try_cnt { 3 };
      adr_len = sizeof(remaddr);
      do
        {
          udp_succes = sendto(fd_udp, send_buffer_udp, send_buffer_udp_size, 0, (struct sockaddr *)&remaddr, adr_len);
        }
      while (udp_succes < 0 && try_cnt-- > 0);
    }

  return 0;
}

#define DBUF_LEN 512
typedef struct
{
  uint8_t len;
  uint8_t dir:1;//0 - to MADO, 1- from MADO
  uint8_t mark:7;//Marker
  time_t t;
  uint8_t buf[32];
} dBuf_t;

static dBuf_t dBuffer[DBUF_LEN];
static uint32_t dBufSize = 0;
static uint32_t dBufHead = 0;
static uint32_t dBufTail = 0;
static uint32_t dBufFlushNow = 0;
static FILE *dBufFile = NULL;
#define DBUF_FLUSH_LIMIT 510

static void dBufAdd(uint8_t dir, uint8_t *buf, uint32_t len)
{
  if(dBufFile == NULL) return;
  uint8_t mark = 0;
  if(len < 1) return;
  switch(buf[0])
    {
    default:
      return;
    case 0x50:
    case 0x51:
    case 0x52:
    case 0x53:
    case 0xA0:
      mark = 1;
      break;
    case 0x58:
      mark = 2;
      dBufFlushNow = 1;
      break;
    case 0xB0:
    case 0xB1:
      switch(buf[3])
        {
        case 0xC1:
        case 0xD1:
          mark++;//5
        case 0xA1 ... 0xA2:
          mark++;//4
        case 0xB0 ... 0xBF:
          mark += 3;//3
          dBufFlushNow = 1;
          break;
        }

      break;
    case 0x55:
    case 0x59:
    case 0x5A:
    case 0x5B:
    case 0x60:
    case 0x70:
    case 0x80:
    case 0x81:
    case 0x90:
    case 0x92:
    case 0x93:
    case 0x94:
    case 0x96:
    case 0x98:
    case 0x9C:
    case 0x9D:
      //case 0xA2:
      //case 0xA3:
      break;
    }

  dBuffer[dBufTail].t = time(NULL);
  memcpy(dBuffer[dBufTail].buf, buf, len);
  dBuffer[dBufTail].len = len;
  dBuffer[dBufTail].dir = dir;
  dBuffer[dBufTail].mark = mark;
  dBufTail  = (dBufTail+1)%DBUF_LEN;
  if(dBufSize == DBUF_LEN)
    {
      dBufHead  = (dBufHead+1)%DBUF_LEN;
    }
  else
    {
      dBufSize++;
    }

}
static const char markCh[] = {' ', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
static void dBufFlush(FILE *file)
{
  int i;

  while(dBufSize)
    {
      char myTime[100];
      ctime_r(&dBuffer[dBufHead].t, myTime);
      myTime[24] = 0;
      fprintf(file, "%s%c %s %s", dBuffer[dBufHead].mark?"\n!!":"  ", markCh[dBuffer[dBufHead].mark], myTime, dBuffer[dBufHead].dir?"<-   ":"  -> ");
      for(i=0; i < dBuffer[dBufHead].len; i++)
        fprintf(file, "%02x ", dBuffer[dBufHead].buf[i]);
      fprintf(file, "\n");

      dBufHead  = (dBufHead+1)%DBUF_LEN;
      dBufSize--;
    }
  fflush(file);
}
#define DO_NOT_AGREGATE
int receive_serial_data_B()
{
  int l;
  bool cmd_valid {false};
  unsigned int send_buffer_udp_size { 0 };
  bool finish_loop { false };
  l = read(fd_serial_B, rec_buffer_B + ptr_B, SER_BUF_SIZE - ptr_B);
  if (l < 0)
    {
      ptr_B = 0;
      timer_B_active = false;
      processing_command = false;
      return -1;
    }
  if ((ptr_B + l) < SER_BUF_SIZE) ptr_B += l;
  while (ptr_B > 0 && !finish_loop)
    {
      cmd_valid = false;
for(auto& msi : messageInformation_)
        {
          if (rec_buffer_B[0] == msi.command)
            {
              if (ptr_B >= msi.length)
                {
                  if (processed_command == (rec_buffer_B[0] & 0xFE))
                    {
                      timer_B_active = false;
                      processing_command = false;
                    }
                  if ((send_buffer_udp_size + msi.length) <= UDP_BUF_SIZE)  				/// leave loop if we would overflow udp_buffer
                    {
#ifndef DO_NOT_AGREGATE
		      if(msi.command != 0x55)
			{
			  memcpy(send_buffer_udp + send_buffer_udp_size, rec_buffer_B, msi.length);
			  send_buffer_udp_size += msi.length;
			}
#else
		      {
			adr_len = sizeof(remaddr);
			int udp_succes, try_cnt { 3 };
			do
			  {
			    udp_succes = sendto(fd_udp, rec_buffer_B, msi.length, 0, (struct sockaddr *)&remaddr, adr_len);
			  }
			while (udp_succes < 0 && try_cnt-- > 0);
		      }
#endif

                      if (host_addr_set)
                        {
                          dBufAdd(1, rec_buffer_B, msi.length);

                          if (msi.command == 0x48 || msi.command == 0x50 || msi.command == 0x52 || msi.command == 0x58 || msi.command == 0x5a || msi.command == 0x80 || msi.command == 0x94)
                            {
for(auto &host : list_of_ids)
                                {
                                  struct sockaddr_in l_remaddr = static_cast<struct sockaddr_in>(std::get<3>(host));
                                  if (l_remaddr.sin_family == remaddr.sin_family && l_remaddr.sin_addr.s_addr == remaddr.sin_addr.s_addr && l_remaddr.sin_port == remaddr.sin_port) continue;
                                  adr_len = sizeof(l_remaddr);
                                  rec_buffer_B[msi.length - 1] = std::get<2>(host);
                                  int udp_succes, try_cnt { 3 };
                                  do
                                    {
                                      udp_succes = sendto(fd_udp, rec_buffer_B, msi.length, 0, (struct sockaddr *)&l_remaddr, adr_len);
                                    }
                                  while (udp_succes < 0 && try_cnt-- > 0);
                                }
                            }
                        }
                      ptr_B -= msi.length;
                      for(int i=0; i<ptr_B; ++i) rec_buffer_B[i] = rec_buffer_B[i + msi.length];
#ifdef HAVE_LEDS
                      set_leds_cmd(msi.command);
#endif
                    }
                  else
                    {
                      finish_loop = true;
                    }
                }
              else
                {
                  finish_loop = true;
                }
              cmd_valid = true;
              break;
            }
        }
      if (!cmd_valid)
        {
//            std::cout << "B cmd not valid" << std::endl;
          ptr_B = 0;
          finish_loop = true;
          timer_B_active = false;
          processing_command = false;
        }
    }

  if (host_addr_set && send_buffer_udp_size > 0)
    {
      adr_len = sizeof(remaddr);
      int udp_succes, try_cnt { 3 };
      do
        {
          udp_succes = sendto(fd_udp, send_buffer_udp, send_buffer_udp_size, 0, (struct sockaddr *)&remaddr, adr_len);
        }
      while (udp_succes < 0 && try_cnt-- > 0);
    }

  return 0;
}

const std::vector<std::string> song_ext { ".ogg", ".mp3", ".flac", ".wav", ".mp4", ".wma", ".m4a", ".aac" };

void list_directory(std::string& dir, std::vector<std::tuple<std::string, int, int>>& file_list)
{
  DIR *dirp;
  struct dirent *dp;
  int a {0};
  dirp = opendir(dir.c_str());
  if (dirp == nullptr) return;
  do
    {
      if ((dp = readdir(dirp)) != nullptr)
        {
          std::string fname(dp->d_name);
          if ((dp->d_type != DT_REG) && (dp->d_type != DT_DIR)) continue;
//            std::cout << "File: '" << fname << "' Type: " << (int)dp->d_type <<  std::endl;
          bool file_music {dp->d_type == DT_DIR};
for(auto& s : song_ext) if (fname.rfind(s) != std::string::npos)
              {
                file_music = true;
                break;
              }
          fname = dir + fname;
          if (file_music)
            {
              std::tuple<std::string, int, int> fl;
              std::get<0>(fl) = fname;
              std::get<1>(fl) = a++;
              std::get<2>(fl) = dp->d_type;
              file_list.push_back(fl);
            }
        }
    }
  while (dp != nullptr);
  closedir(dirp);
  return;
}

std::vector<std::tuple<std::string, int, int>> dir_list;

int list_dir_create_dgram_packet(std::string& dir, int start)
{
  int a0, c, p {8}, cnt;
  unsigned char dgram[65536];
  if (start == 0) dir_list.clear();
  list_directory(dir, dir_list);
  for(cnt=start; cnt<dir_list.size(); ++cnt)
    {
      a0 = std::get<0>(dir_list[cnt]).size();
//		std::cout << "chars count: " << a0 << std::endl;
//		std::cout << "Name: " << std::get<0>(dir_list[cnt]) << std::endl;
      if ((p + a0 + 6) < sizeof(dgram))
        {
          /// put length of the string into buffer
          dgram[p++] = 0;
          dgram[p++] = std::get<2>(dir_list[cnt]) & 0xFF;
          dgram[p++] = std::get<1>(dir_list[cnt]) & 0xFF;
          dgram[p++] = std::get<1>(dir_list[cnt]) >> 8;
          dgram[p++] = a0 & 0xFF;
          dgram[p++] = a0 >> 8;
          std::copy(std::get<0>(dir_list[cnt]).begin(), std::get<0>(dir_list[cnt]).end(), dgram + p);
          p += a0;
        }
    }
  dgram[0] = 0xF0;
  dgram[1] = 1;
  dgram[2] = p & 0xFF;
  dgram[3] = p >> 8;
  dgram[4] = cnt & 0xFF;
  dgram[5] = cnt >> 8;
  dgram[6] = dir_list.size() & 0xFF;
  dgram[7] = dir_list.size() >> 8;
  if (host_addr_set && (p > 0)) if (sendto(fd_udp, dgram, p, 0, (struct sockaddr *)&remaddr, adr_len) < 0) return -1;
//	std::cout << "Sent: " << p << std::endl;
//	for(int i=0; i<p; ++i) std::cout << std::hex << (int)dgram[i] << ", ";
  return 0;
}

void list_song_directory(std::string& dir, std::vector<std::tuple<std::string, std::string, std::string, std::string, int>>& song_list)
{
  DIR *dirp;
  struct dirent *dp;
  int a {0};
  dirp = opendir(dir.c_str());
  if (dirp == nullptr) return;
  do
    {
      if ((dp = readdir(dirp)) != nullptr)
        {
          std::string fname(dp->d_name);
          if (dp->d_type != DT_REG) continue;
//            std::cout << "File: '" << fname << "' Type: " << (int)dp->d_type <<  std::endl;
          bool file_music {false};
for(auto& s : song_ext) if (fname.rfind(s) != std::string::npos)
              {
                file_music = true;
                break;
              }
          fname = dir + fname;
          if (file_music)
            {
              TagLib::FileRef f(fname.c_str());
              std::tuple<std::string, std::string, std::string, std::string, int> fl;
              if (f.tag() != nullptr)
                {
                  std::get<0>(fl) =  f.tag()->artist().size() < 1 ? "nieznany" : f.tag()->artist().to8Bit(true);
                  std::get<1>(fl) = f.tag()->album().size() < 1 ? "nieznany" : f.tag()->album().to8Bit(true);
                  std::get<2>(fl) = f.tag()->title().size() < 1 ? "nieznany" : f.tag()->title().to8Bit(true);
                }
              else
                {
                  std::get<0>(fl) = "nieznany";
                  std::get<1>(fl) = "nieznany";
                  std::get<2>(fl) = "nieznany";
                }
              std::get<3>(fl) = fname;
              std::get<4>(fl) = a++;
              song_list.push_back(fl);
            }
        }
    }
  while (dp != nullptr);
  closedir(dirp);
  return;
}

std::vector<std::tuple<std::string, std::string, std::string, std::string, int>> song_list;

int list_song_create_dgram_packet(std::string& dir, int start)
{
  int a0, a1, a2, a3, c, p {8}, cnt;
  unsigned char dgram[65536];
  if (start == 0) song_list.clear();
  list_song_directory(dir, song_list);
  for(cnt=start; cnt<song_list.size(); ++cnt)
    {
      a0 = std::get<0>(song_list[cnt]).size();
      a1 = std::get<1>(song_list[cnt]).size();
      a2 = std::get<2>(song_list[cnt]).size();
      a3 = std::get<3>(song_list[cnt]).size();
//		std::cout << "chars count: " << a0 + a1 + a2 + a3 << std::endl;
//		std::cout << "Artist: " << std::get<0>(song_list[cnt]) << " Album: " << std::get<1>(song_list[cnt]) << " Title: " << std::get<2>(song_list[cnt]) << std::endl;
      if ((p + a0 + a1 + a2 + a3 + 10) < sizeof(dgram))
        {
          c = 2;
          /// put length of the string into buffer
          dgram[p + c++] = std::get<4>(song_list[cnt]) & 0xFF;
          dgram[p + c++] = std::get<4>(song_list[cnt]) >> 8;
          dgram[p + c++] = a0 & 0xFF;
          dgram[p + c++] = a0 >> 8;
          std::copy(std::get<0>(song_list[cnt]).begin(), std::get<0>(song_list[cnt]).end(), dgram + p + c);
          c += a0;
          dgram[p + c++] = a1 & 0xFF;
          dgram[p + c++] = a1 >> 8;
          std::copy(std::get<1>(song_list[cnt]).begin(), std::get<1>(song_list[cnt]).end(), dgram + p + c);
          c += a1;
          dgram[p + c++] = a2 & 0xFF;
          dgram[p + c++] = a2 >> 8;
          std::copy(std::get<2>(song_list[cnt]).begin(), std::get<2>(song_list[cnt]).end(), dgram + p + c);
          c += a2;
          dgram[p + c++] = a3 & 0xFF;
          dgram[p + c++] = a3 >> 8;
          std::copy(std::get<3>(song_list[cnt]).begin(), std::get<3>(song_list[cnt]).end(), dgram + p + c);
          c += a3;
          dgram[p] = c & 0xFF;
          dgram[p + 1] = c >> 8;
          p += c;
        }
    }
  dgram[0] = 0xF0;
  dgram[1] = 0;
  dgram[2] = p & 0xFF;
  dgram[3] = p >> 8;
  dgram[4] = cnt & 0xFF;
  dgram[5] = cnt >> 8;
  dgram[6] = song_list.size() & 0xFF;
  dgram[7] = song_list.size() >> 8;
  if (host_addr_set && (p > 0)) if (sendto(fd_udp, dgram, p, 0, (struct sockaddr *)&remaddr, adr_len) < 0) return -1;
//	std::cout << "Sent: " << p << std::endl;
//	for(int i=0; i<p; ++i) std::cout << std::hex << (int)dgram[i] << ", ";
  return 0;
}

#ifdef HAVE_LEDS
void set_leds_cmd(int serial_cmd)
{
  switch (serial_cmd)
    {
    case 0xB0:
      leds_action[1] = 1;
      leds_action[0] = 0;
      leds_action_count = 2;
      break;
    case 0x50:
      leds_action[7] = 1;
      leds_action[6] = 0;
      leds_action[5] = 1;
      leds_action[4] = 0;
      leds_action[3] = 1;
      leds_action[2] = 0;
      leds_action[1] = 1;
      leds_action[0] = 0;
      leds_action_count = 8;
      break;
    case 0x52:
      leds_action[5] = 1;
      leds_action[4] = 0;
      leds_action[3] = 1;
      leds_action[2] = 0;
      leds_action[1] = 1;
      leds_action[0] = 0;
      leds_action_count = 6;
      break;
    case 0x94:
      leds_action[3] = 1;
      leds_action[2] = 0;
      leds_action[1] = 1;
      leds_action[0] = 0;
      leds_action_count = 4;
      break;
    case 0x80:
      leds_action[1] = 2;
      leds_action[0] = 0;
      leds_action_count = 2;
      break;
    case 0x28:
      leds_action[3] = 2;
      leds_action[2] = 0;
      leds_action[1] = 2;
      leds_action[0] = 0;
      leds_action_count = 4;
      break;
    case 0x20:
      leds_action[5] = 2;
      leds_action[4] = 0;
      leds_action[3] = 2;
      leds_action[2] = 0;
      leds_action[1] = 2;
      leds_action[0] = 0;
      leds_action_count = 6;
      break;
    case 0x48:
      leds_action[7] = 2;
      leds_action[6] = 0;
      leds_action[5] = 2;
      leds_action[4] = 0;
      leds_action[3] = 2;
      leds_action[2] = 0;
      leds_action[1] = 2;
      leds_action[0] = 0;
      leds_action_count = 8;
      break;
    case 0x58:
      leds_action[5] = 2;
      leds_action[4] = 0;
      leds_action[3] = 2;
      leds_action[2] = 0;
      leds_action[1] = 1;
      leds_action[0] = 0;
      leds_action_count = 6;
      break;
    }
}

int control_leds()
{
  struct timeval act_time, add_time;
  int v;
  gettimeofday(&act_time, NULL);
  if (timercmp(&act_time, &leds_timer, <)) return 0;
  add_time.tv_sec = 0;
  add_time.tv_usec = 100000;
  timeradd(&act_time, &add_time, &leds_timer);
  if (leds_action_count == 0)
    {
      leds_cmd = 0;
      if (!host_addr_set)
        {
          leds_action[1] = 2;
          leds_action[0] = 1;
          leds_action_count = 2;
        }
      return 0;
    }
  v = leds_action[--leds_action_count];
  for(int i=0; i<2; ++i)
    {
      int a = v & (1 << i) ? '1' : '0';
      if (write(fd_leds[i], &a, 2) < 2) return -1;
    }
  return 0;
}
#endif

bool assign_id_to_remote(struct sockaddr_in &adr)
{
  char tmp[INET6_ADDRSTRLEN > INET_ADDRSTRLEN ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &(adr.sin_addr), tmp, sizeof(tmp)) == NULL) return false;
  std::string str(tmp);

  unsigned char tid { 0 };

for(auto& entry : list_of_ids)
    {
      if (str == static_cast<std::string>(std::get<0>(entry)) && (ntohs(adr.sin_port) == std::get<1>(entry)))
        {
          /// the remote device is already on the list
//			std::cout << "Id already assigned to: " << str << ", " << ntohs(adr.sin_port) << ", id: " << std::get<2>(entry) << std::endl;
          tid = std::get<2>(entry);
          std::get<4>(entry) = time(nullptr);
          break;
        }
    }

  if (tid == 0)
    {
      for(int i=1; i<256; ++i)
        {
          bool id_free { true };
for(auto& entry : list_of_ids)
            {
              if (std::get<2>(entry) == i)
                {
                  id_free = false;
                  break;
                }
            }
          if (id_free)
            {
              tid = i;
              break;
            }
        }
      if (tid == 0)
        {
          double max_lease_time { 0.0 };
          unsigned char temp_id { 0 };
for(auto& entry : list_of_ids)
            {
              time_t now;
              time(&now);
              if (difftime(now, std::get<4>(entry)) > max_lease_time)
                {
                  max_lease_time = difftime(now, std::get<4>(entry));
                  temp_id = std::get<2>(entry);
                }
            }
          for(auto it=list_of_ids.begin(); it!=list_of_ids.end(); ++it)
            {
              if (std::get<2>(*it) == temp_id)
                {
                  list_of_ids.erase(it);
                  break;
                }
            }
          tid = temp_id;
        }
//		std::cout << "Assigned id to: " << str << ", " << ntohs(adr.sin_port) << ", id: " << (int)id << std::endl;
      list_of_ids.push_back(std::make_tuple(str, ntohs(adr.sin_port), tid, adr, time(nullptr)));
    }

  unsigned char dgram[] {0xF2, 0x00, tid};
  if (host_addr_set) if (sendto(fd_udp, dgram, sizeof(dgram), 0, (struct sockaddr *)&remaddr, adr_len) < 0) return false;
  std::cout << "Sent udp message" << std::endl;
  return true;
}

bool remove_id_from_list(int id_)
{
  for(auto it=list_of_ids.begin(); it!=list_of_ids.end(); ++it)
    {
      if (std::get<2>(*it) == id_)
        {
          list_of_ids.erase(it);
//			std:: cout << "Freed id: " << id_ << std::endl;
          id = id_;
          break;
        }
    }
  unsigned char dgram[] {0xF2, 0x01, 0x00};
  if (host_addr_set) if (sendto(fd_udp, dgram, sizeof(dgram), 0, (struct sockaddr *)&remaddr, adr_len) < 0) return false;
  return true;
}


int main(int argc, char **argv)
{
  struct timeval act_time, add_time;
  struct termios options;
  int fd_watchdog;
  std::deque<std::pair<std::vector<unsigned char>, struct sockaddr_in>> commands;

  setpriority(PRIO_PROCESS, 0, 10);
  // Increase priority for this thread
//    pthread_t this_thread = pthread_self();
//    struct sched_param sched_params;
//    sched_params.sched_priority = sched_get_priority_min(SCHED_IDLE);
//    if (pthread_setschedparam(this_thread, SCHED_IDLE, &sched_params) != 0) std::cout << "Unsuccessful in setting thread priority\n";
  if (argc < 4)
    {
      std::cout << "Usage: 'bb_controller udp_port serial_port_A serial_port_B [logFile]\n";
      return -1;
    }
  fd_watchdog = open("/dev/watchdog", O_WRONLY);
  if (fd_watchdog < 0)
    {
      std::cout << "Cannot open watchdog\n";
      return -1;
    }
  fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd_udp < 0)
    {
      std::cout << "Cannot open socket\n";
      return -1;
    }

  fd_serial_A = open(argv[2], O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd_serial_A < 0)
    {
      std::cout << "Cannot open port for read " << argv[2] << std::endl;
      return -1;
    }
  memset(&options, 0, sizeof(options));
  options.c_cc[VTIME] = 0;
  options.c_cc[VMIN] = 0;
  options.c_iflag = 0;
  options.c_oflag = 0;
  options.c_cflag = CS8 | CREAD | CLOCAL;
  options.c_lflag = 0;
  cfsetospeed(&options, B115200);
  cfsetispeed(&options, B115200);
  tcsetattr(fd_serial_A, TCSANOW, &options);
  tcflush(fd_serial_A, TCIOFLUSH);

  fd_serial_B = open(argv[3], O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd_serial_B < 0)
    {
      std::cout << "Cannot open port " << argv[3] << std::endl;
      return -1;
    }
  tcsetattr(fd_serial_B, TCSANOW, &options);
  tcflush(fd_serial_B, TCIOFLUSH);


#ifdef HAVE_LEDS
  for(int i=0; i<2; ++i)
    {
      char led_path[] = "/sys/class/leds/beaglebone:green:usr0/brightness";
      led_path[36] = '0' + i;
      std::cout << "Opening " << led_path << std::endl;
      fd_leds[i] = open(led_path, O_WRONLY | O_NONBLOCK);
    }
  for(int i=0; i<2; ++i)
    {
      if (fd_leds[i] < 0)
        {
          std::cout << "Cannot open LED" << i << std::endl;
          return -1;
        }
    }
#endif

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(atoi(argv[1]));
  if (bind(fd_udp, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
      close(fd_udp);
      std::cout << "ERROR on binding\n";
      return -1;
    }

  poll_serial_A.fd = fd_serial_A;
  poll_serial_A.events = POLLIN;
  poll_serial_A.revents = 0;

  poll_serial_B.fd = fd_serial_B;
  poll_serial_B.events = POLLIN;
  poll_serial_B.revents = 0;

  poll_udp.fd = fd_udp;
  poll_udp.events = POLLIN;
  poll_udp.revents = 0;

  if(argc >= 5)
    dBufFile = fopen(argv[4], "a");
  struct timeval dBufTime;
  gettimeofday(&dBufTime, NULL);
  do
    {
      if(dBufFile != NULL)
        {
          struct timeval act_time, add_time;
          gettimeofday(&act_time, NULL);
          if (timercmp(&act_time, &dBufTime, >) || dBufFlushNow || dBufSize > DBUF_FLUSH_LIMIT)
            {
              add_time.tv_sec = 10;
              add_time.tv_usec = 0;
              timeradd(&act_time, &add_time, &dBufTime);
              dBufFlushNow = 0;
              dBufFlush(dBufFile);
            }
        }
#ifdef HAVE_LEDS
      control_leds();
#endif

      if ((poll(&poll_serial_A, 1, 0) > 0) || (ptr_A > 0))
        {
          poll_serial_A.revents = 0;
          receive_serial_data_A();
        }
      if ((poll(&poll_serial_B, 1, 0) > 0) || (ptr_B > 0))
        {
          poll_serial_B.revents = 0;
          receive_serial_data_B();
        }
      if (poll(&poll_udp, 1, 20) > 0)
        {
          struct sockaddr_in remote;
          poll_udp.revents = 0;
          adr_len = sizeof(remote);
          int recvlen = recvfrom(fd_udp, rec_buffer_udp, UDP_BUF_SIZE, 0, (struct sockaddr *)&remote, &adr_len);
          host_addr_set = true;
          std::vector<unsigned char> v(rec_buffer_udp, rec_buffer_udp + recvlen);
          commands.push_back(std::make_pair(v, remote));
        }
      if ((commands.size() > 0) && !processing_command)
        {
          std::vector<unsigned char> command { std::get<0>(commands.front()) };
          remaddr = static_cast<struct sockaddr_in>(std::get<1>(commands.front()));
//        	std::cout << "Processing command from: " << inet_ntoa(remaddr.sin_addr) << ", port: " << ntohs(remaddr.sin_port) << std::endl;
          commands.pop_front();
for(auto& msi : messageInformation_)
            {
              if (command[0] == msi.command)
                {
                  dBufAdd(0, command.data(), msi.length);
                  processing_command = true;
//                    std::cout << "Command: " << (int)msi.command << std::endl;
                  if (command.size() == msi.length || (!msi.length && (command.size() > 0)))
                    {
//                        for(int i=0; i<command.size(); ++i) std::cout << std::hex << (int)command[i] << ", ";
//                        std::cout << std::endl;
                      int len {0};
                      if (command[0] == 0xF0)
                        {
                          int start = (command[3] << 8) | command[2];
                          std::string dir((const char*)command.data() + 6, (command[5] << 8) | command[4]);
                          if (command[1] == 0)  				/// get song list
                            {
                              list_song_create_dgram_packet(dir, start);
                              processing_command = false;
                            }
                          else if (command[1] == 1)  		/// get directory list
                            {
                              list_dir_create_dgram_packet(dir, start);
                              processing_command = false;
                            }
                          else if (command[1] == 2)  		/// forward packet to STM, currently not used
                            {
//                        		len = write(fd_serial_A, command.data() + 4, command.size() - 4);
                            }
                        }
                      else if (command[0] == 0xF2)
                        {
                          if (command[1] == 0)  		/// assign id
                            {
                              assign_id_to_remote(remaddr);
                              processing_command = false;
                            }
                          else if (command[1] == 1)  		///free id
                            {
                              remove_id_from_list(command[2]);
                              processing_command = false;
                            }
                        }
                      else
                        {
                          unsigned char id { command[command.size() - 1] };
for(auto &host : list_of_ids)
                            {
                              if (std::get<2>(host) == id)
                                {
                                  std::get<4>(host) = time(nullptr);
                                  break;
                                }
                            }
                          processed_command = command[0];
                          gettimeofday(&act_time, NULL);
                          if (command[0] < 0x50)
                            {
                              len = write(fd_serial_A, command.data(), msi.length);
                              add_time.tv_sec = msi.timeout / 100;
                              add_time.tv_usec = (msi.timeout % 100) * 100000;
                              timeradd(&act_time, &add_time, &timer_serial_A);
                              timer_A_active = true;
                            }
                          else
                            {
                              if (command[0] == 0x96)
                                {
                                  connected_player_addr = remaddr;
                                  connected_player_addr_set = true;
                                }
                              else if (command[0] == 0x98)
                                {
                                  connected_player_addr_set = false;
                                }
                              len = write(fd_serial_B, command.data(), msi.length);
                              add_time.tv_sec = msi.timeout / 100;
                              add_time.tv_usec = (msi.timeout % 100) * 100000;
                              timeradd(&act_time, &add_time, &timer_serial_B);
                              timer_B_active = true;
                            }
                        }
                      if (len < 0) std::cout << "Cannot write to serial port" << len << std::endl;
                    }
                  else
                    {
                      processing_command = false;
                    }
                  break;
                }
            }
        }

      gettimeofday(&act_time, NULL);
      if (timer_A_active && timercmp(&act_time, &timer_serial_A, >))
        {
          //      	std::cout << "Timeout occured on A\n";
          ptr_A = 0;
          timer_A_active = false;
          processing_command = false;
        }
      if (timer_B_active && timercmp(&act_time, &timer_serial_B, >))
        {
          //      	std::cout << "Timeout occured on B\n";
          ptr_B = 0;
          timer_B_active = false;
          processing_command = false;
        }
      int a {0x0A0D};
      int r = write(fd_watchdog, &a, 2);
    }
  while (true);

  close(fd_udp);
  close(fd_serial_A);
  close(fd_serial_B);
  close(fd_watchdog);

#ifdef HAVE_LEDS
  for(int i=0; i<2; ++i) close(fd_leds[i]);
#endif

  return 0;
}

