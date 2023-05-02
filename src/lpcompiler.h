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
    LPT_KW_LET,
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
enum
{
    LPBT_INT
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
    union
    {
        lp_lex_token *words;
        char *builtin_name;
    };
    union 
    {
        lpsize occupy_bytes;
        lpsize offset_bytes;
    };
}lp_parse_type;

typedef struct 
{
    lp_parse_type root_type;
    lp_vm_array inner_types;
}lp_parse_structed_type;

enum{
    LPCT_NULL,
    LPCT_INT,
    LPCT_STRING,
    LPCT_GENERED=0xff
};
typedef struct 
{
    lp_parse_structed_type *type;
    union 
    {
        lpvmvalue v_number;
        lpvmvalue v_stackoffset;
        lpvmptr v_addr;
    };
    char const_type;
}lp_parse_eval_value;


typedef struct
{
    lp_lex_token *words;
    union
    {
        lpvmvalue stack_offset;
        lpvmptr raw_pointer;
    };
    lp_parse_structed_type *type;
    lpsize field;
}lp_parse_symbol;


typedef struct 
{
    lp_lex_code_buf code_buf;
    lp_vm_array glo_symbol_table;
    lp_vm_array symbol_table;
    lp_vm_array token_table;
    lp_vm_array type_table;
    lp_vm_ctx *vm;
    lpsize stack_offset;
    lpsize func_field;

    lpbool (* input_callback)(char ** newcode, lpsize *sz);
    lpbool interpret_mode;
    lp_lex_token *cur_token;
}lp_compiler;


void lp_compiler_init(lp_compiler *ctx, lp_vm_ctx*vm_ctx, char *codebuf, lpsize bufsize);

lp_lex_token *lp_new_token(lp_compiler *ctx, lptoken type, lpptrsize data);

LP_Err lp_compiler_do(lp_compiler *ctx);

LP_Err lp_lexer_do(lp_compiler *ctx);

LP_Err lp_compiler_codegen(lp_compiler *ctx);

lp_lex_token *lp_lexer_next(lp_compiler *ctx,lpbool skip);

void lp_lexer_back(lp_compiler *ctx);

char *lp_copy_str(char *str,lpsize len);
#endif