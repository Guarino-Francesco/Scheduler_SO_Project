#include <stdio.h>
#include <stdlib.h>
#include "hist_utils.h"

#define LINE_LENGTH 1024

int Hist_loadRecords(int* cpu_distribution, int* io_distribution, int* cpu_cumulative, int* io_cumulative, int* cpu_records, int* io_records, const char* filename) {
  // Apre il file designato in lettura e ritorna se c'è errore
  FILE* f=fopen(filename, "r");
  if (!f) return -1;

  // Inizializza il buffer per la lettura
  char *buffer=NULL;
  size_t line_length=0;

  // Finché si riesce a leggere dal file
  int cpu_range=0, io_range=0, burst=0, res;
  while (getline(&buffer, &line_length, f)>0){

    // Se la riga letta presenta una "C" è un record di un CPU Burst
    res=sscanf(buffer, "C %d", &burst);
    if (res==1){  cpu_range+=burst;
      cpu_cumulative[(*cpu_records)]=cpu_range;
      cpu_distribution[(*cpu_records)]=burst;
      (*cpu_records)++;
      continue;
    }

    // Se la riga letta presenta una "I" è un record di un IO Burst
    res=sscanf(buffer, "I %d", &burst);
    if (res==1){ io_range+=burst;
      io_cumulative[(*io_records)]=io_range;
      io_distribution[(*io_records)]=burst;
      (*io_records)++;
      continue;
    }

  }

  // Se non ha almeno un record per tipologia di burst ritorna errore
  if ((*cpu_records)==0||(*io_records)==0) return 1;

  // Chiusura
  if (buffer) free(buffer);
  fclose(f);
  return 0;

}
