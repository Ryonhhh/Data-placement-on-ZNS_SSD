#ifndef zns_controller_H
#define zns_controller_H

#include </home/wht/Data-placement-on-ZNS_SSD/libzbd-2.0.3/include/libzbd/zbd.h>
#include <assert.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/stat.h>

#include <iostream>

using namespace std;

class BLOCK_SIM {
  unsigned long long block_id;
  unsigned long long start;

 public:
  BLOCK_SIM(){};
  void set_block(unsigned long long _block_id, int block_size);
  unsigned long long get_block_start();
};

class ZONE_SIM {
  int fd;
  int zone_id;
  unsigned long long start;
  unsigned long long capacity;
  unsigned int type;

 public:
  ZONE_SIM(){};
  void set_zone(int _fd, int _zone_id, unsigned long long _capacity);
  void print_zone_info();

  int get_zone_id();
  unsigned long long get_zone_start();
  unsigned long long get_zone_capacity();
  unsigned int get_zone_type();

  unsigned long long get_zone_wp();
  unsigned int get_zone_flags();
  unsigned int get_zone_cond();
  void print_zone_cond();

  void open_zone();
  void close_zone();
  void reset_zone();
  void finish_zone();

  float get_empty_rate();
};

class ZNS_SIM {
  int dev_id;
  struct zbd_info *info;

 public:
  ZNS_SIM();
  unsigned long long get_zone_size();
  unsigned long long get_block_number();
  unsigned int get_zone_number();
  unsigned int get_block_size();
  int get_dev_id();
  void print_zns_info();
  void close_zns_dev();
};

#endif