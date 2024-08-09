#include <linux/futex.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <type_traits>
#include <cassert>

#ifndef FUTEX_WAIT_BITSET
#define FUTEX_WAIT_BITSET 9
#endif

#ifndef FUTEX_PRIVATE_FLAG
#define FUTEX_PRIVATE_FLAG 128
#endif

enum class FutexResult {
    VALUE_CHANGED,
    AWOKEN,
    INTERRUPTED,
    TIMEDOUT,
};

namespace detail {
    template <template <typename> class Atom = std::atomic>
    using Futex = Atom<std::uint32_t>;

    FutexResult nativeFutexWaitImpl(const void* addr, uint32_t expected, std::chrono::system_clock::time_point const* absSystemTime,
                                    std::chrono::steady_clock::time_point const* absSteadyTime, uint32_t waitMask);

    int nativeFutexWakeImpl(const void* addr, int count, uint32_t wakeMask);

    template <class Clock>
    timespec timeSpecFromTimePoint(std::chrono::time_point<Clock> absTime) {
        auto epoch = absTime.time_since_epoch();
        if (epoch.count() < 0) {
            epoch = Clock::duration::zero();
        }

        using time_t_seconds = std::chrono::duration<std::time_t, std::chrono::seconds::period>;
        using long_nanos = std::chrono::duration<long int, std::chrono::nanoseconds::period>;

        auto secs = std::chrono::duration_cast<time_t_seconds>(epoch);
        auto nanos = std::chrono::duration_cast<long_nanos>(epoch - secs);
        struct timespec result = {secs.count(), nanos.count()};
        return result;
    }

    template <typename Futex>
    FutexResult futexWait(const Futex* futex, uint32_t expected, uint32_t waitMask = -1) {
        auto rv = nativeFutexWaitImpl(futex, expected, nullptr, nullptr, waitMask);
        assert(rv != FutexResult::TIMEDOUT);
        return rv;
    }

    template <typename Futex, typename Deadline>
    FutexResult futexWaitImpl(Futex* futex, uint32_t expected, Deadline const& deadline, uint32_t waitMask) {
        if constexpr (Deadline::clock::is_steady) {
            return nativeFutexWaitImpl(futex, expected, nullptr, &deadline, waitMask);
        } else {
            return nativeFutexWaitImpl(futex, expected, &deadline, nullptr, waitMask);
        }
    }

    template <typename Futex, class Clock, class Duration>
    FutexResult futexWaitUntil(const Futex* futex, uint32_t expected, std::chrono::time_point<Clock, Duration> const& deadline, uint32_t waitMask = -1) {
        using Target = typename std::conditional<Clock::is_steady, std::chrono::steady_clock, std::chrono::system_clock>::type;

        auto const converted = time_point_conv<Target>(deadline);
        return converted == Target::time_point::max() ? nativeFutexWaitImpl(futex, expected, nullptr, nullptr, waitMask) : futexWaitImpl(futex, expected, converted, waitMask);
    }

    template <typename Futex>
    int futexWake(const Futex* futex, int count = std::numeric_limits<int>::max(), uint32_t wakeMask = -1) {
        return nativeFutexWakeImpl(futex, count, wakeMask);
    }

    template <typename TargetClock, typename Clock, typename Duration>
    typename TargetClock::time_point time_point_conv(std::chrono::time_point<Clock, Duration> const& time) {
        using std::chrono::duration_cast;
        using TimePoint = std::chrono::time_point<Clock, Duration>;
        using TargetDuration = typename TargetClock::duration;
        using TargetTimePoint = typename TargetClock::time_point;
        if (time == TimePoint::max()) {
            return TargetTimePoint::max();
        } else {
            if constexpr (std::is_same_v<Clock, TargetClock>) {
                auto const delta = time.time_since_epoch();
                return TargetTimePoint(duration_cast<TargetDuration>(delta));
            } else {
                auto const delta = time - Clock::now();
                return TargetClock::now() + duration_cast<TargetDuration>(delta);
            }
        }
    }
}