#ifndef zns_simulation_H
#define zns_simulation_H

#include "parameter.h"
#include "zns_controller.h"
#include "workload.h"

struct Page{
  
};

class ZNS_Simulation {
  ZNS_SIM* zns_sim;
  ZONE_SIM* zone_sim;
  BLOCK_SIM* block_sim;
  Workload_Creator* workload;
  int* zone_lifetime_map;
  int* zone_meta_map;
  int* block_valid_map;
  int* gc_queue;
  float* empty_rate;
  float* garbage_rate;

 public:
  ZNS_Simulation();
  void initialize();
  void generate_workload(int seq_ram);
  int write_block(char* page_in, int zone_id, int size);
  char** data2pages(int* key, int* value_size, int len);
  void get_zone_garbage_rate();
  void request_workload();
  void test();
};

#endif