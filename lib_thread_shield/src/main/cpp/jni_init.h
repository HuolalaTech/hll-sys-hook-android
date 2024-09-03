//
// Created by EdisonLi on 2024/5/20.
//

#ifndef LLM_MOBILE_UNIFY_HOOK_LIB_JNI_INIT_H
#define LLM_MOBILE_UNIFY_HOOK_LIB_JNI_INIT_H

#include <jni.h>

JavaVM* getGlobalJVM();

JNIEnv *getJNIEnv();

void cleanup();

#endif //LLM_MOBILE_UNIFY_HOOK_LIB_JNI_INIT_H
