#ifndef zns_simulation_H
#define zns_simulation_H

#include "parameter.h"
#include "zns_controller.h"
#include "workload.h"

typedef struct Page{
  int key_size;
  int *key;
  int value_size;
  int *value;
}Page;

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
  char* data2page(int* key, int* value_size);
  void get_zone_garbage_rate();
  void request_workload();
  void test();

  //alorithm
  void myInsert(char *page);
  void myUpdate_Delete(int key, int value_size);
  void myGarbageCollection();
  void myGcDetect();

};

#endif