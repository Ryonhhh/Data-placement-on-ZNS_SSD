#include"zns_controller.h"
#define device "/dev/nullb0"
using namespace std;

void ZNS_sim::open_zns_dev(){
    info = (struct zbd_info*)malloc(sizeof(struct zbd_info));
    if((this->dev_id=zbd_open(device, ZBD_RO_RDONLY, this->info))==-1){
            printf("can't open device!");
            exit(0);
    }
}

void ZNS_sim::print_zns_info(){
    printf("vendor_id:\t%s\n", this->info->vendor_id);
    //printf("nr_sectors:\t%lld\n", this->info->nr_sectors);
    //printf("nr_lblocks:\t%lld\n", this->info->nr_lblocks);
    printf("nr_pblocks:\t%lld\n", this->info->nr_pblocks);
    printf("zone_size:\t%lld\n", this->info->zone_size);
    printf("zone_sectors:\t%d\n", this->info->zone_sectors);
    printf("lblock_size:\t%d\n", this->info->lblock_size);
    printf("pblock_size:\t%d\n", this->info->pblock_size);
    printf("nr_zones:\t%d\n", this->info->nr_zones);
    printf("max_nr_open_zones:\t%d\n", this->info->max_nr_open_zones);
    printf("max_nr_active_zones:\t%d\n", this->info->max_nr_active_zones);
    printf("model:\t%d\n", this->info->model);
}

void ZNS_sim::close_zns_dev(){
    zbd_close(this->dev_id);
}

