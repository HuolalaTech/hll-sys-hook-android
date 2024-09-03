//
// Created by EdisonLi on 8/24/24.
//

#include <jni.h>
#include <shadowhook.h>
#include <string>
#include <android/log.h>
#include "art/art_thread.h"
#include "suspend_thread_timeout_v15.h"
#include "base/stringprintf.h"
#include "jni_init.h"

#define TARGET_BASE_LIB_64 "/apex/com.android.art/lib64/libbase.so"
#define TARGET_BASE_LIB_32 "/apex/com.android.art/lib/libbase.so"
#define SYMBOL_HOOK_POINT_STRING_PRINTF "_ZN7android4base12StringPrintfEPKcz"
#define LOG_TAG "suspend_thread_safe_v15"

#if defined(__aarch64__) || defined(__x86_64__) || defined(__mips64)
const char *libName = TARGET_BASE_LIB_64;
#else
const char* libName = TARGET_BASE_LIB_32;
#endif

namespace suspend_thread_safe_v15 {
    namespace {
        jobject callbackObj = nullptr;
        void *stubFunction = nullptr;
        void *originalFunction = nullptr;

        typedef std::string (*StringPrintf_t)(const char *format, ...);

        bool checkFormat(const char *format) {
            return
                    strstr(format, "timed out") != nullptr &&
                    strstr(format, "state&flags") != nullptr &&
                    strstr(format, "priority") != nullptr &&
                    strstr(format, "barriers") != nullptr &&
                    strstr(format, "ours") != nullptr &&
                    strstr(format, "barrier value") != nullptr &&
                    strstr(format, "nsusps") != nullptr &&
                    strstr(format, "ncheckpts") != nullptr &&
                    strstr(format, "thread_info") != nullptr;
        }

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
            jobject currentThread =pEnv->CallStaticObjectMethod(clsThread, midCurrentThread);
            jmethodID midGetClassLoader = pEnv->GetMethodID(clsThread,
                                                            "getContextClassLoader",
                                                            "()Ljava/lang/ClassLoader;");
            if (midGetClassLoader == nullptr) {
                return;
            }
            jobject classLoader =pEnv->CallObjectMethod(currentThread, midGetClassLoader);
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
            jstring className = pEnv->NewStringUTF("cn/huolala/threadshield/SuspendThreadSafeHelper$SuspendThreadCallback");
            auto jThreadHookClass =(jclass) pEnv->CallObjectMethod(classLoader,
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

        std::string proxyStringPrintfFunc(const char *format, ...) {
            if (checkFormat(format)) {
                va_list args;
                va_start(args, format);
                const char *func_name = va_arg(args, const char*);  // func_name
                va_arg(args, int);          // tid
                va_arg(args, const char*);  // name.c_str()
                va_arg(args, int);          // state_and_flags
                va_arg(args, int);          // native_priority
                va_arg(args, void*);        // first_barrier
                using namespace kbArt;
                WrappedSuspend1Barrier *wrappedBarrier = va_arg(args, WrappedSuspend1Barrier*);
                if (wrappedBarrier != nullptr) {
                    if (wrappedBarrier->barrier_.load(std::memory_order_acquire) == 0) {
                        return ((StringPrintf_t) originalFunction)("thread has been suspend : %s",
                                                                   func_name);
                    }
                    // record start time.
                    struct timespec startTime{};
                    clock_gettime(CLOCK_MONOTONIC, &startTime);

                    struct timespec ts{};
                    ts.tv_sec = 0;
                    ts.tv_nsec = 10000000; // 10ms (1ms = 1,000,000ns)

                    while (true) {
                        if (wrappedBarrier->barrier_.load(std::memory_order_acquire) == 0) {
                            struct timespec endTime{};
                            clock_gettime(CLOCK_MONOTONIC, &endTime);
                            long secDuration = endTime.tv_sec - startTime.tv_sec;
                            long nsecDuration = endTime.tv_nsec - startTime.tv_nsec;
                            if (nsecDuration < 0) {
                                --secDuration;
                                nsecDuration += 1e9; // 1s = 1,000,000,000 ns
                            }
                            const double waitDuration =
                                    static_cast<double>(secDuration + nsecDuration) / 1e9;
                            triggerSuspendTimeout(std::round(waitDuration * 1000) / 1000);
                            return ((StringPrintf_t) originalFunction)(
                                    "thread has been suspend : %s, cost time %f", func_name,
                                    waitDuration);
                        }
                        // release cpu resource.
                        nanosleep(&ts, nullptr);
                    }
                }
                va_end(args);
            }

            va_list ap;
            va_start(ap, format);
            std::string result;
            base::StringAppendV(&result, format, ap);
            va_end(ap);
            return result;
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
            stubFunction = shadowhook_hook_sym_name(libName,
                                                    SYMBOL_HOOK_POINT_STRING_PRINTF,
                                                    (void *) proxyStringPrintfFunc,
                                                    (void **) &originalFunction);
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

