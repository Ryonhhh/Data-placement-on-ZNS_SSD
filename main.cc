#include"zns_controller.h"
#include<iostream>
using namespace std;

int main(){
    ZNS_sim *zns_sim;
    zns_sim = new ZNS_sim();
    zns_sim->open_zns_dev();
    zns_sim->print_zns_info();
    zns_sim->close_zns_dev();
    return 0;
}