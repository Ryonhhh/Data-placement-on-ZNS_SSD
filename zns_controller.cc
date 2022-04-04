#include "zns_controller.h"
#define device "/dev/nullb0"
using namespace std;

void BLOCK_SIM::set_block(unsigned long long block_id, int block_size) {
  this->block_id = block_id;
  this->start = block_id * block_size;
}

unsigned long long BLOCK_SIM::get_block_start(){
  return this->start;
}

void ZONE_SIM::set_zone(int fd, int zone_id, unsigned long long capacity) {
  struct zbd_zone info;
  unsigned int nr_zones;
  int flag = zbd_report_zones(fd, zone_id * capacity, capacity, ZBD_RO_ALL,
                              &info, &nr_zones);
  assert(flag == 0);

  this->fd = fd;
  this->zone_id = zone_id;
  this->start = zone_id * capacity;
  this->capacity = capacity;
  this->type = info.type;

  reset_zone();
}

void ZONE_SIM::print_zone_info() {
  struct zbd_zone info;
  unsigned int nr_zones = 0;
  int flag =
      zbd_report_zones(fd, start, capacity, ZBD_RO_ALL, &info, &nr_zones);
  assert(flag == 0);

  std::cout << " zone_id:  " << zone_id << std::endl;
  std::cout << " start:    " << info.start << std::endl;
  std::cout << " capacity: " << info.capacity << std::endl;
  std::cout << " wp:       " << info.wp << std::endl;
  std::cout << " flags:    " << info.flags << std::endl;
  std::cout << " type:     " << info.type << std::endl;
  std::cout << " cond:     " << info.cond << std::endl;
}

int ZONE_SIM::get_zone_id() { return zone_id; }

unsigned long long ZONE_SIM::get_zone_start() { return start; }

unsigned long long ZONE_SIM::get_zone_capacity() { return capacity; }

unsigned int ZONE_SIM::get_zone_type() { return type; }

unsigned long long ZONE_SIM::get_zone_wp() {
  struct zbd_zone info;
  unsigned int nr_zones = 0;
  int flag =
      zbd_report_zones(fd, start, capacity, ZBD_RO_ALL, &info, &nr_zones);
  assert(flag == 0);
  return info.wp;
}

unsigned int ZONE_SIM::get_zone_flags() {
  struct zbd_zone info;
  unsigned int nr_zones = 0;
  int flag =
      zbd_report_zones(fd, start, capacity, ZBD_RO_ALL, &info, &nr_zones);
  assert(flag == 0);
  return info.flags;
}

unsigned int ZONE_SIM::get_zone_cond() {
  struct zbd_zone info;
  unsigned int nr_zones = 1;
  int flag =
      zbd_report_zones(fd, start, capacity, ZBD_RO_ALL, &info, &nr_zones);
  assert(flag == 0);
  return info.cond;
}

void ZONE_SIM::print_zone_cond() {
  unsigned int condition = get_zone_cond();
  cout << "zone " << get_zone_id() << " condition " << condition << ":";
  switch (condition) {
    case ZBD_ZONE_COND_NOT_WP:
      cout << "NO WP CONV" << endl;
      break;
    case ZBD_ZONE_COND_EMPTY:
      cout << "EMPTY" << endl;
      break;
    case ZBD_ZONE_COND_IMP_OPEN:
      cout << "IMPLICITLY OPENED" << endl;
      break;
    case ZBD_ZONE_COND_EXP_OPEN:
      cout << "EXPLICITY OPENED" << endl;
      break;
    case ZBD_ZONE_COND_CLOSED:
      cout << "EXPLICITY CLOSED" << endl;
      break;
    case ZBD_ZONE_COND_FULL:
      cout << "FULL" << endl;
      break;
    case ZBD_ZONE_COND_READONLY:
      cout << "READONLY" << endl;
      break;
    case ZBD_ZONE_COND_OFFLINE:
      cout << "OFFLINE" << endl;
      break;
    default:
      cout << "condition error" << endl;
  }
}

void ZONE_SIM::open_zone() {
  int flag = zbd_open_zones(fd, start, capacity);
  assert(flag == 0);
}

void ZONE_SIM::close_zone() {
  int flag = zbd_close_zones(fd, start, capacity);
  assert(flag == 0);
}

void ZONE_SIM::reset_zone() {
  int flag = zbd_reset_zones(fd, start, capacity);
  assert(flag == 0);
}

void ZONE_SIM::finish_zone() {
  int flag = zbd_finish_zones(fd, start, capacity);
  assert(flag == 0);
}

float ZONE_SIM::get_empty_rate() {
  return (float)(get_zone_wp() - start) / (float)capacity;
}

ZNS_SIM::ZNS_SIM() {
  this->info = (struct zbd_info*)malloc(sizeof(struct zbd_info));
  if ((this->dev_id = zbd_open(device, O_RDWR | O_DIRECT | O_LARGEFILE,
                               this->info)) == -1) {
    printf("can't open device!");
    exit(0);
  }
  // Direct disk operation indicates that each read/write operation does not use
  // the cache provided by the kernel and directly reads/writes the disk device
  cout << "open device successfully!" << endl;
}

unsigned long long ZNS_SIM::get_zone_size() { return this->info->zone_size; }

unsigned int ZNS_SIM::get_block_size() { return this->info->lblock_size; }

unsigned int ZNS_SIM::get_zone_number() { return this->info->nr_zones; }

unsigned long long ZNS_SIM::get_block_number() {
  return this->info->nr_lblocks;
}

int ZNS_SIM::get_dev_id() { return this->dev_id; }

void ZNS_SIM::print_zns_info() {
  zbd_get_info(this->dev_id, this->info);
  // std::cout << " vendor_id: " << this->info->vendor_id << std::endl;
  // std::cout << " nr_sectors: " << this->info->nr_sectors << std::endl;
  std::cout << " nr_lblocks: " << this->info->nr_lblocks << std::endl;
  // std::cout << " nr_pblocks: " << this->info->nr_pblocks << std::endl;
  std::cout << " zone_size: " << this->info->zone_size << std::endl;
  // std::cout << " zone_sectors: " << this->info->zone_sectors << std::endl;
  std::cout << " lblock_size: " << this->info->lblock_size << std::endl;
  // std::cout << " pblock_size: " << this->info->pblock_size << std::endl;
  std::cout << " nr_zones: " << this->info->nr_zones << std::endl;
  std::cout << " max_nr_open_zones: " << this->info->max_nr_open_zones
            << std::endl;
  std::cout << " max_nr_active_zones: " << this->info->max_nr_active_zones
            << std::endl;
  // std::cout << " model: " << this->info->model << std::endl;
}

void ZNS_SIM::close_zns_dev() { zbd_close(this->dev_id); }
