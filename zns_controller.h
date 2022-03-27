#ifndef zns_controller_H
#define zns_controller_H

#include<iostream>
#include<stdlib.h>
#include</usr/include/libzbd/zbd.h>

using namespace std;

class BLOCK_sim{

};

class ZONE_sim{
    int zone_id;
    int zone_mode; //0=conventional, 1=squential
    long long offset;
    long long length;
    long long capacity;
    long long write_point;
    int status;

public:


};

class ZNS_sim{
    int dev_id;
    struct zbd_info *info;

public:
    ZNS_sim(){};
    void open_zns_dev();
    void print_zns_info();
    void close_zns_dev();
};

#endif