#include<stdio.h>
#include<malloc.h>
#include"lpvm.h"
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
    buf_codes[23]=0;
    lp_vm_start(&ctx,0);
}