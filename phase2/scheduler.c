#include "./headers/scheduler.h"
#include "headers/initial.h"
#include <uriscv/const.h>
#include <uriscv/liburiscv.h>

void scheduler(void) {
  // debug_print("Inizio scheduler\n");

  // debug_print("Soft block count");
  // debug_print_hex(softBlockCount);
  // debug_print("\n");
  if (!emptyProcQ(&readyQueue)) {
    // debug_print("La coda non e' vuota \n");

    // 3.1
    currProc = removeProcQ(&readyQueue);

    // 3.2 Load 5 mills on the PLT [section 7.2].
    setTIMER(TIMESLICE);
    // save the start time of the process
    STCK(processTimer);
    // 3.3
    LDST(&currProc->p_s);

  } else {
    if (processCount == 0) {
      // debug_print("Process count a 0\n");

      HALT();
    }
    if (processCount > 0 && softBlockCount > 0) {
      // debug_print("Ci sono processi e alcuni sono bloaccati \n");
      // debug_print("Soft block count");
      // debug_print_hex(softBlockCount);
      // debug_print("\n");

      setMIE(MIE_ALL & ~MIE_MTIE_MASK);
      unsigned int status = getSTATUS();
      status |= MSTATUS_MIE_MASK;
      setSTATUS(status);

      // enter a wait state
      WAIT();
    }
    if (processCount > 0 && softBlockCount == 0) {
      // debug_print("Good luck with that\n");

      PANIC();
    }
  }
}
