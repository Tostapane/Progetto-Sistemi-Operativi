#include "./headers/asl.h"

static semd_t semd_table[MAXPROC];
static struct list_head semdFree_h;
static struct list_head semd_h;

/* Initialize the semdFree list to contain all the elements of the array static semd_t semdTable[MAXPROC].
 * This method will be only called once during data structure initialization. */
void initASL()
{
}

/* Insert the PCB pointed to by p at the tail of the process queue associated with the semaphore
 * whose key is semAdd and set the semaphore address of p to semaphore with semAdd. If the
 * semaphore is currently not active (i.e. there is no descriptor for it in the ASL), allocate a new
 * descriptor from the semdFree list, insert it in the ASL (at the appropriate position), initialize
 * all of the fields (i.e. set s_key to semAdd, and s_procq to mkEmptyProcQ()), and proceed as
 * above. If a new semaphore descriptor needs to be allocated and the semdFree list is empty,
 * return TRUE. In all other cases return FALSE */
int insertBlocked(int *semAdd, pcb_t *p)
{
}

/* Search the ASL for a descriptor of this semaphore. If none is found, return NULL; other-
 * wise, remove the first (i.e. head) PCB from the process queue of the found semaphore de-
 * scriptor and return a pointer to it. If the process queue for this semaphore becomes empty
 * (emptyProcQ(s_procq) is TRUE), remove the semaphore descriptor from the ASL and return
 * it to the semdFree list */
pcb_t *removeBlocked(int *semAdd)
{
}

/* Remove the PCB pointed to by p from the process queue associated with p’s semaphore (p->p_semAdd)
 * on the ASL. If PCB pointed to by p does not appear in the process queue associated with p’s
 * semaphore, which is an error condition, return NULL; otherwise, return p.*/
pcb_t *outBlocked(pcb_t *p)
{
}

/* Return a pointer to the PCB that is at the head of the process queue associated with the
 * semaphore semAdd. Return NULL if semAdd is not found on the ASL or if the process queue
 * associated with semAdd is empty. */
pcb_t *headBlocked(int *semAdd)
{
}
