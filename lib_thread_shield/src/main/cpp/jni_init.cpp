//
// Created by EdisonLi on 8/24/24.
//


#include "jni_init.h"

JavaVM *g_jvm;

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    g_jvm = vm;
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    g_jvm = nullptr;
}

JavaVM *getGlobalJVM() {
    return g_jvm;
}

thread_local bool tlsAttached = false;

JNIEnv *getJNIEnv() {
    JNIEnv *env = nullptr;
    if (getGlobalJVM() == nullptr) {
        return nullptr;
    }
    jint result = getGlobalJVM()->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        if (getGlobalJVM()->AttachCurrentThread(&env, nullptr) != 0) {
            return nullptr;
        }
        tlsAttached = true;
    } else if (result != JNI_OK) {
        return nullptr;
    }
    return env;
}

void cleanup() {
    if (tlsAttached) {
        if (getGlobalJVM()->DetachCurrentThread() == JNI_OK) {
            tlsAttached = false;
        }
    }
}
