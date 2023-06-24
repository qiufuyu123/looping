#include"lpcompiler.h"
#include"lperr.h"
#include "lptypes.h"
#include "lpvm.h"
#include<string.h>
#include<stdlib.h>
#define lp_nexttoken lp_lexer_next(ctx,0,1)
#define lp_nexttoken_skipable lp_lexer_next(ctx,1,1)
#define lp_forwardtoken lp_lexer_next(ctx,0,0)
#define LP_BUILTIN_TYPE_NUM 3
#define LP_ERR(x,t) lpprintf("\033[31m[ERROR] \033[39mAt row:\033[35m%d\033[39m,col:\033[35m%d\033[39m ,around:\n-->\033[33m %20s ...(cutted)\033[39mFor reason here:\n%s\n",(t).row+1,(t).col+1,(t).line_str,(t),x); exit(-1);
#define LP_ERR_MATCH(t,tt) lpprintf("[PARSER] At row:%d,col:%d:\nFail to match symbol: %d\n",(t).row,(t).col,(tt)); exit(-1);
#define LP_ASSERT(x,f) do{if(!(x)){f;return -1;}}while(0);

#define LP_RESET_FIELD(ctx) ctx->stack_offset = 0;ctx->func_field=0;
lp_parse_structed_type lp_builtin_types[LP_BUILTIN_TYPE_NUM] ={
    {
        .root_type={
            .builtin_name = "int",
            .occupy_bytes = 4,
        },
        .inner_types = 0
    },
    {
        .root_type={
            .builtin_name = "str",
            .occupy_bytes = 1,
        },
        .inner_types = 0
    },
    {
        .root_type={
            .builtin_name = "ptr",
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

void lp_new_eval_val(lp_parse_eval_value *r,lp_parse_structed_type *type, lpvmptr value, char const_type)
{
    //lp_parse_eval_value *r = lpmalloc(sizeof(lp_parse_eval_value));
    lpnull(r);
    r->type = type;
    r->v_addr = value;
    r->const_type = const_type;
    r->array_length = 1;
    r->is_loaded = 0;
    r->ptr_depth = 0;
    //return r;
}

lp_parse_structed_type *lp_new_structed_type_root(lp_lex_token *name)
{
    lp_parse_structed_type *r = lpmalloc(sizeof(lp_parse_structed_type));
    lpnull(r);
    r->root_type.words = name;
    lp_array_init(&r->inner_types,16,16);
    return r;
}

lp_parse_symbol *lp_new_symbol(lp_compiler *ctx, lp_parse_structed_type *type, lp_lex_token *name,lpbool auto_offset)
{
    lp_parse_symbol *r = lpmalloc(sizeof(lp_parse_symbol));
    lpnull(r);
    if(auto_offset)
    {
        r->stack_offset = ctx->stack_offset;
        ctx->stack_offset += type->root_type.occupy_bytes;
    }

    //TODO: Add IL Generate

    r->type = type;
    r->words = name;
    r->field = ctx->func_field;
    r->ptr_depth=0;
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
    if(v->is_loaded)
        return;
    if(v->const_type)
    {
        if(v->const_type == LPCT_INT)
        {
            lp_parser_push_op(ctx,LOP_LOADc);
            lp_bin_pushval(ctx->vm,v->v_number);
            lpdebug("[OP]: PUSH 0x%x;\n",v->v_number);
            //ctx->stack_offset += 4;
        }else if(v->const_type == LPCT_STRING)
        {
            //TODO: Use static resource
            char *data = v->v_addr;
            lpvmvalue len = v->array_length;
            lpdebug("Load:%s %d;\n",data,len);
            lp_parser_push_op(ctx,LOP_LOADcn);
            lp_bin_pushval(ctx->vm,len);
            lp_vm_op_push(ctx->vm,data,len);
            //v->v_stackoffset = ctx->stack_offset;
            //ctx->stack_offset += len;
            v->const_type = LPCT_PTR;
            v->v_addr = ctx->stack_offset;
            ctx->stack_offset += v->array_length;
        }
    }else
    {
        lpptrsize data = v->v_stackoffset;
        lpvmvalue len = v->sym->array_length * v->sym->type->root_type.occupy_bytes;
        lpdebug("Load:0x%x %d;\n",data,len);
        lp_parser_push_op(ctx,LOP_LOAD_STACKN);
        lp_bin_pushval(ctx->vm,len);
        lp_bin_pushval(ctx->vm,data);
            //ctx->stack_offset += len;
        //TODO: Multibytes load
    }
    v->is_loaded = 1;
    
}

void lp_parser_gen_assigncode(lp_compiler *ctx, lp_parse_eval_value *left, lp_parse_eval_value *right)
{
    lpdebug("Assign :%d --> %d\n",ctx->stack_offset,left->v_stackoffset);
    if(!right->is_loaded)
    {
        lp_parser_gen_loadcode(ctx,right);
    }
    lp_parser_push_op(ctx,LOP_POP_TO);
    //lp_bin_pushval(ctx->vm,right->type->root_type.occupy_bytes);
    lp_bin_pushval(ctx->vm,left->v_stackoffset);
    //lp_bin_pushval(ctx->vm,ctx->stack_offset);
    

}

lp_parse_structed_type *lp_parser_get_type(lp_compiler*ctx, lp_lex_token *name)
{
    lp_parse_structed_type *type = lp_get_builtin_type(name->v_str);
    if(type)
        return type;
    for (int i = 0; i < ctx->type_table.top; i++)
    {
        type = ctx->type_table.data[i];
        if(!strcmp(type->root_type.words->v_str,name->v_str))
            return type;
    }
    return 0;
    
}

lp_parse_symbol *lp_parser_add_variable(lp_compiler *ctx, lp_lex_token *name, lp_lex_token *typename, lp_parse_structed_type *type)
{
    lp_parse_symbol *redef = 0;
    if(lp_parser_sym_chkglo(ctx,name,&redef))
    {
        
        lpprintf("Symbol Name:%s, First defined here At Row:%d, Col:%d\n",name->v_str,redef->words->row,redef->words->col);
        LP_ERR("Redefine a global symbol",*name);
        return NULL;
    }
    lp_parse_structed_type *t = &lp_builtin_types[LPBT_INT];
    if(typename)
    {
        t = lp_parser_get_type(ctx,typename);
        if(!t)
        {
            LP_ERR("Unknown Data Type",*typename);
        }
    }else if(type)
        t = type;
    lp_parse_symbol* r = lp_new_symbol(ctx,t,name,1);
    
    r->field = ctx->func_field;
    

    if(r->field == 0)
    {
        //lpdebug("GLO VAR!\n");
        lp_array_push(&ctx->glo_symbol_table,r);
    }
    lpdebug("Assgin A Variable:`%s`, addr:`0x%x`, size:%d bytes\n",r->words->v_str,r->stack_offset,r->type->root_type.occupy_bytes);
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

void lp_parser_expr(lp_compiler *ctx,lp_parse_eval_value *r,lpbool gencode);

void lp_parser_left_eval(lp_compiler *ctx,lp_parse_eval_value *r,lpbool gencode)
{
    lp_parse_symbol *sym = lp_parser_lookup_var(ctx,ctx->cur_token);
    if(!sym)
    {
        LP_ERR("Unknown Variable Name!",*ctx->cur_token);
    }
    lpdebug("[EVAL] : A variable;\n");
    lp_new_eval_val(r,sym,sym->stack_offset,0); 
    r->const_type = 0;
    r->ptr_depth = sym->ptr_depth;
    r->sym = sym;
    if(gencode)
        lp_parser_gen_loadcode(ctx,r);
}

void lp_parser_expr_factor(lp_compiler *ctx,lp_parse_eval_value*r,lpbool gencode)
{
    // constant
    // -/+
    // (
    // leftvalue
    lp_lex_token *t = lp_nexttoken;
    if(t->ttype == LPT_NUMBER)
    {
        lp_new_eval_val(r,&lp_builtin_types[LPCT_INT],t->v_int,LPCT_INT);
    }else if(t->ttype == LPT_ADD)
    {
        lp_parser_expr_factor(ctx,r,gencode);
    }else if(t->ttype == LPT_MINUS)
    {
        lp_parser_expr_factor(ctx,r,gencode);
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
    }else if(t->ttype == LPT_WORDS)
    {
        //TODO: realize array visiting
        //lp_parser_gen_loadcode(ctx,v);
        lp_parser_left_eval(ctx,r,gencode);
        t = lp_nexttoken_skipable;
        if(t)
        {
            if(t->ttype == LPT_LEFT_BRACKET)
            {
                // index of array
                t = lp_nexttoken;
                lp_parser_match(t,LPT_NUMBER);
                lpsize index = t->v_int;
                if(index >= r->sym->array_length)
                {
                    //LP_ERR("",t)
                }
            }else {
                lp_lexer_back(ctx);
            }
        }
    }
    else if(t->ttype == LPT_LEFT_PAREN)
    {
        lp_parser_expr(ctx,r,gencode);
        lp_parser_match(lp_nexttoken,LPT_RIGHT_PAREN);
    }
    else if(t->ttype == LPT_STRING)
    {
        lp_new_eval_val(r,&lp_builtin_types[LPBT_STR],t->v_str,LPCT_STRING);
        r->array_length = strlen(t->v_str)+1;
        lp_parser_gen_loadcode(ctx,r);
    }
    else if(t->ttype == LPT_ADDRESS)
    {
        // get pointer
        lp_parser_expr_factor(ctx, r, 0);
        if(r->const_type)
        {
            LP_ERR("Cannot get address of a constant!",*ctx->cur_token);
        }
        lpdebug("[PARSER] Try to pointer");
        lp_parser_push_op(ctx, LOP_GETPTR);
        lp_vm_op_push(ctx->vm, &r->sym->stack_offset, 4);
        r->ptr_depth++;
        r->is_loaded = 1;
    }
    else if(t->ttype == LPT_MUL)
    {
        // de-pointer
        lp_parser_expr_factor(ctx, r, 1);
        if(r->const_type)
        {
            lp_parser_gen_loadcode(ctx, r);
            //lp_parser_push_op(ctx, LOP_DEPTR);
            //lp_vm_op_push(ctx->vm, &ctx->stack_offset, 4);
            // may be direct address?
        }
        else {
            // Since we already generate the load code
            // we do not need to do anything here
            if(!r->ptr_depth)
            {
                LP_ERR("Cannot resolve a variable as a pointer!", *ctx->cur_token);
            }
            r->ptr_depth--;
        }
        lp_parser_push_op(ctx, LOP_DEPTR);
        lpdebug("[PARSER] Deptr(%d level);\n",r->ptr_depth);
        r->is_loaded = 1;

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

void lp_parser_expr_term(lp_compiler *ctx,lp_parse_eval_value *r,lpbool gencode)
{
    lp_parse_eval_value left_val;
    lp_parser_expr_factor(ctx,&left_val,gencode);
    lp_lex_token *op = lp_nexttoken_skipable;
    if(!op)
    {
        *r = left_val;
        return;
    }
    while (op->ttype == LPT_MUL || op->ttype == LPT_DIV)
    {
        lp_parse_eval_value right_val;
        lp_parser_expr_factor(ctx,&right_val,gencode);
        if(left_val.const_type == LPCT_INT && left_val.const_type == LPCT_INT)
        {
            left_val = *lp_parser_const_op(&left_val,&right_val,op);
        }else
        {
            lp_parser_gen_loadcode(ctx,&left_val);
            lp_parser_gen_loadcode(ctx,&right_val);
            lp_parser_gen_binopcode(ctx,*op);
        }
        op = lp_nexttoken_skipable;   
        if(!op)
        {
            *r = left_val;
            return;
        }
    }
    lp_lexer_back(ctx);
    *r=left_val;
    return;
    
}

void lp_parser_expr(lp_compiler *ctx,lp_parse_eval_value *r,lpbool gencode)
{
    lp_parse_eval_value left_val;
    lp_parser_expr_term(ctx,&left_val,gencode);
    lp_lex_token *op = lp_nexttoken_skipable;
    if(!op)
    {
        *r = left_val;
        return;
    }
    while (op->ttype == LPT_ADD || op->ttype == LPT_MINUS)
    {
        lp_parse_eval_value right_val;
        lp_parser_expr_term(ctx,&right_val,gencode);
        if(left_val.const_type == LPCT_INT && left_val.const_type == LPCT_INT)
        {
            left_val = *lp_parser_const_op(&left_val,&right_val,op);
        }else
        {
            lp_parser_gen_loadcode(ctx,&left_val);
            lp_parser_gen_loadcode(ctx,&right_val);
            // Mark this two value as ALREADY generated codes
            //

            lp_parser_gen_binopcode(ctx,*op);
        }
        op = lp_nexttoken_skipable;   
        if(!op)
        {
            *r = left_val;
            return;
        };
    }
    lp_lexer_back(ctx);
    *r = left_val;
}

void lp_parser_root_expr(lp_compiler *ctx,lp_parse_eval_value *r,lpbool gen_load_in_const)
{
    lp_parser_expr(ctx,r,gen_load_in_const);
    // Generate IL codes;
    if(!r->is_loaded && gen_load_in_const)
    {
        
        lp_parser_gen_loadcode(ctx,r);
    }
}

// lp_parse_structed_type *lp_parser_get_type(lp_parse_eval_value *v)
// {
//     if(v->const_type)
//         return v->type;
//     return v->sym->type;
// }



lpbool lp_parser_raw_typechk(lp_parse_eval_value left,lp_parse_eval_value right)
{
    lpvmvalue lsz=0,rsz=0;
    if(left.const_type)
    {
        lsz = left.type->root_type.occupy_bytes * left.array_length;
    }else {
        if(left.ptr_depth)
            lsz = 4;
        else 
            lsz = left.sym->type->root_type.occupy_bytes * left.sym->array_length;
    }
    if(right.const_type)
    {
        rsz = right.type->root_type.occupy_bytes * left.array_length;
    }
    else {
        if(right.ptr_depth)
            rsz = 4;
        else 
            rsz = right.sym->type->root_type.occupy_bytes * right.sym->array_length;
    }
    lpdebug("[PARSER] Raw type check:left:%d,right:%d;\n",lsz,rsz);
    return lsz == rsz;
}

LP_Err lp_parser_statment(lp_compiler *ctx)
{
    lp_lex_token *root_t = lp_nexttoken;
    

    if(root_t->ttype == LPT_WORDS && (lp_forwardtoken->ttype == LPT_WORDS || lp_forwardtoken->ttype == LPT_MUL))
    {
        lp_lex_token *type = root_t;
        while (1) {
            int ptr_depth = 0;
            lp_lex_token *left = lp_nexttoken;
            while (left->ttype == LPT_MUL) {
                ptr_depth++;
                left = lp_nexttoken;
            }
            
            lp_parser_match(left, LPT_WORDS);
            lp_lex_token *nxt = lp_nexttoken;
            lp_parse_eval_value right;
            lp_parse_symbol * sym;
            if(nxt->ttype == LPT_EQ)
            {
                
                lp_parser_root_expr(ctx,&right, 1);
                if(right.const_type)
                {
                    if(ptr_depth)
                    {
                        if(right.const_type == LPCT_STRING)
                        {
                            //TODO: String pointer;
                        }
                        //LP_ERR("Cannot assign a constant to a pointer!", *left);
                    }
                    sym = lp_parser_add_variable(ctx,left,type,0);
                    // Type Check
                    //
                    sym->array_length = right.array_length;
                }else {
                    sym = lp_parser_add_variable(ctx,left,0,right.sym->type);
                    //sym->type = right->sym->type;
                    sym->ptr_depth = ptr_depth;
                    sym->array_length = right.array_length;
                    if (sym->ptr_depth && !right.ptr_depth) {
                        //lp_parser_push_op(ctx, LOP_GETPTR);
                        LP_ERR("Fail to assign a variable to a pointer!",*left);
                    }
                    
                }
            }else if(nxt->ttype == LPT_SEMICOLON)
            {
                sym = lp_parser_add_variable(ctx, left, type,0);
            }
            sym->ptr_depth = ptr_depth;
            LP_ASSERT(left,lpprintf(LPDINFO"[Parse]: Expect a expression after '='"))
            //lpdebug("match :type: %s left:%s; right:",type->v_str?:"auto", left->v_str);
            lp_parser_print_val(&right);
            if(lp_forwardtoken->ttype == LPT_COMMA)
            {
                lp_nexttoken;
            }else {
                break;
            }
        }

    }
    else if(root_t->ttype == LPT_KW_STRUCT)
    {
        lp_lex_token *root_name = lp_nexttoken;
        lp_parser_match(root_name,LPT_WORDS);
        lp_parser_match(lp_nexttoken,LPT_LEFT_CURLY);
        lp_lex_token *t = lp_nexttoken;
        // Build a structure type
        lp_parse_structed_type *new_type = lp_new_structed_type_root(root_name);
        lpsize cur_offset = 0;
        while (t->ttype!=LPT_RIGHT_CURLY)
        {
            lp_parser_match(t,LPT_KW_LET);
            lp_lex_token *name = lp_nexttoken;
            lp_parser_match(name,LPT_WORDS);
            lp_parser_match(lp_nexttoken,LPT_COLON);
            lp_lex_token *type = lp_nexttoken;
            lp_parser_match(type,LPT_WORDS);
            //lpdebug("match inner type...");
            lp_parse_structed_type *inner_type = lp_parser_get_type(ctx,type);
            //lpdebug("[%s]",inner_type->root_type.builtin_name);
            if(!inner_type)
            {
                LP_ERR("Unknown Type!\n",*type);
            }
            lp_parse_symbol *sym = lp_new_symbol(ctx,inner_type,name,0);
            if(!sym)
            {
                LP_ERR("Fail to create a symbol for structure!",*type);
            }
            sym->stack_offset = cur_offset;
            //lp_array_push(&new_type->inner_types,sym);
            cur_offset += inner_type->root_type.occupy_bytes;
            lpdebug("[PARSER]: Add a symbol: %s, type:%s to struct:%s offset:%d;\n",sym->words->v_str,inner_type->root_type.builtin_name,new_type->root_type.words->v_str,sym->stack_offset);
            t=lp_nexttoken;
            if(t->ttype == LPT_SEMICOLON)
            {
                t = lp_nexttoken;
            }
        }
        new_type->root_type.occupy_bytes = cur_offset;
        lpdebug("Create a structure:%s size:%d;\n",new_type->root_type.words->v_str,new_type->root_type.occupy_bytes);
        lp_array_push(&ctx->type_table,new_type);
    }
    else if(root_t->ttype == LPT_WORDS)
    {
        lp_parse_eval_value left;
        //lp_lexer_back(ctx);
        lp_parser_left_eval(ctx,&left,0);
        lp_parser_match(lp_nexttoken,LPT_EQ);
        lp_parse_eval_value right;
        lp_parser_root_expr(ctx,&right,0);
        if(!lp_parser_raw_typechk(left, right))
        {
            LP_ERR("Unmatched type.", *root_t);
        }
        lp_parser_gen_assigncode(ctx,&left,&right);
        lpdebug("Assign a var;\n");
        
    }
    else if(root_t->ttype == LPT_NUMBER)
    {
        lp_lexer_back(ctx);
        lp_parse_eval_value left;
        lp_parser_root_expr(ctx,&left,0);
        lp_parser_print_val(&left);
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