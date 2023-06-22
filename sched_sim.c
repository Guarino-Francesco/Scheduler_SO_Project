#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fake_os/fake_os.h"

FakeOS os;

// Scheduler SPJF (Schortest Predicted Job First)
void scheduler_P_SJF_QP(){
  SchedArgsPSJFQP* scheduler_args=(SchedArgsPSJFQP*)os.scheduler_args;

  // Controlla lo stato dei processi in waiting e li aggiorna di conseguenza
  ListItem* aux=os.waiting.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;

    ProcessEvent* e=(ProcessEvent*) pcb->events.first;
    assert(e->type==IO); printf("\nWaiting %d - ", pcb->pid);
    e->duration--;

    // Se il processo ha terminato l'evento lo gestisce
    if (!e->duration){ printf("time expired --> ");
      List_popFront(&pcb->events);
      free(e);
      List_detach(&os.waiting, (ListItem*)pcb);

      // Se il processo non ha più eventi viene terminato
      if (!pcb->events.first) { printf("the process has finished"); free(pcb); }
      // Altrimenti a seconda del tipo di evento viene assegnato a ready o waiting
      else {
        e=(ProcessEvent*) pcb->events.first;

        // Assegnazione ordinata in ready
        if (e->type == CPU) { printf("moving to ready list");
          FakeOS_SortedInsertInReady(&os.ready, pcb);
        }

        // Assegnazione in waiting
        else { printf("moving to waiting list");
          List_pushBack(&os.waiting, (ListItem*) pcb);
        }

      }
    }
    // Altrimenti procede con gli altri processi
    else printf("ending in %02d - predicted duration: %f", e->duration, pcb->predicted_duration);
  } printf("\n");

  // Controlla lo stato dei processi in running e li aggiorna di conseguenza
  for (int i=0;i<os.num_of_cpu;i++) {

    // Se la CPU corrente ha un processo in running lo esamina altrimenti procede con la prossima
    FakePCB* current_running=os.running[i];
    if (current_running) {
      ProcessEvent* e=(ProcessEvent*) current_running->events.first;
      assert(e->type==CPU); printf("\nCPU(%d): Running %d - ", i, current_running->pid);
      e->duration--;

      // Se il processo ha terminato l'evento lo gestisce
      if (!e->duration){ printf("time expired --> ");
        List_popFront(&current_running->events);
        free(e);

        // Se il processo non ha più eventi viene terminato
        if (!current_running->events.first) { printf("the process has finished"); free(current_running); }
        // Altrimenti a seconda del tipo di evento viene assegnato a ready o waiting
        else {
          e=(ProcessEvent*) current_running->events.first;

          // Assegnazione ordinata in ready
          if (e->type == CPU){ printf("moving to ready list");
            current_running->chunk_duration_sum+=os.timer-current_running->chunk_start_recorder;
            FakeOS_SortedInsertInReady(&os.ready, current_running);
          }
          // Assegnazione in waiting e calcolo della predizione della durata
          else { printf("moving to waiting list");
            int measured_duration=os.timer-current_running->chunk_start_recorder+current_running->chunk_duration_sum;
            current_running->predicted_duration=(scheduler_args->decay_coefficient)*(measured_duration)+(1-(scheduler_args->decay_coefficient))*(current_running->predicted_duration);
            current_running->chunk_duration_sum=0;
            List_pushBack(&os.waiting, (ListItem*) current_running);
          }
        }

        // Rimuove il processo dal running corrente che eventualmente verrà riselezionato in futuro
        os.running[i] = 0;
      }
      // Altrimenti procede
      else printf("ending in %02d", e->duration);
    }

  } printf("\n");

  // Stampa la ready list
  aux=os.ready.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;
    printf("\nReady %d - predicted duration: %f", pcb->pid, pcb->predicted_duration);
  } printf("\n");

  // Controlla quali CPU non sono in running e le imposta su gli eventuali processi in ready
  for (int i=0;i<os.num_of_cpu;i++) {
    if (!os.running[i]) {

      // Cerca il prossimo processo in ready e si ferma se non lo trova
      if (!os.ready.first) return;

      // Se invece lo trova lo seleziona come running
      FakePCB* pcb=(FakePCB*) List_popFront(&os.ready);
      pcb->chunk_start_recorder=os.timer;
      os.running[i]=pcb;

      // Deve avere almeno un evento
      assert(pcb->events.first);
      ProcessEvent* e = (ProcessEvent*)pcb->events.first;
      // L'evento deve essere di tipo CPU
      assert(e->type==CPU);

      // Controlla ed eventualmente modifica l'evento per far si che possa rispettare il quantum
      printf("\nPreparing pid %d for the run - event duration from the file: %d", pcb->pid, e->duration);
      if (e->duration>scheduler_args->quantum) {

        // Nuovo evento di durata quantum
        ProcessEvent* qe=(ProcessEvent*)malloc(sizeof(ProcessEvent));
        qe->list.prev=qe->list.next=0;
        qe->type=CPU;
        qe->duration=scheduler_args->quantum;

        // Modifica l'evento originale dove rimane lo scarto della durata
        e->duration-=scheduler_args->quantum;
        List_pushFront(&pcb->events, (ListItem*)qe);
      }

    }
  }

};

// Scheduler SPJF (Schortest Predicted Job First)
void scheduler_FIFO(){

  // Controlla lo stato dei processi in waiting e li aggiorna di conseguenza
  ListItem* aux=os.waiting.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;

    ProcessEvent* e=(ProcessEvent*) pcb->events.first;
    assert(e->type==IO); printf("\nWaiting %d - ", pcb->pid);
    e->duration--;

    // Se il processo ha terminato l'evento lo gestisce
    if (!e->duration){ printf("time expired --> ");
      List_popFront(&pcb->events);
      free(e);
      List_detach(&os.waiting, (ListItem*)pcb);

      // Se il processo non ha più eventi viene terminato
      if (!pcb->events.first) { printf("the process has finished"); free(pcb); }
      // Altrimenti a seconda del tipo di evento viene assegnato a ready o waiting
      else {
        e=(ProcessEvent*) pcb->events.first;

        // Assegnazione in ready
        if (e->type == CPU) { printf("moving to ready list");
          List_pushBack(&os.ready, (ListItem*) pcb);
        }

        // Assegnazione in waiting
        else { printf("moving to waiting list");
          List_pushBack(&os.waiting, (ListItem*) pcb);
        }

      }
    }
    // Altrimenti procede con gli altri processi
    else printf("ending in %02d", e->duration);
  } printf("\n");

  // Controlla lo stato dei processi in running e li aggiorna di conseguenza
  for (int i=0;i<os.num_of_cpu;i++) {

    // Se la CPU corrente ha un processo in running lo esamina altrimenti procede con la prossima
    FakePCB* current_running=os.running[i];
    if (current_running) {
      ProcessEvent* e=(ProcessEvent*) current_running->events.first;
      assert(e->type==CPU); printf("\nCPU(%d): Running %d - ", i, current_running->pid);
      e->duration--;

      // Se il processo ha terminato l'evento lo gestisce
      if (!e->duration){ printf("time expired --> ");
        List_popFront(&current_running->events);
        free(e);

        // Se il processo non ha più eventi viene terminato
        if (!current_running->events.first) { printf("the process has finished"); free(current_running); }
        // Altrimenti a seconda del tipo di evento viene assegnato a ready o waiting
        else {
          e=(ProcessEvent*) current_running->events.first;

          // Assegnazione in ready
          if (e->type == CPU){ printf("moving to ready list");
            List_pushBack(&os.ready, (ListItem*) current_running);
          }
          // Assegnazione in waiting
          else { printf("moving to waiting list");
            List_pushBack(&os.waiting, (ListItem*) current_running);
          }
        }

        // Rimuove il processo dal running corrente che eventualmente verrà riselezionato in futuro
        os.running[i] = 0;
      }
      // Altrimenti procede
      else printf("ending in %02d", e->duration);
    }

  } printf("\n");

  // Stampa la ready list
  aux=os.ready.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;
    printf("\nReady %d", pcb->pid);
  } printf("\n");

  // Controlla quali CPU non sono in running e le imposta su gli eventuali processi in ready
  for (int i=0;i<os.num_of_cpu;i++) {
    if (!os.running[i]) {

      // Cerca il prossimo processo in ready e si ferma se non lo trova
      if (!os.ready.first) return;

      // Se invece lo trova lo seleziona come running
      FakePCB* pcb=(FakePCB*) List_popFront(&os.ready);
      os.running[i]=pcb;

      // Deve avere almeno un evento
      assert(pcb->events.first);
      ProcessEvent* e = (ProcessEvent*)pcb->events.first;
      // L'evento deve essere di tipo CPU
      assert(e->type==CPU);

      printf("\nPreparing pid %d for the run - event duration from the file: %d", pcb->pid, e->duration);
    }
  }

};

// Funzione main
int main(int argc, char** argv) {

  // Gestisce l'errore nell'inserimento degli argomenti
  if (argc==1) {
    printf("\n"
    "Il primo argomento deve essere il numero di CPU da voler configurare\n\n"
    "Il secondo argomento deve specificare il tipo di scheduler da utilizzare inserendo l'ID ad esso associato:\n\n"
    "Scheduler disponibili:\n"
    "ID:0 = FIFO   - First In First Out\n"
    "ID:1 = PSJFQP - Preemptive Shortest Job First Quantum Prediction\n\n"
    "Dal terzo argomento in poi inserire i path dei file descrittivi dei processi\n\n"
    "Esempio: ./sched_sim [NUMBER OF CPU] [SCHEDULER ID] [FILE PATH 1] ... [FILE PATH n]\n"
    );
    return 0;
  }


  int number_of_cpu = atoi(argv[1]);
  short scheduler_id = atoi(argv[2]);
  ScheduleFn scheduler_fn;

  // Controlla lo scheduler selezionato da riga di comando e inizializza l'OS con il puntatore a funzione dello scheduler e gli eventuali argomenti
  printf("\nNumber of available CPU: %d", number_of_cpu);
  printf("\nSelected scheduler: ");
  switch (scheduler_id) {
    case 0:
      printf("FIFO - First In First Out\n");
      scheduler_fn = scheduler_FIFO;
      break;
    case 1:
      printf("PSJFQP - Preemptive Shortest Job First ith Quantum Prediction\n");
      scheduler_fn = scheduler_P_SJF_QP;
      break;
  }

  FakeOS_init(&os, number_of_cpu, scheduler_id, scheduler_fn);

  // Caricamento dei processi descritti nei file forniti da linea di comando
  int refused = 0;
  printf("\nLoading processes description files:\n");
  for (int i=3; i<argc; ++i){
    FakePCB* new_process=(FakePCB*)malloc(sizeof(FakePCB));

    // Caricamento delle informazioni del processo dal relativo file
    int num_events=FakePCB_loadProcessInfo(new_process, argv[i]);
    // Errore nell'aperture del file
    if (num_events==-1) printf("Errore durante l'apertura del file.\n");
    else {
      printf("file: %s - pid: %d - number of events: %02d", argv[i], new_process->pid, num_events);

      // Inserimento nella lista dei processi del sistema operativo
      if (num_events) { List_pushBack(&os.processes, (ListItem*)new_process); printf(" - loading...\n"); }
      else { refused++; printf(" - Require at least 1 event.\n"); }
    }
  }

  printf("\nProcesses:\nLoaded: %d\nRefused: %d\n\nStarting simulation:", os.processes.size, refused);

  // Avvio del sistema procedendo step by step fino a quando non terminano tutti i processi caricati
  while(1){

    // Controlla se c'è almeno una CPU che sta attualmente lavorando
    int is_running = 0;
    for (int i=0;i<number_of_cpu;i++){
      if ((os.running)[i]) { is_running++; break; }
    }

    // Controllo di terminazione della simulazione
    if ( is_running || os.ready.first || os.waiting.first || os.processes.first ) FakeOS_simStep(&os);
    // Se il controllo non va a buon fine termina la simulazione
    else break;

  }

  printf("\nSimulation ended\n");
  FakeOS_destroy(&os);
}
