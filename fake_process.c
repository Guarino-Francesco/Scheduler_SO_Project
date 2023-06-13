#include <stdio.h>
#include <stdlib.h>
#include "fake_process.h"

#define LINE_LENGTH 1024

int FakeProcess_load(FakeProcess* p, const char* filename) {
  FILE* f=fopen(filename, "r");
  if (!f) return -1;

  char *buffer=NULL;
  size_t line_length=0;

  p->pid=-1;
  p->arrival_time=-1;
  List_init(&p->events);
  p->list.prev=p->list.next=0;
  int num_events=0;

  while (getline(&buffer, &line_length, f) >0){
    int pid=-1;
    int arrival_time=-1;
    int num_tokens=0;
    int duration=-1;

    num_tokens=sscanf(buffer, "P %d %d", &pid, &arrival_time);
    if (num_tokens==2 && p->pid<0){
      p->pid=pid;
      p->arrival_time=arrival_time;
      continue;
    }

    num_tokens=sscanf(buffer, "C %d", &duration);
    if (num_tokens==1){
      ProcessEvent* e=(ProcessEvent*) malloc(sizeof(ProcessEvent));
      e->list.prev=e->list.next=0;
      e->type=CPU;
      e->duration=duration;

      List_pushBack(&p->events, (ListItem*)e);
      num_events++;

      continue;
    }

    num_tokens=sscanf(buffer, "I %d", &duration);
    if (num_tokens==1){
      ProcessEvent* e=(ProcessEvent*) malloc(sizeof(ProcessEvent));
      e->list.prev=e->list.next=0;
      e->type=IO;
      e->duration=duration;

      List_pushBack(&p->events, (ListItem*)e);
      num_events++;

      continue;
    }

  }

  if (buffer) free(buffer);
  fclose(f);
  return num_events;
}

int FakeProcess_save(const FakeProcess* p, const char* filename){
  FILE* f=fopen(filename, "w");
  if (!f) return -1;

  fprintf(f, "P %d %d\n", p->pid, p->arrival_time);

  ListItem* aux=p->events.first;
  int num_events = 0;

  while(aux) {
    ProcessEvent* e=(ProcessEvent*) aux;
    switch(e->type){
      case CPU: fprintf(f, "C %d\n", e->duration);
      case IO:  fprintf(f, "I %d\n", e->duration);
      default:  num_events++;
    }
    aux=aux->next;
  }

  fclose(f);
  return num_events;
}
