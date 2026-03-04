#include "./headers/exceptions.h"
#include <uriscv/const.h>
#include <uriscv/cpu.h>
#include <uriscv/liburiscv.h>
#include <uriscv/types.h>

void exceptionHandler(void) {

  state_t *state = GET_EXCEPTION_STATE_PTR(BIOSDATAPAGE);
}
