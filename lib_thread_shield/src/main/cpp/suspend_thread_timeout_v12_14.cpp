//
// Created by EdisonLi on 8/25/24.
//
#include <jni.h>
#include <shadowhook.h>
#include <android/log.h>
#include "jni_init.h"
#include "art/art_thread.h"

#define TARGET_ART_LIB "libart.so"
#define LOG_TAG "suspend_thread_safe_v12_14"

#define SYMBOL_SUSPEND_THREAD_BY_ID "_ZN3art10ThreadList23SuspendThreadByThreadIdEjNS_13SuspendReasonEPb"
// [12.1, 15)
#define SYMBOL_SUSPEND_THREAD_BY_PEER_NEW "_ZN3art10ThreadList19SuspendThreadByPeerEP8_jobjectNS_13SuspendReasonEPb"
// [9, 12.0]
#define SYMBOL_SUSPEND_THREAD_BY_PEER_OLD "_ZN3art10ThreadList19SuspendThreadByPeerEP8_jobjectbNS_13SuspendReasonEPb"

namespace suspend_thread_safe_v12_14 {

    enum class SuspendReason : char {
        // Suspending for internal reasons (e.g. GC, stack trace, etc.).
        kInternal,
        // Suspending due to non-runtime, user controlled, code. (For example Thread#Suspend()).
        kForUserCode,
    };

    namespace {
        jobject callbackObj = nullptr;
        jfieldID nativePeerFieldId = nullptr;

        void *suspendThreadByThreadId = nullptr;
        void *originalFunctionReplace = nullptr;

        void *stubFunction = nullptr;

        typedef void *(*SuspendThreadByThreadId_t)(void *thread_list,
                                                   uint32_t threadId,
                                                   SuspendReason suspendReason,
                                                   bool *timed_out);

        typedef void *(*SuspendThreadByPeer_t)(void *thread_list,
                                               jobject peer,
                                               SuspendReason suspendReason,
                                               bool *timed_out);

        typedef void *(*SuspendThreadByPeer_compatible_t)(void *thread_list,
                                                          jobject peer,
                                                          bool request_suspension,
                                                          SuspendReason suspendReason,
                                                          bool *timed_out);

        jlong getThreadIdByPeer(jobject peer) {
            JNIEnv *pEnv = getJNIEnv();
            jlong threadIdByPeer = (pEnv != nullptr && nativePeerFieldId != nullptr &&
                                    peer != nullptr)
                                   ? pEnv->GetLongField(peer, nativePeerFieldId)
                                   : 0;
            cleanup();
            return threadIdByPeer;
        }

        // Function to mask thread suspend operation by peer, replacing it with thread suspend by ID.
        void *replaceThreadSuspendFunc(void *thread_list, jobject peer,
                                       SuspendReason suspendReason,
                                       bool *timed_out) {
            jlong thread_id = getThreadIdByPeer(peer);
            auto *thread = reinterpret_cast<kbArt::Thread *>(thread_id);
            __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Replace function %p success", thread);
            if (thread == nullptr) {
                return ((SuspendThreadByPeer_t) originalFunctionReplace)(
                        thread_list,
                        peer,
                        suspendReason,
                        timed_out);
            }
            const int32_t threadId = thread->GetThreadId(android_get_device_api_level());
            if (threadId == UN_SUPPORT || threadId == NOT_FOUND) {
                return ((SuspendThreadByPeer_t) originalFunctionReplace)(
                        thread_list,
                        peer,
                        suspendReason,
                        timed_out);
            }
            return ((SuspendThreadByThreadId_t) suspendThreadByThreadId)(
                    thread_list,
                    threadId,
                    suspendReason,
                    timed_out);
        }

        void *replaceThreadSuspendCompatibleFunc(
                void *thread_list,
                jobject peer,
                bool request_suspension,
                SuspendReason suspendReason,
                bool *timed_out) {
            jlong thread_id = getThreadIdByPeer(peer);
            auto *thread = reinterpret_cast<kbArt::Thread *>(thread_id);
            __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Replace function %p success", thread);
            if (thread == nullptr) {
                return ((SuspendThreadByPeer_compatible_t) originalFunctionReplace)(
                        thread_list,
                        peer,
                        request_suspension,
                        suspendReason,
                        timed_out);
            }
            const int32_t threadId = thread->GetThreadId(android_get_device_api_level());
            if (threadId == UN_SUPPORT || threadId == NOT_FOUND) {
                return ((SuspendThreadByPeer_compatible_t) originalFunctionReplace)(
                        thread_list,
                        peer,
                        request_suspension,
                        suspendReason,
                        timed_out);
            }
            return ((SuspendThreadByThreadId_t) suspendThreadByThreadId)(
                    thread_list,
                    threadId,
                    suspendReason,
                    timed_out);
        }

        void hookPointFailed(const char *errMsg) {
            JNIEnv *pEnv = getJNIEnv();
            if (pEnv == nullptr) {
                return;
            }
            jclass jThreadHookClass = pEnv->FindClass(
                    "cn/huolala/threadshield/SuspendThreadSafeHelper$SuspendThreadCallback");
            if (jThreadHookClass != nullptr) {
                jmethodID jMethodId =
                        pEnv->GetMethodID(jThreadHookClass, "onError", "(Ljava/lang/String;)V");
                if (jMethodId != nullptr) {
                    jstring jErrMsg = pEnv->NewStringUTF(errMsg);
                    pEnv->CallVoidMethod(callbackObj, jMethodId, jErrMsg);
                    pEnv->DeleteLocalRef(jErrMsg);
                }
                pEnv->DeleteLocalRef(jThreadHookClass);
            }
            cleanup();
        }

        void handleHookSetupFailure(const void *stubFunc) {
            if (stubFunc == nullptr) {
                const int err_num = shadowhook_get_errno();
                const char *errMsg = shadowhook_to_errmsg(err_num);
                if (errMsg == nullptr || callbackObj == nullptr) {
                    return;
                }
                __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Hook setup failed: %s", errMsg);
                hookPointFailed(errMsg);
            } else {
                __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Hook setup success");
            }
        }

        void initHookPoint(JNIEnv *env, jobject jCallback) {
            if (callbackObj != nullptr) {
                env->DeleteGlobalRef(callbackObj);
            }
            callbackObj = env->NewGlobalRef(jCallback);
            jclass jThreadHookClass = env->FindClass("java/lang/Thread");

            void *handle = shadowhook_dlopen(TARGET_ART_LIB);
            suspendThreadByThreadId = shadowhook_dlsym(handle, SYMBOL_SUSPEND_THREAD_BY_ID);

            if (jThreadHookClass == nullptr || suspendThreadByThreadId == nullptr) {
                return;
            }

            // 'nativePeer' is a field in the Thread.class
            nativePeerFieldId = env->GetFieldID(jThreadHookClass, "nativePeer", "J");

            if (stubFunction != nullptr) {
                shadowhook_unhook(stubFunction);
                stubFunction = nullptr;
            }

            stubFunction = shadowhook_hook_sym_name(TARGET_ART_LIB,
                                                    SYMBOL_SUSPEND_THREAD_BY_PEER_NEW,
                                                    (void *) replaceThreadSuspendFunc,
                                                    (void **) &originalFunctionReplace);

            if (android_get_device_api_level() != __ANDROID_API_S__) {
                handleHookSetupFailure(stubFunction);
                return;
            }

            if (stubFunction == nullptr) {
                // because can not distinguish 12.0 with 12.1 by any native api.
                // retry with another symbol name.
                stubFunction = shadowhook_hook_sym_name(TARGET_ART_LIB,
                                                        SYMBOL_SUSPEND_THREAD_BY_PEER_OLD,
                                                        (void *) replaceThreadSuspendCompatibleFunc,
                                                        (void **) &originalFunctionReplace);
                handleHookSetupFailure(stubFunction);
            }
        }
    }

    void suspendThreadSafe(JNIEnv *env, jobject jCallback) {
        initHookPoint(env, jCallback);
    }
}