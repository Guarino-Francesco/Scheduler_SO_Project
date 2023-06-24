#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include "hist_utils/hist_utils.h"

#define MAX_RECORDS_NUMBER 10
#define BURST_NUM_UPPER_LIMIT 6
#define BURST_NUM_LOWER_LIMIT 2
#define MIN_BURST_DURATION 1
#define ARRIVAL_UPPER_LIMIT 5

// Funzione main
int main(int argc, char** argv) {

  srand(time(0));
  int current_pid=1;

  // Guida nell'inserimento degli argomenti
  if (argc==1) { printf("\nUsage: ./hist_to_proc [PROCESS HISTOGRAM FILE PATH 1] [PROCESS HISTOGRAM FILE PATH 2] ... [PROCESS HISTOGRAM FILE PATH n]\n"); return 0; }

  // Caricamento dei record degli istogrammi forniti da linea di comando
  printf("\nLoading histogram records files\n");
  for (int i=1; i<argc; ++i){

    int cpu_distribution[MAX_RECORDS_NUMBER]; // Array distributivo dei record di CPU dell'istogramma
    int cpu_cumulative[MAX_RECORDS_NUMBER];   // Array cumulativo dei record di CPU
    int cpu_records=0;                        // Numero di record di CPU

    int io_distribution[MAX_RECORDS_NUMBER]; // Array distributivo dei record di IO dell'istogramma
    int io_cumulative[MAX_RECORDS_NUMBER];   // Array cumulativo dei record di IO
    int io_records=0;                        // Numero di record di IO

    // Inizializzazione a 0
    for (int i=0;i<MAX_RECORDS_NUMBER;i++) cpu_distribution[i]=io_distribution[i]=cpu_cumulative[i]=io_cumulative[i]=0;

    // Aggiorna gli array con i dati nei file degli istogrammi
    int res=Hist_loadRecords(cpu_distribution, io_distribution, cpu_cumulative, io_cumulative, &cpu_records, &io_records, argv[i]);
    // Errore nell'apertura del file
    if (res==-1) { printf("Errore durante l'apertura del file.\n"); continue; }
    else {
      printf("\n\nFILE: %s", argv[i]);
      if (res==1) { printf(" - Not Loaded. Require at least 1 record per burst type (CPU/IO). |\n\n");
                    continue; }
      else printf(" - Loading...\nCPU records: %02d, IO records: %02d", cpu_records, io_records);
    }

    // Genera un arrival time random in range [0:ARRARRIVAL_UPPER_LIMIT)
    int arrival_time=(((float) rand()/RAND_MAX)*ARRIVAL_UPPER_LIMIT);printf(" - Arrival time: %02d", arrival_time);

    // Stampa degli array (cumulativo e di distribuzione)
    printf("\n|___________________________________________________________________________________________________\n| CPU: ");
    for (int i=0;i<cpu_records;i++ ) printf("[ %02d:%02d ]", cpu_distribution[i], cpu_cumulative[i]);
    printf("\n|___________________________________________________________________________________________________|\n| IO:  ");
    for (int i=0;i<io_records;i++ ) printf("[ %02d:%02d ]", io_distribution[i], io_cumulative[i]);
    printf("\n|___________________________________________________________________________________________________|");

    // Inizializza i dati del processo
    FakePCB* new_pcb=(FakePCB*)malloc(sizeof(FakePCB));
    new_pcb->pid=current_pid; new_pcb->arrival_time=arrival_time;
    new_pcb->list.next=new_pcb->list.prev=0;
    List_init(&new_pcb->events);
    new_pcb->first_cpu_burst=new_pcb->response_chunk_start=new_pcb->waiting_chunk_start=0;

    // Genera un numero di burst random in range [BURST_NUM_UPPER_LIMIT:BURST_NUM_LOWER_LIMIT)
    int burst_num=(((float) rand()/RAND_MAX)*(BURST_NUM_UPPER_LIMIT-BURST_NUM_LOWER_LIMIT))+BURST_NUM_LOWER_LIMIT;
    printf("\n| Number of events of random duration generated: %02d                                                 |", burst_num);

    // Ad ogni ciclo genera un evento associato e un random da utilizzare per la durata
    for (int i=0;i<(burst_num);i++) { float rand_start=((float) rand()/RAND_MAX);

      // I pari saranno CPU burst
      if (i%2==0) { int duration=(rand_start*((cpu_cumulative[cpu_records-1])-MIN_BURST_DURATION)+MIN_BURST_DURATION);

        // Cerca a quale intervallo appartiene sull'array cumulativo dei CPU burst
        for (int j=0;j<cpu_records;j++) {
          // Una volta trovato imposta la durata a quella associata nell'array della distribuzione di CPU
          if (duration<cpu_cumulative[j]) { duration=cpu_distribution[j]; break; }
        }

        // Genera l'evento e lo inserisce accoda agli eventi del processo
        ProcessEvent* e=(ProcessEvent*) malloc(sizeof(ProcessEvent));
        e->list.prev=e->list.next=0;
        e->type=CPU;
        e->duration=duration;
        List_pushBack(&new_pcb->events, (ListItem*)e);
      }
      // I pari saranno IO burst
      else { int duration=(rand_start*((io_cumulative[io_records-1])-MIN_BURST_DURATION)+MIN_BURST_DURATION);

        // Cerca a quale intervallo appartiene sull'array cumulativo degli IO burst
        for (int j=0;j<io_records;j++) {
          // Una volta trovato imposta la durata a quella associata nell'array della distribuzione di IO
          if (duration<io_cumulative[j]) { duration=io_distribution[j]; break; }
        }

        // Genera l'evento e lo inserisce accoda agli eventi del processo
        ProcessEvent* e=(ProcessEvent*) malloc(sizeof(ProcessEvent));
        e->list.prev=e->list.next=0;
        e->type=IO;
        e->duration=duration;
        List_pushBack(&new_pcb->events, (ListItem*)e);
      }
    }

    // Stampa gli eventi generati
    printf("\n| Events: ");
    ListItem* aux=new_pcb->events.first;
    while(aux){
      ProcessEvent* e=(ProcessEvent*)aux;
      aux=aux->next;
      printf("%s:%d%s", (e->type==CPU)?"CPU":"IO", e->duration, (aux)?" --> ":"");
    } printf("\n|___________________________________________________________________________________________________|\n");

    // Salvataggio su file
    char desc_proc_filename[30];
    sprintf(desc_proc_filename, "histo_processes_files/hp%d", current_pid);
    FakePCB_saveProcessInfo(new_pcb, desc_proc_filename);
    printf("| Process descriprion file saved in '%s'                                     |\n"
           "|___________________________________________________________________________________________________|\n", desc_proc_filename);

    current_pid++;
  }

  printf("\nCheck 'histo_processes_files' for the results\n\nExiting...\n");
}
