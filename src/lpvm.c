#include "lpvm.h"
#include"lperr.h"
#include"lpmem.h"
#include<stdio.h>

void lp_vm_init(lp_vm_ctx *ctx, char *heap, lpsize heap_size,
                char *stack, lpsize stack_size, char *codes, lpsize code_size,
                char *staticres, lpsize ressize)
{
    lp_msetup(&ctx->mem,heap_size,heap);
    lp_vm_stack_init(&ctx->stack,stack,stack_size);
    lp_vm_code_init(&ctx->opcodes,codes,code_size);
    lp_vm_staticres_init(&ctx->sres, staticres, ressize);
    ctx->vm_flg = LVM_NORMAL;
    lpinfo("Vm inited!");

}

static void lp_vm_dump_stack(lp_vm_ctx *ctx)
{
    lpinfo(" ==> Dumping Vm Stack: ");
    lpsize j=0;
    for (lpptrsize i = ctx->stack.ebp; i < ctx->stack.esp; i+=4)
    {
        lpvmvalue v=*(uint32_t*)i;
        lpdebug("[%d]: %d 0x%x\n",j++,v,v);
    }
    lpinfo(" ==> Dumping Vm Stack End");
    
}

LP_Err lp_vm_start(lp_vm_ctx *ctx, lpptrsize entrypoint)
{
    LP_Vm_Opcodes op = 0;
    int stat = 0;
    lpvmptr old_pc = ctx->opcodes.pc;
    ctx->opcodes.pc += entrypoint;
    while (stat >= 0)
    {
        op = lp_vm_nextop(ctx);
        lpvmvalue val1 = 0, val2 = 0, *pval1=0;
        switch (op)
        {
        case LOP_LOADc:
            // load one constant to the stack top
            val1 = lp_vm_nextop_value(ctx);
            lpdebug("[VM] Op: LOADc, num:%d to stack\n",val1);
            lp_vm_push(ctx,lpvmvalue,val1);
            break;
        case LOP_LOADcn:
            val1 = lp_vm_nextop_value(ctx); // get n
            lpdebug("[VM] Op: LOADcn, num:%d:\n",val1);
            for (int i = 0; i < val1; i++)
            {
                val2 = lp_vm_nextop_value(ctx);
                lp_vm_push(ctx,lpvmvalue,val2);
                lpdebug("[VM] Load a CONST: %d to stack;\n",val2);
            }
            break;
        case LOP_NOP:
            break;
        case LOP_POP:
            lp_vm_dump_stack(ctx);
            val1 = lp_vm_pop(ctx,lpvmvalue);
            val2 = lp_vm_nextop_value(ctx);
            pval1 = lp_vm_stackvisit(ctx,val2,1);
            *pval1 = val1;
            lpdebug("[VM] Pop a value %x to stack #%x\n",val1,val2);
            break;
        default:
            stat = -1;
            break;
        }
    }
    ctx->opcodes.pc = old_pc;
    lpinfo("Vm end");
    lp_vm_dump_stack(ctx);
}
