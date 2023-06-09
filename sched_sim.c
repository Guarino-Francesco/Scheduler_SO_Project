#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fake_os.h"

FakeOS os;

void schedP_SJF_QP(FakeOS* os, void* args_){
  // SchedArgs* args=(SchedArgs*)args_;

  // Cerca il prossimo processo in ready
  if (!os->ready.first) return;

  // Se lo trova lo seleziona come running
  FakePCB* pcb=(FakePCB*) List_popFront(&os->ready);
  if (pcb->predicted_job_length) os->schedule_args->quantum=pcb->predicted_job_length;
  pcb->chunk_start_recorder=os->timer;
  os->running[os->schedule_args->CPU_index]=pcb;

  // Deve avere almeno un evento e deve essere di tipo CPU
  assert(pcb->events.first);
  ProcessEvent* e = (ProcessEvent*)pcb->events.first;
  assert(e->type==CPU);

  // Controlla ed eventualmente modifica l'evento per far si che possa rispettare con il quantum
  printf("\nPreparing pid %d for the run - quantum: %d - duration: %d", pcb->pid, os->schedule_args->quantum, e->duration);
  if (e->duration>os->schedule_args->quantum) {

    // Nuovo evento di durata quantum
    ProcessEvent* qe=(ProcessEvent*)malloc(sizeof(ProcessEvent));
    qe->list.prev=qe->list.next=0;
    qe->type=CPU;
    qe->duration=os->schedule_args->quantum;

    // Modifico l'evento originale dove rimane lo scarto della durata
    e->duration-=os->schedule_args->quantum;
    List_pushFront(&pcb->events, (ListItem*)qe);
  }

};

int main(int argc, char** argv) {

  // Inizializza l'OS
  FakeOS_init(&os);
  SchedArgs* sched_args=(SchedArgs*)malloc(sizeof(SchedArgs));
  sched_args->quantum=10;
  os.schedule_args=sched_args;
  os.schedule_fn=schedP_SJF_QP;

  // Carica i processi dei file forniti negli argv
  for (int i=1; i<argc; ++i){
    FakeProcess new_process;

    int num_events=FakeProcess_load(&new_process, argv[i]);
    printf("file[%s], pid: %d, #events:%02d", argv[i], new_process.pid, num_events);

    if (num_events) { printf(" - loading...\n");
      FakeProcess* new_process_ptr=(FakeProcess*)malloc(sizeof(FakeProcess));
      *new_process_ptr=new_process;
      List_pushBack(&os.processes, (ListItem*)new_process_ptr);
    }
    else { printf(" - Require at least 1 event."); }
  }

  printf("loaded %d processes \n", os.processes.size);

  while(1){
    int is_running = 0;
    for (int i=0;i<NUMBERS_OF_CPU;i++){
      if ((os.running)[i]) {
        is_running++;
        break;
      }
    }
    if (is_running || os.ready.first || os.waiting.first || os.processes.first) {
      FakeOS_simStep(&os);
    }
    else { break; }
  }

  printf("\nSimulation ended\n");

}
