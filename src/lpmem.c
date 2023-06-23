#include"lpmem.h"
#include"lperr.h"
#include <stddef.h>
#include<string.h>
#include<stdio.h>
void li_setup_mem(li_ctx_t *ctx, size_t buf_size, pointer_t buf)
{
    lpnull(ctx && buf_size && buf);
    ctx->mem_header=buf;
    ctx->memsz=buf_size;
    ctx->mem_header->prev=(char*)buf;
    ctx->mem_header->next=(char*)buf+buf_size;
    ctx->mem_end = ctx->mem_header + buf_size;
}   

static li_mem_block_hearder* li_split(li_mem_block_hearder *header,size_t sz)
{
    pointer_t nxt=header->next;
    header->next=(char*)header+sz;
    header->next->next=nxt;
    header->next->prev=header;
    return header;
}

static void li_merge(li_ctx_t *ctx,li_mem_block_hearder *head)
{
    size_t maxptr=(size_t)((char *)ctx->mem_header + ctx->memsz);
    if(head->next == maxptr)
        return;
    if(head->next->used == 0)
    {
        lpdebug("merge: %x %x\n",head,head->next);
        head->next=head->next->next;
        head->next->prev=head;
        
    }

}
#define li_msz(x) x+sizeof(li_mem_block_hearder)
pointer_t li_malloc(li_ctx_t *ctx,size_t size)
{
    li_mem_block_hearder *cur=ctx->mem_header;
    size_t maxptr=(size_t)((char *)ctx->mem_header + ctx->memsz);
    while (1)
    {
        if(cur == maxptr)
        {
            lppanic(LP_OOM);
            return 0;
        }
        if(!cur->used)
        {
            size_t req=li_msz(size);
            size_t dif=(char *)cur->next - (char *)cur->prev;
            cur->used=1;
            if(dif > req)
            {
                char *addr= (char *)li_split(cur,req)+sizeof(li_mem_block_hearder);
                memset(addr,0,size);
                return addr;
            }else if(dif == req)
            {
                char *addr = (char *)cur+sizeof(li_mem_block_hearder);
                memset(addr,0,size);
                return addr;
            }
            cur->used=0;
        }
        cur=cur->next;
    }
    
}


void li_free(li_ctx_t *ctx,pointer_t ptr)
{
    if(!ptr)
        return;
    li_mem_block_hearder *head=(char*)ptr-sizeof(li_mem_block_hearder);
    head->used=0;
    li_merge(ctx,head);
}
