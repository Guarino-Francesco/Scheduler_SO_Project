CC=gcc
CCOPTS=--std=gnu99 -Wall -D_LIST_DEBUG_
AR=ar

# Oggetto ed Headers per hist_to_proc
OBJS_HIST=    linked_list/linked_list.o   fake_process/fake_process.o   hist_utils/hist_utils.o
HEADERS_HIST= linked_list/linked_list.h   fake_process/fake_process.h   hist_utils/hist_utils.h

# Oggetto ed Headers per sched_sim
OBJS_SHED=    linked_list/linked_list.o   fake_process/fake_process.o   fake_os/fake_os.o   sched_sim_utils/scheduler.o sched_sim_utils/sim_card.o
HEADERS_SHED= linked_list/linked_list.h   fake_process/fake_process.h   fake_os/fake_os.h   sched_sim_utils/sched_sim.h

BINS= sched_sim hist_to_proc

.phony: clean all

all:	$(BINS)

%.o:	%.c $(HEADERS_HIST)
	$(CC) $(CCOPTS) -c -o $@  $<

%.o:	%.c $(HEADERS_SHED)
	$(CC) $(CCOPTS) -c -o $@  $<

sched_sim:	sched_sim.c $(OBJS_SHED)
	$(CC) $(CCOPTS) -o $@ $^

hist_to_proc:	hist_to_proc.c $(OBJS_HIST)
	$(CC) $(CCOPTS) -o $@ $^

# Rimuove file oggtto ed eseguibili
clean:
	rm -rf *.o *~ $(OBJS) $(BINS)

# Rimuove i salvataggi delle simulazioni
rsim:
	rm -rf Sim*

# Rimuove i file dei processi generati dagli istogrammi associati
rmhp:
	rm -rf histo_processes_files/hp*

run:
	reset ; make clean ; make ; ./hist_to_proc histogram_files/h*.txt ; ./sched_sim histo_processes_files/hp*