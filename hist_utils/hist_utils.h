#pragma once
#include "../fake_process/fake_process.h"

#define MAX_RECORDS_NUMBER 10
#define BURST_NUM_UPPER_LIMIT 6
#define BURST_NUM_LOWER_LIMIT 2
#define MIN_BURST_DURATION 1
#define ARRIVAL_UPPER_LIMIT 5

int Hist_loadRecords(int* cpu_distribution, int* io_distribution, int* cpu_cumulative, int* io_cumulative, int* cpu_records, int* io_records, const char* filename);
