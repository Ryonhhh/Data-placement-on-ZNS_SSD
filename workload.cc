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

Workload_Creator::Workload_Creator(int length, float hot_data_rate,
                                   float update_rate) {
  int seed = time(0), key, OP, cnt1 = 0, cnt2 = 0, cnt3 = 0;
  int bitmap[MAX_KEY] = {};
  int hot_data = (int)(MAX_KEY * hot_data_rate);
  srand(seed);
  ofstream outFile;
  outFile.open(FILE_PATH, ios::out);
  seq_or_ram = 1;
  for (int i = 0; i < hot_data * 0.1; i++) {
    key = rand() % MAX_KEY;
    while (bitmap[key] != 0) key = rand() % MAX_KEY;
    bitmap[key] = 1;
    OP = ADD_KV;
    outFile << OP << ' ' << key << ' '
            << ((rand() % MAX_VALUE_SIZE / 4) + 1) * 4 << endl;
  }
  for (int k = 0; k < WORKLOAD_SHIFT; k++) {
    int center_shift = rand() % (MAX_KEY - hot_data);
    for (int i = 0; i < length; i++) {
      if ((float)(rand() % 100 + 1) / 100 > update_rate) {
        key = rand() % MAX_KEY;
        while (bitmap[key] != 0) key = (key + 1) % MAX_KEY;
        bitmap[key] = 1;
        cnt1++;
        OP = ADD_KV;
        outFile << OP << ' ' << key << ' '
                << ((rand() % MAX_VALUE_SIZE / 4) + 1) * 4 << endl;
      } else {
        if ((float)(rand() % 100 + 1) / 100 > hot_data_rate) {
          key = rand() % hot_data + center_shift;
          while (bitmap[key] == 0) key = (key + 1) % hot_data + center_shift;
          OP = MODIFY_KV;
          cnt2++;
          outFile << OP << ' ' << key << ' '
                  << ((rand() % MAX_VALUE_SIZE / 4) + 1) * 4 << endl;
        } else {
          cnt3++;
          key = (rand() % (MAX_KEY - hot_data) + hot_data + center_shift) %
                MAX_KEY;
          while (bitmap[key] == 0)
            key = ((key + MAX_KEY - hot_data - center_shift + 1) %
                       (MAX_KEY - hot_data) +
                   hot_data + center_shift) %
                  MAX_KEY;
          if (rand() % 10 == 0) {
            OP = DELETE_KV;
            bitmap[key] = 0;
            outFile << OP << ' ' << key << ' ' << 0 << endl;
          } else {
            OP = MODIFY_KV;
            outFile << OP << ' ' << key << ' '
                    << ((rand() % MAX_VALUE_SIZE / 4) + 1) * 4 << endl;
          }
        }
      }
    }
  }
  cout << cnt1 << " " << cnt2 << " " << cnt3 << endl;
  outFile.close();
}