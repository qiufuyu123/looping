#include "lpvm.h"
#include"lperr.h"
#include"lpmem.h"
#include<stdio.h>
#include<string.h>

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

#define _LP_VM_ARITH(op,v1,v2,dst)\
    if(lp_vm_tresolve(ctx,lp_vm_nextop(ctx),&v1,&v2,&dst))\
    {\
        lpdebug("[VM] ArithOp: %d '%s' %d --> %x;\n",v1,#op,v2,dst); \
        *dst = v1 op v2;\
    }else\
    {\
        lpdebug("[VM] ArithOp: %d '%s' %d;\n",v1,#op,v2); \
        v1 = v1 op v2;\
        lp_vm_push(ctx,lpvmvalue,v1);\
    }
#define _LP_VM_ARITH_SINGLE(op,v1,dst)\
    if(lp_vm_tresolve_single(ctx,lp_vm_nextop(ctx),&v1,&dst))\
    {\
        lpdebug("[VM] ArithOp: '%s' %d --> %x;\n",#op,v1,dst); \
        *dst = op v1;\
    }else\
    {\
        lpdebug("[VM] ArithOp: '%s' %d;\n",#op,v1); \
        v1 = op v1;\
        lp_vm_push(ctx,lpvmvalue,v1);\
    }
#define _LP_VM_ARITH_AUTO(code,op) case code:\
val1=(lp_vm_pop(ctx,lpvmvalue)) op (lp_vm_pop(ctx,lpvmvalue));\
lp_vm_push(ctx,lpvmvalue,val1); \
lpdebug("[VM] BinOP!;\n");\
break;
#define _LP_VM_ARITH_AUTO_SINGLE(code,op) case code:\
break;

lpvmptr lp_vm_from_raw_ptr(lp_vm_ctx *ctx, lpvmptr ptr)
{
    if(ptr >= ctx->stack.stacks && ptr < ctx->stack.stack_ends)
        return ptr;
    if(ptr >= ctx->mem.mem_header && ptr < ctx->mem.mem_end)
        return ptr;
    if(ptr >= ctx->sres.static_data && ptr < ctx->sres.end)
        return ptr;
    if(ptr >= ctx->opcodes.codes && ptr < ctx->opcodes.codes_end)
        return ptr;
    lpprintf(LPDERRO "Illegal memory access :0x%x\n",ptr);
    lppanic(LP_ILLEGAL_POINTER);
}

LP_Err lp_vm_continue(lp_vm_ctx *ctx)
{
    LP_Vm_Opcodes op = 0;
    int stat = 0;
    while (stat >= 0)
    {
        op = lp_vm_nextop(ctx);
        lpvmvalue val1 = 0, val2 = 0, *pval1 = 0, *pval2 = 0;
        char cval1 = 0;
        lpvmptr vptr1 = 0;
        char op1 = 0, op2 = 0, op3 = 0;
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
                cval1 = lp_vm_nextop(ctx);
                lp_vm_push(ctx,char,cval1);
                lpdebug("[VM] Load a CONST: %d to stack;\n",cval1);
            }
            break;
        case LOP_LOAD_STACKN:
            val1 = lp_vm_nextop_value(ctx);
            lp_vm_pushc(ctx,(char *)lp_vm_stackvisit(ctx,lp_vm_nextop_value(ctx),1),val1);
            break;
        case LOP_LOAD_STACK:
            val1 = lp_vm_nextop_value(ctx); // get offset of stack
            lpdebug("[VM] Op: Pushs, offset:0x%x;\n",val1);
            lp_vm_push(ctx,lpvmvalue,*lp_vm_stackvisit(ctx,val1,1));
            lp_vm_dump_stack(ctx);
            break;
        case LOP_NOP:
            break;
        case LOP_SET_STACKN:
            val1 = lp_vm_nextop_value(ctx);
            pval1 = lp_vm_stackvisit(ctx,lp_vm_nextop_value(ctx),1);
            pval2 = lp_vm_stackvisit(ctx,lp_vm_nextop_value(ctx),1);
            memcpy(pval1,pval2,val1);
            ctx->stack.esp -= val1;
            break;
        case LOP_SET_STACK:
            pval1 = lp_vm_stackvisit(ctx,lp_vm_nextop_value(ctx),1);
            *pval1 = lp_vm_nextop_value(ctx);
            break;
        
        case LOP_LEA:
            lp_vm_stack_lea(ctx, lp_vm_nextop_value(ctx));
            break;
        
        
        _LP_VM_ARITH_AUTO(LOP_ADD, +)
        _LP_VM_ARITH_AUTO(LOP_MINUS, -)
        _LP_VM_ARITH_AUTO(LOP_MUL, *)
        _LP_VM_ARITH_AUTO(LOP_DIV, /)
        _LP_VM_ARITH_AUTO(LOP_OR, |)
        _LP_VM_ARITH_AUTO(LOP_AND, &)
        _LP_VM_ARITH_AUTO(LOP_MOD, %)
        _LP_VM_ARITH_AUTO_SINGLE(LOP_NOT,~)
        _LP_VM_ARITH_AUTO(LOP_EQ,==)
        _LP_VM_ARITH_AUTO(LOP_LG_OR,||)
        _LP_VM_ARITH_AUTO(LOP_LG_AND,&&)
        _LP_VM_ARITH_AUTO(LOP_LG_NOTEQ,!=)
        _LP_VM_ARITH_AUTO(LOP_GRT,>)
        _LP_VM_ARITH_AUTO(LOP_LESS,<)
        _LP_VM_ARITH_AUTO(LOP_GRT_EQ,>=)
        _LP_VM_ARITH_AUTO(LOP_LESS_EQ,<=)

        default:
            stat = -1;
            lpdebug("[UNKNOWN VM OP:0x%x]\n",op);
            break;
        }
    }
    lpinfo("Vm end");
    lp_vm_dump_stack(ctx);
}

LP_Err lp_vm_start(lp_vm_ctx *ctx, lpptrsize entrypoint)
{
    lpvmptr old_pc = ctx->opcodes.pc;
    ctx->opcodes.pc += entrypoint;
    lp_vm_continue(ctx);
    ctx->opcodes.pc = old_pc;
    
}
