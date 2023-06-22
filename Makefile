CC=gcc
CCOPTS=--std=gnu99 -Wall -D_LIST_DEBUG_
AR=ar

OBJS= linked_list/linked_list.o\
      fake_process/fake_process.o\
      fake_os/fake_os.o

HEADERS= linked_list/linked_list.h\
         fake_process/fake_process.h\
         fake_os/fake_os.h

BINS= sched_sim

.phony: clean all

all:	$(BINS)

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

sched_sim:	sched_sim.c $(OBJS)
	$(CC) $(CCOPTS) -o $@ $^

clean:
	rm -rf *.o *~ $(OBJS) $(BINS)