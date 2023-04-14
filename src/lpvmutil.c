#include"lpvm.h"
#include"lperr.h"
#include<string.h>

void lp_vm_stack_init(lp_stack_ctx *ctx, char *stacks, lpptrsize size)
{
    lpnull(ctx && stacks && size);
    ctx->ebp=ctx->esp = lp2ptr(stacks);
    ctx->stacks = stacks;
    ctx->stack_ends = stacks + size;
}

void lp_vm_pushc(lp_vm_ctx *ctx, char *ptr, lpptrsize size)
{
    lpnull(ctx && ptr && size);
    memcpy(ctx->stack.esp,ptr,size);
    ctx->stack.esp+=size;
}

void *lp_vm_popc(lp_vm_ctx *ctx, lpptrsize size)
{
    lpnull(ctx && size);
    lpptrsize p = ctx->stack.esp;
    ctx->stack.esp-=size;
    return p;
}

void lp_vm_code_init(lp_opcodes_ctx *ctx, char *codes, lpptrsize size)
{
    lpnull(ctx && size && codes);
    ctx->code_size = lp2ptr(size);
    ctx->codes = codes;
    ctx->pc = codes;
    ctx->codes_end = ctx->codes + ctx->code_size;
}

char lp_vm_nextop(lp_vm_ctx *ctx)
{
    lp_opcodes_ctx opcodes = ctx->opcodes;
    if(opcodes.pc >= opcodes.codes_end)
        return LOP_ERR;
    char c = *opcodes.pc;
    ctx->opcodes.pc++;
    return c;
}

lpvmptr lp_vm_nextop_ptr(lp_vm_ctx *ctx)
{
    lp_opcodes_ctx opcodes = ctx->opcodes;
    if(opcodes.pc >= opcodes.codes_end - sizeof(lpvmptr))
        return LOP_ERR;
    lpvmptr r = *(lpvmptr*)opcodes.pc;
    ctx->opcodes.pc+=sizeof(lpvmptr);
    return r;
}