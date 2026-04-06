# Refactoring PCB Search Logic in `exceptions.c`

## Objective

The current `find_pcb` function in `phase2/exceptions.c` only searches for a PCB within the tree structure containing the current process (`currProc`). This is limited and potentially incorrect if the system supports multiple root processes or if the target process is in a different tree.

The goal is to implement a system-wide search for a PCB by PID, iterating over the global pool of all possible PCBs (`pcbFree_table`) and ensuring the target is not currently in the free list.

## Key Files & Context

- `phase2/exceptions.c`: Contains the `find_pcb` and `find_pcb_recursive` functions.
- `phase1/pcb.c` / `phase1/headers/pcb.h`: Defines `pcbFree_table` and `pcbFree_h`.
- `headers/const.h`: Defines `MAXPROC`.

## Implementation Steps

1. **Modify `find_pcb` in `phase2/exceptions.c`**:
   - Change the implementation to iterate through all PCBs in the `pcbFree_table` array (size `MAXPROC`).
   - For each PCB with a matching PID, verify it is not in the `pcbFree_h` list.
   - If a matching PID is found and it is not free, return the PCB pointer.

2. **Remove `find_pcb_recursive` in `phase2/exceptions.c`**:
   - Since the global search replaces the tree search, this helper function is no longer needed.

## Detailed Changes

### `phase2/exceptions.c`

Replace:

```c
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

static pcb_t *find_pcb(int pid) {
  // 1. Trova la radice dell'albero partendo da currProc (che è sempre parte
  // dell'albero)
  pcb_t *root = currProc;
  while (root->p_parent != NULL)
    root = root->p_parent;
  // 2. Cerca nel sistema partendo dalla radice
  return find_pcb_recursive(root, pid);
}
```

With:

```c
/**
 * @brief Trova un PCB dato un PID, cercandolo in tutto il sistema.
 * @param pid Il Process ID da cercare.
 * @return Puntatore al PCB se trovato e attivo, altrimenti NULL.
 */
static pcb_t *find_pcb(int pid) {
  for (int i = 0; i < MAXPROC; i++) {
    // Se il PID corrisponde
    if (pcbFree_table[i].p_pid == pid) {
      // Controlla che il PCB non sia nella lista libera (quindi è attivo)
      struct list_head *pos;
      list_for_each(pos, &pcbFree_h) {
        if (pos == &pcbFree_table[i].p_list) {
          return NULL; // È nella pcbFree_h, quindi non è attivo
        }
      }
      return &pcbFree_table[i];
    }
  }
  return NULL;
}
```

## Verification & Testing

1. **Compilation**: Ensure the code compiles correctly.
2. **NSYS2 Test**: Test process termination with a PID of a child.
3. **NSYS2 Robustness**: Test terminating a process by its PID when it is not in the immediate progeny of the caller (if possible in the test environment).
4. **Invalid PID**: Ensure `find_pcb` returns `NULL` for PIDs that don't exist or correspond to freed processes.
