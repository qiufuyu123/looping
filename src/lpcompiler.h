#ifndef _H_LEX
#define _H_LEX

#include"lpvm.h"

typedef uint8_t lptoken;

enum
{
    LPT_NULL,
    LPT_NUMBER,
    LPT_STRING,
    LPT_FLOAT,
    LPT_WORDS,
    LPT_KW_IF,
    LPT_KW_ELSE,
    LPT_KW_WHILE,
    LPT_KW_FOR,
    LPT_KW_FN,
    LPT_KW_RET,
    LPT_KW_INT,
    LPT_KW_STRING,
    LPT_EXCLAMATION, // !
    LPT_DOUBLEQUOTE, // "
    LPT_SHARP,   // #
    LPT_DOLLAR,  // $
    LPT_PERCENT, // %
    LPT_ADDRESS, // &
    LPT_SINGLEQUOTE, // '
    LPT_LEFT_PAREN, // (
    LPT_RIGHT_PAREN, // )
    LPT_MUL,
    LPT_ADD,
    LPT_COMMA,
    LPT_MINUS,
    LPT_DOT,
    LPT_DIV,
    LPT_SLASH,
    LPT_COLON, //:
    LPT_SEMICOLON, //;
    LPT_LESS,
    LPT_EQ,
    LPT_GREAT,
    LPT_QUESTION,
    LPT_AT,
    LPT_LEFT_BRACKET,
    LPT_ANTI_SLASH,
    LPT_RIGHT_BRACKET,
    LPT_POWER,
    LPT_LEFT_CURLY,
    LPT_RIGHT_CURLY
};

typedef struct
{
    lptoken ttype;
    lpsize col, row;
    union
    {
        lpvmvalue v_int;
        char *v_str;
    };
    
}lp_lex_token;

typedef struct 
{
    char *codes;
    lpsize max_size;
    lpsize cur_pos;
    lpsize col,row;
}lp_lex_code_buf;

typedef struct 
{
    union name
    {
        char *words;
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
    char *words;
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
    lp_vm_array symbol_table;
    lp_vm_array token_table;
    lp_vm_array type_table;
    lp_vm_ctx *vm;
}lp_compiler;


void lp_compiler_init(lp_compiler *ctx, lp_vm_ctx*vm_ctx, char *codebuf, lpsize bufsize);

lp_lex_token *lp_new_token(lp_compiler *ctx, lptoken type, lpptrsize data);

LP_Err lp_compiler_do(lp_compiler *ctx);

LP_Err lp_lexer_do(lp_compiler *ctx);

LP_Err lp_compiler_codegen(lp_compiler *ctx);

lp_lex_token lp_lexer_next(lp_compiler *ctx);

#endif