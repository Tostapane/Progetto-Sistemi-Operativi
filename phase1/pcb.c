#include "./headers/pcb.h"

static struct list_head pcbFree_h;
static pcb_t pcbFree_table[MAXPROC];
static int next_pid = 1;

void initPcbs()
{
  INIT_LIST_HEAD(&pcbFree_h);

  for (int i = 0; i < MAXPROC; i++)
  {
    list_add(&pcbFree_table[i].p_list, &pcbFree_h);
  }
}

// ogni pcb ha un campo p_list che rappresenta il suo "collegamento"
// alla list_head di pcb
void freePcb(pcb_t *p)
{
  list_add(&p->p_list, &pcbFree_h);
}

/* return NULL if the pcbFree list is empty. Otherwise,
remove an element from the pcbFree list, provide initial values for ALL of the
PCBs fields and then return a pointer to the removed element. PCBs get reused,
so it is important that no previous value persist in a PCB when it gets reallocated */
pcb_t *allocPcb()
{
  if (list_empty(&pcbFree_h))
  {
    return NULL;
  }
  else
  {
    // remove an element from the pcbFree list
    list_del(&pcbFree_h);
    // provide initial values for ALL of the PCBs fields
    struct pcb_t *p;
    list_for_each_entry(p, &pcbFree_h, p_list)
    {

      INIT_LIST_HEAD(&p->p_list);
      INIT_LIST_HEAD(&p->p_child);
      INIT_LIST_HEAD(&p->p_sib);

      p->p_parent = NULL;
      p->p_semAdd = NULL;
      p->p_supportStruct = NULL;
      p->p_time = 0;
      p->p_prio = 0;
      p->p_pid = 0;
    }
    return p;
  }
}

// tentativi lasciati mooolto incompleti

// inizializza una variabile che sarà la testa di una nuova coda di un pcb
void mkEmptyProcQ(struct list_head *head)
{
  INIT_LIST_HEAD(head);
}

int emptyProcQ(struct list_head *head)
{
  return list_empty(head);
}

/* insert p into the process queue pointed by head
 * the list must be ordered by priority
 * in case of same priority the element will be placed
 * after the last pcb with his priority
 */

/*
devo inserire p dentro la lista puntata da head
l'elemento deve essere inserito nella lista in ordine di priorità

devo trovare la posizione in cui inserire l'elemento

funzione ricorsiva che
  se la priorità di p è >= a quella dell'elemento puntato da head
    inserisce l'elemento in quella posizione
  altrimenti
    chiama la funzione sul successore di head
*/
// sbagliata
void insertProcQ(struct list_head *head, pcb_t *p)
{
  pcb_t *n = container_of(head, struct pcb_t, p_list);
  if (p->p_prio >= n->p_prio)
    list_add(&p->p_list, head);
  else
    return insertProcQ(head->next, p);
}

pcb_t *headProcQ(struct list_head *head)
{
}

pcb_t *removeProcQ(struct list_head *head)
{
}

pcb_t *outProcQ(struct list_head *head, pcb_t *p)
{
}

int emptyChild(pcb_t *p)
{
}

void insertChild(pcb_t *prnt, pcb_t *p)
{
}

pcb_t *removeChild(pcb_t *p)
{
}

pcb_t *outChild(pcb_t *p)
{
}
