#include "log/futex.h"

namespace detail {
    FutexResult nativeFutexWaitImpl(const void* addr, uint32_t expected, std::chrono::system_clock::time_point const* absSystemTime, 
                                    std::chrono::steady_clock::time_point const* absSteadyTime, uint32_t waitMask) {
        assert(absSystemTime == nullptr || absSteadyTime == nullptr);

        int op = FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG;
        struct timespec ts;
        struct timespec* timeout = nullptr;

        if (absSystemTime != nullptr) {
            op |= FUTEX_CLOCK_REALTIME;
            ts = timeSpecFromTimePoint(*absSystemTime);
            timeout = &ts;
        } else if (absSteadyTime != nullptr) {
            ts = timeSpecFromTimePoint(*absSystemTime);
            timeout = &ts;
        }

        int rv = syscall(
            __NR_futex,
            addr,
            op,
            expected,
            timeout,
            nullptr,
            waitMask);

        if (rv == 0) {
            return FutexResult::AWOKEN;
        } else {
            switch (errno) {
                case ETIMEDOUT:
                    assert(timeout != nullptr);
                    return FutexResult::TIMEDOUT;
                case EINTR:
                    return FutexResult::INTERRUPTED;
                case EWOULDBLOCK:
                    return FutexResult::VALUE_CHANGED;
                default:
                    assert(false);
                    return FutexResult::VALUE_CHANGED;
            }
        }
    }

    int nativeFutexWakeImpl(const void* addr, int count, uint32_t wakeMask) {
        int rv = syscall(
            __NR_futex,
            addr,
            FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG,
            count,
            nullptr,
            nullptr,
            wakeMask);
        
        if (rv < 0) {
            return 0;
        }
        return rv;
    }
}