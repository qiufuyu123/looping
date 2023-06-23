#include"lpcompiler.h"
#include"lperr.h"
#include "lptypes.h"
#include<ctype.h>
#include<string.h>
#include<stdlib.h>
#define LP_WORDS_MAX 100

char *lp_copy_str(char *str,lpsize len)
{
    char *r = lpmalloc(len);
    lpnull(r);
    memcpy(r,str,len);
    return r;
}

lp_lex_token *lp_new_token(lp_compiler *ctx, lptoken type, lpptrsize data)
{
    lp_lex_token *r = lpmalloc(sizeof(lp_lex_token));
    lpnull(r);
    r->col = ctx->code_buf.col;
    r->row = ctx->code_buf.row;
    r->ttype = type;
    r->v_str = (char*)data;
    return r;
}
char lp_nextc(lp_compiler *ctx)
{
    char c = ctx->code_buf.codes[ctx->code_buf.cur_pos++];
    ctx->code_buf.col++;
    if(c == '\n')
    {
        ctx->code_buf.col = 0;
        ctx->code_buf.row ++;
    }
    return c;
}

#define LEX_ERR(x) lpprintf(LPDERRO "[LEXER] At: row:%d col:%d, \n" x "\n", ctx->code_buf.row,ctx->code_buf.col)
char lp_backc(lp_compiler *ctx)
{
    if(!ctx->code_buf.cur_pos)
    {
        LEX_ERR("Unexcepted NULL character before!");
        exit(-1);
    }
    ctx->code_buf.cur_pos -- ;
    char c = ctx->code_buf.codes[ctx->code_buf.cur_pos];
    ctx->code_buf.col--;
    if(c == '\n')
    {
        ctx->code_buf.row--;
    }
    return c;
}

#define isspecial(x) (x == ' ' || x == '\r' || x == '\n' || x == '\0')
#define LEX_FAIL_MATCH(s) if(!ispunct(c) && !isspecial(c))\
            {\
                LEX_ERR(s);\
                return LP_LEXER_ERR;\
            }

lptoken lp_analyze_keyword(char *code)
{
    if(!strcmp(code,"if"))
        return LPT_KW_IF;
    else if(!strcmp(code,"else"))
        return LPT_KW_ELSE;
    else if(!strcmp(code,"while"))
        return LPT_KW_WHILE;
    else if(!strcmp(code,"let"))
        return LPT_KW_LET;
    else if(!strcmp(code,"struct"))
        return LPT_KW_STRUCT;
    return LPT_NULL;
}
lptoken lp_analyze_split(char c)
{
    if(c >= '!' && c <= '/')
        return LPT_EXCLAMATION + c - '!';
    else if(c >= ':' && c <= '@')
        return LPT_COLON + c - ':';
    else if(c >= '[' && c <= '^')
        return LPT_LEFT_BRACKET + c - '[';
    else if(c == '{')
        return LPT_LEFT_CURLY;
    else if(c == '}')
        return LPT_RIGHT_CURLY;
    return LPT_NULL;

}
static void lp_lexer_dump(lp_compiler *ctx)
{
    lpprintf("===== Lexer Dump Token =====\n");
    lpprintf("Bottom:%d to Top:%d\n",ctx->token_table.bottom,ctx->token_table.top);
    for (int i = ctx->token_table.bottom; i < ctx->token_table.top; i++)
    {
        if(ctx->token_table.data[i])
        {
            lp_lex_token *t = (lp_lex_token*)ctx->token_table.data[i];
            lpprintf("TYPE:%d, Val:%d",t->ttype,t->v_int);
            if(t->ttype == LPT_STRING || t->ttype == LPT_WORDS)
            {
                lpprintf(", (%s);\n",t->v_str);
            }
            else
                lpprintf(";\n");
        }
    }
    lpprintf("===== Lexer Dump Token End =====\n");
}

LP_Err lp_lexer_do(lp_compiler *ctx)
{
    char c = 0;
    char bufs[LP_WORDS_MAX] = {0};
    while (1)
    {
        c = lp_nextc(ctx);
        if(c == '\0')
            break;
        if(isdigit(c))
        {
            memset(bufs,0,LP_WORDS_MAX);
            int i = 0;
            while (isdigit(c))
            {
                bufs[i++] = c;
                c = lp_nextc(ctx);
            }
            bufs[i] = '\0';
            LEX_FAIL_MATCH("Bad Number Format!")
            lp_array_push(&ctx->token_table,lp_new_token(ctx,LPT_NUMBER,atoi(bufs)));
            lp_backc(ctx);
        }
        else if(isalpha(c))
        {
            memset(bufs,0,LP_WORDS_MAX);
            int i = 0;
            while (isalpha(c) || isdigit(c) || c == '_')
            {
                bufs[i++] = c;
                c = lp_nextc(ctx);
            }
            bufs[i] = '\0';
            LEX_FAIL_MATCH("Bad Words or Keyword Format!")
            lptoken tk = lp_analyze_keyword(bufs);
            if(tk == LPT_NULL)
                lp_array_push(&ctx->token_table,lp_new_token(ctx,LPT_WORDS,(lpptrsize)lp_copy_str(bufs,i+1)));
            else
                lp_array_push(&ctx->token_table,lp_new_token(ctx,tk,0));
            lp_backc(ctx);
        }
        else if(c == '\"')
        {
            // handle string
            memset(bufs,0,LP_WORDS_MAX);
            int i = 0;
            c = lp_nextc(ctx);
            while(c != '\"') 
            {
                bufs[i++] = c;
                c = lp_nextc(ctx);
            }
            bufs[i] = '\0';
            lp_array_push(&ctx->token_table,lp_new_token(ctx,LPT_STRING,(lpptrsize)lp_copy_str(bufs,i+1)));
        }
        else if(ispunct(c))
        {
            lptoken tk = lp_analyze_split(c);
            if(tk == LPT_NULL)
            {
                LEX_ERR("Unexcepted Split Symbol!");
            }
            lp_array_push(&ctx->token_table,lp_new_token(ctx,tk,0));
        }
    }
    lp_lexer_dump(ctx);
    return LP_OK;
}

static void lp_lexer_expand_codes(lp_compiler *ctx, char *codes, lpsize sz)
{
    if(ctx->code_buf.cur_pos + sz >= ctx->code_buf.codes + ctx->code_buf.max_size)
    {
        char *r = lprealloc(ctx->code_buf.codes, ctx->code_buf.max_size + 256);
        lpnull(r);
        ctx->code_buf.codes = r;
        ctx->code_buf.max_size += 256;
    }
    memcpy(ctx->code_buf.codes + ctx->code_buf.cur_pos,codes,sz);
}

void lp_lexer_back(lp_compiler *ctx)
{
    ctx->token_table.bottom--;
}

lp_lex_token *lp_lexer_next(lp_compiler *ctx,lpbool skip)
{
    lp_lex_token *r = lp_array_bottom(&ctx->token_table);
    while(!r)
    {
        if(skip)
        {
            return 0;
        }
        char *newcode;
        lpsize new_sz;
        if(!ctx->input_callback(&newcode,&new_sz))
            return 0;
        lpdebug("expand:%s\n",newcode);
        lp_lexer_expand_codes(ctx,newcode,new_sz);
        lp_lexer_do(ctx);
        r = lp_array_bottom(&ctx->token_table);
    }
    ctx->cur_token = r;
    return r;
}