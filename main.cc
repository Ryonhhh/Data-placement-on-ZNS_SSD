#include <ctime>
#include <iostream>

#include "compare.h"
#include "zns_simulation.h"

using namespace std;

clock_t start_t, end_t;

int main() {
  ZNS_Simulation zns_simulation;
  cout << "start to initialize ZNS_SSD" << endl;
  zns_simulation.initialize();
  cout << "finish initialize!" << endl;
  // cout << "creating workload..." << endl;
  // zns_simulation.generate_workload(RAM);
  // cout << "finish generating workload!" << endl;
  cout << "start simulation..." << endl;
  start_t = clock();
  zns_simulation.request_workload(start_t);
  end_t = clock();
  cout << "finish  simulation!" << endl;
  double t = (end_t - start_t) / CLOCKS_PER_SEC;
  cout << "running time: " << t << "s" << endl;

  // Compare compare;
  // cout << "start to initialize ZNS_SSD" << endl;
  // compare.initialize();
  // cout << "finish initialize!" << endl;
  // cout << "creating workload..." << endl;
  // compare.generate_workload(RAM);
  // cout << "finish generating workload!" << endl;
  // cout << "start simulation..." << endl;
  // start_t = clock();
  // compare.request_workload();
  // end_t = clock();
  // cout << "finish  simulation!" << endl;
  // double t=(end_t-start_t)/CLOCKS_PER_SEC;
  // cout<<"running time: " <<t<<"s"<<endl;

  return 0;
}