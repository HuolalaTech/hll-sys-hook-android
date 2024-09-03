//
// Created by EdisonLi on 2024/7/16.
//
#include <string>

#ifndef LLM_MOBILE_UNIFY_HOOK_LIB_STRINGPRINTF_H
#define LLM_MOBILE_UNIFY_HOOK_LIB_STRINGPRINTF_H


namespace base {
    void StringAppendV(std::string* dst, const char* format, va_list ap);
}

#endif //LLM_MOBILE_UNIFY_HOOK_LIB_STRINGPRINTF_H
