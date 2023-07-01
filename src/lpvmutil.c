#include "lptypes.h"
#include"lpvm.h"
#include"lperr.h"
#include<string.h>
#include<malloc.h>
void lp_vm_stack_init(lp_stack_ctx *ctx, char *stacks, lpptrsize size)
{
    lpnull(ctx);
    ctx->increable = 0;
    if(stacks == -1)
    {
        stacks = lpmalloc(1024);
        size = 1024;
        ctx->increable = 1;
    }
    lpnull(stacks && size);
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

void *lp_vm_pushc(lp_vm_ctx *ctx, char *ptr, lpptrsize size)
{
    lpnull(ctx && ptr && size);
    if(ctx->stack.esp + size >= ctx->stack.stack_ends)
    {
        lpdebug("esp:%x size:%d end:%x\n",ctx->stack.esp,size,ctx->stack.stack_ends);
        lppanic(LP_STACK_OVER_FLOW);
    }
    memcpy(ctx->stack.esp,ptr,size);
    lpptrsize old = ctx->stack.esp;
    ctx->stack.esp+=size;
    return old;
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

lpvmvalue *lp_vm_stackvisit(lp_vm_ctx *ctx, lpvmvalue offset, lpbool notabs)
{
    if(notabs)
    {
        return (lpvmvalue*)(((char*)ctx->stack.ebp)+ offset);
    }
    else
    {
        return (lpvmvalue*)(((char*)ctx->stack.stacks)+ offset);
    }
}

void lp_vm_code_init(lp_opcodes_ctx *ctx, char *codes, lpptrsize size)
{
    lpnull(ctx && size && codes);
    ctx->code_size = lp2ptr(size);
    ctx->codes = codes;
    ctx->pc = codes;
    ctx->codes_end = codes;
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

lpbool lp_vm_nextn(lp_vm_ctx *ctx,lpsize n)
{
    if(ctx->opcodes.pc +n > ctx->opcodes.codes_end)
        return 0;
    ctx->opcodes.pc += n;
    return 1;
}

void *lp_vm_op_push(lp_vm_ctx *ctx, char *v, lpsize sz)
{
    if(ctx->opcodes.codes_end + sz >= ctx->opcodes.codes + ctx->opcodes.code_size)
    {
        char *n = lprealloc(ctx->opcodes.codes,ctx->opcodes.code_size + 256);
        lpnull(n);
        ctx->opcodes.codes = n;
        ctx->opcodes.code_size += 256;
    }
    memcpy(ctx->opcodes.codes_end,v,sz);
    void *r = ctx->opcodes.codes_end;
    ctx->opcodes.codes_end += sz;
    return r;
    
}

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

void lp_array_init(lp_vm_array *ctx, lpsize sz, lpsize unit_sz)
{
    lpnull(ctx && sz && unit_sz);
    ctx->unit = unit_sz;
    ctx->capacity = sz;
    ctx->data = lpmalloc(sz*sizeof(lpptrsize));
    lpnull(ctx->data);
    ctx->top=0;
    ctx->bottom=0;
    //return LP_OK;
}

void lp_array_push(lp_vm_array *ctx, char *data)
{
    lpnull(ctx && data);
    if(ctx->top >= ctx->capacity)
    {
        ctx->capacity += ctx->unit;
        void *n = lprealloc(ctx->data,ctx->capacity*sizeof(lpptrsize));
        lpnull(n);
        ctx->data = n;
    }
    ctx->data[ctx->top] = (lpptrsize)data;
    ctx->top++;
}

void *lp_array_get(lp_vm_array *ctx, lpsize idx)
{
    lpnull(ctx);
    if(idx >= ctx->capacity)
        lppanic(LP_OUT_OF_RANGE);
    return ctx->data[idx];
}

void *lp_array_top(lp_vm_array *ctx)
{
    return  ctx->data[ctx->top-1];
}

void *lp_array_bottom(lp_vm_array *ctx)
{
    if(ctx->bottom >= ctx->top)
        return 0;
    void *r = ctx->data[ctx->bottom];
    ctx->bottom++;
    return r;
}

void *lp_array_remove(lp_vm_array *ctx, lpsize idx)
{
    lpnull(ctx);
    if(idx >= ctx->capacity)
        lppanic(LP_OUT_OF_RANGE);
    char *r = ctx->data[idx];
    ctx->data[idx] = 0;
    return r;
}

void lp_array_clean(lp_vm_array *ctx)
{
    lpnull(ctx);
    lpfree(ctx->data);
}
