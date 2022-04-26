#include "zns_simulation.h"

#include <string.h>

#include <cmath>
#include <fstream>
#include <iomanip>
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
  gc_flag = 0;
  gc_num = 0;

  zone_number = zns_sim->get_zone_number();
  block_number = zns_sim->get_block_number();
  block_per_zone = block_number / zone_number;

  block_valid_map = (int *)malloc(sizeof(int) * block_number);
  block_lifetime_map = (int *)malloc(sizeof(int) * block_number);

  key_index = (unsigned long long *)malloc(sizeof(unsigned long long) * MAX_KEY);
  key_lifetime_map = (int *)malloc(sizeof(int) * MAX_KEY);

  zone_conds = (unsigned int *)malloc(sizeof(unsigned int) * zone_number);
  gc_queue = (int *)malloc(sizeof(int) * zone_number);
  zone_lifetime_map = (float *)malloc(sizeof(float) * zone_number);
  empty_rate = (float *)malloc(sizeof(float) * zone_number);
  garbage_rate = (float *)malloc(sizeof(float) * zone_number);

  for (int i = 0; i < MAX_KEY; i++) {
    key_lifetime_map[i] = RECENT_MODIFY;
    key_index[i] = -1;
  }
  for (int i = 0; i < (int)block_number; i++) {
    block_valid_map[i] = BLOCK_EMPTY;
    block_lifetime_map[i] = -1;
  }
  for (int i = 0; i < zone_number; i++) {
    zone_lifetime_map[i] = 1000;
    empty_rate[i] = 1;
    garbage_rate[i] = 0;
    gc_queue[i] = 0;
  }
}

void ZNS_Simulation::generate_workload(int seq_ram) {
  if (seq_ram == 0)
    workload = new Workload_Creator(MAX_WORKLOAD);
  else
    workload = new Workload_Creator(MAX_WORKLOAD, HOT_DATA_RATE, UPDATE_RATE);
}

int ZNS_Simulation::write_block(char *page_in, int zone_id, int size,
                                int *block_id) {
  int cond = zone_sim[zone_id].get_zone_cond();
  if (cond != ZBD_ZONE_COND_EMPTY && cond != ZBD_ZONE_COND_EXP_OPEN &&
      cond != ZBD_ZONE_COND_IMP_OPEN) {
    cout << "zone " << zone_id << " is not writeable!" << endl;
    zone_sim[zone_id].print_zone_cond();
    print_info();
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

void ZNS_Simulation::get_zone_conds() {
  for (int i = 0; i < (int)zone_number; i++)
    zone_conds[i] = zone_sim[i].get_zone_cond();
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

void ZNS_Simulation::request_workload(clock_t start) {
  int cap = 0, OP, key_read, value_size_read, cnt = 0;
  int key[MAX_KV_PER_BLOCK], value_size[MAX_KV_PER_BLOCK];
  int load = 0;
  char OUT_PATH[] = "./aware_nolifetime";
  clock_t period;

  ifstream inFile;
  ofstream outFile;
  outFile.open(OUT_PATH, ios::out);
  inFile.open(FILE_PATH, ifstream::in);
  while (inFile.good()) {
    inFile >> OP >> key_read >> value_size_read;
    load++;
    if ((load * 1000) % (MAX_WORKLOAD * WORKLOAD_SHIFT) == 0) {
      period = clock();
      double t = (period - start) / CLOCKS_PER_SEC;
      cout << (float)load / (MAX_WORKLOAD * WORKLOAD_SHIFT) * 100 << "% " << t
           << "s" << endl;
      outFile << (float)load / (MAX_WORKLOAD * WORKLOAD_SHIFT) * 100 << "% "
              << t << "s" << endl;
      if ((load * 10) % (MAX_WORKLOAD * WORKLOAD_SHIFT) == 0) {
        print_info();
        get_zone_empty_rate();
        get_zone_garbage_rate();
        outFile << left;
        outFile << setw(6) << "zone" << setw(10) << "em %" << setw(10) << "gb %"
                << setw(10) << "zone lt" << setw(10) << "zone var" << endl;
        for (int i = 0; i < zone_number; i++) {
          outFile << setprecision(3) << fixed;
          outFile << setw(6) << i << setw(10) << empty_rate[i] << setw(10)
                  << garbage_rate[i] << setw(10) << zone_lifetime_map[i]
                  << setw(10) << lifetimeVarience(i) << endl;
        }
      }
    }
    if (cap + sizeof(int) * 3 + value_size_read > BLOCK_SIZE ||
        OP == MODIFY_KV || OP == DELETE_KV) {
      if (cnt != 0) {
        char *pages2zns = data2page(key, value_size, cnt);
        myInsert(pages2zns, key, cnt, ADD_KV);
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
  float sum = 0, avg = 0;
  int cnt = 0;
  for (int i = 0; i < block_per_zone; i++)
    if (block_lifetime_map[zone_id * block_per_zone + i] > 0) {
      sum += block_lifetime_map[zone_id * block_per_zone + i];
      cnt++;
    }
  avg = sum / cnt;
  sum = 0;
  for (int i = 0; i < block_per_zone; i++)
    if (block_lifetime_map[zone_id * block_per_zone + i] > 0) {
      sum += pow(block_lifetime_map[zone_id * block_per_zone + i] - avg, 2);
    }
  sum /= cnt;
  return sum;
}

int ZNS_Simulation::get_page_lifetime(int *key, int len) {
  int lifetime = RECENT_MODIFY;
  for (int i = 0; i < len; i++) {
    if (lifetime > key_lifetime_map[key[i]])
      lifetime = key_lifetime_map[key[i]];
  }
  return lifetime;
}

void ZNS_Simulation::refreshLifetime(int zone_id, int *key, int len,
                                     int block_id, int pageLifetime, int OP) {
  int cnt = 0;
  block_valid_map[block_id] = BLOCK_VALID;
  if (OP != ADD_KV) {
    for (int j = 0; j < MAX_KEY; j++)
      if (key_lifetime_map[j] != RECENT_MODIFY) key_lifetime_map[j]++;
  }
  for (int i = 0; i < len; i++) {
    key_index[key[i]] = block_id;
  }
  block_lifetime_map[block_id] = pageLifetime;
  zone_lifetime_map[zone_id] = 0;
  for (int i = 0; i < block_per_zone; i++) {
    if (block_lifetime_map[zone_id * block_per_zone + i] >= 0 &&
        block_valid_map[zone_id * block_per_zone + i] == BLOCK_VALID) {
      zone_lifetime_map[zone_id] +=
          block_lifetime_map[zone_id * block_per_zone + i];
      cnt++;
    }
  }
  zone_lifetime_map[zone_id] /= cnt;
}

void ZNS_Simulation::myGcDetect() {
  int gczone;
  get_zone_empty_rate();
  get_zone_garbage_rate();
  for (int i = 0; i < (int)zone_number; i++) {
    if (gc_queue[i] == 0 && empty_rate[i] < se &&
        //(garbage_rate[i] > sg || lifetimeVarience(i) > sl) && gc_flag == 0) {
        (garbage_rate[i] > sg || lifetimeVarience(i) < 0) && gc_flag == 0) {  
      gc_queue[i] = 1;
      gczone = 0;
      for (int j = 0; j < (int)zone_number; j++) gczone += gc_queue[j];
      if (gczone >= gl && gc_flag == 0) {
        myGarbageCollection();
        break;
      }
    }
  }
}

void ZNS_Simulation::myInsert(char *page, int *key, int len, int OP) {
  int zone_in, szone = 0, block_id = 0;
  int pageLifetime = get_page_lifetime(key, len);
  int *can_zone = (int *)malloc(sizeof(int) * zone_number);
  get_zone_conds();

  for (int i = 0; i < zone_number; i++)
    if (fabs(zone_lifetime_map[i] - 1000) > 1) szone++;
  for (int i = 0; i < zone_number; i++) {
    if (gc_queue[i] == 0 && zone_conds[i] != ZBD_ZONE_COND_FULL) {
      zone_in = i;
      break;
    }
  }
  if (szone < sz * zone_number) {
    get_zone_empty_rate();
    float max_er = 0;
    for (int i = 0; i < zone_number; i++) {
      if (max_er < empty_rate[i] && gc_queue[i] == 0 &&
          zone_conds[i] != ZBD_ZONE_COND_FULL) {
        zone_in = i;
        max_er = empty_rate[i];
      }
    }
  } else {
    for (int j = 0; j < gl; j++) {
      float delta = 100000;
      for (int i = 0; i < zone_number; i++) {
        if (delta > fabs(pageLifetime - zone_lifetime_map[i]) &&
            can_zone[i] == 0 && gc_queue[i] == 0 &&
            zone_conds[i] != ZBD_ZONE_COND_FULL) {
          zone_in = i;
          delta = fabs(pageLifetime - zone_lifetime_map[i]);
        }
      }
      can_zone[zone_in] = 1;
    }
    float max_er = 0;
    for (int j = 0; j < zone_number; j++) {
      if (can_zone[j] == 1 && empty_rate[j] > max_er) {
        max_er = empty_rate[j];
        zone_in = j;
      }
    }
  }
  
  int flag = write_block(page, zone_in, BLOCK_SIZE, &block_id);
  assert(flag == 1);
  refreshLifetime(zone_in, key, len, block_id, pageLifetime, OP);
  if (gc_flag == 0) myGcDetect();

  /*---polling distribution---*/
  // int block_id = 0;
  // static int zone_in = zone_number;
  // zone_in = (zone_in + 1) % zone_number;
  // get_zone_conds();

  // int pageLifetime = get_page_lifetime(key, len);
  // while (gc_queue[zone_in] != 0 || zone_conds[zone_in] == ZBD_ZONE_COND_FULL)
  //   zone_in = (zone_in + 1) % zone_number;

  // int flag = write_block(page, zone_in, BLOCK_SIZE, &block_id);
  // assert(flag == 1);
  // refreshLifetime(zone_in, key, len, block_id, pageLifetime, OP);
  // if (gc_flag == 0) myGcDetect();

  /*---random distribution---*/
  // int block_id = 0;
  // int zone_in = rand() % zone_number;
  // get_zone_conds();

  // int pageLifetime = get_page_lifetime(key, len);
  // while (gc_queue[zone_in] != 0 || zone_conds[zone_in] == ZBD_ZONE_COND_FULL)
  //   zone_in = rand() % zone_number;

  // int flag = write_block(page, zone_in, BLOCK_SIZE, &block_id);
  // assert(flag == 1);
  // refreshLifetime(zone_in, key, len, block_id, pageLifetime, OP);
  // if (gc_flag == 0) myGcDetect();

  /*---equal distribution---*/
  // int block_id = 0;
  // int zone_in = rand() % zone_number;
  // get_zone_conds();
  // get_zone_empty_rate();

  // int pageLifetime = get_page_lifetime(key, len);
  // float max = 0;
  // for (int i = 0; i < zone_number; i++)
  //   if (gc_queue[i] == 0 && zone_conds[i] != ZBD_ZONE_COND_FULL &&
  //       max < empty_rate[i]) {
  //     zone_in = i;
  //     max = empty_rate[i];
  //   }

  // int flag = write_block(page, zone_in, BLOCK_SIZE, &block_id);
  // assert(flag == 1);
  // refreshLifetime(zone_in, key, len, block_id, pageLifetime, OP);
  // if (gc_flag == 0) myGcDetect();

}

void ZNS_Simulation::myUpdate_Delete(int key_op, int value_size_op) {
  int key_size_read, key_read, value_size_read, addr;
  unsigned long long offset, block_id;
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
  block_lifetime_map[block_id] = -1;

  if (value_size_op == 0) key_index[key_op] = -1;

  int cap = 0, cnt = 0, flag = 0;
  int key[MAX_KV_PER_BLOCK] = {}, value_size[MAX_KV_PER_BLOCK] = {};

  addr = 0;
  while (addr <= 4096) {
    memcpy(&key_size_read, buf + addr, sizeof(int));
    if ((key_size_read != sizeof(int) && cnt > 0) ||
        (addr > 4096 - (int)sizeof(int) * 4)) {
      char *pages2zns = data2page(key, value_size, cnt);
      myInsert(pages2zns, key, cnt, MODIFY_KV);
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
      key_lifetime_map[key_op] = 0;
      flag = 1;
    }

    if (cap + sizeof(int) * 3 + value_size_read > BLOCK_SIZE &&
        value_size_read != 0) {
      char *pages2zns = data2page(key, value_size, cnt);
      myInsert(pages2zns, key, cnt, MODIFY_KV);
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

void ZNS_Simulation::myGarbageCollection() {
  gc_num++;
  cout << "garbage collection " << gc_num << ": ";
  for (int i = 0; i < zone_number; i++)
    if (gc_queue[i] == 1) cout << "zone " << i << " ";
  cout << endl;
  print_info();
  gc_flag = 1;
  int ret, fd = zns_sim->get_dev_id(), block_ofst = 0;
  unsigned long long offset;
  for (int i = 0; i < zone_number; i++) {
    if (gc_queue[i] == 1) {
      block_ofst = 0;
      while (block_valid_map[i * block_per_zone + block_ofst] != BLOCK_VALID)
        block_ofst++;
      offset = (i * block_per_zone + block_ofst) * BLOCK_SIZE;
      char *buf = reinterpret_cast<char *>(memalign(4096, BLOCK_SIZE));
      assert(buf != nullptr);
      ret = pread(fd, buf, BLOCK_SIZE, offset);
      assert(ret == BLOCK_SIZE);

      int cap = 0, cnt = 0, key_size_read, key_read, value_size_read;
      char value[BLOCK_SIZE];
      int key[MAX_KV_PER_BLOCK], value_size[MAX_KV_PER_BLOCK];

      int addr = 0;
      while (1) {
        memcpy(&key_size_read, buf + addr, sizeof(int));
        if ((key_size_read != sizeof(int)) ||
            (addr > 4096 - (int)sizeof(int) * 4)) {
          block_ofst++;
          while (block_valid_map[i * block_per_zone + block_ofst] !=
                 BLOCK_VALID)
            block_ofst++;
          if (block_ofst >= block_per_zone) {
            if (cnt != 0) {
              char *pages2zns = data2page(key, value_size, cnt);
              myInsert(pages2zns, key, cnt, MODIFY_KV);
              cnt = cap = 0;
            }
            break;
          }
          offset = (i * block_per_zone + block_ofst) * BLOCK_SIZE;
          free(buf);
          buf = reinterpret_cast<char *>(memalign(4096, BLOCK_SIZE));
          assert(buf != nullptr);
          ret = pread(fd, buf, BLOCK_SIZE, offset);
          assert(ret == BLOCK_SIZE);
          addr = 0;
          memcpy(&key_size_read, buf + addr, sizeof(int));
        }
        addr += sizeof(int);

        memcpy(&key_read, buf + addr, key_size_read);
        addr += key_size_read;

        memcpy(&value_size_read, buf + addr, sizeof(int));
        addr += sizeof(int);

        memcpy(value, buf + addr, value_size_read);
        addr += value_size_read;

        if (cap + sizeof(int) * 3 + value_size_read > BLOCK_SIZE &&
            value_size_read != 0) {
          char *pages2zns = data2page(key, value_size, cnt);
          myInsert(pages2zns, key, cnt, MODIFY_KV);
          cnt = cap = 0;
        }
        key[cnt] = key_read;
        value_size[cnt] = value_size_read;
        cnt++;
        cap += sizeof(int) * 3 + value_size_read;
      }
      gc_queue[i] = 0;
      zone_sim[i].reset_zone();
      zone_lifetime_map[i] = 0;
      for (int j = 0; j < block_per_zone; j++) {
        block_lifetime_map[i * block_per_zone + j] = -1;
        block_valid_map[i * block_per_zone + j] = BLOCK_EMPTY;
      }
    }
  }
  gc_flag = 0;
}

void ZNS_Simulation::print_info() {
  get_zone_empty_rate();
  get_zone_garbage_rate();
  cout << left;
  cout << setw(6) << "zone" << setw(10) << "em %" << setw(10) << "gb %"
       << setw(10) << "zone lt" << setw(10) << "zone var" << endl;
  for (int i = 0; i < zone_number; i++) {
    cout << setprecision(3) << fixed;
    cout << setw(6) << i << setw(10) << empty_rate[i] << setw(10)
         << garbage_rate[i] << setw(10) << zone_lifetime_map[i] << setw(10)
         << lifetimeVarience(i) << endl;
  }
}