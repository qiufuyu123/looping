#ifndef _H_LPTYPES
#define _H_LPTYPES

#include<stdint.h>
#include<stddef.h>

#define _LP_DEBUG

typedef void* pointer_t;

typedef enum
{
    LP_OK = 0,
    LP_NULL = -1,
    LP_OOM = -2
}LP_Err;
#define LP_ERR_MAX 3
#define lpprintf printf
#define LPDINFO "[INFO]"
#define LPEOL ".\n"

#ifdef _LP_DEBUG
#define lpdebug lpprintf
#else
#define lpdebug
#endif
#define lpinfo(str) lpdebug(LPDINFO str LPEOL)

#define _LP_BIT_SZ sizeof(char*)

#if __SIZEOF_POINTER__==8
#define lpsize uint64_t
typedef uint64_t lpvmptr;
#else
#define lpsize uint32_t
typedef uint32_t lpvmptr;
#endif

#define lpptrsize lpsize

#define lp2ptr(x) ((lpptrsize)(x))

typedef uint16_t lpvmflg;

#endif