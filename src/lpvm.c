#include "lpvm.h"
#include"lperr.h"
#include"lpmem.h"
#include "lptypes.h"
#include<stdio.h>
#include<string.h>

void lp_vm_init(lp_vm_ctx *ctx, char *mem,lpsize mem_size, lpsize heap_size,
                lpsize stack_size,lpsize code_size, 
                lpsize ressize)
{
    ctx->mem_start = mem;
    lp_msetup(&ctx->mem,heap_size,mem);
    lp_vm_stack_init(&ctx->stack,mem+heap_size,stack_size);
    lp_vm_code_init(&ctx->opcodes,mem+heap_size+stack_size,code_size);
    lp_vm_staticres_init(&ctx->sres, mem+heap_size+stack_size+code_size, ressize);
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

#define _LP_VM_ARITH_AUTO(code,op) case code:\
val1=(lp_vm_pop(ctx,lpvmvalue)) op (lp_vm_pop(ctx,lpvmvalue));\
lp_vm_push(ctx,lpvmvalue,val1); \
lpdebug("[VM] BinOP!;\n");\
sprintf(debugbuf, "OP,%s\n",#op);\
debugasm;\
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
#define debugasm fwrite(debugbuf,strlen(debugbuf),1,fp);
LP_Err lp_vm_continue(lp_vm_ctx *ctx)
{
    FILE *fp = fopen("asm_debug.txt", "a");
    char debugbuf[50];
    char op = 0;
    int stat = 0;
    while (stat >= 0)
    {
        op = lp_vm_nextop(ctx);
        lpvmvalue val1 = 0, val2 = 0, *pval1 = 0, *pval2 = 0;
        char cval1 = 0;
        lpvmptr vptr1 = 0;
        char op1 = 0, op2 = 0, op3 = 0;
        lpbool notabs = 1;
        if(op>64)
        {
            op -= 64;
            notabs = 0;
        }
        switch (op)
        {
        case LOP_LOADc:
            // load one constant to the stack top
            val1 = lp_vm_nextop_value(ctx);
            lpdebug("[VM] Op: LOADc, num:%d to stack\n",val1);
            lp_vm_push(ctx,lpvmvalue,val1);
            sprintf(debugbuf, "LOADc 0x%x\n",val1);
            debugasm;
            break;
        case LOP_LOAD_STACK:
            val1 = lp_vm_nextop_value(ctx); // get offset of stack
            lpdebug("[VM] Op: Pushs, offset:0x%x;\n",val1);
            lp_vm_push(ctx,lpvmvalue,*lp_vm_stackvisit(ctx,val1,notabs));
            lp_vm_dump_stack(ctx);
            sprintf(debugbuf, "LOADs(%d) 0x%x\n",notabs,val1);
            debugasm;
            break;
        case LOP_POP_TO:
            val1 = lp_vm_nextop_value(ctx); // get offset of stack
            lpdebug("[VM] Op: POP to, offset:0x%x;\n",val1);
            pval1 = lp_vm_stackvisit(ctx, val1, notabs);
            *pval1 = *(lpvmvalue*)lp_vm_popc(ctx, 4);
            sprintf(debugbuf, "POPto(%d) 0x%x\n",notabs,val1);
            debugasm;
            break;
        case LOP_GETPTR:
            val1 = lp_vm_nextop_value(ctx);
            lpdebug("[VM] Op: GetPtr of stack offset:0x%x;\n",val1);
            pval1 = lp_vm_stackvisit(ctx, val1, 1);
            val2 = (lpptrsize)pval1-(lpptrsize)ctx->mem_start;
            lp_vm_push(ctx,lpvmvalue,val2);
            sprintf(debugbuf, "GETPTR\n",val1);
            debugasm;
            break;
        case LOP_DEPTR:
            val1 = lp_vm_pop(ctx, lpvmvalue);
            lpdebug("[VM] Op: DePtr of vaddr:0x%x;\n",val1);
            pval1 = ctx->mem_start+val1;
            val2 = *pval1;
            lp_vm_push(ctx,lpvmvalue,val2);
            sprintf(debugbuf, "DEPTR\n",val1);
            debugasm;
            break;
        case LOP_MEMSET:
            val1 = lp_vm_pop(ctx, lpvmvalue); // val
            val2 = lp_vm_pop(ctx, lpvmvalue); // dest
            pval1 = (lpvmvalue*)(val2+ctx->mem_start);
            *pval1 = val1;
            lpdebug("[VM] Op: Memset of address:0x%x,as val:%x;\n",pval1,val1);
            sprintf(debugbuf, "MEMSET\n",val1);
            debugasm;
            break;
        case LOP_NOP:
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
    fclose(fp);
}

LP_Err lp_vm_start(lp_vm_ctx *ctx, lpptrsize entrypoint)
{
    lpvmptr old_pc = ctx->opcodes.pc;
    ctx->opcodes.pc += entrypoint;
    lp_vm_continue(ctx);
    ctx->opcodes.pc = old_pc;
    
}
