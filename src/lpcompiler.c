#include"lpcompiler.h"
#include"lperr.h"
#include "lptypes.h"
#include <stdint.h>
#include <stdio.h>
#include<string.h>
#include<stdlib.h>

typedef struct
{
    uint32_t magic;
    uint32_t ver;
    uint32_t sz;
}lp_bin_header_t;
void lp_compiler_init(lp_compiler *ctx,lp_vm_ctx* vmctx, char *codebuf, lpsize bufsize)
{
    lpnull(ctx&&vmctx&&codebuf&&bufsize);
    ctx->code_buf.codes = lp_copy_str(codebuf,strlen(codebuf)+1);
    ctx->code_buf.cur_pos = 0;
    ctx->code_buf.col = ctx->code_buf.row = 0;
    ctx->code_buf.max_size = bufsize;
    ctx->code_buf.cur_line = ctx->code_buf.codes;
    lp_array_init(&ctx->symbol_table,64,64);
    lp_array_init(&ctx->token_table,64,64);
    lp_array_init(&ctx->type_table,64,64);
    lp_array_init(&ctx->glo_symbol_table,64,64);
    ctx->vm = vmctx;
    //ctx->vm->opcodes.pc = 0;
    ctx->stack_offset = 0;
    ctx->interpret_mode = 1;
}

static void lp_compiler_exit(int s)
{
    lpprintf(LPDERRO "Looping Comilier exit!" LPEOL);
    exit(s);
}

LP_Err lp_compiler_do(lp_compiler *ctx)
{
    LP_Err e = LP_OK;
    lpprintf(LPDINFO "===== Looping Comilier Start! =====" LPEOL);
    e = lp_lexer_do(ctx);
    if(e != LP_OK)
        lp_compiler_exit(e);
    e = lp_compiler_codegen(ctx);
    if(e != LP_OK)
        lp_compiler_exit(e);
    lpprintf(LPDINFO "===== Looping Comilier Finish! =====" LPEOL);
    uint32_t sz = ctx->vm->opcodes.codes_end - ctx->vm->opcodes.codes;
    lpprintf(LPDINFO "total:%db(text) " LPEOL,sz);
    FILE *fp = fopen("output.bin", "w");
    if(!fp)
    {
        lpprintf(LPDERRO "Fail to open output file!" LPEOL);
        lp_compiler_exit(-1);
    }
    lp_bin_header_t h={.magic = 0xc14ff41c,.ver=1,.sz=sz};
    fwrite(&h, 1, sizeof(lp_bin_header_t), fp);
    fwrite(ctx->vm->opcodes.codes, sz, 1, fp);
    fclose(fp);
    lpprintf(LPDINFO "Write all binary [DONE]" LPEOL);
    return e;
}
