#ifndef zns_simulation_H
#define zns_simulation_H

#include "parameter.h"
#include "workload.h"
#include "zns_controller.h"

class ZNS_Simulation {
  ZNS_SIM* zns_sim;
  ZONE_SIM* zone_sim;
  BLOCK_SIM* block_sim;
  Workload_Creator* workload;
  int zone_number;
  int block_number;
  int block_per_zone;
  int* block_valid_map;
  int* gc_queue;
  int* key_lifetime_map;
  int* page_lifetime_map;
  float* zone_lifetime_map;
  float* empty_rate;
  float* garbage_rate;

 public:
  ZNS_Simulation();
  void initialize();
  void generate_workload(int seq_ram);
  int write_block(char* page_in, int zone_id, int size, int block_id);
  char* data2page(int* key, int* value_size, int len);
  void get_zone_garbage_rate();
  void get_zone_empty_rate();
  void request_workload();

  // ZNS_aware_alorithm
  float lifetimeVarience(int zone_id);
  int get_page_lifetime(int* key, int len);
  void refreshLifetime(int zone_id, int* key, int len, int block_id, int pageLifetime);
  void GC_insert(char *page, int lifetime);
  void myInsert(char* page, int* key, int len);
  void myUpdate_Delete(
      int key,
      int value_size);  // delete: value_size=0, update: value_size != 0
  void myGarbageCollection();
  void myGcDetect();

  // test
  void test();
};

#endif