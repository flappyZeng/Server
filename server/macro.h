#pragma once

#include<string.h>
#include<assert.h>   //函数栈以及断言支持


#define SERVER_ASSERT(x) \
    if(SERVER_UNLICKLY(!(x))){  \
         SERVER_LOG_ERROR(SERVER_LOG_ROOT()) << "ASSERTION: "#x \
         << "\nbacktrack:\n" \
         << server::BackTrace(100, 2, "    "); \
         assert(x); \
    }

#define SERVER_ASSERT2(x, w) \
    if(SERVER_UNLICKLY(!(x))){  \
         SERVER_LOG_ERROR(SERVER_LOG_ROOT()) << "ASSERTION: "#x \
         << "\n " << w \
         << "\nbacktrack:\n" \
         << server::BackTrace(100, 2, "    "); \
         assert(x); \
    }

//优化编译器加速
#if defined __GNUC__ || defined __llvm__
#    define SERVER_LICKLY(x)       __builtin_expect(!!(x), 1) //编译器优化，条件大概率成立
#    define SERVER_UNLICKLY(x)     __builtin_expect(!!(x), 0)
#else
#    define SERVER_LICKLY(x)       (x)     
#    define SERVER_UNLICKLY(x)     (x)
#endif