#include "./headers/interrupts.h"
// #include "../../uriscv-latest/src/include/uriscv/cpu.h"
#include "headers/initial.h"
#include <uriscv/const.h>
#include <uriscv/liburiscv.h>
#include <uriscv/cpu.h>
#include <uriscv/types.h>
volatile unsigned int *bitmap = (unsigned int *)BITMAP_BASE;

void interruptHandler(void) {

  unsigned int exceptCode = getCAUSE() & CAUSE_EXCCODE_MASK;
  unsigned int intlineNo;
  switch (exceptCode) {
  case IL_CPUTIMER:
    intlineNo = 1;
    PLTInterrupt();
    break;
  case IL_TIMER:
    intlineNo = 2;
    ITInterrupt();
    break;
  case IL_DISK:
    intlineNo = 3;
    deviceInterrupt(intlineNo);
    break;
  case IL_FLASH:
    intlineNo = 4;
    deviceInterrupt(intlineNo);
    break;
  case IL_ETHERNET:
    intlineNo = 5;
    deviceInterrupt(intlineNo);
    break;
  case IL_PRINTER:
    intlineNo = 6;
    deviceInterrupt(intlineNo);
    break;
  case IL_TERMINAL:
    intlineNo = 7;
    deviceInterrupt(intlineNo);
    break;
  default:
    PANIC();
    break;
  }
}

void deviceInterrupt(unsigned int intlineNo) {
  unsigned int word = intlineNo - 3;
  unsigned int DevNo;
  if (bitmap[word] & DEV0ON) {
    DevNo = 0;
  } else if (bitmap[word] & DEV1ON) {
    DevNo = 1;
  } else if (bitmap[word] & DEV2ON) {
    DevNo = 2;
  } else if (bitmap[word] & DEV3ON) {
    DevNo = 3;
  } else if (bitmap[word] & DEV4ON) {
    DevNo = 4;
  } else if (bitmap[word] & DEV5ON) {
    DevNo = 5;
  } else if (bitmap[word] & DEV6ON) {
    DevNo = 6;
  } else if (bitmap[word] & DEV7ON) {
    DevNo = 7;
  } else {
    PANIC();
  }
  volatile memaddr devAddrBase =
      START_ADDR + ((intlineNo - 3) * 0x80) + (DevNo * 0x10);
  // /*
  volatile devreg_t *device_register = (volatile devreg_t *)devAddrBase;
  unsigned int status;
  int semNum = -1;

  if (word != 4) {
    // Se non è un terminale, leggiamo lo stato del dispositivo e diamo l'ACK
    status = device_register->dtp.status;
    device_register->dtp.command = ACK;
    // Calcoliamo l'indice del semaforo corrispondente: 8 dispositivi per linea
    semNum = (intlineNo - 3) * 8 + DevNo;
  } else {
    // I terminali hanno due sottomoduli: trasmissione e ricezione
    // Verifichiamo quale dei due abbia generato l'interrupt (stato pari a 5 nel
    // byte meno significativo)
    unsigned int tran_status = device_register->term.transm_status;
    unsigned int recv_status = device_register->term.recv_status;
    if ((tran_status & 0xFF) == 5) {
      status = tran_status;
      device_register->term.transm_command = ACK;
      semNum = 32 + DevNo; // Transmission
    } else if ((recv_status & 0xFF) == 5) {
      status = recv_status;
      device_register->term.recv_command = ACK;
      semNum = 32 + DevNo + 8; // Receipt
    }
  } // */
  if (semNum == -1) // Errore hardware fatale: non sono stati trovati
                    // dispositivi eleggibili
    PANIC();

  // Otteniamo la locazione del semaforo per il corretto device dall'array
  // dedicato
  int *semValue = (int *)&subDevice[semNum];

  // Sveglia e rimuovi un apposito processo bloccato ad aspettare questo
  // input/output
  pcb_t *pcb = removeBlocked(semValue);

  if (pcb) {
    // Passa lo status del dispositivo nel registro a0 del PCB risvegliato per
    // ritornarglielo come valore di ritorno
    pcb->p_s.reg_a0 = status;

    // Ripristina l'eleggibilità del processo per la CPU spostandolo in Ready
    // Queue
    insertProcQ(&readyQueue, pcb);
    softBlockCount--;

    // Il processo entra nello stato 'ready' (non è più in Wait I/O), perciò
    // azzeriamo la dipendenza col semaforo
    pcb->p_semAdd = NULL;
  } else {
    // Se c'è l'interrupt ma NESSUN processo ha attualmente bloccato quel
    // dispositivo, "verhogen" sul semaforo salvando la capacità per il prossimo
    // che lo richiederà.
    (*semValue)++;
  }

  // Routine di ripristino post-interrupt
  unsigned int cpuNum = getPRID();
  if (currProc) {
    // C'era già un processo esecutivo interrotto; non invochiamo scheduler ma
    // lo riprendiamo immediatamente
    STCK(processTimer);
    LDST(GET_EXCEPTION_STATE_PTR(cpuNum));
  } else
    scheduler();
}

// Gestione interrupt causati dal Process Local Timer
// Il PLT scatta quando si esaurisce il quanto di tempo (Time Slice)
// Preassegnato al processo in esecuzione
void PLTInterrupt(void) {
  // Identifica l'ID CPU ed estrae lo stato salvato (il momento esatto del trap
  // da timer)
  unsigned int cpuNum = getPRID();
  state_t *state = GET_EXCEPTION_STATE_PTR(cpuNum);

  // Salva questo stato attuale nel descrittore del processo in modo da
  // riprenderlo dopo
  currProc->p_s = *state;

  // Ricarica il timer a 5 millisecondi (il Time Slice intero)
  setTIMER(TIMESLICE);

  // Rimette il processo corrente alla coda di ready. Round Robining in azione.
  insertProcQ(&readyQueue, currProc);

  // TODO: BUG PRESENTE (Commentato per non toccare il codice): aver fatto =
  // NULL qua impedisce al prossimo step se (!= NULL) di aggiungere tempo p_time
  // alla struct.
  currProc = NULL;

  cpu_t curr_time;
  STCK(curr_time);

  if (currProc != NULL) {
    // Salva il tempo trascorso
    currProc->p_time += (curr_time - processTimer);
    processTimer = curr_time;
  }

  // Affida il comando generale allo scheduler perché avvii il processo
  // successivo o il prossimo in Ready Queue
  scheduler();
}

void ITInterrupt(void) {
  // Riavvia/Ripristina l'Interval Timer a 100 millisecondi
  LDIT(PSECOND);
  int *sem = (int *)&subDevice[NRSEMAPHORES - 1];
  pcb_t *pcb;

  // Svuota e risveglia indiscriminatamente tutti i processi che attendevano e
  // si erano fermati (CLOCKWAIT)
  while (headBlocked(sem)) {
    pcb = removeBlocked(sem);
    pcb->p_semAdd = NULL;
    softBlockCount--;
    insertProcQ(&readyQueue, pcb);
  }

  // Resetta forzosamente il semaforo a 0 essendo stati svuotati i blocchi
  subDevice[NRSEMAPHORES - 1] = 0;

  // Come per le procedure post-interrupt se avevamo interrotto un programma lo
  // riavviamo. Altrimenti Scheduler.
  unsigned int cpuNum = getPRID();
  if (currProc) {
    STCK(processTimer);
    LDST(GET_EXCEPTION_STATE_PTR(cpuNum));
  } else {
    scheduler();
  }
}
