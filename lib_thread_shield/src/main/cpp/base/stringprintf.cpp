//
// Created by EdisonLi on 2024/7/16.
//

#include "stringprintf.h"
#include <string>

namespace base {
    // copy from android base.
    void StringAppendV(std::string* dst, const char* format, va_list ap) {
        // First try with a small fixed size buffer
        char space[1024] __attribute__((__uninitialized__));

        // It's possible for methods that use a va_list to invalidate
        // the data in it upon use.  The fix is to make a copy
        // of the structure before using it and use that copy instead.
        va_list backup_ap;
        va_copy(backup_ap, ap);
        int result = vsnprintf(space, sizeof(space), format, backup_ap);
        va_end(backup_ap);

        if (result < static_cast<int>(sizeof(space))) {
            if (result >= 0) {
                // Normal case -- everything fit.
                dst->append(space, result);
                return;
            }

            if (result < 0) {
                // Just an error.
                return;
            }
        }

        // Increase the buffer size to the size requested by vsnprintf,
        // plus one for the closing \0.
        int length = result + 1;
        char* buf = new char[length];

        // Restore the va_list before we use it again
        va_copy(backup_ap, ap);
        result = vsnprintf(buf, length, format, backup_ap);
        va_end(backup_ap);

        if (result >= 0 && result < length) {
            // It fit
            dst->append(buf, result);
        }
        delete[] buf;
    }

    std::string StringPrintf(const char* fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        std::string result;
        StringAppendV(&result, fmt, ap);
        va_end(ap);
        return result;
    }
}