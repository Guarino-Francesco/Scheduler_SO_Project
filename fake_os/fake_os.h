#pragma once
#include "../fake_process/fake_process.h"

typedef void (*ScheduleFn)();


typedef struct {
  int quantum;             // Quanto di tempo per l'interruzione preemptive dei running
  float decay_coefficient; // Coefficiente di decadimento utilizzato nella stima della durata del prossimo Burst
} SchedArgsPSJFQP;


typedef struct FakeOS{
  FakePCB** running;       // Array contente all'indice "i" l'eventuale FakePCB in running sulla i-esima CPU
  ListHead ready;          // Lista dei PCB di cui i processi sono in stato di ready
  ListHead waiting;        // Lista dei PCB di cui i processi sono in stato di waiting (esecuzione dell'IO Burst)

  int timer;               // Tempo assoluto dell'OS (unità di misura dei Burst)
  int num_of_cpu;          // Numero di processori a disposizione (inserito da linea di comando)
  short scheduler_id;      // ID della tipologia di scheduler
  ScheduleFn scheduler_fn; // Puntatore alla funzione dello scheduler
  void* scheduler_args;    // Argomenti utilizzati per lo scheduler

  ListHead processes;      // Lista dei processi caricati dei file descrittivi (inseriti da linea di comando)
} FakeOS;


void FakeOS_init(FakeOS* os, int num_of_cpu, short scheduler_id, ScheduleFn scheduler_fn);
void FakeOS_SortedInsertInReady(ListHead* ready, FakePCB* pcb_to_insert);
void FakeOS_simStep(FakeOS* os);
void FakeOS_destroy(FakeOS* os);
