#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "fake_os.h"

void FakeOS_init(FakeOS* os) {
  os->running=(FakePCB**)malloc(sizeof(FakePCB*)*NUMBERS_OF_CPU);
  List_init(&os->ready);
  List_init(&os->waiting);

  os->timer=0;
  os->schedule_fn=0;
  os->schedule_args=(SchedArgs*)malloc(sizeof(SchedArgs));

  List_init(&os->processes);
}

void FakeOS_createProcess(FakeOS* os, FakeProcess* p) {
  // Il deve effettivamente avere l'arrival_time corrente
  assert((p->arrival_time==os->timer) && "time mismatch in creation");
  // Il pid non deve essere uguale a quello degli eventuali processi in running
  for (int i=0;i<NUMBERS_OF_CPU;i++) { assert((!(os->running[i]) || ((os->running[i])->pid!=p->pid)) && "pid taken"); }

  // Controlla che non ci sia gia il pid nella coda di ready
  ListItem* aux=os->ready.first;
  while(aux){
    FakePCB* pcb=(FakePCB*)aux;
    assert(pcb->pid!=p->pid && "pid taken");
    aux=aux->next;
  }

  // Controlla che non ci sia gia il pid nella coda di waiting
  aux=os->waiting.first;
  while(aux){
    FakePCB* pcb=(FakePCB*)aux;
    assert(pcb->pid!=p->pid && "pid taken");
    aux=aux->next;
  }

  // Il processo deve avere almeno un evento
  assert(p->events.first && "process without events");

  // Crea il processo
  FakePCB* new_pcb=(FakePCB*) malloc(sizeof(FakePCB));
  new_pcb->list.next=new_pcb->list.prev=0;
  new_pcb->pid=p->pid;
  new_pcb->chunk_start_recorder=0;
  new_pcb->chunk_length_sum=0;
  new_pcb->predicted_job_length=10;
  new_pcb->events=p->events;

  // Assegna il processo all'opportuna coda in base al tipo del suo primo evento
  ProcessEvent* e=(ProcessEvent*)new_pcb->events.first;
  if (e->type==CPU){
    // Assegnazione ordinata in ready
    ListItem* aux=os->ready.first;
    int inserted=0;

    if (!aux) {
      List_pushBack(&os->ready, (ListItem*) new_pcb);
      inserted=1;
    }
    while(!inserted && aux) {
      FakePCB* ready_pcb=(FakePCB*)aux;
      aux=aux->next;
      FakePCB* next_pcb=(FakePCB*)aux;

      if (new_pcb->predicted_job_length<ready_pcb->predicted_job_length) {
        List_pushFront(&os->ready, (ListItem*) new_pcb);
        break;
      }
      else {
        if (!next_pcb) {
          List_pushBack(&os->ready, (ListItem*) new_pcb);
          break;
        }
        else if (new_pcb->predicted_job_length<next_pcb->predicted_job_length) {
          List_insert(&os->ready, (ListItem*)ready_pcb, (ListItem*)new_pcb);
          break;
        }
        else next_pcb=(FakePCB*)aux->next;
      }
    }
  }
  else if (e->type==IO) {
    List_pushBack(&os->waiting, (ListItem*) new_pcb);
  }
  else assert(0 && "illegal resource");
}

void FakeOS_simStep(FakeOS* os){
  printf("\n\nTime: %04d -------------------------------------------------------", os->timer);

  // Controlla se sono arrivati nuovi processi
  ListItem* aux=os->processes.first;
  while (aux){
    FakeProcess* proc=(FakeProcess*)aux;
    aux=aux->next;
    if (proc->arrival_time==os->timer) {
      printf("\nCreating process... pid:%d", proc->pid);
      FakeProcess* new_process=(FakeProcess*)List_detach(&os->processes, (ListItem*)proc);
      FakeOS_createProcess(os, new_process);
      free(new_process);
    }
  }
  printf("\n");

  // Controlla lo stato dei processi in waiting e li aggiorna di conseguenza
  aux=os->waiting.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;

    ProcessEvent* e=(ProcessEvent*) pcb->events.first;
    assert(e->type==IO); printf("\nWaiting %d - ", pcb->pid);
    e->duration--;
    if (e->duration) printf("ending in %02d",e->duration);
    else printf("time expired");

    if (e->duration==0){
      printf(" --> ");
      List_popFront(&pcb->events);
      free(e);
      List_detach(&os->waiting, (ListItem*)pcb);

      if (!pcb->events.first) {
        printf("ending process");
        free(pcb);
      }
      else {
        e=(ProcessEvent*) pcb->events.first;
        if (e->type == CPU){
          List_pushBack(&os->ready, (ListItem*) pcb);
          printf("moving to ready list");
        }
        else {
          List_pushBack(&os->waiting, (ListItem*) pcb);
          printf("moving to waiting list");
        }
      }
    }
    else {
      printf(" - Predicted job duration: %d",pcb->predicted_job_length);
    }
  }
  printf("\n");

  // Controlla lo stato dei processi in running
  for (int i=0;i<NUMBERS_OF_CPU;i++) {
    printf("\nCPU(%d)",i);
    FakePCB* current_running=os->running[i];
    if (current_running) {
      ProcessEvent* e=(ProcessEvent*) current_running->events.first;
      assert(e->type==CPU); printf(": Running %d - ", current_running->pid);
      e->duration--; 
      if (e->duration) printf("ending in %02d",e->duration);
      else printf("time expired");

      if (e->duration==0){
        printf(" --> ");
        List_popFront(&current_running->events);
        free(e);

        if (!current_running->events.first) {
          printf("ending process");
          free(current_running);
        }
        else {
          e=(ProcessEvent*) current_running->events.first;
          if (e->type == CPU){
            current_running->chunk_length_sum+=os->timer-current_running->chunk_start_recorder;

            // Assegnazione ordinata in ready
            ListItem* aux=os->ready.first;
            int inserted=0;

            if (!aux) {
              List_pushBack(&os->ready, (ListItem*) current_running);
              inserted=1;
            }
            while(!inserted && aux) {
              FakePCB* ready_pcb=(FakePCB*)aux;
              aux=aux->next;
              FakePCB* next_pcb=(FakePCB*)aux;

              if (current_running->predicted_job_length<=ready_pcb->predicted_job_length) {
                List_pushFront(&os->ready, (ListItem*) current_running);
                break;
              }
              else {
                if (!next_pcb) {
                  List_pushBack(&os->ready, (ListItem*) current_running);
                  break;
                }
                else if (current_running->predicted_job_length<next_pcb->predicted_job_length) {
                  List_insert(&os->ready, (ListItem*)ready_pcb, (ListItem*)current_running);
                  break;
                }
                else next_pcb=(FakePCB*)aux->next;
              }
            }

            printf("moving to ready list");
          }
          else {
            int measured_time=os->timer-current_running->chunk_start_recorder+current_running->chunk_length_sum;
            int predicted_time=DECAY_COEFFICIENT*(measured_time)+(1-DECAY_COEFFICIENT)*(current_running->predicted_job_length);
            current_running->predicted_job_length=(int)predicted_time;
            current_running->chunk_length_sum=0;
            List_pushBack(&os->waiting, (ListItem*) current_running);
            printf("moving to waiting list");
          }
        }

        // Rimuove il processo running corrente che verrà riselezionato successivamente
        os->running[i] = 0;
      }
    }
    else {
      printf(": not running");
    }
  }
  printf("\n");

  // Stampa la ready list
  aux=os->ready.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;
    printf("\nReady %d - Predicted job duration: %d", pcb->pid, pcb->predicted_job_length);
  }
  printf("\n");

  // Controlla quali running sono terminati e se è impostato uno scheduler lo chiama per scegliere il running mancante
  if (os->schedule_fn) {
    for (int i=0;i<NUMBERS_OF_CPU;i++) {
      os->schedule_args->CPU_index=i;
      if (!os->running[i]) (*os->schedule_fn)(os, os->schedule_args);
    }
  }

  // Se lo scheduler non ha impostato i running terminati, verrà impostato l'eventuale primo processo in ready
  for (int i=0;i<NUMBERS_OF_CPU;i++) {
    if (!os->running[i] && os->ready.first) os->running[i]=(FakePCB*) List_popFront(&os->ready);
  }

  os->timer++;
  printf("\n------------------------------------------------------------------\n");
}

void FakeOS_destroy(FakeOS* os) { }