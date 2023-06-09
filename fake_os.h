#include "fake_process.h"
#include "linked_list.h"
#pragma once

#define NUMBERS_OF_CPU 3
#define DECAY_COEFFICIENT 0.5

typedef struct {
  ListItem list;
  int pid;
  int chunk_start_recorder;
  int chunk_length_sum;
  int predicted_job_length;
  ListHead events;
} FakePCB;

struct FakeOS;
typedef void (*ScheduleFn)(struct FakeOS* os, void* args);

typedef struct {
  int CPU_index;
  int quantum;
} SchedArgs;

typedef struct FakeOS{
  FakePCB** running;
  ListHead ready;
  ListHead waiting;

  int timer;
  ScheduleFn schedule_fn;
  SchedArgs* schedule_args;

  ListHead processes;
} FakeOS;

void FakeOS_init(FakeOS* os);
void FakeOS_simStep(FakeOS* os);
void FakeOS_destroy(FakeOS* os);
