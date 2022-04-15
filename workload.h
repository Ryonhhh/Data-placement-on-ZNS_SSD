#ifndef workload_H
#define workload_H

#include <malloc.h>
#include <string.h>

#include <iostream>
#define FILE_PATH "./wl"
using namespace std;

class Workload_Creator {
  int seq_or_ram;  // seq=0,ram=1

 public:
  Workload_Creator(int length);
  Workload_Creator(int length, float hot_data_rate, float update_rate);
};

#endif