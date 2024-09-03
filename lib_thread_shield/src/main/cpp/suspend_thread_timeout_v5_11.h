//
// Created by EdisonLi on 8/26/24.
//

#ifndef HLL_UNIFY_HOOK_LIB_SUSPEND_THREAD_TIMEOUT_V5_11_H
#define HLL_UNIFY_HOOK_LIB_SUSPEND_THREAD_TIMEOUT_V5_11_H

#include <jni.h>

namespace suspend_thread_safe_v5_11 {
    void suspendThreadSafe(JNIEnv *env, jobject jCallback);
}

#endif //HLL_UNIFY_HOOK_LIB_SUSPEND_THREAD_TIMEOUT_V5_11_H
