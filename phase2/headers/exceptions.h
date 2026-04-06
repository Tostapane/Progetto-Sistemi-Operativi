#ifndef PANDOS_EXCEPTIONS_H
#define PANDOS_EXCEPTIONS_H

// #include "../../../uriscv-latest/src/include/uriscv/cpu.h"
#include "../../headers/const.h"
#include "../../headers/types.h"
#include "../../phase1/headers/asl.h"
#include "../../phase1/headers/pcb.h"
#include "./interrupts.h"
#include "./scheduler.h"
#include "initial.h"
#include <uriscv/liburiscv.h>
#include <uriscv/cpu.h>

/**
 * @brief Punto di ingresso principale per la gestione di tutte le eccezioni.
 *
 * Questa funzione viene chiamata dal BIOS ogni volta che si verifica
 * un'eccezione (esclusi i TLB-Refill). Il suo compito è determinare la causa
 * dell'eccezione e delegare il lavoro al gestore appropriato.
 */
void exceptionHandler(void);

/**
 * @brief Gestore per le eccezioni di tipo SYSCALL.
 *
 * Questa funzione gestisce le richieste di servizi del Nucleo fatte dai
 * processi (SYSCALL da -1 a -10).
 */
void syscallHandler(void);

/**
 * @brief Gestore per le eccezioni di tipo Program Trap.
 *
 * Gestisce errori di programma come istruzioni illegali, errori di
 * indirizzamento, ecc. Implementa la logica "Pass Up or Die".
 */
void programTrapHandler(void);

/**
 * @brief Gestore per le eccezioni relative alla TLB (Translation Lookaside
 * Buffer).
 *
 * Gestisce errori di traduzione degli indirizzi.
 * Implementa la logica "Pass Up or Die".
 */
void tlbHandler(void);

/**
 * @brief Gestore "placeholder" per gli eventi di TLB-Refill.
 *
 * Questa funzione è un gestore speciale che viene chiamato solo per eventi di
 * TLB-Refill. Per la Fase 2, il suo codice è fisso.
 */
void uTLB_RefillHandler(void);

#endif // PANDOS_EXCEPTIONS_H