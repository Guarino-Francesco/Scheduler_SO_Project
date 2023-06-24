#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../fake_os/fake_os.h"

#define AVAILABLE_SCHEDULERS 3


void scheduler_RR(FakeOS* os, SimCard* sim_card);
void scheduler_P_SJF_QP(FakeOS* os, SimCard* sim_card);
void scheduler_FIFO(FakeOS* os, SimCard* sim_card);


void SimCard_Print(SimCard* sim_card);
void SimCard_SaveOnFile(SimCard* sim_card, char* filename);
void SimCard_SaveAndPrint(SimCard* sim_card, FakeOS* os);