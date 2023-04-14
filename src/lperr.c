#include "lperr.h"
#include<stdlib.h>
const char* err_strs[LP_ERR_MAX]={"OK","Passing through a NULL parameter!","Out Of Memory!"};
void lppanic(LP_Err e)
{
    int idx=-((int)e);
    if(idx<LP_ERR_MAX+1 && idx>=0)
        lpprintf("Looping Panic, Fatal Errors (%d):\n%s\nexit.\n",idx,err_strs[idx]);
    exit(-idx);
}