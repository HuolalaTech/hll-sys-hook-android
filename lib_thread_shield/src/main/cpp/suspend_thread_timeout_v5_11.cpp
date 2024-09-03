//
// Created by EdisonLi on 8/26/24.
//

#include "suspend_thread_timeout_v5_11.h"
#include "jni_init.h"
#include <jni.h>
#include <android/log.h>
#include <cstring>
#include <shadowhook.h>

#define TARGET_ART_LIB "libart.so"
#define SYMBOL_THREAD_SUSPEND_BY_PEER_WARNING_14 "_ZN3artL26ThreadSuspendByPeerWarningERNS_18ScopedObjectAccessEN7android4base11LogSeverityEPKcP8_jobject"
#define SYMBOL_THREAD_SUSPEND_BY_PEER_WARNING_8_11 "_ZN3artL26ThreadSuspendByPeerWarningEPNS_6ThreadEN7android4base11LogSeverityEPKcP8_jobject"
// #define SYMBOL_THREAD_SUSPEND_BY_PEER_WARNING_8_13
// "_ZN3artL26ThreadSuspendByPeerWarningERNS_18ScopedObjectAccessEN7android4base11LogSeverityEPKcP8_jobject"
#define SYMBOL_THREAD_SUSPEND_BY_PEER_WARNING_6_7 "_ZN3artL26ThreadSuspendByPeerWarningEPNS_6ThreadENS_11LogSeverityEPKcP8_jobject"
#define SYMBOL_THREAD_SUSPEND_BY_PEER_WARNING_5 "_ZN3artL26ThreadSuspendByPeerWarningEPNS_6ThreadEiPKcP8_jobject"
#define LOG_TAG "suspend_thread_safe_v5_11"
#define SUSPEND_LOG_MSG "Thread suspension timed out"

enum LogSeverity {
    VERBOSE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL_WITHOUT_ABORT,  // For loggability tests, this is considered identical to FATAL.
    FATAL,
};

namespace suspend_thread_safe_v5_11 {

    namespace {
        jobject callbackObj = nullptr;

        void *stubFunction = nullptr;
        void *originalFunction = nullptr;

        typedef void (*ThreadSuspendByPeerWarning)(void *self, LogSeverity severity,
                                                   const char *message, jobject peer);

        void triggerSuspendTimeout(double withConsumedTime) {
            JNIEnv *pEnv = getJNIEnv();
            if (pEnv == nullptr) {
                return;
            }
            jclass clsThread = pEnv->FindClass("java/lang/Thread");
            if (clsThread == nullptr) {
                return;
            }
            // java.lang.NoClassDefFoundError: Class not found using the boot class
            // loader; no stack trace available
            jmethodID midCurrentThread = pEnv->GetStaticMethodID(clsThread,
                                                                 "currentThread",
                                                                 "()Ljava/lang/Thread;");
            jobject currentThread = pEnv->CallStaticObjectMethod(clsThread, midCurrentThread);
            jmethodID midGetClassLoader = pEnv->GetMethodID(clsThread,
                                                            "getContextClassLoader",
                                                            "()Ljava/lang/ClassLoader;");
            if (midGetClassLoader == nullptr) {
                return;
            }
            jobject classLoader = pEnv->CallObjectMethod(currentThread, midGetClassLoader);
            if (classLoader == nullptr) {
                return;
            }
            jclass clsClassLoader = pEnv->FindClass("java/lang/ClassLoader");
            if (clsClassLoader == nullptr) {
                return;
            }
            jmethodID midLoadClass = pEnv->GetMethodID(clsClassLoader,
                                                       "loadClass",
                                                       "(Ljava/lang/String;)Ljava/lang/Class;");
            if (midLoadClass == nullptr) {
                return;
            }
            jstring className = pEnv->NewStringUTF(
                    "cn/huolala/threadshield/SuspendThreadSafeHelper$SuspendThreadCallback");
            auto jThreadHookClass = (jclass) pEnv->CallObjectMethod(classLoader,
                                                                    midLoadClass,
                                                                    className);
            if (jThreadHookClass == nullptr) {
                return;
            }
            jmethodID jMethodId = pEnv->GetMethodID(jThreadHookClass,
                                                    "suspendThreadTimeout",
                                                    "(D)V");
            if (jMethodId == nullptr) {
                return;
            }
            pEnv->CallVoidMethod(callbackObj, jMethodId, withConsumedTime);
            cleanup();
        }

        void threadSuspendByPeerWarning(void *self,
                                        LogSeverity severity,
                                        const char *message,
                                        jobject peer) {
            __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Hooked point success : %s", message);
            if (severity == FATAL && strcmp(message, SUSPEND_LOG_MSG) == 0) {
                // set low level log that can not send abort signal.
                severity = INFO;
                __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "hit target: %s", message);
                ((ThreadSuspendByPeerWarning) originalFunction)(self, severity, message, peer);
                triggerSuspendTimeout(0);
            }
        }

        const char *getThreadSuspendByPeerWarningFunctionName() {
            int apiLevel = android_get_device_api_level();
            // Simplified logic based on Android API levels
            if (apiLevel < __ANDROID_API_M__) {
                return SYMBOL_THREAD_SUSPEND_BY_PEER_WARNING_5;
            } else if (apiLevel < __ANDROID_API_O__) {
                // below android 8
                return SYMBOL_THREAD_SUSPEND_BY_PEER_WARNING_6_7;
            } else if (apiLevel <= __ANDROID_API_S__) {
                // android 12.0
                // here can not distinguish 12.0 and 12.1.
                // 12.1 need to use SYMBOL_THREAD_SUSPEND_BY_PEER_WARNING as symbol name.
                // but now android 12 13 14 not use this logic.
                // hook `SuspendThreadByPeer` to call `SuspendThreadByThreadId` as substitute.
                // 12.0/12.1 can not call by java side control.
                return SYMBOL_THREAD_SUSPEND_BY_PEER_WARNING_8_11;
            } else {
                // in fact, unreachable here.
                // android 14+
                // now [12 - 14] use SuspendThreadByThreadId to suspend thread.
                return SYMBOL_THREAD_SUSPEND_BY_PEER_WARNING_14;
            }
        }

        void hookPointFailed(const char *errMsg) {
            JNIEnv *pEnv = getJNIEnv();
            if (pEnv == nullptr || callbackObj == nullptr) {
                return;
            }
            jclass jThreadHookClass = pEnv->FindClass(
                    "cn/huolala/threadshield/SuspendThreadSafeHelper$SuspendThreadCallback");
            if (jThreadHookClass != nullptr) {
                jmethodID jMethodId = pEnv->GetMethodID(jThreadHookClass,
                                                        "onError",
                                                        "(Ljava/lang/String;)V");
                if (jMethodId != nullptr) {
                    jstring jErrMsg = pEnv->NewStringUTF(errMsg);
                    pEnv->CallVoidMethod(callbackObj, jMethodId, jErrMsg);
                    pEnv->DeleteLocalRef(jErrMsg);
                }
                pEnv->DeleteLocalRef(jThreadHookClass);
            }
            cleanup();
        }

        void initHookPoint() {
            if (stubFunction != nullptr) {
                shadowhook_unhook(stubFunction);
                stubFunction = nullptr;
            }
            stubFunction = shadowhook_hook_sym_name(
                    TARGET_ART_LIB,
                    getThreadSuspendByPeerWarningFunctionName(),
                    (void *) threadSuspendByPeerWarning,
                    (void **) &originalFunction
            );
            if (stubFunction == nullptr) {
                const int err_num = shadowhook_get_errno();
                const char *errMsg = shadowhook_to_errmsg(err_num);
                if (errMsg == nullptr) {
                    return;
                }
                __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Hook setup failed: %s", errMsg);
                hookPointFailed(errMsg);
            } else {
                __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Hook setup success");
            }
        }
    }

    void suspendThreadSafe(JNIEnv *env, jobject jCallback) {
        if (callbackObj != nullptr) {
            env->DeleteGlobalRef(callbackObj);
        }
        callbackObj = env->NewGlobalRef(jCallback);
        initHookPoint();
    }
}