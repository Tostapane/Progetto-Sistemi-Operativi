#include "./headers/initial.h"
#include <uriscv/const.h>
#include <uriscv/types.h>

// Numero di processi avviati ma non ancora terminati
unsigned int processCount;

// Numero di processi avviati, ma non terminati,
// che si trovano nello stato "bloccato" a causa di un'operazione di I/O o di
// una richiesta al timer
unsigned int softBlockCount;

// Coda dei PCB (Process Control Block) che si trovano nello stato "ready"
// (pronti per l'esecuzione)
struct list_head readyQueue;

// Il processo attualmente in esecuzione
pcb_t *currProc;

// Semafori per i dispositivi esterni
// L'ultimo elemento dell'array è dedicato allo pseudoclock
int subDevice[NRSEMAPHORES];

// Variabile utilizzata per salvare il tempo di inizio dell'esecuzione di un
// processo
cpu_t processTimer;

int main() {

  /* Struttura che serve a gestire le eccezioni incluse quelle relative alla
   * TLB. L'hardware sa di dovere condultare questa locazione di memoria
   * (predefinita) quando si verificano eccezioni.
   * */
  passupvector_t *passupvector = (passupvector_t *)
      PASSUPVECTOR; // indirizzo di memoria definito in headers/const.h

  // Questo codice verrà rimpiazzato quando sarà implementato il livello di
  // supporto (Support Level). Indirizzo della funzione che deve gestire i TLB
  // miss.
  passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;

  /* Assegnazione di un area di memoria sicura e privata (dedicata al KERNEL)
   * contenente lo stack dedicato alla gestione del TLB-Refill.
   * Quando avviene un TLB miss, la funzione viene eseguita all'interno di
   * questa area di memoria. Questo stack è come un'area di lavoro. I processi
   * normali non possono ne leggere ne scrivere nel KERNELSTACK.
   *
   * Memoria utilizzata dalla funzione uTLB_RefillHandler sopra.*/
  passupvector->tlb_refill_stackPtr = (memaddr)KERNELSTACK;

  /* Indirizzo della funzione che deve gestire eccezioni di altro tipo. */
  passupvector->exception_handler = (memaddr)exceptionHandler;

  /* uTLB_RefillHandler e exceptionHandler usano la stessa area di memoria.
   * Ovviamente questa area viene acceduta in mutua esclusione; quando la CPU
   * sta gestendo un TLB-miss, le interruzioni sono disabilitate (non possono
   * verificarsi eccezioni di altro tipo).
   * In particolare durante la gestione di una eccezione:
   * - Le interruzioni (esterne al kernel), vengono "prenotate" e gestite al
   * termine dell'eccezione.
   * - Le eccezioni (interne al kernel), sono errori fatali che causano il crash
   * del sistema. Si assume che non avvengano errori fatali all'interno del
   * kernel. Esso non è progettato per gestire una eccezione dentro l'altra
   * tutte sullo stesso stack. Sollevare una eccezione (interna) comporterebbe
   * richiamare una funzione che utilizza lo stesso stack che stiamo usando al
   * momento, sovrascrivendolo!!*/
  passupvector->exception_stackPtr = (memaddr)KERNELSTACK;

  // Inizializza la coda dei pcb
  initPcbs();

  // Inizializza la coda dei semafori
  // Lista di semafori attivi, che hanno almeno un processo in attesa
  initASL();

  // 2.4 Inizializzazione delle variabili globali
  // Nessun processo è ancora stato creato, quindi contatori e clock a zero
  processCount = 0;
  softBlockCount = 0;

  // Svuota/inizializza la coda dei processi pronti (Ready Queue)
  mkEmptyProcQ(&readyQueue);

  // Nessun processo è attualmente in esecuzione
  currProc = NULL;

  // Inizializza a zero l'array di semafori dei dispositivi e lo pseudoclock
  for (int i = 0; i < NRSEMAPHORES; i++)
    subDevice[i] = 0;

  // 2.5
  // Carica nell'Interval Timer di sistema il valore di 100 millisecondi
  // L'Interval Timer genererà un interrupt allo scadere del tempo.
  LDIT(PSECOND);

  // 2.6 Istanziazione e inizializzazione del primo processo
  pcb_t *proc = allocPcb(); // Alloca un nuovo Process Control Block

  // Inserisce il processo appena allocato nella coda dei processi pronti
  insertProcQ(&readyQueue, proc);

  // Incrementa il contatore globale per includere il processo appena creato
  processCount++;

  // Abilita tutti i tipi di interrupt per questo processo
  proc->p_s.mie = MIE_ALL;

  // Imposta lo stato del processo: mantiene la modalità Kernel (MPP_M) e
  // abilita gli interrupt precedenti (MPIE)
  proc->p_s.status = MSTATUS_MPIE_MASK | MSTATUS_MPP_M;

  // Imposta lo Stack Pointer (SP) alla cima della RAM (RAMTOP), poiché lo stack
  // cresce verso il basso
  RAMTOP(proc->p_s.reg_sp);

  // Imposta il Program Counter (PC) all'indirizzo iniziale della funzione
  // 'test'
  proc->p_s.pc_epc = (memaddr)test;

  // Inizializza i campi dell'albero genealogico del processo a valori nulli o
  // liste vuote
  INIT_LIST_HEAD(&proc->p_child);
  INIT_LIST_HEAD(&proc->p_sib);
  proc->p_parent = NULL;

  // Inizializza il tempo CPU accumulato a 0
  proc->p_time = 0;

  // Il processo non è inizialmente bloccato in attesa di alcun semaforo
  proc->p_semAdd = NULL;
  proc->p_supportStruct = NULL;

  // L'avvio finale: chiama lo scheduler per passare il controllo alla CPU in
  // favore del primo processo
  scheduler();
}
