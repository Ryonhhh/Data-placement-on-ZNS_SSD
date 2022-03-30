#include"zns_controller.h"
#define device "/dev/nullb0"
using namespace std;

void BLOCK_sim::set_block(unsigned long long block_id, int block_size){
    this->block_id = block_id;
    this->start = block_id * block_size;
    this->status = 0;
}

int BLOCK_sim::get_block_status(){
    return this->status;
}

void BLOCK_sim::set_block_status(int status){
    this->status = status;
}

void ZONE_sim::set_zone(int fd, int zone_id, unsigned long long capacity){
    struct zbd_zone info;
    unsigned int *nr_zones;
    int flag = zbd_report_zones(fd, zone_id * capacity, capacity, ZBD_RO_ALL, &info, nr_zones);
    assert(flag == 0);
    
    this->fd = fd;
    this->zone_id = zone_id;
    this->start = zone_id * capacity;
    this->capacity = capacity;
    this->type = info.type;
}

void ZONE_sim::print_zone_info(){
    struct zbd_zone info;
    unsigned int *nr_zones;
    cout<<"ok"<<endl;
    int flag = zbd_report_zones(fd, start, capacity, ZBD_RO_ALL, &info, nr_zones);
    assert(flag == 0);

    std::cout<<" zone_id:  " <<zone_id       <<std::endl;
    std::cout<<" start:    " <<info.start    <<std::endl;
    std::cout<<" capacity: " <<info.capacity <<std::endl;
    std::cout<<" wp:       " <<info.wp       <<std::endl;
    std::cout<<" flags:    " <<info.flags    <<std::endl;
    std::cout<<" type:     " <<info.type     <<std::endl;
    std::cout<<" cond:     " <<info.cond     <<std::endl;

}

int ZONE_sim::get_zone_id(){
    return zone_id;
}

unsigned long long ZONE_sim::get_zone_start(){
    return start;
}

unsigned long long ZONE_sim::get_zone_capacity(){
    return capacity;
}

unsigned int ZONE_sim::get_zone_type(){
    return type;
}

unsigned long long ZONE_sim::get_zone_wp(){
    struct zbd_zone *info=(struct zbd_zone *)malloc(sizeof(struct zbd_zone));
    unsigned int *nr_zones; 
    int flag = zbd_list_zones(fd, (off_t)start, (off_t)capacity, ZBD_RO_ALL, &info, nr_zones);
    assert(flag == 0);
    return (*info).wp;
}

unsigned int ZONE_sim::get_zone_status(){
    struct zbd_zone *info=(struct zbd_zone *)malloc(sizeof(struct zbd_zone));
    unsigned int *nr_zones; 
    int flag = zbd_list_zones(fd, (off_t)start, (off_t)capacity, ZBD_RO_ALL, &info, nr_zones);
    assert(flag == 0);
    return (*info).flags;
}   

unsigned int ZONE_sim::get_zone_cond(){
    struct zbd_zone *info=(struct zbd_zone *)malloc(sizeof(struct zbd_zone));
    unsigned int *nr_zones; 
    int flag = zbd_list_zones(fd, (off_t)start, (off_t)capacity, ZBD_RO_ALL, &info, nr_zones);
    assert(flag == 0);
    return (*info).cond;
}

void ZONE_sim::open_zone(){
    int flag = zbd_open_zones(fd, start, capacity);
    assert(flag == 0);
}

void ZONE_sim::close_zone(){
    int flag = zbd_close_zones(fd, start, capacity);
    assert(flag == 0);
}

void ZONE_sim::reset_zone(){
    int flag = zbd_reset_zones(fd, start, capacity);
    assert(flag == 0);
}

void ZONE_sim::finish_zone(){
    int flag = zbd_finish_zones(fd, start, capacity);
    assert(flag == 0);
}

float ZONE_sim::get_empty_rate(){
    return (float)(get_zone_wp() - start) / (float)capacity;
}

/*
float ZONE_sim::get_garbage_rate(){

}
*/



ZNS_sim::ZNS_sim(){
    this->info = (struct zbd_info*)malloc(sizeof(struct zbd_info));
    if((this->dev_id = zbd_open(device, O_RDWR, this->info))==-1){
            printf("can't open device!");
            exit(0);
    }
    cout<<"open device successfully!"<<endl;
}

unsigned long long ZNS_sim::get_zone_size(){
    return this->info->zone_size;
}

unsigned int ZNS_sim::get_block_size(){
    return this->info->nr_lblocks;
}

unsigned int ZNS_sim::get_zone_number(){
    return this->info->nr_zones;
}

int ZNS_sim::get_dev_id(){
    return this->dev_id;
}

void ZNS_sim::print_zns_info(){
    zbd_get_info(this->dev_id, this->info);
    //std::cout <<" vendor_id: "           <<this->info->vendor_id           <<std::endl;
    //std::cout <<" nr_sectors: "          <<this->info->nr_sectors          <<std::endl;
    //std::cout <<" nr_lblocks: "          <<this->info->nr_lblocks          <<std::endl;
    std::cout   <<" nr_pblocks: "          <<this->info->nr_pblocks          <<std::endl;
    std::cout   <<" zone_size: "           <<this->info->zone_size           <<std::endl;
    //std::cout <<" zone_sectors: "        <<this->info->zone_sectors        <<std::endl;
    std::cout   <<" lblock_size: "         <<this->info->lblock_size         <<std::endl;
    std::cout   <<" pblock_size: "         <<this->info->pblock_size         <<std::endl;
    std::cout   <<" nr_zones: "            <<this->info->nr_zones            <<std::endl;
    std::cout   <<" max_nr_open_zones: "   <<this->info->max_nr_open_zones   <<std::endl;
    std::cout   <<" max_nr_active_zones: " <<this->info->max_nr_active_zones <<std::endl;
    //std::cout <<" model: "               <<this->info->model               <<std::endl;
}

void ZNS_sim::close_zns_dev(){
    zbd_close(this->dev_id);
}

