#include"lpcompiler.h"
#include"lperr.h"
#include<stdlib.h>
void lp_compiler_init(lp_compiler *ctx,lp_vm_ctx* vmctx, char *codebuf, lpsize bufsize)
{
    lpnull(ctx&&vmctx&&codebuf&&bufsize);
    ctx->code_buf.codes = codebuf;
    ctx->code_buf.cur_pos = 0;
    ctx->code_buf.col = ctx->code_buf.row = 0;
    ctx->code_buf.max_size = bufsize;
    lp_array_init(&ctx->symbol_table,64,64);
    lp_array_init(&ctx->token_table,64,64);
    lp_array_init(&ctx->type_table,64,64);
    ctx->vm = vmctx;
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
    return e;
}

LP_Err lp_compiler_codegen(lp_compiler *ctx)
{
    return LP_OK;
}