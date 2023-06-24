#include "sched_sim.h"


// Scheduler RR (Round Robin)
void scheduler_RR(FakeOS* os, SimCard* sim_card) {
  SchedArgsRR* scheduler_args=(SchedArgsRR*)os->scheduler_args;

  // Controlla lo stato dei processi in waiting e li aggiorna di conseguenza
  ListItem* aux=os->waiting.first;
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
      List_detach(&os->waiting, (ListItem*)pcb);

      // Se il processo non ha più eventi viene terminato
      if (!pcb->events.first) { printf(" - the process has finished");
        sim_card->avg_turnaround_time+=os->timer-pcb->arrival_time;
        free(pcb);
      }
      // Altrimenti a seconda del tipo di evento viene assegnato a ready o waiting
      else {
        e=(ProcessEvent*) pcb->events.first;

        // Assegnazione ordinata in ready
        if (e->type == CPU) { printf(" - next event: CPU Burst --> moving to ready list");
          pcb->waiting_chunk_start=os->timer;
          pcb->response_chunk_start=os->timer;
          List_pushBack(&os->ready, (ListItem*) pcb);
        }

        // Assegnazione in waiting
        else { printf(" - next event: IO Burst --> moving to waiting list");
          List_pushBack(&os->waiting, (ListItem*) pcb);
        }

      }
    }
    // Altrimenti procede con gli altri processi
    else printf("ending in %02d", e->duration);
  } printf("\n|");

  // Controlla lo stato dei processi in running e li aggiorna di conseguenza
  for (int i=0;i<os->num_of_cpu;i++) {

    // Se la CPU corrente ha un processo in running lo esamina altrimenti procede con la prossima
    FakePCB* current_running=os->running[i];
    if (current_running) {
      ProcessEvent* e=(ProcessEvent*) current_running->events.first;
      assert(e->type==CPU); printf("\n| CPU(%d) working on PID: %d - ", i, current_running->pid);
      e->duration--;
      sim_card->cpu_work[i]++;

      // Se il processo ha terminato l'evento lo gestisce
      if (!e->duration){ printf("time expired");
        List_popFront(&current_running->events);
        free(e);

        // Se il processo non ha più eventi viene terminato
        if (!current_running->events.first) { printf(" - the process has finished");
          sim_card->avg_turnaround_time+=os->timer-current_running->arrival_time;
          free(current_running);
        }
        // Altrimenti a seconda del tipo di evento viene assegnato a ready o waiting
        else {
          e=(ProcessEvent*) current_running->events.first;

          // Assegnazione ordinata in ready
          if (e->type == CPU){ printf(" - next event: CPU Burst --> moving to ready list");
            current_running->waiting_chunk_start=os->timer;
            List_pushBack(&os->ready, (ListItem*) current_running);
          }
          // Assegnazione in waiting e aggiornamento del tempo medio di risposta
          else { printf(" - next event: IO Burst --> moving to waiting list");
            if (current_running->first_cpu_burst==1) sim_card->avg_response_time+=os->timer-current_running->response_chunk_start;
            else current_running->first_cpu_burst=1;
            List_pushBack(&os->waiting, (ListItem*) current_running);
          }
        }

        // Rimuove il processo dal running corrente che eventualmente verrà riselezionato in futuro
        os->running[i] = 0;
      }
      // Altrimenti procede
      else printf("ending in %02d", e->duration);
    }

  } printf("\n|");

  // Stampa la ready list
  aux=os->ready.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;
    printf("\n| Ready PID: %d", pcb->pid);
  } printf("\n|");

  // Controlla quali CPU non sono in running e le imposta su gli eventuali processi in ready
  for (int i=0;i<os->num_of_cpu;i++) {
    if (!os->running[i]) {

      // Cerca il prossimo processo in ready e si ferma se non lo trova
      if (!os->ready.first) return;

      // Se invece lo trova lo seleziona come running
      FakePCB* pcb=(FakePCB*) List_popFront(&os->ready);
      sim_card->avg_waiting_time+=os->timer-pcb->waiting_chunk_start;
      os->running[i]=pcb;

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
void scheduler_P_SJF_QP(FakeOS* os, SimCard* sim_card) {
  SchedArgsPSJFQP* scheduler_args=(SchedArgsPSJFQP*)os->scheduler_args;

  // Controlla lo stato dei processi in waiting e li aggiorna di conseguenza
  ListItem* aux=os->waiting.first;
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
      List_detach(&os->waiting, (ListItem*)pcb);

      // Se il processo non ha più eventi viene terminato
      if (!pcb->events.first) { printf(" - the process has finished");
        sim_card->avg_turnaround_time+=os->timer-pcb->arrival_time;
        free(pcb);
      }
      // Altrimenti a seconda del tipo di evento viene assegnato a ready o waiting
      else {
        e=(ProcessEvent*) pcb->events.first;

        // Assegnazione ordinata in ready
        if (e->type == CPU) { printf(" - next event: CPU Burst --> moving to ready list");
          pcb->waiting_chunk_start=os->timer;
          pcb->response_chunk_start=os->timer;
          FakeOS_SortedInsertInReady(&os->ready, pcb);
        }

        // Assegnazione in waiting
        else { printf(" - next event: IO Burst --> moving to waiting list");
          List_pushBack(&os->waiting, (ListItem*) pcb);
        }

      }
    }
    // Altrimenti procede con gli altri processi
    else printf("ending in %02d - predicted duration: %f", e->duration, pcb->predicted_duration);
  } printf("\n|");

  // Controlla lo stato dei processi in running e li aggiorna di conseguenza
  for (int i=0;i<os->num_of_cpu;i++) {

    // Se la CPU corrente ha un processo in running lo esamina altrimenti procede con la prossima
    FakePCB* current_running=os->running[i];
    if (current_running) {
      ProcessEvent* e=(ProcessEvent*) current_running->events.first;
      assert(e->type==CPU); printf("\n| CPU(%d) working on PID: %d - ", i, current_running->pid);
      e->duration--;
      sim_card->cpu_work[i]++;

      // Se il processo ha terminato l'evento lo gestisce
      if (!e->duration){ printf("time expired");
        List_popFront(&current_running->events);
        free(e);

        // Se il processo non ha più eventi viene terminato
        if (!current_running->events.first) { printf(" - the process has finished");
          sim_card->avg_turnaround_time+=os->timer-current_running->arrival_time;
          free(current_running);
        }
        // Altrimenti a seconda del tipo di evento viene assegnato a ready o waiting
        else {
          e=(ProcessEvent*) current_running->events.first;

          // Assegnazione ordinata in ready
          if (e->type == CPU){ printf(" - next event: CPU Burst --> moving to ready list");
            current_running->waiting_chunk_start=os->timer;
            current_running->chunk_duration_sum+=os->timer-current_running->chunk_start_recorder;
            FakeOS_SortedInsertInReady(&os->ready, current_running);
          }
          // Assegnazione in waiting, calcolo della predizione della durata e aggiornamento del tempo medio di risposta
          else { printf(" - next event: IO Burst --> moving to waiting list");
            if (current_running->first_cpu_burst==1) sim_card->avg_response_time+=os->timer-current_running->response_chunk_start;
            else current_running->first_cpu_burst=1;
            int measured_duration=os->timer-current_running->chunk_start_recorder+current_running->chunk_duration_sum;
            current_running->predicted_duration=(scheduler_args->decay_coefficient)*(measured_duration)+(1-(scheduler_args->decay_coefficient))*(current_running->predicted_duration);
            current_running->chunk_duration_sum=0;
            List_pushBack(&os->waiting, (ListItem*) current_running);
          }
        }

        // Rimuove il processo dal running corrente che eventualmente verrà riselezionato in futuro
        os->running[i] = 0;
      }
      // Altrimenti procede
      else printf("ending in %02d", e->duration);
    }

  } printf("\n|");

  // Stampa la ready list
  aux=os->ready.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;
    printf("\n| Ready PID: %d - predicted duration: %f", pcb->pid, pcb->predicted_duration);
  } printf("\n|");

  // Controlla quali CPU non sono in running e le imposta su gli eventuali processi in ready
  for (int i=0;i<os->num_of_cpu;i++) {
    if (!os->running[i]) {

      // Cerca il prossimo processo in ready e si ferma se non lo trova
      if (!os->ready.first) return;

      // Se invece lo trova lo seleziona come running
      FakePCB* pcb=(FakePCB*) List_popFront(&os->ready);
      pcb->chunk_start_recorder=os->timer;
      sim_card->avg_waiting_time+=os->timer-pcb->waiting_chunk_start;
      os->running[i]=pcb;

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
void scheduler_FIFO(FakeOS* os, SimCard* sim_card) {

  // Controlla lo stato dei processi in waiting e li aggiorna di conseguenza
  ListItem* aux=os->waiting.first;
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
      List_detach(&os->waiting, (ListItem*)pcb);

      // Se il processo non ha più eventi viene terminato
      if (!pcb->events.first) { printf(" - the process has finished");
        sim_card->avg_turnaround_time+=os->timer-pcb->arrival_time;
        free(pcb);
      }
      // Altrimenti a seconda del tipo di evento viene assegnato a ready o waiting
      else {
        e=(ProcessEvent*) pcb->events.first;

        // Assegnazione in ready
        if (e->type == CPU) { printf(" - next event: CPU Burst --> moving to ready list");
          pcb->waiting_chunk_start=os->timer;
          pcb->response_chunk_start=os->timer;
          List_pushBack(&os->ready, (ListItem*) pcb);
        }

        // Assegnazione in waiting
        else { printf(" - next event: IO Burst --> moving to waiting list");
          List_pushBack(&os->waiting, (ListItem*) pcb);
        }

      }
    }
    // Altrimenti procede con gli altri processi
    else printf("ending in %02d", e->duration);
  } printf("\n|");

  // Controlla lo stato dei processi in running e li aggiorna di conseguenza
  for (int i=0;i<os->num_of_cpu;i++) {

    // Se la CPU corrente ha un processo in running lo esamina altrimenti procede con la prossima
    FakePCB* current_running=os->running[i];
    if (current_running) {
      ProcessEvent* e=(ProcessEvent*) current_running->events.first;
      assert(e->type==CPU); printf("\n| CPU(%d) working on PID: %d - ", i, current_running->pid);
      e->duration--;
      sim_card->cpu_work[i]++;

      // Se il processo ha terminato l'evento lo gestisce
      if (!e->duration){ printf("time expired");
        List_popFront(&current_running->events);
        free(e);

        // Se il processo non ha più eventi viene terminato
        if (!current_running->events.first) { printf(" - the process has finished");
          sim_card->avg_turnaround_time+=os->timer-current_running->arrival_time;
          free(current_running);
        }
        // Altrimenti a seconda del tipo di evento viene assegnato a ready o waiting
        else {
          e=(ProcessEvent*) current_running->events.first;

          // Assegnazione in ready
          if (e->type == CPU){ printf(" - next event: CPU Burst --> moving to ready list");
            current_running->waiting_chunk_start=os->timer;
            List_pushBack(&os->ready, (ListItem*) current_running);
          }
          // Assegnazione in waiting e aggiornamento del tempo medio di risposta
          else { printf(" - next event: IO Burst --> moving to waiting list");
            if (current_running->first_cpu_burst==1) sim_card->avg_response_time+=os->timer-current_running->response_chunk_start;
            else current_running->first_cpu_burst=1;
            List_pushBack(&os->waiting, (ListItem*) current_running);
          }
        }

        // Rimuove il processo dal running corrente che eventualmente verrà riselezionato in futuro
        os->running[i] = 0;
      }
      // Altrimenti procede
      else printf("ending in %02d", e->duration);
    }

  } printf("\n|");

  // Stampa la ready list
  aux=os->ready.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;
    printf("\n| Ready PID: %d", pcb->pid);
  } printf("\n|");

  // Controlla quali CPU non sono in running e le imposta su gli eventuali processi in ready
  for (int i=0;i<os->num_of_cpu;i++) {
    if (!os->running[i]) {

      // Cerca il prossimo processo in ready e si ferma se non lo trova
      if (!os->ready.first) return;

      // Se invece lo trova lo seleziona come running
      FakePCB* pcb=(FakePCB*) List_popFront(&os->ready);
      sim_card->avg_waiting_time+=os->timer-pcb->waiting_chunk_start;
      os->running[i]=pcb;

      // Deve avere almeno un evento
      assert(pcb->events.first);
      ProcessEvent* e = (ProcessEvent*)pcb->events.first;
      // L'evento deve essere di tipo CPU
      assert(e->type==CPU);

      printf("\n| Starting pid %d on CPU(%d) - event duration from the file: %d", pcb->pid, i, e->duration);
    }
  }

};

