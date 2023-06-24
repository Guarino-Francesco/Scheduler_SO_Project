#pragma once
#include "../fake_process/fake_process.h"

int Hist_loadRecords(int* cpu_distribution, int* io_distribution, int* cpu_cumulative, int* io_cumulative, int* cpu_records, int* io_records, const char* filename);
