#include"lpvm.h"
#include"lperr.h"
#include<string.h>

void lp_vm_stack_init(lp_stack_ctx *ctx, char *stacks, lpptrsize size)
{
    lpnull(ctx && stacks && size);
    ctx->ebp=ctx->esp = lp2ptr(stacks);
    ctx->stacks = stacks;
    ctx->stack_ends = ctx->esp + size;
    lpdebug("Stack: esp:%x size:%d end:%x\n",ctx->esp,size,ctx->stack_ends);
}

lpvmbyte* lp_vm_stack_lea(lp_vm_ctx *ctx, lpsize sz)
{
    lpnull(ctx);
    if(ctx->stack.esp + sz >= ctx->stack.stack_ends)
    {
        lpprintf(LPDERRO "Fail to alloc data in stack!;\n");
        lppanic(LP_OOM);
    }
    lpvmbyte *r = ctx->stack.esp;
    ctx->stack.esp += sz;
    return r;
}

void lp_vm_pushc(lp_vm_ctx *ctx, char *ptr, lpptrsize size)
{
    lpnull(ctx && ptr && size);
    if(ctx->stack.esp + size >= ctx->stack.stack_ends)
    {
        lpdebug("esp:%x size:%d end:%x\n",ctx->stack.esp,size,ctx->stack.stack_ends);
        lppanic(LP_STACK_OVER_FLOW);
    }
    memcpy(ctx->stack.esp,ptr,size);
    ctx->stack.esp+=size;
}

void *lp_vm_popc(lp_vm_ctx *ctx, lpptrsize size)
{
    lpnull(ctx && size);
    if(ctx->stack.esp - size <0)
        lppanic(LP_STACK_UNDER_FLOW);
    ctx->stack.esp-=size;
    lpptrsize p = ctx->stack.esp;
    return p;
}

lpvmvalue *lp_vm_stackvisit(lp_vm_ctx *ctx, lpvmvalue offset, lpbool is_ebp_based)
{
    if(is_ebp_based)
    {
        return ((lpvmvalue*)ctx->stack.ebp)+ offset;
    }
    else
    {
        return ((lpvmvalue*)ctx->stack.esp)+ offset;
    }
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

#define _lp_vm_next(type)    lp_opcodes_ctx opcodes = ctx->opcodes; \
    if(opcodes.pc >= opcodes.codes_end - sizeof(type))\
    { \
        return LOP_ERR; \
    }\
    type r = *(type*)opcodes.pc; \
    ctx->opcodes.pc+=sizeof(type); 

lpvmptr lp_vm_nextop_ptr(lp_vm_ctx *ctx)
{
    _lp_vm_next(lpvmptr)
    return r;
}

lpvmvalue lp_vm_nextop_value(lp_vm_ctx *ctx)
{
    _lp_vm_next(lpvmvalue)
    return r;
}

void lp_vm_staticres_init(lp_staticres_ctx *ctx, char *data, lpsize ressize)
{
    lpnull(ctx && data && ressize);
    ctx->static_data = data;
    ctx->size = ressize;
    ctx->end = ctx->static_data + ctx->size;
}