#include"lpcompiler.h"
#include"lperr.h"
#include<string.h>
#include<stdlib.h>
#define lp_nexttoken lp_lexer_next(ctx,0)
#define lp_nexttoken_skipable lp_lexer_next(ctx,1)
#define LP_BUILTIN_TYPE_NUM 1
#define LP_ERR(x,t) lpprintf("[PARSER] At row:%d,col:%d:\n%s\n",(t).row,(t).col,x); exit(-1);
#define LP_ERR_MATCH(t,tt) lpprintf("[PARSER] At row:%d,col:%d:\nFail to match symbol: %d\n",(t).row,(t).col,(tt)); exit(-1);

const lp_parse_structed_type lp_builtin_types[LP_BUILTIN_TYPE_NUM] ={
    {
        .root_type={
            .builtin_name = "int",
            .occupy_bytes = 4,
        },
        .inner_types = 0
    }
};

lp_parse_structed_type *lp_get_builtin_type(lp_lex_token *name)
{
    for (lpsize i = 0; i < LP_BUILTIN_TYPE_NUM; i++)
    {
        if(!strcmp(lp_builtin_types[i].root_type.builtin_name,name->v_str))
        {
            return &lp_builtin_types[i];
        }
    }
    return 0;
}

lp_parse_eval_value *lp_new_eval_val(lp_parse_structed_type *type, lpvmptr value, char const_type)
{
    lp_parse_eval_value *r = lpmalloc(sizeof(lp_parse_eval_value));
    lpnull(r);
    r->type = type;
    r->v_addr = value;
    r->const_type = const_type;
    return r;
}

lp_parse_symbol *lp_new_symbol(lp_compiler *ctx, lp_parse_structed_type *type, lp_lex_token *name)
{
    lp_parse_symbol *r = lpmalloc(sizeof(lp_parse_symbol));
    lpnull(r);
    r->stack_offset = ctx->stack_offset;
    ctx->stack_offset += type->root_type.occupy_bytes;
    r->type = type;
    r->words = name;
    return r;
}

#define lp_codegen_push(v,t) lp_vm_push(ctx->vm,t,v)

lp_parse_symbol *lp_parser_add_variable(lp_compiler *ctx, lp_lex_token *name, lp_lex_token *type)
{
    lp_parse_structed_type *t = lp_get_builtin_type(type->v_str);
    if(!t)
    {
        LP_ERR("Unknown Data Type",*type);
    }
    return lp_new_symbol(ctx,t,name);
}

void lp_parser_match(lp_lex_token *token,lptoken t)
{
    if(token->ttype != t)
    {
        LP_ERR_MATCH(*token,t);
    }
}

static void lp_parser_print_val(lp_parse_eval_value *v)
{
    lpdebug("Type:%d,Number:%d,Addr:0x%x\n",v->type,v->v_number,v->v_addr);
}

lp_parse_eval_value *lp_parser_expr_factor(lp_compiler *ctx)
{
    // constant
    // -/+
    // (
    // variable
    lp_lex_token *t = lp_nexttoken;
    if(t->ttype == LPT_NUMBER)
    {
        return lp_new_eval_val(&lp_builtin_types[LPBT_INT],t->v_int,LPCT_INT);
    }else if(t->ttype == LPT_ADD)
    {
        lp_parse_eval_value *r = lp_parser_expr_factor(ctx);
        return r;
    }else if(t->ttype == LPT_MINUS)
    {
        lp_parse_eval_value *r = lp_parser_expr_factor(ctx);
        r->v_number = -r->v_number;
        return r;
    }else 
    {
        LP_ERR("Bad Expression Format!",*t);
    }
}

lp_parse_eval_value *lp_parser_const_op(lp_parse_eval_value *left, lp_parse_eval_value *right,lp_lex_token *op)
{
    if(op->ttype == LPT_ADD)
        left->v_number = left->v_number + right->v_number;
    else if(op->ttype == LPT_MINUS)
        left->v_number = left->v_number - right->v_number;
    else if(op->ttype == LPT_MUL)
        left->v_number = left->v_number * right->v_number;
    else if(op->ttype == LPT_DIV)
        left->v_number = left->v_number / right->v_number;
    return left;
}

lp_parse_eval_value *lp_parser_expr_term(lp_compiler *ctx)
{
    lp_parse_eval_value *left_val = lp_parser_expr_factor(ctx);
    lp_lex_token *op = lp_nexttoken_skipable;
    if(!op)
        return left_val;
    while (op->ttype == LPT_MUL || op->ttype == LPT_DIV)
    {
        lp_parse_eval_value *right_val = lp_parser_expr_factor(ctx);
        if(left_val->const_type == LPCT_INT && left_val->const_type == LPCT_INT)
        {
            left_val = lp_parser_const_op(left_val,right_val,op);
        }
        op = lp_nexttoken_skipable;   
        if(!op)
            return left_val;
    }
    lp_lexer_back(ctx);
    return left_val;
    
}

lp_parse_eval_value *lp_parser_root_expr(lp_compiler *ctx)
{
    lp_parse_eval_value *left_val = lp_parser_expr_term(ctx);
    lp_lex_token *op = lp_nexttoken_skipable;
    if(!op)
        return left_val;
    while (op->ttype == LPT_ADD || op->ttype == LPT_MINUS)
    {
        lp_parse_eval_value *right_val = lp_parser_expr_term(ctx);
        if(left_val->const_type == LPCT_INT && left_val->const_type == LPCT_INT)
        {
            left_val = lp_parser_const_op(left_val,right_val,op);
        }
        op = lp_nexttoken_skipable;   
        if(!op)
            return left_val;
    }
    lp_lexer_back(ctx);
    return left_val;
}

LP_Err lp_parser_statment(lp_compiler *ctx)
{
    lp_lex_token *root_t = lp_nexttoken;
    if(root_t->ttype == LPT_KW_LET)
    {
        // let word1[,word2 ... ] : type = exp1[,exp2]
        lp_lex_token *left = lp_nexttoken;
        lp_parser_match(left,LPT_WORDS);
        lp_parser_match(lp_nexttoken,LPT_COLON);
        lp_lex_token *type = lp_nexttoken;
        lp_parser_match(type,LPT_WORDS);
        lp_parser_match(lp_nexttoken,LPT_EQ);
        lpdebug("match :type: %s left:%s;\n",type->v_str, left->v_str);

    }
    else if(root_t->ttype == LPT_WORDS)
    {
        
    }
    else if(root_t->ttype == LPT_NUMBER)
    {
        lp_lexer_back(ctx);
        lp_parse_eval_value *left = lp_parser_root_expr(ctx);
        lp_parser_print_val(left);
    }
}

LP_Err lp_compiler_codegen(lp_compiler *ctx)
{
    lp_parser_statment(ctx);
}