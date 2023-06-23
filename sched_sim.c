#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include "fake_os/fake_os.h"

FakeOS os;
SimCard sim_card;
int schedulers_count = 3;


// Scheduler RR (Round Robin)
void scheduler_RR() {
  SchedArgsRR* scheduler_args=(SchedArgsRR*)os.scheduler_args;

  // Controlla lo stato dei processi in waiting e li aggiorna di conseguenza
  ListItem* aux=os.waiting.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;

    ProcessEvent* e=(ProcessEvent*) pcb->events.first;
    assert(e->type==IO); printf("\n| Waiting PID: %d - ", pcb->pid);
    e->duration--;

    // Se il processo ha terminato l'evento lo gestisce
    if (!e->duration){ printf("time expired");
      List_popFront(&pcb->events);
      free(e);
      List_detach(&os.waiting, (ListItem*)pcb);

      // Se il processo non ha più eventi viene terminato
      if (!pcb->events.first) { printf(" - the process has finished");
        sim_card.avg_turnaround_time+=os.timer-pcb->arrival_time;
        free(pcb);
      }
      // Altrimenti a seconda del tipo di evento viene assegnato a ready o waiting
      else {
        e=(ProcessEvent*) pcb->events.first;

        // Assegnazione ordinata in ready
        if (e->type == CPU) { printf(" - next event: CPU Burst --> moving to ready list");
          pcb->waiting_chunk_start=os.timer;
          pcb->response_chunk_start=os.timer;
          List_pushBack(&os.ready, (ListItem*) pcb);
        }

        // Assegnazione in waiting
        else { printf(" - next event: IO Burst --> moving to waiting list");
          List_pushBack(&os.waiting, (ListItem*) pcb);
        }

      }
    }
    // Altrimenti procede con gli altri processi
    else printf("ending in %02d", e->duration);
  } printf("\n|");

  // Controlla lo stato dei processi in running e li aggiorna di conseguenza
  for (int i=0;i<os.num_of_cpu;i++) {

    // Se la CPU corrente ha un processo in running lo esamina altrimenti procede con la prossima
    FakePCB* current_running=os.running[i];
    if (current_running) {
      ProcessEvent* e=(ProcessEvent*) current_running->events.first;
      assert(e->type==CPU); printf("\n| CPU(%d) working on PID: %d - ", i, current_running->pid);
      e->duration--;
      sim_card.cpu_work[i]++;

      // Se il processo ha terminato l'evento lo gestisce
      if (!e->duration){ printf("time expired");
        List_popFront(&current_running->events);
        free(e);

        // Se il processo non ha più eventi viene terminato
        if (!current_running->events.first) { printf(" - the process has finished");
          sim_card.avg_turnaround_time+=os.timer-current_running->arrival_time;
          free(current_running);
        }
        // Altrimenti a seconda del tipo di evento viene assegnato a ready o waiting
        else {
          e=(ProcessEvent*) current_running->events.first;

          // Assegnazione ordinata in ready
          if (e->type == CPU){ printf(" - next event: CPU Burst --> moving to ready list");
            current_running->waiting_chunk_start=os.timer;
            List_pushBack(&os.ready, (ListItem*) current_running);
          }
          // Assegnazione in waiting e aggiornamento del tempo medio di risposta
          else { printf(" - next event: IO Burst --> moving to waiting list");
            if (current_running->first_cpu_burst==1) sim_card.avg_response_time+=os.timer-current_running->response_chunk_start;
            else current_running->first_cpu_burst=1;
            List_pushBack(&os.waiting, (ListItem*) current_running);
          }
        }

        // Rimuove il processo dal running corrente che eventualmente verrà riselezionato in futuro
        os.running[i] = 0;
      }
      // Altrimenti procede
      else printf("ending in %02d", e->duration);
    }

  } printf("\n|");

  // Stampa la ready list
  aux=os.ready.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;
    printf("\n| Ready PID: %d", pcb->pid);
  } printf("\n|");

  // Controlla quali CPU non sono in running e le imposta su gli eventuali processi in ready
  for (int i=0;i<os.num_of_cpu;i++) {
    if (!os.running[i]) {

      // Cerca il prossimo processo in ready e si ferma se non lo trova
      if (!os.ready.first) return;

      // Se invece lo trova lo seleziona come running
      FakePCB* pcb=(FakePCB*) List_popFront(&os.ready);
      sim_card.avg_waiting_time+=os.timer-pcb->waiting_chunk_start;
      os.running[i]=pcb;

      // Deve avere almeno un evento
      assert(pcb->events.first);
      ProcessEvent* e = (ProcessEvent*)pcb->events.first;
      // L'evento deve essere di tipo CPU
      assert(e->type==CPU);

      // Controlla ed eventualmente modifica l'evento per far si che possa rispettare il quantum
      printf("\n| Starting pid %d on CPU(%d) - event duration from the file: %d", pcb->pid, i, e->duration);
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

}

// Scheduler PSJFQP (Preemptive Shortest Job First with Quantum Prediction)
void scheduler_P_SJF_QP() {
  SchedArgsPSJFQP* scheduler_args=(SchedArgsPSJFQP*)os.scheduler_args;

  // Controlla lo stato dei processi in waiting e li aggiorna di conseguenza
  ListItem* aux=os.waiting.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;

    ProcessEvent* e=(ProcessEvent*) pcb->events.first;
    assert(e->type==IO); printf("\n| Waiting PID: %d - ", pcb->pid);
    e->duration--;

    // Se il processo ha terminato l'evento lo gestisce
    if (!e->duration){ printf("time expired");
      List_popFront(&pcb->events);
      free(e);
      List_detach(&os.waiting, (ListItem*)pcb);

      // Se il processo non ha più eventi viene terminato
      if (!pcb->events.first) { printf(" - the process has finished");
        sim_card.avg_turnaround_time+=os.timer-pcb->arrival_time;
        free(pcb);
      }
      // Altrimenti a seconda del tipo di evento viene assegnato a ready o waiting
      else {
        e=(ProcessEvent*) pcb->events.first;

        // Assegnazione ordinata in ready
        if (e->type == CPU) { printf(" - next event: CPU Burst --> moving to ready list");
          pcb->waiting_chunk_start=os.timer;
          pcb->response_chunk_start=os.timer;
          FakeOS_SortedInsertInReady(&os.ready, pcb);
        }

        // Assegnazione in waiting
        else { printf(" - next event: IO Burst --> moving to waiting list");
          List_pushBack(&os.waiting, (ListItem*) pcb);
        }

      }
    }
    // Altrimenti procede con gli altri processi
    else printf("ending in %02d - predicted duration: %f", e->duration, pcb->predicted_duration);
  } printf("\n|");

  // Controlla lo stato dei processi in running e li aggiorna di conseguenza
  for (int i=0;i<os.num_of_cpu;i++) {

    // Se la CPU corrente ha un processo in running lo esamina altrimenti procede con la prossima
    FakePCB* current_running=os.running[i];
    if (current_running) {
      ProcessEvent* e=(ProcessEvent*) current_running->events.first;
      assert(e->type==CPU); printf("\n| CPU(%d) working on PID: %d - ", i, current_running->pid);
      e->duration--;
      sim_card.cpu_work[i]++;

      // Se il processo ha terminato l'evento lo gestisce
      if (!e->duration){ printf("time expired");
        List_popFront(&current_running->events);
        free(e);

        // Se il processo non ha più eventi viene terminato
        if (!current_running->events.first) { printf(" - the process has finished");
          sim_card.avg_turnaround_time+=os.timer-current_running->arrival_time;
          free(current_running);
        }
        // Altrimenti a seconda del tipo di evento viene assegnato a ready o waiting
        else {
          e=(ProcessEvent*) current_running->events.first;

          // Assegnazione ordinata in ready
          if (e->type == CPU){ printf(" - next event: CPU Burst --> moving to ready list");
            current_running->waiting_chunk_start=os.timer;
            current_running->chunk_duration_sum+=os.timer-current_running->chunk_start_recorder;
            FakeOS_SortedInsertInReady(&os.ready, current_running);
          }
          // Assegnazione in waiting, calcolo della predizione della durata e aggiornamento del tempo medio di risposta
          else { printf(" - next event: IO Burst --> moving to waiting list");
            if (current_running->first_cpu_burst==1) sim_card.avg_response_time+=os.timer-current_running->response_chunk_start;
            else current_running->first_cpu_burst=1;
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

  } printf("\n|");

  // Stampa la ready list
  aux=os.ready.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;
    printf("\n| Ready PID: %d - predicted duration: %f", pcb->pid, pcb->predicted_duration);
  } printf("\n|");

  // Controlla quali CPU non sono in running e le imposta su gli eventuali processi in ready
  for (int i=0;i<os.num_of_cpu;i++) {
    if (!os.running[i]) {

      // Cerca il prossimo processo in ready e si ferma se non lo trova
      if (!os.ready.first) return;

      // Se invece lo trova lo seleziona come running
      FakePCB* pcb=(FakePCB*) List_popFront(&os.ready);
      pcb->chunk_start_recorder=os.timer;
      sim_card.avg_waiting_time+=os.timer-pcb->waiting_chunk_start;
      os.running[i]=pcb;

      // Deve avere almeno un evento
      assert(pcb->events.first);
      ProcessEvent* e = (ProcessEvent*)pcb->events.first;
      // L'evento deve essere di tipo CPU
      assert(e->type==CPU);

      // Controlla ed eventualmente modifica l'evento per far si che possa rispettare il quantum
      printf("\n| Starting pid %d on CPU(%d) - event duration from the file: %d", pcb->pid, i, e->duration);
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

// Scheduler FIFO (First In First Out)
void scheduler_FIFO() {

  // Controlla lo stato dei processi in waiting e li aggiorna di conseguenza
  ListItem* aux=os.waiting.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;

    ProcessEvent* e=(ProcessEvent*) pcb->events.first;
    assert(e->type==IO); printf("\n| Waiting PID: %d - ", pcb->pid);
    e->duration--;

    // Se il processo ha terminato l'evento lo gestisce
    if (!e->duration){ printf("time expired");
      List_popFront(&pcb->events);
      free(e);
      List_detach(&os.waiting, (ListItem*)pcb);

      // Se il processo non ha più eventi viene terminato
      if (!pcb->events.first) { printf(" - the process has finished");
        sim_card.avg_turnaround_time+=os.timer-pcb->arrival_time;
        free(pcb);
      }
      // Altrimenti a seconda del tipo di evento viene assegnato a ready o waiting
      else {
        e=(ProcessEvent*) pcb->events.first;

        // Assegnazione in ready
        if (e->type == CPU) { printf(" - next event: CPU Burst --> moving to ready list");
          pcb->waiting_chunk_start=os.timer;
          pcb->response_chunk_start=os.timer;
          List_pushBack(&os.ready, (ListItem*) pcb);
        }

        // Assegnazione in waiting
        else { printf(" - next event: IO Burst --> moving to waiting list");
          List_pushBack(&os.waiting, (ListItem*) pcb);
        }

      }
    }
    // Altrimenti procede con gli altri processi
    else printf("ending in %02d", e->duration);
  } printf("\n|");

  // Controlla lo stato dei processi in running e li aggiorna di conseguenza
  for (int i=0;i<os.num_of_cpu;i++) {

    // Se la CPU corrente ha un processo in running lo esamina altrimenti procede con la prossima
    FakePCB* current_running=os.running[i];
    if (current_running) {
      ProcessEvent* e=(ProcessEvent*) current_running->events.first;
      assert(e->type==CPU); printf("\n| CPU(%d) working on PID: %d - ", i, current_running->pid);
      e->duration--;
      sim_card.cpu_work[i]++;

      // Se il processo ha terminato l'evento lo gestisce
      if (!e->duration){ printf("time expired");
        List_popFront(&current_running->events);
        free(e);

        // Se il processo non ha più eventi viene terminato
        if (!current_running->events.first) { printf(" - the process has finished");
          sim_card.avg_turnaround_time+=os.timer-current_running->arrival_time;
          free(current_running);
        }
        // Altrimenti a seconda del tipo di evento viene assegnato a ready o waiting
        else {
          e=(ProcessEvent*) current_running->events.first;

          // Assegnazione in ready
          if (e->type == CPU){ printf(" - next event: CPU Burst --> moving to ready list");
            current_running->waiting_chunk_start=os.timer;
            List_pushBack(&os.ready, (ListItem*) current_running);
          }
          // Assegnazione in waiting e aggiornamento del tempo medio di risposta
          else { printf(" - next event: IO Burst --> moving to waiting list");
            if (current_running->first_cpu_burst==1) sim_card.avg_response_time+=os.timer-current_running->response_chunk_start;
            else current_running->first_cpu_burst=1;
            List_pushBack(&os.waiting, (ListItem*) current_running);
          }
        }

        // Rimuove il processo dal running corrente che eventualmente verrà riselezionato in futuro
        os.running[i] = 0;
      }
      // Altrimenti procede
      else printf("ending in %02d", e->duration);
    }

  } printf("\n|");

  // Stampa la ready list
  aux=os.ready.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;
    printf("\n| Ready PID: %d", pcb->pid);
  } printf("\n|");

  // Controlla quali CPU non sono in running e le imposta su gli eventuali processi in ready
  for (int i=0;i<os.num_of_cpu;i++) {
    if (!os.running[i]) {

      // Cerca il prossimo processo in ready e si ferma se non lo trova
      if (!os.ready.first) return;

      // Se invece lo trova lo seleziona come running
      FakePCB* pcb=(FakePCB*) List_popFront(&os.ready);
      sim_card.avg_waiting_time+=os.timer-pcb->waiting_chunk_start;
      os.running[i]=pcb;

      // Deve avere almeno un evento
      assert(pcb->events.first);
      ProcessEvent* e = (ProcessEvent*)pcb->events.first;
      // L'evento deve essere di tipo CPU
      assert(e->type==CPU);

      printf("\n| Starting pid %d on CPU(%d) - event duration from the file: %d", pcb->pid, i, e->duration);
    }
  }

};



// Stampa i risultati e le metriche della simulazione sm
void SimCard_print() {

  printf("\n\n"
  "________________________________________\n"
  "|Scheduler: %s\n"
  "|Available CPU: %d - Duration: %d\n"
  "|______________________________________|\n"
  "|Throughput: %f                  |\n"
  "|______________________________________|\n"
  "|CPU utilization:                      |",
  (sim_card.scheduler_id==2)? "RR                         |":
  (sim_card.scheduler_id==1)? "PSJFQP                     |":"FIFO                       |",
  sim_card.num_of_cpu,
  sim_card.sim_duration,
  sim_card.throughput);
  for (int i=0;i<sim_card.num_of_cpu;i++) printf("\n|  CPU(%d): [%6.2f%c]                   |", i, sim_card.cpu_work[i], 37);
  printf("\n"
  "|______________________________________|\n"
  "|Average times:                        |\n"
  "|  Turnaround: [%7.2f]               |\n"
  "|  Response:   [%7.2f]               |\n"
  "|  Waiting:    [%7.2f]               |\n"
  "|______________________________________|\n\n",
  sim_card.avg_turnaround_time,
  sim_card.avg_response_time,
  sim_card.avg_waiting_time);

}

// Stampa i risultati e le metriche della simulazione sm
void SimCard_fprint(const char* filename) {
  // Apre il file designato in scrittura e ritorna se c'è errore
  FILE* f=fopen(filename, "a");
  assert(f && "Error open file");

  fprintf(f,
  "________________________________________\n"
  "|Scheduler: %s\n"
  "|Available CPU: %d - Duration: %d\n"
  "|______________________________________|\n"
  "|Throughput: %f                  |\n"
  "|______________________________________|\n"
  "|CPU utilization:                      |",
  (sim_card.scheduler_id==2)? "RR                         |":
  (sim_card.scheduler_id==1)? "PSJFQP                     |":"FIFO                       |",
  sim_card.num_of_cpu,
  sim_card.sim_duration,
  sim_card.throughput);
  for (int i=0;i<sim_card.num_of_cpu;i++) fprintf(f, "\n|  CPU(%d): [%6.2f%c]                   |", i, sim_card.cpu_work[i], 37);
  fprintf(f, "\n"
  "|______________________________________|\n"
  "|Average times:                        |\n"
  "|  Turnaround: [%7.2f]               |\n"
  "|  Response:   [%7.2f]               |\n"
  "|  Waiting:    [%7.2f]               |\n"
  "|______________________________________|\n\n\n",
  sim_card.avg_turnaround_time,
  sim_card.avg_response_time,
  sim_card.avg_waiting_time);

}

// Salva i risultati e chiama le stampe
void SimCard_SaveAndPrint() {

  sim_card.sim_duration=os.timer;        // Durata
  sim_card.scheduler_id=os.scheduler_id; // ID Scheduler
  sim_card.num_of_cpu=os.num_of_cpu;     // Numero di CPU

  // Calcolo del Throughput
  sim_card.throughput=((float)sim_card.procs_count)/((float)os.timer);

  // Calcolo degli utilizzi delle CPU
  for (int i=0;i<os.num_of_cpu;i++) sim_card.cpu_work[i]=sim_card.cpu_work[i]/((float)os.timer)*100;

  // Calcolo dei tempi medi: turnaround, response e waiting.
  sim_card.avg_turnaround_time=sim_card.avg_turnaround_time/((float)sim_card.procs_count);
  sim_card.avg_response_time=sim_card.avg_response_time/((float)sim_card.procs_count);
  sim_card.avg_waiting_time=sim_card.avg_waiting_time/((float)sim_card.procs_count);

  // Salvataggio su file totale delle simulazioni
  SimCard_fprint("SimulationsSummary");

  // Salvataggio su file della singola simulazione
  char filename[50]; sprintf(filename, "SimSched %s #CPU %03d",
    (sim_card.scheduler_id==2)? "RR":
    (sim_card.scheduler_id==1)? "PSJFQP":"FIFO",
    sim_card.num_of_cpu);
  SimCard_fprint(filename);

  // Stampa della simulazione
  SimCard_print();

}



// Funzione main
int main(int argc, char** argv) {

  // Guida nell'inserimento degli argomenti
  if (argc==1) { printf("\nUsage: ./sched_sim [PROCESS DESCRIPTION FILE PATH 1] [PROCESS DESCRIPTION FILE PATH 2] ... [PROCESS DESCRIPTION FILE PATH n]\n"); return 0; }


  printf("\n\n______________________________________________________________________________________\n"
             "### INITIALIZING A NEW SIMULATION ####################################################\n");

  // Richiede la tipologia di scheduler
  int scheduler_id;
  printf("\n\n"
  "Available scheduler:\n"
  "________________________________________________________________________________\n"
  "|ID:0 = FIFO   - First In First Out                                            |\n"
  "|______________________________________________________________________________|\n"
  "|ID:1 = PSJFQP - Preemptive Shortest Job First with Quantum Prediction         |\n"
  "|______________________________________________________________________________|\n"
  "|ID:2 = RR     - Round Robin                                                   |\n"
  "|______________________________________________________________________________|\n\n"
  "Insert here the ID of the scheduler that you want to run in this simulation: "
  );
  while (scanf("%d", &scheduler_id)!=1 || (scheduler_id<0 || scheduler_id>=schedulers_count)) {
      printf("\nERROR: you have to insert an ID among the ID in the scheduler's table!\nRetry: ");
      if (feof(stdin) || ferror(stdin)) return 0;
      int c;
      while ((c=getchar())!=EOF && c!='\n');
      if (c == EOF) return 0;
  }

  // Richiede il numero di CPU
  int number_of_cpu;
  printf("\nInsert the NUMBER (in digits and greater than 0) of CPU to enable in the simulator: ");
  while (scanf("%d", &number_of_cpu)!=1 || (number_of_cpu<=0)) {
      printf("\nERROR: you have to insert a NUMBER in digits and greater than 0!\nRetry: ");
      if (feof(stdin) || ferror(stdin)) return 0;
      int c;
      while ((c=getchar())!=EOF && c!='\n');
      if (c == EOF) return 0;
  }

  // Inizializza l'OS e la Simulazione
  ScheduleFn scheduler_fn;
  printf("\nSelected scheduler: ");
  switch (scheduler_id) {
    case 0:
      printf("FIFO - First In First Out\n");
      scheduler_fn = scheduler_FIFO;
      break;
    case 1:
      printf("PSJFQP - Preemptive Shortest Job First with Quantum Prediction\n");
      scheduler_fn = scheduler_P_SJF_QP;
      break;
    case 2:
      printf("RR - Round Robin\n");
      scheduler_fn = scheduler_RR;
      break;
  } printf("\nNumber of available CPU: %d\n", number_of_cpu);
  // Inizializza l'OS
  FakeOS_init(&os, number_of_cpu, scheduler_id, scheduler_fn);
  // Inizializza il record delle metriche
  SimCard_init(&os, &sim_card);

  // Caricamento dei processi descritti nei file forniti da linea di comando
  int refused = 0;
  printf("\nLoading processes description files:\n");
  for (int i=1; i<argc; ++i){
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
    if ( is_running || os.ready.first || os.waiting.first || os.processes.first ) FakeOS_simStep(&os, &sim_card);
    // Se il controllo non va a buon fine termina la simulazione
    else break;

  } printf("Simulation ended\n");

  // Aggiornamento dei risultati e delle metriche della simulazione
  SimCard_SaveAndPrint();

  // Libera lo spazio dell'OS
  FakeOS_destroy(&os);
  printf("Exiting...\n");
}
