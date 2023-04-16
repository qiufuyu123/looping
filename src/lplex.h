#ifndef _H_LEX
#define _H_LEX

#include"lpvm.h"

typedef uint8_t lptoken;

typedef struct
{
    char *str;
    lpsize len;
}lp_lex_words;

typedef struct
{
    lptoken ttype;
    lpsize col, row;
    union tvalue
    {
        lpvmvalue v_int;
        lp_lex_words v_str;
    };
    
}lp_lex_token;

typedef struct 
{
    char *codes;
    lpsize max_size;
    lpsize cur_pos;
}lp_lex_code_buf;

typedef struct 
{
    union name
    {
        lp_lex_words words;
        char builtin_name[16];
    };
    union 
    {
        lpsize occupy_bytes;
        lpsize offset_bytes;
    };
}lp_lex_type;

typedef struct 
{
    lp_lex_type root_type;
    lp_vm_array inner_types;
}lp_lex_structed_type;


typedef struct
{
    lp_lex_words *words;
    union data
    {
        lpvmvalue stack_offset;
        lpvmptr raw_pointer;
    };
    lp_lex_structed_type *type;
}lp_lex_symbol;


typedef struct 
{
    lp_lex_code_buf code_buf;
    lp_stack_ctx bins;
    lp_vm_array symbol_table;
    lp_vm_array type_table;
}lp_lexer;


#endif