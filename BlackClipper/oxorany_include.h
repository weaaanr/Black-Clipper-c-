#ifndef OXORANY_INCLUDE_H
#define OXORANY_INCLUDE_H

#define OXORANY_DISABLE_OBFUSCATION
#define OXORANY_USE_BIT_CAST

#include "oxorany.h"

#define WRAPPER_MARCO oxorany
#define WRAPPER_MARCO_FLT oxorany_flt

template <typename T>
static OXORANY_FORCEINLINE void copy_string_without_return(T* target, const T* source)
{
	while (*source) {
		*target = *source;
		++source;
		++target;
	}
	*target = 0;
}

#define WRAPPER_MARCO_DEFINE_STRING_ARRAY(type,name,s) \
	type name[sizeof(s)/sizeof(type)]; \
	copy_string_without_return(name, WRAPPER_MARCO(s))

// Обфускация строк
#define HIDE_STR(str) []() { \
    constexpr auto encrypted = encrypt_string<sizeof(str)>(str); \
    return decrypt_string(encrypted); \
}()

// Запутывание потока выполнения
#define OBFUSCATE_FLOW() \
    if(rand() % 2) { \
        volatile int x = 1; \
        x += rand(); \
    }

// Антиотладка
void check_debugger() {
    if(IsDebuggerPresent()) {
        ExitProcess(0);
    }
}

#endif