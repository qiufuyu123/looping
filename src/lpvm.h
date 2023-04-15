#ifndef _H_LPVM
#define _H_LPVM
#include"lpmem.h"
typedef enum
{
    // 0000_0000b
    LOP_ERR,
    LOP_TPOPS,      // pop v1, pop v2, push v1 + v2
    LOP_TCONST,     // const const
    LOP_TCONSTSTACK, // stack const
    LOP_TSTACK,     // stack stack
    LOP_TSTACKDST,   // stack stack dst
    LOP_TCONSTSTACKDST, // const stack dst
    LOP_TADDRCONST, // addr(const) <- const
    LOP_TADDRSTACK, // addr -> stack
    LOP_TSTACK2ADDR, // addr <- stack
    LOP_TNR,
    LOP_NOP,
    LOP_LOADc,
    LOP_LOADcn,
    LOP_LOADst, // load a const to static
    LOP_LEA,
    LOP_POP,
    LOP_SMOV,
    LOP_MOV,
    LOP_SMOVc,
    LOP_MOVc,
    LOP_ADD,
    LOP_MINUS,
    LOP_MUL,
    LOP_DIV,
    LOP_MOD,
    LOP_OR,
    LOP_AND,
    LOP_NOT,
    LOP_EQ,
    LOP_LG_OR,
    LOP_LG_AND,
    LOP_LG_NOTEQ,
    LOP_GRT,
    LOP_LESS,
    LOP_GRT_EQ,
    LOP_LESS_EQ
}LP_Vm_Opcodes;
/**
 * In Looping virtual machine, we only support 2 types of instructions
 * 1st:
 *      I  Param1, Param2....
 * 2st:
 *      I T Param1, Param2...
 *      (T) indicates instruction subtype:
 *          In details:
 *              LOP_ADD 0 1 2 (type=0 means the addition of 2 constant)
 *              LOP_ADD 1 2 33 (type=1 means the addition of 1 constant and 1 stack value)
 *              LOP_ADD 2 33 44 (type=2 means the addition of 2 stack value, return to stack top)
 *              LOP_ADD 3 <0x11> <0x22>  
 *              LOP_ADD 4 dst 44 55                                     ...   return to stack : dst
 *              LOP_ADD 5 dst <0x44> <0x55> 
 *              
*/

typedef struct 
{
    char *stacks;
    lpptrsize stack_ends;
    lpptrsize esp;
    lpptrsize ebp;
}lp_stack_ctx;

typedef struct 
{
    char *codes;
    char *codes_end;
    lpsize code_size;
    char* pc;
}lp_opcodes_ctx;

typedef struct 
{
    char *static_data;
    lpptrsize size;
}lp_staticres_ctx;

typedef enum
{
    LVM_NORMAL = 1,
    LVM_ENCRY_ADDR = 2
}LP_Vm_Flg;

typedef struct 
{
    lp_mem_ctx mem;
    lp_stack_ctx stack;
    lp_opcodes_ctx opcodes;
    lpvmflg vm_flg;
    lp_staticres_ctx sres;
}lp_vm_ctx;

// #define LP_BIN_MAGIC0 0xfa
// #define LP_BIN_MAGIC1 0x99
// #define LP_BIN_MAGIC2 0xdf
// typedef struct
// {
//     char magic[3]; // fa 99 df

// }lp_bin_header;


void lp_vm_init(lp_vm_ctx *ctx, char *heap, lpsize heap_size, char *stack,
                lpsize stack_size,char *codes, lpsize code_size, 
                char *staticres, lpsize ressize);

LP_Err lp_vm_start(lp_vm_ctx *ctx, lpptrsize entrypoint);

void lp_vm_staticres_init(lp_staticres_ctx *ctx, char *data, lpsize ressize);

void lp_vm_stack_init(lp_stack_ctx *ctx, char *stacks, lpptrsize size);

void lp_vm_pushc(lp_vm_ctx *ctx, char *ptr, lpptrsize size);
#define lp_vm_push(ctx,type,v) lp_vm_pushc(ctx,&v,sizeof(type))

void *lp_vm_popc(lp_vm_ctx *ctx, lpptrsize size);
#define lp_vm_pop(ctx,type) (*(type*)(lp_vm_popc(ctx,sizeof(type))))

lpvmvalue *lp_vm_stackvisit(lp_vm_ctx *ctx, lpvmvalue offset, lpbool is_ebp_based);

void lp_vm_code_init(lp_opcodes_ctx *ctx, char *codes, lpptrsize size);

char lp_vm_nextop(lp_vm_ctx *ctx);

lpvmptr lp_vm_nextop_ptr(lp_vm_ctx *ctx);

lpvmvalue lp_vm_nextop_value(lp_vm_ctx *ctx);
#endif