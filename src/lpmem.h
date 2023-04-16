#ifndef _H_LPMEM
#define _H_LPMEM
#include"lptypes.h"
typedef struct li_mem_block
{
    struct li_mem_block *next;
    struct li_mem_block *prev;
    char used : 1;
}li_mem_block_hearder;
typedef struct 
{
    size_t memsz;
    li_mem_block_hearder *mem_header;
    char *mem_end;
}li_ctx_t;
void li_setup_mem(li_ctx_t *ctx, size_t buf_size, pointer_t buf);
pointer_t li_malloc(li_ctx_t *ctx,size_t size);
void li_free(li_ctx_t *ctx,pointer_t ptr);

typedef li_ctx_t lp_mem_ctx;

#define lp_msetup li_setup_mem
#define lp_malloc li_malloc
#define lp_free li_free

#endif