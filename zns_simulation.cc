#include "zns_simulation.h"

#include <string.h>

#include <fstream>
#include <sstream>
#include <vector>

#include "parameter.h"

ZNS_Simulation::ZNS_Simulation() {
  zns_sim = new ZNS_SIM();
  zns_sim->print_zns_info();
  zone_sim = (ZONE_SIM *)malloc(sizeof(ZONE_SIM) * zns_sim->get_zone_number());
  block_sim =
      (BLOCK_SIM *)malloc(sizeof(BLOCK_SIM) * zns_sim->get_block_number());
  for (int i = 0; i < (int)zns_sim->get_zone_number(); i++)
    zone_sim[i].set_zone(zns_sim->get_dev_id(), i, zns_sim->get_zone_size());
  for (int i = 0; i < (int)zns_sim->get_block_number(); i++)
    block_sim[i].set_block_status(BLOCK_EMPTY);
  cout << "created zone controller successfully!" << endl;
}

void ZNS_Simulation::initialize() {
  block_valid_map = (int *)malloc(sizeof(int) * zns_sim->get_block_number());
  gc_queue = (int *)malloc(sizeof(int) * gl);
  zone_lifetime_map = (int *)malloc(sizeof(int) * zns_sim->get_zone_number());
  empty_rate = (float *)malloc(sizeof(float) * zns_sim->get_zone_number());
  garbage_rate = (float *)malloc(sizeof(float) * zns_sim->get_zone_number());
  for (int i = 0; i < (int)zns_sim->get_block_number(); i++) {
    block_valid_map[i] = 0;
  }
  for (int i = 0; i < (int)zns_sim->get_zone_number(); i++) {
    zone_lifetime_map[i] = 0;
    empty_rate[i] = 1;
    garbage_rate[i] = 0;
  }
}

void ZNS_Simulation::generate_workload(int seq_ram) {
  if (seq_ram == 0)
    workload = new Workload_Creator(MAX_WORKLOAD);
  else
    workload = new Workload_Creator(MAX_WORKLOAD, HOT_RATE);
}

int ZNS_Simulation::write_block(char *page_in, int zone_id, int size) {
  int cond = zone_sim[zone_id].get_zone_cond();
  if (cond != ZBD_ZONE_COND_EMPTY && cond != ZBD_ZONE_COND_EXP_OPEN &&
      cond != ZBD_ZONE_COND_IMP_OPEN) {
    cout << "zone " << zone_id << " is not writeable!" << endl;
    zone_sim[zone_id].print_zone_cond();
    return 0;
  }

  assert(size == (int)zns_sim->get_block_size());
  int fd = zns_sim->get_dev_id();

  unsigned long long start = zone_sim[zone_id].get_zone_start();
  unsigned long long capacity = zone_sim[zone_id].get_zone_capacity();
  unsigned long long wp = zone_sim[zone_id].get_zone_wp();

  if (wp + size > start + capacity) {
    cout << "zone " << zone_id << " has no more space to write the page!"
         << endl;
    cout << " write point: " << wp << " page size: " << size
         << " end: " << start + capacity << endl;
    return 0;
  }
  int ret = pwrite(fd, page_in, size, wp);
  assert(ret == size);

  return 1;
}

void ZNS_Simulation::get_zone_garbage_rate() {
  int block_per_zone = zns_sim->get_block_number() / zns_sim->get_zone_number();
  for (int i = 0; i < (int)zns_sim->get_zone_number(); i++) {
    int tol = 0, sum = 0;
    for (int j = 0; j < block_per_zone; j++) {
      tol++;
      sum += block_valid_map[i * block_per_zone + j] == BLOCK_INVALID ? 1 : 0;
    }
    garbage_rate[i] = (float)sum / (float)tol;
  }
}

char *ZNS_Simulation::data2page(int *key, int *value_size, int len) {
  char *page;
  for (int i = 0; i < len; i++) {
  }
  return page;
}

void ZNS_Simulation::request_workload() {
  int cap = 0, OP, key_read, value_size_read, cnt = 0, tol_page;
  int key[MAX_KV_PER_BLOCK], value_size[MAX_KV_PER_BLOCK];

  ifstream inFile;
  inFile.open(FILE_PATH, ifstream::in);
  while (1) {
    inFile >> OP >> key_read >> value_size_read;
    if (cap + sizeof(int) * 3 + value_size_read > BLOCK_SIZE) {
      char *pages2zns = data2page(key, value, cnt - 1);
      cnt = cap = 0;
      myInsert(pages2zns);
    }
    key[cnt] = key_read;
    value_size[cnt] = value_size_read;
    cnt++;
    cap += sizeof(int) * 3 + value_size_read;
  }
}

void ZNS_Simulation::test() {
  /*int size = 4ul << 10;
  char *write_buffer = reinterpret_cast<char *>(memalign(4096, size));
  assert(write_buffer != nullptr);

  uint64_t value = 100;
  memcpy(write_buffer, &value, sizeof(value));
  zone_sim[1].print_zone_cond();
  zone_sim[1].open_zone();
  zone_sim[1].print_zone_cond();
  cout << zone_sim[1].get_zone_wp() << endl;
  write_block(write_buffer, 1, size);
  zone_sim[1].print_zone_cond();
  cout << zone_sim[1].get_zone_wp() << endl;
  zone_sim[1].reset_zone();
  zone_sim[1].print_zone_cond();
  cout << zone_sim[1].get_zone_wp() << endl;
  write_block(write_buffer, 1, size);
  zone_sim[1].print_zone_cond();
  cout << zone_sim[1].get_zone_wp() << endl;
  write_block(write_buffer, 1, size);
  zone_sim[1].print_zone_cond();
  cout << zone_sim[1].get_zone_wp() << endl;*/
}

void ZNS_Simulation::myInsert(char *page) {}