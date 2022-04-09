#include "zns_simulation.h"

#include <string.h>

#include <cmath>
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
    block_sim[i].set_block(i, BLOCK_SIZE);
  cout << "created zone controller successfully!" << endl;
}

void ZNS_Simulation::initialize() {
  zone_number = zns_sim->get_zone_number();
  block_number = zns_sim->get_block_number();
  block_per_zone = block_number / zone_number;

  block_valid_map = (int *)malloc(sizeof(int) * block_number);
  page_lifetime_map = (int *)malloc(sizeof(int) * block_number);

  key_index = (int *)malloc(sizeof(int) * MAX_KEY);
  key_lifetime_map = (int *)malloc(sizeof(int) * MAX_KEY);

  gc_queue = (int *)malloc(sizeof(int) * zone_number);
  zone_lifetime_map = (float *)malloc(sizeof(float) * zone_number);
  empty_rate = (float *)malloc(sizeof(float) * zone_number);
  garbage_rate = (float *)malloc(sizeof(float) * zone_number);

  for (int i = 0; i < MAX_KEY; i++) {
    key_lifetime_map[i] = 0;
    key_index[i] = -1;
  }
  for (int i = 0; i < (int)block_number; i++) {
    block_valid_map[i] = 0;
    page_lifetime_map[i] = -1;
  }
  for (int i = 0; i < zone_number; i++) {
    zone_lifetime_map[i] = -1;
    empty_rate[i] = 1;
    garbage_rate[i] = 0;
    gc_queue[i] = 0;
  }
}

void ZNS_Simulation::generate_workload(int seq_ram) {
  if (seq_ram == 0)
    workload = new Workload_Creator(MAX_WORKLOAD);
  else
    workload = new Workload_Creator(MAX_WORKLOAD, HOT_RATE);
}

int ZNS_Simulation::write_block(char *page_in, int zone_id, int size,
                                int *block_id) {
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
  *block_id = zone_id * block_per_zone + (wp - start) / BLOCK_SIZE;
  int ret = pwrite(fd, page_in, size, wp);
  assert(ret == size);

  return 1;
}

void ZNS_Simulation::get_zone_garbage_rate() {
  for (int i = 0; i < (int)zone_number; i++) {
    int tol = 0, sum = 0;
    for (int j = 0; j < block_per_zone; j++) {
      tol++;
      sum += block_valid_map[i * block_per_zone + j] == BLOCK_INVALID ? 1 : 0;
    }
    garbage_rate[i] = (float)sum / (float)tol;
  }
}

void ZNS_Simulation::get_zone_empty_rate() {
  for (int i = 0; i < (int)zone_number; i++)
    empty_rate[i] = zone_sim[i].get_empty_rate();
}

char *ZNS_Simulation::data2page(int *key, int *value_size, int len) {
  char *page = reinterpret_cast<char *>(memalign(4096, BLOCK_SIZE));
  assert(page != nullptr);
  int addr = 0;
  int key_size = sizeof(int), align = 0;
  for (int i = 0; i < len; i++) {
    memcpy(page + addr, &key_size, sizeof(key_size));
    addr += sizeof(key_size);
    memcpy(page + addr, &key[i], sizeof(key[i]));
    addr += sizeof(key[i]);
    memcpy(page + addr, &value_size[i], sizeof(value_size[i]));
    addr += sizeof(value_size[i]);
    memcpy(page + addr, &value_size[i], value_size[i]);
    addr += value_size[i];
  }
  while (addr < BLOCK_SIZE) {
    memcpy(page + addr, &align, sizeof(align));
    addr += sizeof(align);
  }
  return page;
}

void ZNS_Simulation::request_workload() {
  int cap = 0, OP, key_read, value_size_read, cnt = 0;
  int key[MAX_KV_PER_BLOCK], value_size[MAX_KV_PER_BLOCK];
  int load = 0;

  ifstream inFile;
  inFile.open(FILE_PATH, ifstream::in);
  while (inFile.good()) {
    inFile >> OP >> key_read >> value_size_read;
    load++;
    if ((load * 100) % MAX_WORKLOAD == 0) {
      cout << (float)load / MAX_WORKLOAD * 100 << "%" << endl;
      if ((load * 10) % MAX_WORKLOAD == 0) print_info();
    }
    if (cap + sizeof(int) * 3 + value_size_read > BLOCK_SIZE ||
        OP == MODIFY_KV || OP == DELETE_KV) {
      if (cnt != 0) {
        char *pages2zns = data2page(key, value_size, cnt);
        myInsert(pages2zns, key, cnt);
        cnt = cap = 0;
      }
      if (OP == MODIFY_KV) {
        myUpdate_Delete(key_read, value_size_read);
      } else if (OP == DELETE_KV) {
        myUpdate_Delete(key_read, 0);
      }
    }
    if (OP == ADD_KV) {
      key[cnt] = key_read;
      value_size[cnt] = value_size_read;
      cnt++;
      cap += sizeof(int) * 3 + value_size_read;
    }
  }
}

float ZNS_Simulation::lifetimeVarience(int zone_id) {
  float sum = 0, avg = 0, cnt = 0;
  for (int i = 0; i < block_per_zone; i++)
    if (page_lifetime_map[zone_id * block_per_zone + i] > 0) {
      sum += page_lifetime_map[zone_id * block_per_zone + i];
      cnt++;
    }
  avg = sum / cnt;
  sum = 0;
  for (int i = 0; i < block_per_zone; i++)
    if (page_lifetime_map[zone_id * block_per_zone + i] > 0) {
      sum += pow(page_lifetime_map[zone_id * block_per_zone + i] - avg, 2);
    }
  sum /= cnt;
  return sum;
}

int ZNS_Simulation::get_page_lifetime(int *key, int len) {
  int lifetime = 0;
  for (int i = 0; i < len; i++) {
    if (lifetime < key_lifetime_map[key[i]])
      lifetime = key_lifetime_map[key[i]];
  }
  return lifetime;
}

void ZNS_Simulation::refreshLifetime(int zone_id, int *key, int len,
                                     int block_id, int pageLifetime) {
  int cnt = 0;
  for (int i = 0; i < len; i++) {
    key_lifetime_map[key[i]]++;
    key_index[key[i]] = block_id;
  }
  page_lifetime_map[block_id] = pageLifetime;
  zone_lifetime_map[zone_id] = -1;
  for (int i = 0; i < block_per_zone; i++) {
    if (page_lifetime_map[zone_id * block_per_zone + i] > 0 &&
        block_valid_map[zone_id * block_per_zone + i] == BLOCK_VALID) {
      zone_lifetime_map[zone_id] +=
          page_lifetime_map[zone_id * block_per_zone + i];
      cnt++;
    }
  }
  if (zone_lifetime_map[zone_id] != -1) zone_lifetime_map[zone_id] /= cnt;
}

void ZNS_Simulation::myGcDetect() {
  int gczone = 0;
  get_zone_empty_rate();
  get_zone_garbage_rate();
  for (int i = 0; i < (int)zone_number; i++) {
    if (gc_queue[i] == 0) {
      if (garbage_rate[i] > sg)
        gc_queue[i] = 1;
      else if (empty_rate[i] > se && lifetimeVarience(i) > sl)
        gc_queue[i] = 1;
    }
  }
  for (int j = 0; j < (int)zone_number; j++) gczone += gc_queue[j];
  if (gczone >= gl) myGarbageCollection();
}

void ZNS_Simulation::myInsert(char *page, int *key, int len) {
  int zone_in, szone = 0, block_id = 0;
  int pageLifetime = get_page_lifetime(key, len);
  for (int i = 0; i < zone_number; i++)
    if (zone_lifetime_map[i] > 0) szone++;
  for (int i = 0; i < zone_number; i++) {
    if (gc_queue[i] == 0) {
      zone_in = i;
      break;
    }
  }
  if (szone < sz * zone_number) {
    get_zone_empty_rate();
    float max = 0;
    for (int i = 0; i < zone_number; i++) {
      if (max < empty_rate[i]) {
        zone_in = i;
        max = empty_rate[i];
      }
    }
  } else {
    static int hhh = 0;
    cout << hhh++ << endl;
    float delta = 100000;
    for (int i = 0; i < zone_number; i++) {
      if (delta > fabs(pageLifetime - zone_lifetime_map[i]) &&
          gc_queue[i] == 0) {
        zone_in = i;
        delta = fabs(pageLifetime - zone_lifetime_map[i]);
      }
    }
  }
  int flag = write_block(page, zone_in, BLOCK_SIZE, &block_id);
  assert(flag == 1);
  refreshLifetime(zone_in, key, len, block_id, pageLifetime);
  myGcDetect();
}

void ZNS_Simulation::myUpdate_Delete(int key_op, int value_size_op) {
  int block_id, key_size_read, key_read, value_size_read, addr;
  unsigned long long offset;
  int ret;
  int fd = zns_sim->get_dev_id();
  char value[BLOCK_SIZE];
  char *buf = reinterpret_cast<char *>(memalign(4096, BLOCK_SIZE));

  block_id = key_index[key_op];
  offset = block_id * BLOCK_SIZE;
  addr = 0;
  ret = pread(fd, buf, BLOCK_SIZE, offset);
  assert(ret == BLOCK_SIZE);

  block_valid_map[block_id] = BLOCK_INVALID;
  page_lifetime_map[block_id] = -1;

  if (value_size_op == 0) key_index[key_op] = -1;

  int cap = 0, cnt = 0, flag = 0;
  int key[MAX_KV_PER_BLOCK], value_size[MAX_KV_PER_BLOCK];

  addr = 0;
  while (addr <= 4096 - (int)sizeof(int) * 4) {
    memcpy(&key_size_read, buf + addr, sizeof(int));
    if (key_size_read != sizeof(int) && cnt > 0) {
      char *pages2zns = data2page(key, value_size, cnt);
      myInsert(pages2zns, key, cnt);
      break;
    }
    addr += sizeof(int);

    memcpy(&key_read, buf + addr, key_size_read);
    addr += key_size_read;

    memcpy(&value_size_read, buf + addr, sizeof(int));
    addr += sizeof(int);

    memcpy(value, buf + addr, value_size_read);
    addr += value_size_read;

    if (key_read == key_op) {
      value_size_read = value_size_op;
      flag = 1;
    }

    if (cap + sizeof(int) * 3 + value_size_read > BLOCK_SIZE &&
        value_size_read != 0) {
      char *pages2zns = data2page(key, value_size, cnt);
      myInsert(pages2zns, key, cnt);
      cnt = cap = 0;
    }
    if (value_size_read != 0) {
      key[cnt] = key_read;
      value_size[cnt] = value_size_read;
      cnt++;
      cap += sizeof(int) * 3 + value_size_read;
    }
  }
  assert(flag == 1);
  free(buf);
}

// void ZNS_Simulation::GC_insert(char *page, int lifetime) {
//   int zone_in, szone = 0, block_id = 0;
//   for (int i = 0; i < zone_number; i++)
//     if (zone_lifetime_map[i] < 0) szone++;
//   for (int i = 0; i < zone_number; i++) {
//     if (gc_queue[i] == 0) {
//       zone_in = i;
//       break;
//     }
//   }
//   if (szone < sz * zone_number) {
//     get_zone_empty_rate();
//     float max = 0;
//     for (int i = 0; i < zone_number; i++) {
//       if (max < empty_rate[i]) {
//         zone_in = i;
//         max = empty_rate[i];
//       }
//     }
//   } else {
//     float delta = 100000;
//     for (int i = 0; i < zone_number; i++) {
//       if (delta > fabs(lifetime - zone_lifetime_map[i]) && gc_queue[i] == 0)
//       {
//         zone_in = i;
//         delta = fabs(lifetime - zone_lifetime_map[i]);
//       }
//     }
//   }
//   int flag = write_block(page, zone_in, BLOCK_SIZE, block_id);
//   assert(flag == 1);
//   page_lifetime_map[block_id] = lifetime;
//   int cnt = 0;
//   zone_lifetime_map[zone_in] = 0;
//   for (int i = 0; i < block_per_zone; i++) {
//     if (page_lifetime_map[zone_in * block_per_zone + i] > 0) {
//       zone_lifetime_map[zone_in] +=
//           page_lifetime_map[zone_in * block_per_zone + i];
//       cnt++;
//     }
//   }
//   zone_lifetime_map[zone_in] /= cnt;
// }

void ZNS_Simulation::myGarbageCollection() {
  cout << "hhhhhhhh" << endl;
  int ret, fd = zns_sim->get_dev_id();
  unsigned long long offset;
  for (int i = 0; i < zone_number; i++) {
    if (gc_queue[i] == 1) {
      for (int j = 0; j < block_per_zone; j++) {
        if (block_valid_map[i * block_per_zone + j] == BLOCK_VALID) {
          offset = (i * block_per_zone + j) * BLOCK_SIZE;
          char *buf = reinterpret_cast<char *>(memalign(4096, BLOCK_SIZE));
          assert(buf != nullptr);
          ret = pread(fd, &buf, BLOCK_SIZE, offset);
          assert(ret == sizeof(int));

          int cap = 0, cnt = 0, key_size_read, key_read, value_size_read;
          char value[BLOCK_SIZE];
          int key[MAX_KV_PER_BLOCK], value_size[MAX_KV_PER_BLOCK];

          int addr = 0;
          while (addr < 4096 - (int)sizeof(int) * 4) {
            memcpy(&key_size_read, buf + addr, sizeof(int));
            if (key_size_read != sizeof(int) && cnt > 0) {
              char *pages2zns = data2page(key, value_size, cnt);
              myInsert(pages2zns, key, cnt);
              break;
            }
            addr += sizeof(int);

            memcpy(&key_read, buf + addr, key_size_read);
            addr += key_size_read;

            memcpy(&value_size_read, buf + addr, sizeof(int));
            addr += sizeof(int);

            memcpy(value, buf + addr, value_size_read);
            addr += value_size_read;

            key[cnt] = key_read;
            value_size[cnt] = value_size_read;
            cnt++;
            cap += sizeof(int) * 3 + value_size_read;
          }
        }
      }
      zone_sim[i].reset_zone();
      zone_lifetime_map[i] = -1;
      for (int j = 0; j < block_per_zone; j++) {
        page_lifetime_map[i * block_per_zone + j] = -1;
        block_valid_map[i * block_per_zone + j] = BLOCK_EMPTY;
      }
    }
  }
}

void ZNS_Simulation::print_info() {
  get_zone_empty_rate();
  get_zone_garbage_rate();
  cout << "\tzone\tempty rate\tgarbage rate\tzone lifetime\t" << endl;
  for (int i = 0; i < zone_number; i++)
    cout << "\t" << i << "\t" << empty_rate[i] << "\t" << garbage_rate[i]
         << "\t" << zone_lifetime_map[i] << "\t" << endl;
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