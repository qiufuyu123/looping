#ifndef _H_LPERR
#define _H_LPERR

#include"lptypes.h"

void lppanic(LP_Err e);
#define lpnull(x) do{if(!(x)){lppanic(LP_NULL);}}while(0)
#endif