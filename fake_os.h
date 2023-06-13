#include "fake_process.h"
#include "linked_list.h"
#pragma once


typedef struct {
  ListItem list;
  int pid;
  int chunk_start_recorder;
  int chunk_duration_sum;
  float predicted_duration;
  ListHead events;
} FakePCB;


struct FakeOS;
typedef void (*ScheduleFn)(struct FakeOS* os, void* args);

typedef struct {
  int CPU_index;
  int quantum;
  float decay_coefficient;
} SchedArgs;


typedef struct FakeOS{
  FakePCB** running;
  ListHead ready;
  ListHead waiting;

  int timer;
  int num_of_cpu;
  ScheduleFn schedule_fn;
  SchedArgs* schedule_args;

  ListHead processes;
} FakeOS;


void FakeOS_init(FakeOS* os, int num_of_cpu);
void FakeOS_simStep(FakeOS* os);
void FakeOS_destroy(FakeOS* os);
