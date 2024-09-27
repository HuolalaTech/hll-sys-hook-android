//
// Created by EdisonLi on 8/24/24.
//

#include <jni.h>
#include <android/log.h>
#include <android/api-level.h>
#include "cn_huolala_threadshield_suspend_thread_safe_helper.h"
#include "suspend_thread_timeout_apex_upgrade_logic_checker.h"
#include "suspend_thread_timeout_v15.h"
#include "suspend_thread_timeout_v12_14.h"
#include "suspend_thread_timeout_v5_11.h"

extern "C" JNIEXPORT void JNICALL
Java_cn_huolala_threadshield_SuspendThreadSafeHelper_nativeSuspendThreadSafe(JNIEnv *env,
                                                                             jobject /*this*/,
                                                                             jobject callback) {
    int apiLevel = android_get_device_api_level();
    if (apiLevel >= __ANDROID_API_V__) {
        // android [15 , +∞)
        using namespace suspend_thread_safe_v15;
        suspendThreadSafe(env, callback);
    } else if (apiLevel > __ANDROID_API_R__) {
        using namespace suspend_thread_timeout_apex_upgrade_logic_checker;
        if (checkApexUpgradeToAndroid15Logic()) {
            using namespace suspend_thread_safe_v15;
            suspendThreadSafe(env, callback);
        } else {
            // android [12 , 14]
            using namespace suspend_thread_safe_v12_14;
            suspendThreadSafe(env, callback);
        }
    } else {
        // android [5 , 11]
        using namespace suspend_thread_safe_v5_11;
        suspendThreadSafe(env, callback);
    }
}