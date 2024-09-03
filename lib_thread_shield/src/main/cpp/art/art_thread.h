//
// Created by knight-zxw on 2023/1/3.
// Email: nimdanoob@163.com
//

#ifndef KB_ART_THREAD_H_
#define KB_ART_THREAD_H_
#include <atomic>
#include "macro.h"
#include "tls.h"

#define UN_SUPPORT (-1)
#define NOT_FOUND (0)

namespace kbArt {
class Thread {
  struct PACKED(4) tls_32bit_sized_values {
    // We have no control over the size of 'bool', but want our boolean fields
    // to be 4-byte quantities.
    using bool32_t = uint32_t;

    // The state and flags field must be changed atomically so that flag values
    // aren't lost. See `StateAndFlags` for bit assignments of `ThreadFlag` and
    // `ThreadState` values. Keeping the state and flags together allows an
    // atomic CAS to change from being Suspended to Runnable without a suspend
    // request occurring.
    std::atomic<uint32_t> state_and_flags;
    // A non-zero value is used to tell the current thread to enter a safe point

    // at the next poll.
    int suspend_count;

    // Thin lock thread id. This is a small integer used by the thin lock
    // implementation. This is not to be confused with the native thread's tid,
    // nor is it the value returned by java.lang.Thread.getId --- this is a
    // distinct value, used only for locking. One important difference between
    // this id and the ids visible to managed code is that these ones get reused
    // (to ensure that they fit in the number of bits available).
    uint32_t thin_lock_thread_id;

    // System thread id.
    uint32_t tid;

  } tls32_;

 public:
  // Guaranteed to be non-zero.
  inline int32_t GetThreadId(int api_level) const {
    if (api_level < __ANDROID_API_S__ ||
        api_level > __ANDROID_API_U__) {  // < Android 12 || > android 14
      // now only support Android  12 13 14.
      return UN_SUPPORT;
    }
    uint32_t offset = 0;
    // calculate the offset of the field `thin_lock_thread_id` in the struct.
    offset = offsetof(tls_32bit_sized_values, thin_lock_thread_id);
    return *(int32_t *)((char *)this + offset);
  }

  inline uint32_t GetTid() { return tls32_.tid; }

  inline static Thread *current() {
    void *thread = __get_tls()[TLS_SLOT_ART_THREAD_SELF];
    return reinterpret_cast<Thread *>(thread);
  }
};


    // only for android 15+
    // See Thread.tlsPtr_.active_suspend1_barriers below for explanation.
    struct WrappedSuspend1Barrier {
        // TODO(b/23668816): At least weaken CHECKs to DCHECKs once the bug is fixed.
        static constexpr int kMagic = 0xba8;

        WrappedSuspend1Barrier() : magic_(kMagic), barrier_(1), next_(nullptr) {}

        int magic_;
        std::atomic<int32_t> barrier_;
        struct WrappedSuspend1Barrier *next_;
    };
}  // namespace kbArt

#endif  // KB_ART_THREAD_H_
