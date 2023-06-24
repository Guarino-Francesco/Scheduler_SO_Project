#include "sched_sim_utils/sched_sim.h"

FakeOS os;
SimCard sim_card;

// Funzione main
int main(int argc, char** argv) {

  // Guida nell'inserimento degli argomenti
  if (argc==1) { printf("\nUsage: ./sched_sim [PROCESS DESCRIPTION FILE PATH 1] [PROCESS DESCRIPTION FILE PATH 2] ... [PROCESS DESCRIPTION FILE PATH n]\n"); return 0; }


  printf("\n\n______________________________________________________________________________________\n"
             "### INITIALIZING A NEW SIMULATION ####################################################\n");

  // Richiede la tipologia di scheduler
  int scheduler_id;
  printf("\n\n"
  " _____________________\n"
  "| Available scheduler |\n"
  "_______________________________________________________________________________ \n"
  "|ID:0 = FIFO   - First In First Out                                            |\n"
  "|______________________________________________________________________________|\n"
  "|ID:1 = PSJFQP - Preemptive Shortest Job First with Quantum Prediction         |\n"
  "|______________________________________________________________________________|\n"
  "|ID:2 = RR     - Round Robin                                                   |\n"
  "|______________________________________________________________________________|\n\n"
  "Insert here the ID of the scheduler that you want to run in this simulation: "
  );
  while (scanf("%d", &scheduler_id)!=1 || (scheduler_id<0 || scheduler_id>=AVAILABLE_SCHEDULERS)) {
      printf("\nERROR: you have to insert an ID among the ID in the scheduler's table!\nRetry: ");
      if (feof(stdin) || ferror(stdin)) return 0;
      int c;
      while ((c=getchar())!=EOF && c!='\n');
      if (c == EOF) return 0;
  }

  // Richiede il numero di CPU
  int number_of_cpu;
  printf("\nInsert the NUMBER (in digits and greater than 0) of CPU to enable in the simulator: ");
  while (scanf("%d", &number_of_cpu)!=1 || (number_of_cpu<=0)) {
      printf("\nERROR: you have to insert a NUMBER in digits and greater than 0!\nRetry: ");
      if (feof(stdin) || ferror(stdin)) return 0;
      int c;
      while ((c=getchar())!=EOF && c!='\n');
      if (c == EOF) return 0;
  }

  // Inizializza l'OS e la Simulazione
  ScheduleFn scheduler_fn;
  printf("\nSelected scheduler: ");
  switch (scheduler_id) {
    case 0:
      printf("FIFO - First In First Out\n");
      scheduler_fn = scheduler_FIFO;
      break;
    case 1:
      printf("PSJFQP - Preemptive Shortest Job First with Quantum Prediction\n");
      scheduler_fn = scheduler_P_SJF_QP;
      break;
    case 2:
      printf("RR - Round Robin\n");
      scheduler_fn = scheduler_RR;
      break;
  } printf("\nNumber of available CPU: %d\n", number_of_cpu);
  // Inizializza l'OS
  FakeOS_init(&os, number_of_cpu, scheduler_id, scheduler_fn);
  // Inizializza il record delle metriche
  SimCard_init(&os, &sim_card);



  // Caricamento dei processi descritti nei file forniti da linea di comando
  int refused = 0;
  printf("\nLoading processes description files:\n");
  for (int i=1; i<argc; ++i){
    FakePCB* new_process=(FakePCB*)malloc(sizeof(FakePCB));

    // Caricamento delle informazioni del processo dal relativo file
    int num_events=FakePCB_loadProcessInfo(new_process, argv[i]);
    // Errore nell'aperture del file
    if (num_events==-1) printf("Errore durante l'apertura del file.\n");
    else {
      printf("file: %s - pid: %d - number of events: %02d", argv[i], new_process->pid, num_events);

      // Inserimento nella lista dei processi del sistema operativo
      if (num_events) { List_pushBack(&os.processes, (ListItem*)new_process); printf(" - loading...\n"); }
      else { refused++; printf(" - Require at least 1 event.\n"); }
    }
  }

  printf("\nProcesses:\nLoaded: %d\nRefused: %d\n\nStarting simulation:", os.processes.size, refused);



  // Avvio del sistema procedendo step by step fino a quando non terminano tutti i processi caricati
  while(1){

    // Controlla se c'è almeno una CPU che sta attualmente lavorando
    int is_running = 0;
    for (int i=0;i<number_of_cpu;i++){
      if ((os.running)[i]) { is_running++; break; }
    }

    // Controllo di terminazione della simulazione
    if ( is_running || os.ready.first || os.waiting.first || os.processes.first ) FakeOS_simStep(&os, &sim_card);
    // Se il controllo non va a buon fine termina la simulazione
    else break;

  } printf("Simulation ended\n\n\nSIMULATION RESULTS");



  // Aggiornamento dei risultati e delle metriche della simulazione
  SimCard_SaveAndPrint(&sim_card, &os);

  // Libera lo spazio dell'OS
  FakeOS_destroy(&os);
  printf("Exiting...\n");
}
