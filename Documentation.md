# PandOSsh Phase 1 Documentation

## 1. Introduction
This documentation describes the implementation of Phase 1 (Level 2) of the PandOSsh operating system. This level implements the Queue Manager, which is responsible for managing Process Control Blocks (PCBs) and the Active Semaphore List (ASL). The implementation creates the foundation for process management and synchronization used in subsequent levels.

## 2. Implementation Details

### 2.1 Process Control Blocks (PCB) - `pcb.c`
The PCB module manages the allocation, deallocation, and organization of process control blocks.

* **PCB Structure**: Defined in `pcb.h` and `types.h`, the `pcb_t` structure utilizes the `list_head` structure from `listx.h` (Linux Kernel style lists) to manage queues.
* **Free List (`pcbFree_h`)**: A static list maintaining unused PCBs. `allocPcb` retrieves a node from this list, while `freePcb` returns one.
* **Priority Queues**: The `insertProcQ` function maintains queues ordered by priority (descending). Crucially, to satisfy the requirement of **FIFO behavior for equal priorities**, the insertion logic scans the list and inserts the new element *before* the first element with a strictly lower priority. If the priority is equal, it continues scanning, effectively placing the new element after all existing elements of the same priority.
* **Process Trees**: The module implements a tree structure where each PCB maintains a list of children (`p_child`) and a sibling link (`p_sib`). `p_child` acts as a sentinel for the list of siblings.

### 2.2 Active Semaphore List (ASL) - `asl.c`
The ASL module implements a list of active semaphores, where each semaphore descriptor (`semd_t`) maintains a queue of blocked processes.

* **Organization**: The ASL (`semd_h`) is a linked list sorted by the semaphore key (physical address `s_key`) in ascending order. This optimization allows search functions (`getSemd`) to abort early if the key is exceeded.
* **Dynamic Allocation**: Semaphore descriptors are allocated from a static free list (`semdFree_h`) only when a semaphore becomes active (i.e., has at least one blocked process).
* **Management**:
    * `insertBlocked`: Searches for the semaphore. If not found, allocates a new `semd_t` and inserts it into the sorted ASL. The process is then added to the semaphore's process queue.
    * `removeBlocked` / `outBlocked`: Removes processes from the semaphore's queue. If the queue becomes empty, the `semd_t` descriptor is removed from the ASL and returned to the free list to save resources.

## 3. Design Choices & Algorithms

* **List Implementation**: We utilized the provided `listx.h` which mimics the Linux Kernel circular doubly-linked lists. This avoids manual pointer manipulation for `next` and `prev` fields, relying instead on `list_add`, `list_del`, and `container_of` macros.
* **Priority Insertion Algorithm**:
    To ensure O(N) insertion with correct ordering:
    ```c
    list_for_each(it, head) {
        if (p->p_prio > item->p_prio) {
            list_add_tail(&p->p_list, it); // Insert BEFORE current item
            return;
        }
    }
    list_add_tail(&p->p_list, head); // Insert at TAIL if lowest/equal priority
    ```
    This logic guarantees that among processes with equal priority, the one inserted first remains ahead of the one inserted later.

* **ASL Sorting**: The ASL is kept sorted to improve performance on `insertBlocked` operations, reducing the average search time compared to an unsorted list, especially when many semaphores are active.

## 4. File Structure
* `phase1/pcb.c`: Implementation of PCB queues and trees.
* `phase1/asl.c`: Implementation of Semaphore management.
* `headers/`: Contains `pcb.h`, `asl.h`, `types.h`, `listx.h`, `const.h`.
* `klog.c`: Debugging utility.