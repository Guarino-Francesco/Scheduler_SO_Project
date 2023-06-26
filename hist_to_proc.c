#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include "hist_utils/hist_utils.h"

// Funzione main
int main(int argc, char** argv) {

  srand(time(0));
  int current_pid=1;

  // Guida nell'inserimento degli argomenti
  if (argc==1) { printf("\nUsage: ./hist_to_proc [NUMERO DI PROCESSI DA VOLER GENERARE DA OGNI HISTOGRAMMA] [PROCESS HISTOGRAM FILE PATH 1] [PROCESS HISTOGRAM FILE PATH 2] ... [PROCESS HISTOGRAM FILE PATH n]\n"); return 0; }

  // Per ogni file inserto da argv[2] in poi genererà argv[1] file descrittivi dei processi seguendo i record nell'istogramma fornito
  printf("\nLoading histogram records files\n");
  for (int file_arg=2;file_arg<argc;file_arg++){
    printf("\n\n\nFILE: %s", argv[file_arg]);

    // Per ogni file genera argv[1] file descrittivi
    for (int proc=0;proc<atoi(argv[1]);proc++) {

      int cpu_distribution[MAX_RECORDS_NUMBER]; // Array distributivo dei record di CPU dell'istogramma
      int cpu_cumulative[MAX_RECORDS_NUMBER];   // Array cumulativo dei record di CPU
      int cpu_records=0;                        // Numero di record di CPU

      int io_distribution[MAX_RECORDS_NUMBER]; // Array distributivo dei record di IO dell'istogramma
      int io_cumulative[MAX_RECORDS_NUMBER];   // Array cumulativo dei record di IO
      int io_records=0;                        // Numero di record di IO

      // Inizializzazione a 0
      for (int i=0;i<MAX_RECORDS_NUMBER;i++) cpu_distribution[i]=io_distribution[i]=cpu_cumulative[i]=io_cumulative[i]=0;

      // Se non ci sono errori, aggiorna gli array con i dati nei file degli istogrammi
      int res=Hist_loadRecords(cpu_distribution, io_distribution, cpu_cumulative, io_cumulative, &cpu_records, &io_records, argv[file_arg]);

      // Se c'è errore dato dall'apertura del file oppure dato dai dati dell'istogramma non sufficienti passa al prossimo file
      if (res==-1) { printf(" - Errore durante l'apertura del file.\n\n"); break; }
      if (res==1)  { printf(" - Not Loaded. Require at least 1 record per burst type.\n#########################################################################################\n\n"); break; }

      // Se non ci sono stati errori procede con il caricamento
      printf("\n"
        "Generating the %d° description file: Loading...\n"
        " ___________________________________________________________________________________________________\n"
        "| PID: %2d, CPU records: %2d, IO records: %2d",
        proc+1, current_pid, cpu_records, io_records);

      // Genera un arrival time random in range [0:ARRARRIVAL_UPPER_LIMIT)
      int arrival_time=(((float) rand()/RAND_MAX)*ARRIVAL_UPPER_LIMIT);printf(", Arrival time: %2d", arrival_time);

      // Stampa degli array insieme con delle celle del tipo: i-esima stampa = [ i-esimo distribuzione : i-esimo cumulativo ]
      printf("\n|___________________________________________________________________________________________________|\n| CPU: ");
      for (int i=0;i<cpu_records;i++) printf("[ %2d:%2d ]", cpu_distribution[i], cpu_cumulative[i]);
      printf("\n|\n| IO:  ");
      for (int i=0;i<io_records;i++) printf("[ %2d:%2d ]", io_distribution[i], io_cumulative[i]);
      printf("\n|___________________________________________________________________________________________________|");

      // Inizializza i dati del processo
      FakePCB* new_pcb=(FakePCB*)malloc(sizeof(FakePCB));
      new_pcb->pid=current_pid; new_pcb->arrival_time=arrival_time;
      new_pcb->list.next=new_pcb->list.prev=0;
      List_init(&new_pcb->events);
      new_pcb->first_cpu_burst=new_pcb->response_chunk_start=new_pcb->waiting_chunk_start=0;

      // Genera un numero di burst random in range [BURST_NUM_UPPER_LIMIT:BURST_NUM_LOWER_LIMIT)
      int burst_num=(((float) rand()/RAND_MAX)*(BURST_NUM_UPPER_LIMIT-BURST_NUM_LOWER_LIMIT))+BURST_NUM_LOWER_LIMIT;
      printf("\n| Number of events of random duration generated: %2d                                                 |", burst_num);

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
      printf("| Process descriprion file saved in '%s'\n"
             "|___________________________________________________________________________________________________|\n\n", desc_proc_filename);

      current_pid++;
    }
  }

  printf("\n _____________________________________________________________\n"
           "| Check the 'histo_processes_files' directory for the results |\n"
           " #############################################################\n\n"
           "Exiting...\n");
}
