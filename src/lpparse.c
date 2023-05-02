#include"lpcompiler.h"
#include"lperr.h"
#include<string.h>
#include<stdlib.h>
#define lp_nexttoken lp_lexer_next(ctx,0)
#define lp_nexttoken_skipable lp_lexer_next(ctx,1)
#define LP_BUILTIN_TYPE_NUM 1
#define LP_ERR(x,t) lpprintf("[PARSER] At row:%d,col:%d:\n%s\n",(t).row,(t).col,x); exit(-1);
#define LP_ERR_MATCH(t,tt) lpprintf("[PARSER] At row:%d,col:%d:\nFail to match symbol: %d\n",(t).row,(t).col,(tt)); exit(-1);
#define LP_ASSERT(x,f) do{if(!(x)){f;return -1;}}while(0);

#define LP_RESET_FIELD(ctx) ctx->stack_offset = 0;ctx->func_field=0;
const lp_parse_structed_type lp_builtin_types[LP_BUILTIN_TYPE_NUM] ={
    {
        .root_type={
            .builtin_name = "int",
            .occupy_bytes = 4,
        },
        .inner_types = 0
    }
};

void lp_parser_push_op(lp_compiler *ctx, char op)
{
    lp_bin_pushop(ctx->vm,op);
}

lp_parse_structed_type *lp_get_builtin_type(char *name)
{
    for (lpsize i = 0; i < LP_BUILTIN_TYPE_NUM; i++)
    {
        if(!strcmp(lp_builtin_types[i].root_type.builtin_name,name))
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

    //TODO: Add IL Generate

    r->type = type;
    r->words = name;
    r->field = ctx->func_field;

    return r;
}

#define lp_codegen_push(v,t) lp_vm_push(ctx->vm,t,v)

lpbool lp_parser_sym_chkglo(lp_compiler *ctx, lp_lex_token *name, lp_parse_symbol **redef)
{
    for (lpsize i = 0; i < ctx->glo_symbol_table.top; i++)
    {
        lp_parse_symbol *n = ctx->glo_symbol_table.data[i];
        lpnull(n);
        if(!strcmp(n->words->v_str,name->v_str))
        {
            if(redef)
                *redef = n;
            return 1;
        }
    }
    return 0;
}

void lp_parser_gen_binopcode(lp_compiler *ctx, lp_lex_token t)
{
    if(t.ttype == LPT_ADD)
    {
        lp_parser_push_op(ctx,LOP_ADD);
    }else if(t.ttype == LPT_MINUS)
    {
        lp_parser_push_op(ctx,LOP_MINUS);
    }
    else if(t.ttype == LPT_MUL)
    {
        lp_parser_push_op(ctx,LOP_MUL);
    }else if(t.ttype == LPT_DIV)
    {
        lp_parser_push_op(ctx,LOP_DIV);
    }
    lpdebug("[OP] Push a binop;\n");
}

void lp_parser_gen_loadcode(lp_compiler *ctx,lp_parse_eval_value *v)
{
    if(v->const_type == LPCT_GENERED)
        return;
    if(v->const_type)
    {
        if(v->const_type == LPCT_INT)
        {
            lp_parser_push_op(ctx,LOP_LOADc);
            lp_bin_pushval(ctx->vm,v->v_number);
            lpdebug("[OP]: PUSH 0x%x;\n",v->v_number);
        }
    }else
    {
        lp_parser_push_op(ctx,LOP_PUSHs);
        lp_bin_pushval(ctx->vm,v->v_stackoffset);
        lpdebug("[OP]: PUSHs 0x%x;\n",v->v_stackoffset);
    }
    v->const_type = LPCT_GENERED;
    
}

lp_parse_symbol *lp_parser_add_variable(lp_compiler *ctx, lp_lex_token *name, lp_lex_token *type)
{
    lp_parse_symbol *redef = 0;
    if(lp_parser_sym_chkglo(ctx,name,&redef))
    {
        
        lpprintf("Symbol Name:%s, First defined here At Row:%d, Col:%d\n",name->v_str,redef->words->row,redef->words->col);
        LP_ERR("Redefine a global symbol",*name);
        return NULL;
    }
    lp_parse_structed_type *t = &lp_builtin_types[LPBT_INT];
    if(type)
    {
        t = lp_get_builtin_type(type->v_str);
        if(!t)
        {
            LP_ERR("Unknown Data Type",*type);
        }
    }
    lp_parse_symbol* r = lp_new_symbol(ctx,t,name);
    
    r->field = ctx->func_field;
    

    if(r->field == 0)
    {
        //lpdebug("GLO VAR!\n");
        lp_array_push(&ctx->glo_symbol_table,r);
    }
    lpdebug("Assgin A Variable:`%s`, addr:`0x%x`\n",r->words->v_str,r->stack_offset);
    return r;
}

lp_parse_symbol *lp_parser_lookup_var(lp_compiler *ctx, lp_lex_token *name)
{
    lp_parse_symbol *ret = 0;
    lp_parser_sym_chkglo(ctx,name,&ret);

    //TODO: Check field-var
    //lpdebug("Find Glo Var:0x%x %s\n",ret,ret->words->v_str);
    return ret;
}

#define lp_parser_match_chk(token,t) (token == t)
void lp_parser_match(lp_lex_token *token,lptoken t)
{
    if(token->ttype != t)
    {
        LP_ERR_MATCH(*token,t);
    }
}

static void lp_parser_print_val(lp_parse_eval_value *v)
{
    if(!v)
    {
        lpdebug("Type:Null Evaluation Value!\n");
        return;
    }
    lpdebug("Type:%d,Number:%d,Addr:0x%x\n",v->type,v->v_number,v->v_addr);
}

lp_parse_eval_value *lp_parser_expr(lp_compiler *ctx);

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
        if(r->const_type == LPCT_INT)
            r->v_number = -r->v_number;
        else
        {
            lpvmvalue tmp = 0;
            lp_parser_push_op(ctx,LOP_LOADc);
            lp_bin_pushval(ctx->vm,tmp);
            lpdebug("[OP] Get Minus;\n");
            lp_parser_gen_loadcode(ctx,r);
            lp_parser_push_op(ctx,LOP_MINUS);
        }
        return r;
    }else if(t->ttype == LPT_WORDS)
    {
        lp_parse_symbol *sym = lp_parser_lookup_var(ctx,t);
        if(!sym)
        {
            LP_ERR("Unknown Variable Name!",*t);
        }
        lpdebug("[EVAL] : A variable;\n");
        
        lp_parse_eval_value *v = lp_new_eval_val(sym->type,sym->stack_offset,0);   
        //lp_parser_gen_loadcode(ctx,v);
        return v;
    }
    else if(t->ttype == LPT_LEFT_PAREN)
    {
        lp_parse_eval_value *v = lp_parser_expr(ctx);
        lp_parser_match(lp_nexttoken,LPT_RIGHT_PAREN);
        return v;
    }
    else 
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
        }else
        {
            lp_parser_gen_loadcode(ctx,left_val);
            lp_parser_gen_loadcode(ctx,right_val);
            lp_parser_gen_binopcode(ctx,*op);
        }
        op = lp_nexttoken_skipable;   
        if(!op)
            return left_val;
    }
    lp_lexer_back(ctx);
    return left_val;
    
}

lp_parse_eval_value *lp_parser_expr(lp_compiler *ctx)
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
        }else
        {
            lp_parser_gen_loadcode(ctx,left_val);
            lp_parser_gen_loadcode(ctx,right_val);
            // Mark this two value as ALREADY generated codes
            //

            lp_parser_gen_binopcode(ctx,*op);
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
    lp_parse_eval_value *v = lp_parser_expr(ctx);
    // Generate IL codes;
    if(v->const_type)
    {
        lp_parser_gen_loadcode(ctx,v);
    }
    return v;
}

LP_Err lp_parser_statment(lp_compiler *ctx)
{
    lp_lex_token *root_t = lp_nexttoken;
    if(root_t->ttype == LPT_KW_LET)
    {
        // let word1[,word2 ... ] : type = exp1[,exp2]
        lp_lex_token *left = lp_nexttoken;
        lp_parser_match(left,LPT_WORDS);
        lp_lex_token *sep = lp_nexttoken;
        lp_lex_token *type = 0;
        if(sep->ttype == LPT_COLON)
        {
            type = lp_nexttoken;
            lp_parser_match(type,LPT_WORDS);
            lp_parser_match(lp_nexttoken,LPT_EQ);
        }
        else if(sep->ttype != LPT_EQ)
        {
            LP_ERR("Expect a '=' after `let`",*sep);
        }
        lp_parse_eval_value *right = lp_parser_root_expr(ctx);
        lp_parse_symbol * sym = lp_parser_add_variable(ctx,left,type);
        if(!type)
        {
            // Auto Derivation
            sym->type = right->type;
            if(!sym->type)
            {
                LP_ERR("Fail to auto-derivate type here!",*left);
            }
        }
        //lpdebug("Let ok");
        LP_ASSERT(left,lpprintf(LPDINFO"[Parse]: Expect a expression after '='"))
        //lpdebug("match :type: %s left:%s; right:",type->v_str?:"auto", left->v_str);
        lp_parser_print_val(right);

        //lp_parse_symbol * sym = lp_parser_add_variable(ctx,left,type);
        

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
    lp_parser_match(lp_nexttoken,LPT_SEMICOLON);
    if(ctx->interpret_mode)
    {
        // TODO: run IL codes here
        lp_parser_push_op(ctx,LOP_ERR);
        lp_vm_continue(ctx->vm);
        ctx->vm->opcodes.codes_end --;
        ctx->vm->opcodes.pc --;
    }
}

LP_Err lp_compiler_codegen(lp_compiler *ctx)
{
    LP_RESET_FIELD(ctx);
    while(1)
    {
        lp_parser_statment(ctx);
    }
    
}