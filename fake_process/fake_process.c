#include <stdio.h>
#include <stdlib.h>
#include "fake_process.h"

#define LINE_LENGTH 1024

// Carica le informazioni del processo "p" leggendole dal file "filename"
int FakePCB_loadProcessInfo(FakePCB* p, const char* filename) {
  // Apre il file designato in lettura e ritorna se c'è errore
  FILE* f=fopen(filename, "r");
  if (!f) return -1;

  // Inizializza il buffer per la lettura
  char *buffer=NULL;
  size_t line_length=0;

  // Inizializza i dati del processo
  p->pid=-1;
  p->arrival_time=-1;
  p->list.next=p->list.prev=0;
  List_init(&p->events);
  p->list.prev=p->list.next=0;

  // Numero totale di eventi da ritornare al chiamante
  int num_events=0;

  // Finché si riesce a leggere dal file
  while (getline(&buffer, &line_length, f)>0){
    // Inizializzo le variabili di lettura del buffer corrente
    int pid=-1;
    int arrival_time=-1;
    int num_tokens=0;
    int duration=-1;

    // Se la riga letta presenta una "P" è la riga di descrizione del processo
    num_tokens=sscanf(buffer, "P %d %d", &pid, &arrival_time);
    if (num_tokens==2 && p->pid<0){
      p->pid=pid;
      p->arrival_time=arrival_time;
      continue;
    }

    // Se la riga letta presenta una "C" è una riga di descrizione di un CPU Burst
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

    // Se la riga letta presenta una "I" è una riga di descrizione di un IO Burst
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

  // Chiusura
  if (buffer) free(buffer);
  fclose(f);
  return num_events;
}

// Salva le informazioni del processo "p" scrivendole nel file "filename"
int FakePCB_saveProcessInfo(const FakePCB* p, const char* filename){
  // Apre il file designato in scrittura e ritorna se c'è errore
  FILE* f=fopen(filename, "w");
  if (!f) return -1;

  // Scrive la riga descrittiva del processo
  fprintf(f, "P %d %d\n", p->pid, p->arrival_time);

  // Per tuti gli eventi ne stampa la righa descrittiva corrispondente al tipo di Burst (CPU/IO)
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

  // Chiusura
  fclose(f);
  return num_events;
}
