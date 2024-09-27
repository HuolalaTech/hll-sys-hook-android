//
// Created by EdisonLi on 9/27/24.
//

#include <shadowhook.h>
#include "suspend_thread_timeout_apex_upgrade_logic_checker.h"

#define TARGET_ART_LIB "libart.so"
#define SYMBOL_SUSPEND_THREAD "_ZN3art10ThreadList19SuspendThreadByPeerEP8_jobjectNS_13SuspendReasonE"
#define SYMBOL_ABORT_IN_THIS "_ZN3art6Thread11AbortInThisERKNSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE"

namespace suspend_thread_timeout_apex_upgrade_logic_checker {

    // According to Android 15 new suspend thread logic.
    bool checkApexUpgradeToAndroid15Logic() {
        void *handle = shadowhook_dlopen(TARGET_ART_LIB);
        void *threadSuspendHandle = shadowhook_dlsym(handle, SYMBOL_SUSPEND_THREAD);
        void *abortInThisHandle = shadowhook_dlsym(handle, SYMBOL_ABORT_IN_THIS);
        bool result = threadSuspendHandle != nullptr && abortInThisHandle != nullptr;
        shadowhook_dlclose(threadSuspendHandle);
        shadowhook_dlclose(abortInThisHandle);
        return result;
    }
}