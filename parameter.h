#ifndef parameter_H
#define parameter_H

#define ZONE_NUMBER 16
#define BLOCK_SIZE 4096
#define MAX_KV_PER_BLOCK (BLOCK_SIZE / (sizeof(int) * 4))
#define sg 0.4 //threshold of zone's garbage rate
#define se 0.4 //threshold of zone's empty rate
#define sz 0.8 //threshold of lifetime_recorded zone's rate
#define sl 2800 ////threshold of zone's lifetime varience
#define gl 2  // the maximun length of gc queue
#define MAX_KEY (2<<19)
#define MAX_VALUE_SIZE 2048
#define WORKLOAD_SHIFT 10
#define RECENT_MODIFY 1000
#define MAX_WORKLOAD 40000
#define HOT_DATA_RATE 0.05
#define UPDATE_RATE 0.5

enum WORKLOAD { SEQ, RAM };

enum BLOCK_STATUS { BLOCK_EMPTY, BLOCK_VALID, BLOCK_INVALID };

enum BLOCK_OPERATE { ADD_KV, DELETE_KV, MODIFY_KV };

#endif