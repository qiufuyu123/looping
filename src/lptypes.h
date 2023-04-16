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
    LP_OOM = -2,
    LP_STACK_OVER_FLOW = -3,
    LP_STACK_UNDER_FLOW = -4,
    LP_ILLEGAL_POINTER = -5,
    LP_BAD_INSTR_FORMAT =-6
}LP_Err;
#define LP_ERR_MAX 3
#define lpprintf printf
#define LPDINFO "[INFO]"
#define LPDERRO "[ERRO]"
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

#else
#define lpsize uint32_t

#endif

#define lpptrsize lpsize

typedef lpptrsize lpvmptr;
typedef uint32_t lpvmvalue;
typedef uint8_t lpvmbyte;
typedef uint8_t lpopcode;
typedef uint8_t lpbool;
#define lp2ptr(x) ((lpptrsize)(x))

typedef uint16_t lpvmflg;

#endif