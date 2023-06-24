#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "fake_os.h"

// Inizializza l'OS e assegna il numero di processori disponibili
void FakeOS_init(FakeOS* os, int num_of_cpu, int scheduler_id, ScheduleFn scheduler_fn) {
  os->running=(FakePCB**)malloc(sizeof(FakePCB*)*num_of_cpu);
  List_init(&os->ready);
  List_init(&os->waiting);

  os->timer=0;
  os->num_of_cpu=num_of_cpu;
  os->scheduler_id=scheduler_id;

  switch (scheduler_id) {
    case 0:
      os->scheduler_fn=scheduler_fn;
      os->scheduler_args=0;
      break;
    case 1:
      os->scheduler_fn=scheduler_fn;
      SchedArgsPSJFQP* psjfqp_scheduler_args=(SchedArgsPSJFQP*)malloc(sizeof(SchedArgsPSJFQP));
      psjfqp_scheduler_args->quantum=10;
      psjfqp_scheduler_args->decay_coefficient=0.5;
      os->scheduler_args=psjfqp_scheduler_args;
      break;
    case 2:
      os->scheduler_fn=scheduler_fn;
      SchedArgsRR* rr_scheduler_args=(SchedArgsRR*)malloc(sizeof(SchedArgsRR));
      rr_scheduler_args->quantum=5;
      os->scheduler_args=rr_scheduler_args;
      break;
  }

  List_init(&os->processes);
}

// Inizializza la struttura contenente le metriche dello scheduler
void SimCard_init(FakeOS* os, SimCard* sm) {
  sm->list.prev=sm->list.next=0;
  sm->procs_count=0;

  sm->throughput=0;

  sm->cpu_work=(float*)malloc(sizeof(float)*os->num_of_cpu);
  for (int i=0;i<os->num_of_cpu;i++) sm->cpu_work[i]=0;

  sm->avg_turnaround_time=0;
  sm->avg_response_time=0;
  sm->avg_waiting_time=0;
}



// Inserire i PCB nella lista di ready in ordine di stima della durata del loro prossimo Burst
void FakeOS_SortedInsertInReady(ListHead* ready, FakePCB* pcb_to_insert) {
  ListItem* aux=ready->first;

  // Se la ready list è vuota inserisce direttamente
  if (!aux) { List_pushBack(ready, (ListItem*) pcb_to_insert); return; }

  // Altrimenti scorre la ready list che è ordinata per costruzione
  while(aux) {
    FakePCB* current_pcb=(FakePCB*)aux;
    aux=aux->next;
    FakePCB* next_pcb=(FakePCB*)aux;

    // Inserimento in testa
    if (pcb_to_insert->predicted_duration<current_pcb->predicted_duration) {
      List_pushFront(ready, (ListItem*) pcb_to_insert);
      return;
    }
    else {

      // Inserimento in coda
      if (!next_pcb) {
        List_pushBack(ready, (ListItem*) pcb_to_insert);
        return;
      }

      // Inserimento in mezzo
      if (pcb_to_insert->predicted_duration<next_pcb->predicted_duration) {
        List_insert(ready, (ListItem*)current_pcb, (ListItem*)pcb_to_insert);
        return;
      }

    }

    // Procede con lo scorrimento della lista
    next_pcb=(FakePCB*)aux->next;
  }

}



// Se il PCB "p" passa i controlli viene inserito nella lista opportuna (ready/waiting) dell'OS
void FakeOS_createProcess(FakeOS* os, FakePCB* p) {
  // Il processo deve effettivamente avere il tempo di arrivo corrente
  assert((p->arrival_time==os->timer) && "time mismatch in creation");
  // Il processo deve avere almeno un evento
  assert(p->events.first && "process without events");
  // Controlla che non ci sia gia il pid tra gli eventuali processi in running
  for (int i=0;i<os->num_of_cpu;i++) assert((!(os->running[i]) || ((os->running[i])->pid!=p->pid)) && "pid taken");

  // Controlla che non ci sia gia il pid tra i processi nella coda di ready
  ListItem* aux=os->ready.first;
  while(aux){
    FakePCB* pcb=(FakePCB*)aux;
    assert(pcb->pid!=p->pid && "pid taken");
    aux=aux->next;
  }

  // Controlla che non ci sia gia il pid tra i processi nella coda di waiting
  aux=os->waiting.first;
  while(aux){
    FakePCB* pcb=(FakePCB*)aux;
    assert(pcb->pid!=p->pid && "pid taken");
    aux=aux->next;
  }

  // Se ha pasato tutti i controlli il processo viene assegnato all'opportuna coda in base al tipo di scheduler e del suo primo evento
  ProcessEvent* e=(ProcessEvent*)p->events.first;
  switch(os->scheduler_id) {
    case 0:
      // Assegnazione in ready e registra l'inizio di un chunk di waiting
      if (e->type==CPU) { printf(" - first event: CPU Burst --> moving to ready list");
        p->waiting_chunk_start=os->timer;
        List_pushBack(&os->ready, (ListItem*) p);
      }
      // Assegnazione in waiting
      else if (e->type==IO) { printf(" - first event: IO Burst --> moving to waiting list");
        List_pushBack(&os->waiting, (ListItem*) p);
      }
      // Errore
      else assert(0 && "illegal resource");
      break;
    case 1:
      p->chunk_start_recorder=0;
      p->chunk_duration_sum=0;
      p->predicted_duration=((SchedArgsPSJFQP*)(os->scheduler_args))->quantum;
      // Assegnazione ordinata in ready e registra l'inizio di un chunk di waiting
      if (e->type==CPU) { printf(" - first event: CPU Burst --> moving to ready list");
        p->waiting_chunk_start=os->timer;
        FakeOS_SortedInsertInReady(&os->ready, p);
      }
      // Assegnazione in waiting
      else if (e->type==IO) { printf(" - first event: IO Burst --> moving to waiting list");
        List_pushBack(&os->waiting, (ListItem*) p);
      }
      // Errore
      else assert(0 && "illegal resource");
      break;
    case 2:
      // Assegnazione in ready e registra l'inizio di un chunk di waiting
      if (e->type==CPU) { printf(" - first event: CPU Burst --> moving to ready list");
        p->waiting_chunk_start=os->timer;
        List_pushBack(&os->ready, (ListItem*) p);
      }
      // Assegnazione in waiting
      else if (e->type==IO) { printf(" - first event: IO Burst --> moving to waiting list");
        List_pushBack(&os->waiting, (ListItem*) p);
      }
      // Errore
      else assert(0 && "illegal resource");
      break;
  }
}



// Simula il passaggio di una unità di tempo assoluto dell'OS
void FakeOS_simStep(FakeOS* os, SimCard* sm){
  printf("\n\n"
  "Time: %04d_____________________________________________________________________________________\n|                                                                                              |",
  os->timer);

  // Controlla se sono arrivati nuovi processi e li crea
  ListItem* list_iter=os->processes.first;
  while (list_iter){
    FakePCB* proc_iter=(FakePCB*)list_iter;
    list_iter=list_iter->next;
    if (proc_iter->arrival_time==os->timer) {
      printf("\n| Process arrived - pid:%d", proc_iter->pid);
      FakePCB* new_process=(FakePCB*)List_detach(&os->processes, (ListItem*)proc_iter);
      FakeOS_createProcess(os, new_process);
      sm->procs_count++;
    }
  }

  // Chiama lo scheduler (se è stato impostato)
  if (os->scheduler_fn) (*os->scheduler_fn)(os, sm);

  os->timer++;
  printf("\n|______________________________________________________________________________________________|\n\n\n");
}




// Libera lo spazio dell'OS
void FakeOS_destroy(FakeOS* os) {
  free(os->running);
  free(os->scheduler_args);
}
