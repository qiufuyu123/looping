#include"lpcompiler.h"
#include"lperr.h"
#include "lptypes.h"
#include "lpvm.h"
#include <stdint.h>
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
            .builtin_name = "char",
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

void lp_new_eval_val(lp_parse_eval_value *r,lp_parse_structed_type *type, lpvmptr value,lpbool isvar)
{
    //lp_parse_eval_value *r = lpmalloc(sizeof(lp_parse_eval_value));
    lpnull(r);
    r->type = type;
    r->v_addr = value;
    r->is_var = isvar;
    r->array_length = 1;
    r->is_loaded = 0;
    r->ptr_depth = 0;
    r->is_deptred = 0;
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
        ctx->stack_offset += (type->root_type.occupy_bytes+3)&~3; // 4bytes aligned
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

void lp_parser_gen_szcvt(lp_compiler *ctx, uint8_t from)
{
    int c = 0;
    if(from == 1)
        c = 0x000000ff;
    else if(from ==2)
        c = 0x0000ffff;
    else
        return;
    lp_parser_push_op(ctx, LOP_LOADc);
    lp_bin_pushval(ctx->vm, c);
    lp_parser_push_op(ctx, LOP_AND);
}

void lp_parser_gen_loadcode(lp_compiler *ctx,lp_parse_eval_value *v)
{
    lpvmvalue len = v->array_length * v->type->root_type.occupy_bytes;
    if(v->is_loaded)
        return;
    if(!v->is_var)
    {
        lp_parser_push_op(ctx,LOP_LOADc);
        lp_bin_pushval(ctx->vm,v->v_number);
        lpdebug("[OP]: PUSH 0x%x;\n",v->v_number);
    }else
    {
        lpptrsize data = v->v_stackoffset;
        lpdebug("Load:0x%x %d;\n",data,len);
        lp_parser_push_op(ctx,ctx->func_field?LOP_LOAD_STACK:LOP_GET_GLO);
        //lp_bin_pushval(ctx->vm,len);
        lp_bin_pushval(ctx->vm,data);
        lp_parser_gen_szcvt(ctx, v->type->root_type.occupy_bytes);
            //ctx->stack_offset += len;
        //TODO: Multibytes load
    }
    v->is_loaded = 1;
    
}

void lp_parser_gen_binop_loadcode(lp_compiler *ctx, lp_parse_eval_value *left, lp_parse_eval_value *right)
{
    if(left->is_var || right->is_var)
    {
        lp_parser_gen_loadcode(ctx, left);
        lp_parser_gen_loadcode(ctx, right);
    }
}

void lp_parser_gen_assigncode(lp_compiler *ctx, lp_parse_eval_value *left, lp_parse_eval_value *right,lpbool isptr)
{
    lpdebug("Assign :%d --> %d\n",ctx->stack_offset,left->v_stackoffset);
    if(!right->is_loaded)
    {
        lp_parser_gen_loadcode(ctx,right);
    }
    if(isptr)
    {
        lp_parser_push_op(ctx, LOP_MEMSET);
    }else {
        lp_parser_gen_szcvt(ctx, left->ptr_depth?4:left->type->root_type.occupy_bytes);
        lp_parser_push_op(ctx,ctx->func_field?LOP_POP_TO:LOP_SET_GLO);
        //lp_bin_pushval(ctx->vm,right->type->root_type.occupy_bytes);
        lp_bin_pushval(ctx->vm,left->v_stackoffset);
    }
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
int lp_parse_ptrdepth(lp_compiler *ctx)
{
    int r=0;
    while (lp_nexttoken->ttype == LPT_MUL) {
        r++;
    }
    return r;
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

lpbool lp_parser_raw_typechk(lp_parse_eval_value left,lp_parse_eval_value right)
{
    lpvmvalue lsz=0,rsz=0;
    if(left.type->root_type.occupy_bytes<=4 && right.type->root_type.occupy_bytes<=4)
        return 1; // TODO : More specific check 
    if(left.ptr_depth)
        lsz = 4;
    else
        lsz = left.type->root_type.occupy_bytes * left.array_length;
    if(right.ptr_depth)
        rsz = 4;
    else
        rsz = right.type->root_type.occupy_bytes * right.array_length;
    lpdebug("[PARSER] Raw type check:left:%d,right:%d;\n",lsz,rsz);
    return lsz == rsz;
}

int lp_parser_expression(lp_compiler*ctx,lp_parse_eval_value* r, int level,lpbool gencode)
{
    // unit-op always has the highest priority.
    //lp_lex_token *tk = lp_nexttoken;
    //lp_parse_eval_value left;
    lp_parse_eval_value right;
    lpbool isptr = 0;
    //lp_parser_left_eval(ctx, r, &isptr);
    lptoken t = ctx->cur_token->ttype;
    lp_lex_token *tk = ctx->cur_token;
    if(t == LPT_NUMBER)
    {
        lp_new_eval_val(r, &lp_builtin_types[LPBT_INT], tk->v_int,0);
        //lp_parser_gen_loadcode(ctx, r);
    }else if(t == LPT_STRING)
    {
        lp_parser_push_op(ctx, LOP_LRES);
        int l = strlen(tk->v_str)+1;
        lp_bin_pushval(ctx->vm, l);
        char *s = ctx->vm->opcodes.codes_end;
        lp_vm_op_push(ctx->vm, tk->v_str, l);
        lp_new_eval_val(r, &lp_builtin_types[LPBT_CHAR], (lpvmptr)(s-ctx->vm->opcodes.codes),0);
        r->ptr_depth = 1;
    }
    else if(t == LPT_ADD)
    {
        lp_nexttoken;
        lp_parser_expression(ctx, r, level,gencode);
    }else if(t == LPT_MINUS)
    {
        lp_nexttoken;
        lp_parser_expression(ctx, r, level,gencode);
        if(!r->is_var)
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
    }
    else if(t == LPT_EXCLAMATION)
    {
        lp_nexttoken;
        lp_parser_expression(ctx, r, level,gencode);
        if(!r->is_var)
            r->v_number = !r->v_number;
        else
        {
            lpvmvalue tmp = 0;
            lp_parser_push_op(ctx,LOP_LOADc);
            lp_bin_pushval(ctx->vm,tmp);
            lpdebug("[OP] Logic NOT;\n");
            lp_parser_gen_loadcode(ctx,r);
            lp_parser_push_op(ctx,LOP_NOT);
        }
    }
    else if(t == LPT_ADDRESS)
    {
        lp_nexttoken;
        lp_parser_expression(ctx, r, level,0);
        if(!r->is_var || r->is_var == 2)
        {
            LP_ERR("Cannot get address of a constant!",*ctx->cur_token);
        }
        lp_parser_push_op(ctx, LOP_GETPTR);
        lp_vm_op_push(ctx->vm, &r->v_stackoffset, 4);
        r->ptr_depth++;
        r->is_loaded = 1;
    }else if(t == LPT_MUL)
    {
        lp_nexttoken;
        lp_parser_expression(ctx, r, LPP_LOW, 1);
        if(r->is_var)
        {
            if(!r->ptr_depth)
            {
                LP_ERR("Cannot resolve a variable as a pointer", *ctx->cur_token);
            }else {
                r->ptr_depth --;
            }
        }
        lp_parser_push_op(ctx, LOP_DEPTR);
        isptr=1;
    }
    else if(t == LPT_LEFT_PAREN)
    {
        lp_parse_structed_type *tp = 0;
        int ptr_depth = 0;
        lp_lex_token *nxt = lp_forwardtoken;
        if(nxt->ttype == LPT_WORDS)
        {
            // perhaps type-converter?
            tp = lp_parser_get_type(ctx, nxt);
            if(tp)
            {
                lp_nexttoken;
                ptr_depth = lp_parse_ptrdepth(ctx);
                lp_parser_match(ctx->cur_token, LPT_RIGHT_PAREN);
            }
        }  
        lp_nexttoken;
        lp_parser_expression(ctx, r, LPP_ASSIGN, gencode);
        if(tp)
        {
            //lp_parser_gen_szcvt(ctx, uint8_t from)

            // TODO: real convert in IL
            r->type = tp;
            r->array_length = 1;
            r->ptr_depth = ptr_depth;
        }else {
            lp_parser_match(lp_nexttoken, LPT_RIGHT_PAREN);
        }
    }else if(t == LPT_WORDS){
        lp_parse_symbol *sym= lp_parser_lookup_var(ctx, ctx->cur_token);
        if(!sym)
        {
            LP_ERR("Unknown variable name!", *ctx->cur_token);
        }
        lp_new_eval_val(r, sym->type, sym->stack_offset, 1);
        r->array_length = sym->array_length;
        r->ptr_depth = sym->ptr_depth;
        if(gencode)
            lp_parser_gen_loadcode(ctx, r);
    }
    int prio = 0,rprio=0;
    while (level <= (prio=lp_parser_priority(*lp_nexttoken))) {

        t = ctx->cur_token->ttype;
        if(prio == LPP_ASSIGN)
        {
            lp_nexttoken;
            if(isptr)
                ctx->vm->opcodes.codes_end--;
            lp_parser_expression(ctx, &right, LPP_ASSIGN, gencode);
            if(!lp_parser_raw_typechk(*r,right))
            {
                LP_ERR("Unmatched type", *ctx->cur_token);
            }
            lp_parser_gen_assigncode(ctx, r, &right, isptr);
        }
        else {
            lp_nexttoken;
            lp_parser_expression(ctx, &right, prio+1, 1);
            lp_parser_gen_binop_loadcode(ctx, r, &right);
            if(r->is_var)
                r->is_var = 2;    
            if(right.is_var)
            {
                r->is_var = 2;
                // '2' means it is a by-product while evaluating
                r->is_loaded = 1;
                // Notice here:
                // Since we set is_loaded to true, we do not need
                // to care about the stack_offset.
            }
            if(prio == LPP_BIT_SHIFT)
            {
                if(!r->is_var)
                {
                    if(t == LPT_LSL)
                        r->v_number = r->v_number << right.v_number;
                    else 
                        r->v_number = r->v_number >> right.v_number;
                }else {
                    lp_parser_push_op(ctx, t == LPT_LSL? LOP_LSL:LOP_LSR);
                }
            }
            else if(prio == LPP_ADD)
            {
                if(!r->is_var)
                {
                    if(t == LPT_ADD)
                        r->v_number = r->v_number + right.v_number;
                    else
                        r->v_number = r->v_number - right.v_number;
                }else {
                    lp_parser_push_op(ctx, t == LPT_ADD ? LOP_ADD : LOP_MINUS);
                }
            }else if(prio == LPP_MUL)
            {
                if(!r->is_var)
                {
                    if(t == LPT_MUL)
                        r->v_number = r->v_number * right.v_number;
                    else
                        r->v_number = r->v_number / right.v_number;
                }else {
                    lp_parser_push_op(ctx, t == LPT_MUL ? LOP_MUL : LOP_DIV);
                }
            }
            else if(prio == LPP_COMP)
            {
                if(!r->is_var)
                {
                    if(t == LPT_LESS)
                        r->v_number = (r->v_number < right.v_number);
                    else if(t == LPT_GREAT)
                        r->v_number = (r->v_number > right.v_number);
                }else {
                    lp_parser_push_op(ctx, t == LPT_LESS?LOP_LESS:LOP_GRT);
                }
            }
            else if(prio == LPP_EQNEQ)
            {
                if(!r->is_var)
                {
                    if(t == LPT_EQEQ)
                        r->v_number = (r->v_number == right.v_number);
                    else 
                        r->v_number = (r->v_number != right.v_number);
                }else {
                    lp_parser_push_op(ctx, t == LPT_EQEQ?LOP_EQ:LOP_LG_NOTEQ);      
                }
            }
            else if(prio == LPP_BIT_AND)
            {
                if(!r->is_var)
                {
                    r->v_number = r->v_number & right.v_number;
                }else {
                    lp_parser_push_op(ctx, LOP_AND);
                }
            }
            else if(prio == LPP_BIT_XOR)
            {
                if(!r->is_var)
                {
                    r->v_number = r->v_number ^ right.v_number;
                }else {
                    lp_parser_push_op(ctx, LOP_XOR);
                }
            }
            else if(prio == LPP_BIT_OR)
            {
                if(!r->is_var)
                {
                    r->v_number = r->v_number | right.v_number;
                }else {
                    lp_parser_push_op(ctx, LOP_OR);
                }
            }
            else if(prio == LPP_LOGIC_AND){
                if(!r->is_var)
                {
                    r->v_number = r->v_number && right.v_number;
                }else {
                    lp_parser_push_op(ctx, LOP_LG_AND);
                }
            }else if(prio == LPP_LOGIC_OR){
                if(!r->is_var)
                {
                    r->v_number = r->v_number || right.v_number;
                }else {
                    lp_parser_push_op(ctx, LOP_LG_OR);
                }
            }
            else{
                LP_ERR("Unknonw binary op", *ctx->cur_token);
            }
            //r->is_loaded = 1;
        }
        rprio = prio;
    }
    lp_lexer_back(ctx);
    return rprio;
}

void lp_parser_kw_paren_eval(lp_compiler *ctx, lp_parse_eval_value *v)
{
    lp_parser_match(lp_nexttoken, LPT_LEFT_PAREN);
    lp_nexttoken;
    lp_parser_expression(ctx, v, LPP_ASSIGN, 1);
    lp_parser_gen_loadcode(ctx, v);
    lp_parser_match(lp_nexttoken, LPT_RIGHT_PAREN);
}

LP_Err lp_parser_statment(lp_compiler *ctx);

void lp_parser_kw_curly(lp_compiler *ctx)
{
    lp_parser_match(lp_nexttoken, LPT_LEFT_CURLY);
    lpbool old = ctx->interpret_mode;
    ctx->interpret_mode = 0;
    while (lp_forwardtoken->ttype != LPT_RIGHT_CURLY) {
        lp_parser_statment(ctx);
    }
    ctx->interpret_mode = old;
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
                lp_nexttoken;
                lp_parser_expression(ctx,&right, LPP_ASSIGN,1);
                lp_parser_gen_loadcode(ctx, &right);
                if(!right.is_var)
                {
                    if(ptr_depth)
                    {
                        
                        //LP_ERR("Cannot assign a constant to a pointer!", *left);
                    }
                    sym = lp_parser_add_variable(ctx,left,type,0);
                    // Type Check
                    //
                    sym->array_length = right.array_length;
                }else {
                    sym = lp_parser_add_variable(ctx,left,0,right.type);
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
    else if(root_t->ttype == LPT_KW_WHILE)
    {
        lp_parse_eval_value v;
        lpvmvalue loops = ctx->vm->opcodes.codes_end - ctx->vm->opcodes.codes;
        lp_parser_kw_paren_eval(ctx, &v);
        lp_parser_push_op(ctx, LOP_TEST);
        lp_parser_push_op(ctx, LOP_JNE);
        lpvmvalue *jneaddr = ctx->vm->opcodes.codes_end;
        ctx->vm->opcodes.codes_end += 4; // prepare for code injection
        lp_parser_kw_curly(ctx);
        lp_nexttoken;
        lp_parser_push_op(ctx, LOP_J);
        lp_bin_pushval(ctx->vm, loops);
        *jneaddr = (ctx->vm->opcodes.codes_end - ctx->vm->opcodes.codes);
    }
    else if(root_t->ttype == LPT_KW_IF)
    {
        lp_parse_eval_value v;
        lp_parser_kw_paren_eval(ctx, &v);
        lp_parser_push_op(ctx, LOP_TEST);
        lp_parser_push_op(ctx, LOP_JNE);
        lpvmvalue *elsep = ctx->vm->opcodes.codes_end;
        ctx->vm->opcodes.codes_end += 4; // prepare for code injection
        lp_parser_kw_curly(ctx);
        *elsep = (ctx->vm->opcodes.codes_end - ctx->vm->opcodes.codes);
        lp_nexttoken;
        if(lp_forwardtoken->ttype == LPT_KW_ELSE)
        {
            lp_nexttoken;
            lp_parser_match(lp_nexttoken, LPT_LEFT_CURLY);
            *elsep += 5;
            lp_parser_push_op(ctx, LOP_J);
            elsep = ctx->vm->opcodes.codes_end;
            ctx->vm->opcodes.codes_end += 4;
            ctx->interpret_mode = 0;
            while (lp_forwardtoken->ttype != LPT_RIGHT_CURLY) {
                lp_parser_statment(ctx);
            }
            ctx->interpret_mode = 1;
            *elsep = (ctx->vm->opcodes.codes_end - ctx->vm->opcodes.codes);
            lp_nexttoken;
            
        }
        lp_parser_push_op(ctx, LOP_NOP);
        //lp_nexttoken;

    }
    else {
        lp_parse_eval_value left;
        if(lp_parser_expression(ctx, &left, LPP_ASSIGN,0)!=LPP_ASSIGN)
        {
            LP_ERR("Unknown statement, expect a expression", *root_t);
        }
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