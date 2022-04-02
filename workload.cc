#include "workload.h"

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>

#include "parameter.h"
using namespace std;

Workload_Creator::Workload_Creator(int length) {
  int seed = time(0);
  srand(seed);
  ofstream outFile;
  outFile.open(FILE_PATH, ios::out);
  seq_or_ram = 0;
  for (int i = 0; i < length; i++)
    outFile << std::to_string(ADD_KV) << ',' << std::to_string(rand() % MAX_KEY)
            << ',' << std::to_string(((rand() % MAX_VALUE_SIZE / 4) + 1) * 4)
            << endl;
  outFile.close();
}

Workload_Creator::Workload_Creator(int length, float hot_rate) {
  int seed = time(0), key, OP;
  int bitmap[MAX_KEY] = {};
  srand(seed);
  ofstream outFile;
  outFile.open(FILE_PATH, ios::out);
  seq_or_ram = 1;
  for (int i = 0; i < length; i++) {
    key = rand() % MAX_KEY;
    if (bitmap[key] == 0) {
      bitmap[key] = 1;
      OP = ADD_KV;
    } else {
      if (rand() % 3 == 0) {
        OP = DELETE_KV;
        bitmap[i] = 0;
      } else
        OP = MODIFY_KV;
    }
    outFile << std::to_string(OP) << ',' << std::to_string(rand() % MAX_KEY)
            << ',' << std::to_string(((rand() % MAX_VALUE_SIZE / 4) + 1) * 4)
            << endl;
  }
  outFile.close();
}