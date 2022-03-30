#include"zns_controller.h"
#include"parameter.h"
#include<iostream>
using namespace std;

int main(){
    ZNS_sim *zns_sim;
    zns_sim = new ZNS_sim();
    //zns_sim->print_zns_info();

    ZONE_sim zone_arr[ZONE_NUMBER];
    for(int i = 0; i < zns_sim->get_zone_number(); i++)
        zone_arr[i].set_zone(zns_sim->get_dev_id(), i, zns_sim->get_zone_size());
    
    //zone_arr[0].print_zone_info();cout<<endl;
    //zone_arr[0].print_zone_info();cout<<endl;
    zone_arr[0].reset_zone();
    zone_arr[0].open_zone();
    //zone_arr[0].print_zone_info();cout<<endl;
    zone_arr[0].finish_zone();
    zone_arr[0].print_zone_info();cout<<endl;
    zone_arr[0].reset_zone();

    zns_sim->close_zns_dev();
    return 0;
}