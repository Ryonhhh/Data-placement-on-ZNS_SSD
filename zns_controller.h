#ifndef zns_controller_H
#define zns_controller_H

#include <iostream>
#include <malloc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include</usr/include/libzbd/zbd.h>

using namespace std;

class BLOCK_sim{
    unsigned long long block_id;
    unsigned long long start;
    int status; //0 = empty, 1 = valid, -1 = invalid

public:
    BLOCK_sim(){};
    void set_block(unsigned long long _block_id, int block_size);
    int get_block_status();
    void set_block_status(int _status);
};

class ZONE_sim{
    int fd;
    int zone_id;
    unsigned long long start;
    unsigned long long capacity;
    unsigned int type;

public:
    ZONE_sim(){};
    void set_zone(int _fd, int _zone_id, unsigned long long _capacity);
    void print_zone_info();

    int get_zone_id();
    unsigned long long get_zone_start();
    unsigned long long get_zone_capacity();
    unsigned int get_zone_type();

    unsigned long long get_zone_wp();
    unsigned int get_zone_status();
    unsigned int get_zone_cond();

    void open_zone();
    void close_zone();
    void reset_zone();
    void finish_zone();

    float get_empty_rate();
    //float get_garbage_rate();
};

class ZNS_sim{
    int dev_id;
    struct zbd_info *info;

public:
    ZNS_sim();
    unsigned long long get_zone_size();
    unsigned int get_block_size();
    unsigned int get_zone_number();
    int get_dev_id();
    void print_zns_info();
    void close_zns_dev();
};

#endif