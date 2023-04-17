#include<stdio.h>
#include<malloc.h>
#include"lpvm.h"
#include"lpcompiler.h"
int main()
{
    lp_vm_ctx ctx;
    char *buf_stack = malloc(4096);
    char *buf_heap = malloc(4096);
    char *buf_codes = malloc(4096);
    char *static_res = malloc(4096);
    lp_vm_init(&ctx,buf_heap,4096,buf_stack,4096,
    buf_codes,4096,static_res, 4096);
    buf_codes[0] = LOP_LOADc;
    *(uint32_t*)(&buf_codes[1])=233;
    buf_codes[5] = LOP_LOADcn;
    *(uint32_t*)(&buf_codes[6])=2;
    *(uint32_t*)(&buf_codes[10])=123;
    *(uint32_t*)(&buf_codes[14])=456;
    buf_codes[18] = LOP_POP;
    *(uint32_t*)(&buf_codes[19])=0;
    buf_codes[23]=LOP_EQ;
    buf_codes[24]=LOP_TCONST;
    *(uint32_t*)(&buf_codes[25])=1;
    *(uint32_t*)(&buf_codes[29])=1;
    //*(uint32_t*)(&buf_codes[33])=1;
    buf_codes[33]=LOP_MOV;
    buf_codes[34]=LOP_MVTCONST|(LOP_MVTSTACK<<4);
    *(uint32_t*)(&buf_codes[35])=0;
    *(uint32_t*)(&buf_codes[39])=666;
    *(uint32_t*)(&buf_codes[43])=0;
    lp_vm_start(&ctx,0);
    lp_compiler comp;
    lp_compiler_init(&comp,&ctx,"if(a==233){",12);
    lp_compiler_do(&comp);
}