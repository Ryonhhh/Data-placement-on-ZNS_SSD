#include <iostream>

#include "zns_simulation.h"

using namespace std;

int main() {
  ZNS_Simulation zns_simulation;
  cout << "start to initialize ZNS_SSD" << endl;
  zns_simulation.initialize();
  cout << "finish initialize!" << endl;
  cout << "creating workload..." << endl;
  zns_simulation.generate_workload(RAM);
  cout << "finish generating workload!" << endl;
  //zns_simulation.test();
  return 0;
}