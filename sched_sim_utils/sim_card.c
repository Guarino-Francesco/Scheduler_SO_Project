#include "sched_sim.h"


// Stampa i risultati e le metriche della simulazione sm
void SimCard_Print(SimCard* sim_card) {

  printf("\n\n"
  " ______________________________________ \n"
  "|Scheduler: %s\n"
  "|Available CPU: %d - Duration: %d\n"
  "|______________________________________|\n"
  "|Throughput: %f                  |\n"
  "|______________________________________|\n"
  "|Average CPU utilization:%6.2f%c       |",
  (sim_card->scheduler_id==2)? "RR                         |":
  (sim_card->scheduler_id==1)? "PSJFQP                     |":"FIFO                       |",
  sim_card->num_of_cpu,
  sim_card->sim_duration,
  sim_card->throughput,
  sim_card->avg_work, 37);

  for (int i=0;i<sim_card->num_of_cpu;i++) printf("\n|  CPU[%03d]:%6.2f%c                    |", i, sim_card->cpu_work[i], 37);

  printf("\n"
  "|______________________________________|\n"
  "|Average times:                        |\n"
  "|  Turnaround:%7.2f TimeUnit         |\n"
  "|  Response:  %7.2f TimeUnit         |\n"
  "|  Waiting:   %7.2f TimeUnit         |\n"
  "|______________________________________|\n\n",
  sim_card->avg_turnaround_time,
  sim_card->avg_response_time,
  sim_card->avg_waiting_time);

}

// Stampa i risultati e le metriche della simulazione sm
void SimCard_SaveOnFile(SimCard* sim_card, char* filename) {
  // Apre il file designato in scrittura e ritorna se c'è errore
  FILE* f=fopen(filename, "a");
  assert(f && "Error open file");

  fprintf(f,
  " ______________________________________ \n"
  "|Scheduler: %s\n"
  "|Available CPU: %d - Duration: %d\n"
  "|______________________________________|\n"
  "|Throughput: %f                  |\n"
  "|______________________________________|\n"
  "|Average CPU utilization:%6.2f%c       |",
  (sim_card->scheduler_id==2)? "RR                         |":
  (sim_card->scheduler_id==1)? "PSJFQP                     |":"FIFO                       |",
  sim_card->num_of_cpu,
  sim_card->sim_duration,
  sim_card->throughput,
  sim_card->avg_work, 37);

  for (int i=0;i<sim_card->num_of_cpu;i++) fprintf(f, "\n|  CPU[%03d]:%6.2f%c                    |", i, sim_card->cpu_work[i], 37);

  fprintf(f, "\n"
  "|______________________________________|\n"
  "|Average times:                        |\n"
  "|  Turnaround:%7.2f TimeUnit         |\n"
  "|  Response:  %7.2f TimeUnit         |\n"
  "|  Waiting:   %7.2f TimeUnit         |\n"
  "|______________________________________|\n\n\n",
  sim_card->avg_turnaround_time,
  sim_card->avg_response_time,
  sim_card->avg_waiting_time);

}

// Salva i risultati e chiama le stampe
void SimCard_SaveAndPrint(SimCard* sim_card, FakeOS* os) {

  sim_card->sim_duration=os->timer;        // Durata
  sim_card->scheduler_id=os->scheduler_id; // ID Scheduler
  sim_card->num_of_cpu=os->num_of_cpu;     // Numero di CPU

  // Calcolo del Throughput
  sim_card->throughput=((float)sim_card->procs_count)/((float)os->timer);

  // Calcolo degli utilizzi delle CPU
  float cpu_work_sum;
  for (int i=0;i<os->num_of_cpu;i++) {
    float curremt_percentage_of_work=sim_card->cpu_work[i]/((float)os->timer)*100;
    sim_card->cpu_work[i]=curremt_percentage_of_work;
    cpu_work_sum+=curremt_percentage_of_work;
  }
  sim_card->avg_work=cpu_work_sum/sim_card->num_of_cpu;

  // Calcolo dei tempi medi: turnaround, response e waiting.
  sim_card->avg_turnaround_time=sim_card->avg_turnaround_time/((float)sim_card->procs_count);
  sim_card->avg_response_time=sim_card->avg_response_time/((float)sim_card->procs_count);
  sim_card->avg_waiting_time=sim_card->avg_waiting_time/((float)sim_card->procs_count);

  // Salvataggio su file totale delle simulazioni
  SimCard_SaveOnFile(sim_card, "SimulationsSummary");

  // Salvataggio su file della singola simulazione
  char filename[50]; sprintf(filename, "SimSched %s #CPU %3d",
    (sim_card->scheduler_id==2)? "RR":
    (sim_card->scheduler_id==1)? "PSJFQP":"FIFO",
    sim_card->num_of_cpu);
  SimCard_SaveOnFile(sim_card, filename);

  // Stampa della simulazione
  SimCard_Print(sim_card);

}
