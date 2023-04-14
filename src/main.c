#include<stdio.h>
#include<malloc.h>
#include"lpvm.h"
int main()
{
    lp_vm_ctx ctx;
    char *buf_stack = malloc(4096);
    char *buf_heap = malloc(4096);
    char *buf_codes = malloc(4096);
    lp_vm_init(&ctx,buf_heap,4096,buf_stack,4096,
    buf_codes,4096);
}