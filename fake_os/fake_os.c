#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "fake_os.h"

// Inizializza l'OS e assegna il numero di processori disponibili
void FakeOS_init(FakeOS* os, int num_of_cpu, short scheduler_id, ScheduleFn scheduler_fn) {
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
      SchedArgsPSJFQP* scheduler_args=(SchedArgsPSJFQP*)malloc(sizeof(SchedArgsPSJFQP));
      scheduler_args->quantum=10;
      scheduler_args->decay_coefficient=0.5;
      os->scheduler_args=scheduler_args;
      break;
  }

  List_init(&os->processes);
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
  // Il pid non deve essere uguale a quello degli eventuali processi in running
  for (int i=0;i<os->num_of_cpu;i++) { assert((!(os->running[i]) || ((os->running[i])->pid!=p->pid)) && "pid taken"); }

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
      // Assegnazione in ready
      if (e->type==CPU) List_pushBack(&os->ready, (ListItem*) p);
      // Assegnazione in waiting
      else if (e->type==IO) List_pushBack(&os->waiting, (ListItem*) p);
      // Errore
      else assert(0 && "illegal resource");
      break;
    case 1:
      p->chunk_start_recorder=0;
      p->chunk_duration_sum=0;
      p->predicted_duration=((SchedArgsPSJFQP*)(os->scheduler_args))->quantum;
      // Assegnazione ordinata in ready
      if (e->type==CPU) FakeOS_SortedInsertInReady(&os->ready, p);
      // Assegnazione in waiting
      else if (e->type==IO) List_pushBack(&os->waiting, (ListItem*) p);
      // Errore
      else assert(0 && "illegal resource");
      break;
  }
}


// Simula il passaggio di una unità di tempo assoluto dell'OS
void FakeOS_simStep(FakeOS* os){
  printf("\n\nTime: %04d -------------------------------------------------------", os->timer);

  // Controlla se sono arrivati nuovi processi e li crea
  ListItem* list_iter=os->processes.first;
  while (list_iter){
    FakePCB* proc_iter=(FakePCB*)list_iter;
    list_iter=list_iter->next;
    if (proc_iter->arrival_time==os->timer) {
      printf("\nCreating process... pid:%d", proc_iter->pid);
      FakePCB* new_process=(FakePCB*)List_detach(&os->processes, (ListItem*)proc_iter);
      FakeOS_createProcess(os, new_process);
    }
  }
  printf("\n");

  // Chiama lo scheduler (se è stato impostato)
  if (os->scheduler_fn) (*os->scheduler_fn)();

  os->timer++;
  printf("\n------------------------------------------------------------------\n");
}


// Libera lo spazio dell'OS
void FakeOS_destroy(FakeOS* os) {
  free(os->running);
  free(os->scheduler_args);
}
