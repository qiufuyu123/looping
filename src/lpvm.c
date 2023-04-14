#include "lpvm.h"
#include"lperr.h"
#include"lpmem.h"
#include<stdio.h>

void lp_vm_init(lp_vm_ctx *ctx, char *heap, lpsize heap_size,
                char *stack, lpsize stack_size, char *codes, lpsize code_size)
{
    lp_msetup(&ctx->mem,heap_size,heap);
    lp_vm_stack_init(ctx,stack,stack_size);
    lp_vm_code_init(&ctx->opcodes,codes,code_size);
    ctx->vm_flg = LVM_NORMAL;
    lpinfo("Vm inited!");
}


