#pragma once
#include "../fake_process/fake_process.h"

typedef void (*ScheduleFn)();

typedef struct {
  ListItem list;             // Struct della quale ShedMetrics è ereditaria
  int sim_duration;          // Durata della simulazione
  int scheduler_id;          // ID dello scheduler utilizzato nella simulazione
  int num_of_cpu;            // Numero delle CPU disponibili per questa simulazione
  int procs_count;           // Contatore del numero di processi

  float throughput;          // Numero di processi terminati per uinità di tempo

  float* cpu_work;           // Array che inizialmente contiene all'indice i quante unità di tempo la CPU(i) è stata utilizzata. Successivamente dividendo per la durata della simulazione conterrà la % di utilizzo della CPU(i)

  float avg_turnaround_time; // Inizialmente contiene la somma dei tempi di turnaround. Successivamente dividendo per process_counter conterrà la media
  float avg_response_time;   // Inizialmente contiene la somma dei tempi di response. Successivamente dividendo per process_counter conterrà la media
  float avg_waiting_time;    // Inizialmente contiene la somma dei tempi di waiting. Successivamente dividendo per process_counter conterrà la media
} SimCard;

typedef struct {
  int quantum;             // Quanto di tempo per l'interruzione preemptive dei running
} SchedArgsRR;

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
  int scheduler_id;        // ID della tipologia di scheduler

  ScheduleFn scheduler_fn; // Funzione dello scheduler
  void* scheduler_args;    // Argomenti utilizzati per lo scheduler

  ListHead processes;      // Lista dei processi caricati dei file descrittivi (inseriti da linea di comando)
} FakeOS;


void FakeOS_init(FakeOS* os, int num_of_cpu, int scheduler_id, ScheduleFn scheduler_fn);
void SimCard_init(FakeOS* os, SimCard* sm);
void FakeOS_SortedInsertInReady(ListHead* ready, FakePCB* pcb_to_insert);
void FakeOS_simStep(FakeOS* os, SimCard* sm);
void FakeOS_destroy(FakeOS* os);
