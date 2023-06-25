#include<stdio.h>
#include<malloc.h>
#include<stdlib.h>
#include<string.h>
#include"lpvm.h"
#include"lpcompiler.h"
char *globuf=0;
lpbool input(char **codes, lpsize *sz)
{
    gets(globuf);
    int len=strlen(globuf);
    globuf[len]='\n';
    globuf[len+1]='\0';
    *codes = globuf;
    *sz = len+1;
    globuf+=len+1;
    return 1;
}

int main()
{
    lp_vm_ctx ctx;
    char *membuf = malloc(4096*4);//16k
    globuf = malloc(1024);
    lp_vm_init(&ctx,membuf,4096*4,0x1000,0x1000,
     0x1000,0x1000);
    // buf_codes[0] = LOP_LOADc;
    // *(uint32_t*)(&buf_codes[1])=233;
    // buf_codes[5] = LOP_LOADcn;
    // *(uint32_t*)(&buf_codes[6])=2;
    // *(uint32_t*)(&buf_codes[10])=123;
    // *(uint32_t*)(&buf_codes[14])=456;
    // buf_codes[18] = LOP_POP;
    // *(uint32_t*)(&buf_codes[19])=0;
    // buf_codes[23]=LOP_EQ;
    // buf_codes[24]=LOP_TCONST;
    // *(uint32_t*)(&buf_codes[25])=1;
    // *(uint32_t*)(&buf_codes[29])=1;
    // //*(uint32_t*)(&buf_codes[33])=1;
    // buf_codes[33]=LOP_MOV;
    // buf_codes[34]=LOP_MVTCONST|(LOP_MVTSTACK<<4);
    // *(uint32_t*)(&buf_codes[35])=0;
    // *(uint32_t*)(&buf_codes[39])=666;
    // *(uint32_t*)(&buf_codes[43])=0;
    // lp_vm_start(&ctx,0);
    lp_compiler comp;
    FILE *fp = fopen("asm_debug.txt", "w");
    fclose(fp);
    // lp_compiler_init(&comp,&ctx,"let x:int = 2*3-7;let y:int = 2;struct s1{let x:int;let y:int};",64);
    lp_compiler_init(&comp,&ctx,"int z =233,*y=((int*)&z);int w=z ; int ww=w;\n",46);
    comp.input_callback = input;
    lp_compiler_do(&comp);
}