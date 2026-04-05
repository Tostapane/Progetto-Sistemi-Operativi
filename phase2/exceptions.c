#include "headers/exceptions.h"
#include "headers/initial.h"
#include "headers/interrupts.h"
#include <uriscv/liburiscv.h>
// Funzioni di supporto statiche per la gestione dei processi
static pcb_t *find_pcb(int pid);
static void recursive_terminate(pcb_t *proc);

// TODO da ricontrollare
void *memcpy(void *dest, const void *src, int n) {
  char *d = (char *)dest;
  const char *s = (const char *)src;
  for (int i = 0; i < n; i++) {
    d[i] = s[i];
  }
  return dest;
}

/**
 * SEZIONE 5: Exception Handling
 *
 * Punto di ingresso principale per la gestione di tutte le eccezioni.
 *
 * Questa funzione viene chiamata dal BIOS ogni volta che si verifica
 * un'eccezione (esclusi i TLB-Refill). Il suo compito è determinare la causa
 * dell'eccezione e delegare il lavoro al gestore appropriato.
 *
 * Istruzioni:
 * 1. Ottieni lo stato del processore salvato dal BIOS (`BIOSDATAPAGE`).
 * 2. Leggi il registro `Cause` da questo stato per capire il motivo
 * dell'eccezione.
 * 3. Usa la macro `CAUSE_IS_INT` per distinguere tra Interrupt e altre
 * eccezioni.
 *    - Se è un interrupt, chiama il gestore degli interrupt (`interruptHandler`
 * in interrupts.c).
 *    - Altrimenti, estrai l'Exception Code (`ExcCode`).
 * 4. In base all'Exception Code, chiama la funzione specifica:
 *    - `ExcCode` 8 o 11: chiama `syscallHandler()`.
 *    - `ExcCode` da 24 a 28 (TLB exceptions): chiama `tlbHandler()`.
 *    - Tutti gli altri `ExcCode` (Program Traps): chiama
 * `programTrapHandler()`.
 */
void exceptionHandler(void) {

  cpu_t curr_time;
  STCK(curr_time);
  if (currProc != NULL) {
    currProc->p_time += (curr_time - processTimer);
    processTimer = curr_time; // aggiunta
  }
  // id del processore che ha causato l'eccezione
  unsigned int procsrID = getPRID();

  // processor state at the time of the exception
  // otteniamo un puntatore allo stato di quel processore
  state_t *exceptionState = GET_EXCEPTION_STATE_PTR(procsrID);

  // causa dello stato salvato
  unsigned int cause = exceptionState->cause;

  if (CAUSE_IS_INT(cause)) {
    // gestore interrupt
    interruptHandler();
  } else {
    // estrazione excode dallo stato salvato
    unsigned int exCode = (cause & CAUSE_EXCCODE_MASK); // a CAUSESHIFT was here

    // indirizzamento al gestore dell'eccezione corretto

    if (exCode == 8 || exCode == 11) {
      syscallHandler();
    } else if (exCode >= 24 && exCode <= 28) {
      tlbHandler();
    } else {
      programTrapHandler();
    }
  }
}

/**
 * SEZIONE 6: SYSCALL Exception Handling
 * Le SYSCALL che devi implementare qui dentro sono:
 * - 6.1: CreateProcess (NSYS1)
 * - 6.2: TerminateProcess (NSYS2)
 * - 6.8: GetSupportData (NSYS8)
 * - 6.9: GetProcessID (NSYS9)
 * - 6.10: Yield (NSYS10)
 * - 6.11: Controllo per SYSCALL in User-Mode.
 * - 6.12: Gestione del PC per il ritorno corretto dalla SYSCALL.
 *
 * Gestore per le eccezioni di tipo SYSCALL.
 *
 * Questa funzione gestisce le richieste di servizi del Nucleo fatte dai
 * processi.
 *
 * Istruzioni:
 * 1. Controlla se la SYSCALL è stata chiamata da un processo in user-mode
 * (Sez. 6.11).
 *    - Se sì, e se il numero della SYSCALL (in `a0`) è negativo, simula
 * un'eccezione di tipo "Program Trap" (PRIVINSTR) e passa il controllo a
 * `programTrapHandler()`.
 * 2. Se il controllo dei privilegi passa, usa uno `switch` sul valore del
 * registro `a0` dello stato salvato per determinare quale SYSCALL eseguire (da
 * -1 a -10).
 * 3. Per ogni SYSCALL, implementa la logica descritta nella documentazione.
 * 4. Gestisci correttamente il ritorno dalla SYSCALL (Sez. 6.12):
 *    - Per chiamate non bloccanti: incrementa il PC di 4, metti il valore di
 * ritorno in `a0` dello stato salvato e fai `LDST` su quello stato.
 *    - Per chiamate bloccanti: salva lo stato nel PCB del processo (dopo aver
 * incrementato il PC), aggiorna il tempo di CPU, esegui l'azione bloccante e
 * chiama lo `scheduler()`.
 */
void syscallHandler(void) {
  state_t *exception_state = GET_EXCEPTION_STATE_PTR(getPRID());

  // Estrae il numero della SYSCALL dal registro a0
  int syscall_number = exception_state->reg_a0;

  // Flag per gestire il ritorno dalle SYSCALL
  unsigned int is_blocking = 0;

  // 6.11: Controllo per SYSCALL in User-Mode
  if (syscall_number < 0 &&
      (exception_state->status & MSTATUS_MPP_MASK) == MSTATUS_MPP_U) {
    exception_state->cause =
        (exception_state->cause & ~GETEXECCODE) | (PRIVINSTR << CAUSESHIFT);
    programTrapHandler();
    return;
  }

  // 6.12: Incremento del PC per evitare loop.
  // Per le chiamate bloccanti, questo stato aggiornato verrà salvato nel PCB.
  // if (syscall_number <= -1 && syscall_number >= -10)
  exception_state->pc_epc += WORDLEN;

  switch (syscall_number) {

  // 6.1: CreateProcess
  case CREATEPROCESS: {

    pcb_t *new_proc = allocPcb();
    if (new_proc == NULL) { // Codice di errore: No More PCB
      exception_state->reg_a0 = -1;
    } else {
      state_t *new_state = (state_t *)exception_state->reg_a1;
      new_proc->p_s = *new_state;
      new_proc->p_prio = exception_state->reg_a2;
      new_proc->p_supportStruct = (support_t *)exception_state->reg_a3;
      new_proc->p_time = 0;
      new_proc->p_semAdd = NULL;

      insertChild(currProc, new_proc);
      insertProcQ(&readyQueue, new_proc);
      processCount++;

      // Ritorna il PID del nuovo processo
      exception_state->reg_a0 = new_proc->p_pid;
    }
    break;
  }

  // 6.2: TerminateProcess
  case TERMPROCESS: {

    pcb_t *target = (exception_state->reg_a1 == 0)
                        ? currProc
                        : find_pcb(exception_state->reg_a1);
    if (target) {
      recursive_terminate(target);
    }
    if (currProc != NULL) {
      currProc->p_s = *exception_state;
      insertProcQ(&readyQueue, currProc);
    }
    // La terminazione è sempre un'operazione che blocca il flusso normale
    // e richiede di chiamare lo scheduler.
    is_blocking = 1;
    break;
  }

  // 6.3: Passeren
  case PASSEREN: {
    unsigned int *sem = (unsigned int *)exception_state->reg_a1;
    if (*sem == 0) {
      is_blocking = 1;
      currProc->p_s = *exception_state; // nuovo
                                        // currProc -> p_semAdd = sem;
      insertBlocked(sem, currProc);
    } else {
      (*sem)--;
    }
    break;
  }
  // 6.4: Verhogen
  case VERHOGEN: {
    unsigned int *sem = (unsigned int *)exception_state->reg_a1;
    if (*sem == 0 && headBlocked(sem)) {
      pcb_t *p = removeBlocked(sem);
      // currProc -> p_semAdd = NULL;
      insertProcQ(&readyQueue, p);
    } else {
      (*sem)++;
    }
    break;
  }

    // 6.5: DoIO

  case DOIO: {

    unsigned int cmd_addr = (unsigned int)exception_state->reg_a1;
    *(unsigned int *)cmd_addr = (unsigned int)exception_state->reg_a2;

    // 1. Distanza esatta in byte dalla base di tutti i dispositivi hardware
    unsigned int total_offset = cmd_addr - START_ADDR;

    // 2. Ogni registro è grande 0x10 (16) byte.
    // Dividendo, otteniamo l'indice del dispositivo come se fosse un array
    // unico da 0 a 47.
    unsigned int flat_device_index = total_offset / 0x10;
    unsigned int index;

    if (flat_device_index < 32) {
      // Dispositivi normali (Disk, Flash, Net, Printer) occupano i primi 32
      // posti
      index = flat_device_index;
    } else {
      // Terminali (gli indici piatti vanno da 32 a 39).
      // Essendo due sotto-dispositivi, dobbiamo capire se è trasmissione o
      // ricezione.
      unsigned int devNo = flat_device_index - 32;

      // Il resto della divisione ci dà l'offset del registro specifico

      if (total_offset % 0x10 == 0x0C) { // Offset Comando Trasmissione
        index = 32 + devNo;
      } else { // Offset Comando Ricezione
        index = 40 + devNo;
      }
    }

    int *sem_ptr = &subDevice[index];
    // (*sem_ptr)--; non serve
    currProc->p_s = *exception_state; // nuovo
    insertBlocked(sem_ptr, currProc);
    softBlockCount++;
    is_blocking = 1;
    break;
  }

  // 6.6: GetCPUTime
  case GETTIME: {
    //  gestione precedente del clock alla chiamata di exceptionHandler()
    cpu_t now;
    STCK(now);
    exception_state->reg_a0 = currProc->p_time + (now - processTimer);
    break;
  }

  // 6.7: WaitForClock
  case CLOCKWAIT: {

    int *sem_ptr = &subDevice[NRSEMAPHORES - 1];
    (*sem_ptr)--;
    currProc->p_s = *exception_state; // nuovo
    insertBlocked(sem_ptr, currProc);
    softBlockCount++;
    is_blocking = 1;
    break;
  }

  // 6.8: GetSupportData
  case GETSUPPORTPTR: {

    exception_state->reg_a0 = (unsigned int)currProc->p_supportStruct;
    break;
  }

  // 6.9: GetProcessID
  case GETPROCESSID: {

    if (exception_state->reg_a1 == 0) { // PID del processo corrente
      exception_state->reg_a0 = currProc->p_pid;
    } else { // PID del genitore
      exception_state->reg_a0 =
          (currProc->p_parent) ? currProc->p_parent->p_pid : 0;
    }
    break;
  }

  // 6.10: Yield
  case YIELD: {

    // Salva lo stato corrente nel PCB
    currProc->p_s = *exception_state;
    // Rimette il processo in coda
    /* Per specifica, il processo che fa yield NON deve essere ri-eseguito
     * immediatamente se ci sono altri processi in readyQueue, anche se ha la
     * priorità massima. Per garantirlo, lo mettiamo in fondo alla lista. */
    list_add_tail(&currProc->p_list, &readyQueue);
    is_blocking = 1;
    break;
  }

  // SYSCALL non di competenza o non valide
  default: { // 6.11: Tratta le SYSCALL non esistenti come Program Trap, stessa
             // logica precedente

    // Se syscall_number > 0, è una richiesta per il Support Level (6.8.1)
    if (syscall_number > 0) {
      programTrapHandler(); // Passa lo stato (già incrementato) al Support
                            // Level
    } else {
      // Se syscall_number < -10 o sconosciuta negativa, è una Trap (Privileged
      // Instruction)
      exception_state->cause = (exception_state->cause & ~CAUSE_EXCCODE_MASK) |
                               (PRIVINSTR << CAUSESHIFT);
      // IMPORTANTE: in questo caso specifico di errore,
      // il PC non dovrebbe essere avanzato perché l'istruzione è illegale.
      exception_state->pc_epc -= WORDLEN;
      programTrapHandler();
    }
  }
  }

  // Prima di uscire, carichiamo il tempo speso nel kernel sul processo
  cpu_t kernel_exit_time;
  STCK(kernel_exit_time);
  if (currProc != NULL) {
    currProc->p_time += (kernel_exit_time - processTimer);
  }

  //   6.12: Ritorno da una SYSCALL non bloccante
  if (is_blocking == 0) { // Ricarica lo stato per riprendere l'esecuzione
    STCK(processTimer);
    LDST(exception_state);
  } else { // 6.13: ritorno da una SYSCALL bloccante
    currProc = NULL;
    scheduler();
  }
}

/**
 * @brief Funzione ricorsiva per cercare un PCB dato un PID, partendo da una
 * radice.
 * @param root Il PCB da cui iniziare la ricerca (tipicamente la radice
 * dell'albero).
 * @param pid Il Process ID da cercare.
 * @return Puntatore al PCB se trovato, altrimenti NULL.
 */
static pcb_t *find_pcb_recursive(pcb_t *root, int pid) {
  if (root == NULL)
    return NULL;
  if (root->p_pid == pid)
    return root;
  pcb_t *found = NULL;
  struct list_head *pos;
  // Esplora i figli ricorsivamente
  list_for_each(pos, &root->p_child) {
    pcb_t *child = container_of(pos, pcb_t, p_sib);
    found = find_pcb_recursive(child, pid);
    if (found)
      return found;
  }
  return NULL;
}

/**
 * @brief Trova un PCB nella pcbFree list dato un PID, partendo da currProc.
 * @param pid Il Process ID da cercare.
 * @return Puntatore al PCB se trovato, altrimenti NULL.
 */
static pcb_t *find_pcb(int pid) {
  // 1. Trova la radice dell'albero partendo da currProc (che è sempre parte
  // dell'albero)
  pcb_t *root = currProc;
  while (root->p_parent != NULL)
    root = root->p_parent;
  // 2. Cerca nel sistema partendo dalla radice
  return find_pcb_recursive(root, pid);
}

/**
 * @brief Termina ricorsivamente un processo e i suoi figli (Sezione 10).
 *
 * @param proc Il PCB del processo da terminare.
 */
static void recursive_terminate(pcb_t *proc) {
  if (proc == NULL)
    return;

  // 1. Termina ricorsivamente tutti i figli
  while (!emptyChild(proc)) {
    recursive_terminate(removeChild(proc));
  }

  // 2. Scollega sempre dal genitore (safe anche se già rimosso da removeChild)
  outChild(proc);

  // 3. Rimuovi dallo stato di esecuzione
  if (proc == currProc) {
    // currProc non è in nessuna lista, basta non ricaricarlo (lo farà lo
    // scheduler)
    currProc = NULL;
  } else {
    // Prova a rimuoverlo dalla Ready Queue
    if (outProcQ(&readyQueue, proc) == NULL) {
      // Se non era in Ready, potrebbe essere bloccato
      int *sem_addr = proc->p_semAdd; // Salva l'indirizzo PRIMA di outBlocked
      if (outBlocked(proc) != NULL) {
        // Se era un semaforo di device o pseudo-clock, aggiorna softBlockCount
        if ((sem_addr >= (int *)&subDevice[0] &&
             sem_addr <= (int *)&subDevice[NRSEMAPHORES - 1])) {
          softBlockCount--;
        }
      }
    }
  }

  // 4. Restituisci il PCB e aggiorna il conteggio totale
  freePcb(proc);
  processCount--;
}

/**
 * SEZIONE 8.2: Program Trap Exception Handling
 * SEZIONE 10: Process Termination
 *
 * Gestore per le eccezioni di tipo Program Trap.
 *
 * Gestisce errori di programma come istruzioni illegali, errori di
 * indirizzamento, ecc. Implementa la logica "Pass Up or Die".
 *
 * Istruzioni:
 * 1. Accedi al PCB del `CurrentProcess`.
 * 2. Controlla se il suo puntatore `p_supportStruct` è `NULL`.
 *    - Se è `NULL` ("Die"): termina il processo e tutta la sua progenie. Questo
 *      richiede di chiamare la logica di terminazione (Sez. 10) che implementa
 * NSYS2. Infine, chiama lo `scheduler()`.
 *    - Se NON è `NULL` ("Pass Up"):
 *      a. Copia lo stato d'eccezione dal `BIOSDATAPAGE` in
 * `sup_exceptState[GENERALEXCEPT]` nella `support_t` del processo. b. Esegui un
 * `LDCXT` usando il contesto salvato in `sup_exceptContext[GENERALEXCEPT]` per
 * passare il controllo al gestore di eccezioni del Livello di Supporto.
 */
void programTrapHandler(void) {
  support_t *support = currProc->p_supportStruct;
  if (support) {
    unsigned int cpuNum = getPRID();
    support->sup_exceptState[GENERALEXCEPT] =
        *(GET_EXCEPTION_STATE_PTR(cpuNum));
    context_t exeptCon = support->sup_exceptContext[GENERALEXCEPT];
    STCK(processTimer);
    LDCXT(exeptCon.stackPtr, exeptCon.status, exeptCon.pc);

  } else {
    recursive_terminate(currProc);
    currProc = NULL;
    scheduler();
  }
}

/**
 * SEZIONE 8.3: TLB Exception Handling
 *
 * Gestore per le eccezioni relative alla TLB (Translation Lookaside Buffer).
 *
 * Gestisce errori di traduzione degli indirizzi. Implementa la logica "Pass Up
 * or Die".
 *
 * Istruzioni:
 * 1. La logica è quasi identica a `programTrapHandler`.
 * 2. L'unica differenza è che, nel caso "Pass Up", devi usare gli indici
 * `PGFAULTEXCEPT` invece di `GENERALEXCEPT` per accedere ai campi
 * `sup_exceptState` e `sup_exceptContext`. a. Copia lo stato d'eccezione in
 * `sup_exceptState[PGFAULTEXCEPT]`. b. Esegui `LDCXT` con il contesto
 * `sup_exceptContext[PGFAULTEXCEPT]`.
 */

void tlbHandler(void) {
  support_t *support = currProc->p_supportStruct;
  if (support) {
    unsigned int cpuNum = getPRID();
    support->sup_exceptState[PGFAULTEXCEPT] =
        *(GET_EXCEPTION_STATE_PTR(cpuNum));
    context_t exeptCon = support->sup_exceptContext[PGFAULTEXCEPT];
    STCK(processTimer);
    LDCXT(exeptCon.stackPtr, exeptCon.status, exeptCon.pc);

  } else {
    recursive_terminate(currProc);
    currProc = NULL;
    scheduler();
  }
}
