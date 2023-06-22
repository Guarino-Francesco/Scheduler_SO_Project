#pragma once
#include "../linked_list/linked_list.h"

typedef enum {CPU=0, IO=1} ResourceType;


typedef struct {
  ListItem list;     // Struct della quale ProcessEvent è ereditaria
  ResourceType type; // Tipo di Burst (CPU/IO)
  int duration;      // Durata del Burst
} ProcessEvent;


typedef struct {
  ListItem list;            // Struct della quale FakePCB è ereditaria
  int pid;                  // Process ID (letto dal file descrittivo)
  int arrival_time;         // Tempo di arrivo del processo rispetto al timer dell'OS (letto dal file descrittivo)

  int chunk_start_recorder; // Tempo di inizio misurazione della porzione di durata del Burst
  int chunk_duration_sum;   // Somma di tutte le porzioni di durata del Burst
  float predicted_duration; // Durata stimata del prossimo Burst (di default è uguale al quanto di tempo)

  ListHead events;          // Lista dei ProcessEvent associati al processo
} FakePCB;


int FakePCB_loadProcessInfo(FakePCB* p, const char* filename);
int FakePCB_saveProcessInfo(const FakePCB* p, const char* filename);
