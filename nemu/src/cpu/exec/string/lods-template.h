#include "cpu/exec/template-start.h"

#define instr lods

static void do_execute() {
    REG(R_EAX) = swaddr_read(reg_l(R_ESI), DATA_BYTE);
    if (cpu.DF == 0) reg_l(R_ESI) += DATA_BYTE;
    else reg_l(R_ESI) -= DATA_BYTE;

    print_asm("lods%s", str(SUFFIX));
}

make_instr_helper(n);

#include "cpu/exec/template-end.h"