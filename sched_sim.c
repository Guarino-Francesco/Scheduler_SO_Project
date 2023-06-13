#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fake_os.h"

FakeOS os;

void schedP_SJF_QP(FakeOS* os, void* args_){

  // Cerca il prossimo processo in ready e si ferma se non lo trova
  if (!os->ready.first) return;

  // Se invece lo trova lo seleziona come running
  FakePCB* pcb=(FakePCB*) List_popFront(&os->ready);
  pcb->chunk_start_recorder=os->timer;
  os->running[os->schedule_args->CPU_index]=pcb;

  // Deve avere almeno un evento e deve essere di tipo CPU
  assert(pcb->events.first);
  ProcessEvent* e = (ProcessEvent*)pcb->events.first;
  assert(e->type==CPU);

  // Controlla ed eventualmente modifica l'evento per far si che possa rispettare con il quantum
  printf("\nPreparing pid %d for the run - event duration from the file: %d", pcb->pid, e->duration);
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
  if (argc==1) { printf("\nArgomenti sbagliati:\nIl primo argomento deve essere il numero di CPU da voler configurare\nDal secondo argomento in poi inserire i path dei file descrittivi dei processi\n\nEsempio: ./sched_sim [NUMBER OF CPU] [FILE PATH 1] ... [FILE PATH n]\n"); }
  else {
    int number_of_cpu = atoi(argv[1]);

    // Inizializzazione dell'OS
    FakeOS_init(&os, number_of_cpu);
    // Argomenti per lo scheduler
    SchedArgs* sched_args=(SchedArgs*)malloc(sizeof(SchedArgs));
    sched_args->quantum=10;
    sched_args->decay_coefficient=0.5;
    os.schedule_args=sched_args;
    // Puntatore a funzione dello scheduler
    os.schedule_fn=schedP_SJF_QP;

    // Caricamento dei processi descritti nei file forniti da linea di comando
    int refused = 0;
    printf("\n");
    for (int i=2; i<argc; ++i){
      FakeProcess new_process;

      int num_events=FakeProcess_load(&new_process, argv[i]);
      printf("file[%s], pid: %d, #events:%02d", argv[i], new_process.pid, num_events);

      if (num_events) { printf(" - loading...\n");
        FakeProcess* new_process_ptr=(FakeProcess*)malloc(sizeof(FakeProcess));
        *new_process_ptr=new_process;
        List_pushBack(&os.processes, (ListItem*)new_process_ptr);
      }
      else { printf(" - Require at least 1 event.\n"); refused++; }
    }

    printf("\n%d processes loaded.\n%d processes refused.\n\nNumber of CPU: %d\nThe default predictet duration for a new process is equal to the quantum.\n\nStarting simulation:", os.processes.size, refused, number_of_cpu);

    // Avvio del sistema procedendo step by step fino a quando non terminano tutti i processi caricati
    while(1){

      int is_running = 0;
      for (int i=0;i<number_of_cpu;i++){
        if ((os.running)[i]) { is_running++; break; }
      }

      if (is_running || os.ready.first || os.waiting.first || os.processes.first) {
        FakeOS_simStep(&os);
      }
      else { break; }

    }

    printf("\nSimulation ended\n");
  }
}
