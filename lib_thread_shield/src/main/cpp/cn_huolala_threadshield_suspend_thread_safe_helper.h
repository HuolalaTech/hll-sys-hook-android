//
// Created by EdisonLi on 8/24/24.
//

#ifndef HLL_UNIFY_HOOK_LIB_CN_HUOLALA_THREADSHIELD_SUSPEND_THREAD_SAFE_HELPER_H
#define HLL_UNIFY_HOOK_LIB_CN_HUOLALA_THREADSHIELD_SUSPEND_THREAD_SAFE_HELPER_H


#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

extern "C" JNIEXPORT void JNICALL
Java_cn_huolala_threadshield_SuspendThreadSafeHelper_nativeSuspendThreadSafe(JNIEnv *env,
                                                                             jobject /*this*/,
                                                                             jobject callback);

#ifdef __cplusplus
}
#endif

#endif //HLL_UNIFY_HOOK_LIB_CN_HUOLALA_THREADSHIELD_SUSPEND_THREAD_SAFE_HELPER_H
