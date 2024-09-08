#pragma once
#include <atomic>
#include <limits>

#include "futex.h"

template <template <typename> class Atom> 
struct TurnSequencer {
    explicit TurnSequencer(const uint32_t firstTurn = 0) noexcept : state_(encode(firstTurn << kTurnShift, 0)) {}

    bool isTurn(const uint32_t turn) const noexcept {
        auto state = state_.load(std::memory_order_acquire);
        return decodeCurrentSturn(state) == (turn << kTurnShift);
    }

    enum class TryWaitResult { SUCCESS, PAST, TIMEDOUT };

    void waitForTurn(const uint32_t turn, Atom<uint32_t>& spinCutoff, const bool updateSpinCutoff) noexcept {
        const auto ret = tryWaitForTurn(turn, spinCutoff, updateSpinCutoff);
    }

    template <class Clock = std::chrono::steady_clock, class Duration = typename Clock::duration>
    TryWaitResult tryWaitForTurn(const uint32_t turn, Atom<uint32_t>& spinCutoff, 
                                 const bool updateSpinCutoff, const std::chrono::time_point<Clock, Duration>* absTime = nullptr) noexcept {
        uint32_t prevThresh = spinCutoff.load(std::memory_order_relaxed);
        const uint32_t effectiveSpinCutoff = updateSpinCutoff || prevThresh == 0 ? kMaxSpinLimit : prevThresh;

        uint64_t begin = 0;
        uint32_t tries;
        const uint32_t sturn = turn << kTurnShift;

        for (tries = 0;; ++tries) {
            uint32_t state = state_.load(std::memory_order_acquire);
            uint32_t current_sturn = decodeCurrentSturn(state); 
            if (current_sturn == sturn) {
                break;
            }

            if (sturn - current_sturn >= std::numeric_limits<uint32_t>::max() / 2) {
                return TryWaitResult::PAST;
            }

            if (kSpinUsingHardwareClock) {
                auto now = __builtin_ia32_rdtsc();
                if (tries == 0) {
                    begin = now;
                }
                if (tries == 0 || now < begin + effectiveSpinCutoff) {
                    asm volatile("pause");
                    continue;
                }
            } else {
                if (tries < effectiveSpinCutoff) {
                    asm volatile("pause");
                    continue;
                }
            }

            uint32_t current_max_waiter_delta = decodeMaxWaitersDelta(state);
            uint32_t our_waiter_delta = (sturn - current_sturn) >> kTurnShift;
            uint32_t new_state;
            if (our_waiter_delta <= current_max_waiter_delta) {
                new_state = state;
            } else {
                new_state = encode(current_sturn, our_waiter_delta);
                if (state != new_state && !state_.compare_exchange_strong(state, new_state)) {
                    continue;
                }
            }
            if (absTime) {
                auto futexResult = detail::futexWaitUntil(&state_, new_state, *absTime, futexChannel(turn));
                if (futexResult == FutexResult::TIMEDOUT) {
                    return TryWaitResult::TIMEDOUT;
                }
            } else {
                detail::futexWait(&state_, new_state, futexChannel(turn));
            }
        } 
        
        if (updateSpinCutoff || prevThresh == 0) {
            uint32_t target;
            uint64_t elapsed = !kSpinUsingHardwareClock || tries == 0 ? tries : __builtin_ia32_rdtsc() - begin;
            if (tries >= kMaxSpinLimit) {
                target = kMinSpinLimit;
            } else {
                target = std::min(uint32_t{kMaxSpinLimit}, std::max(uint32_t{kMinSpinLimit}, static_cast<uint32_t>(elapsed * 2)));
            }

            if (prevThresh == 0) {
                spinCutoff.store(target);
            } else {
                spinCutoff.compare_exchange_weak(prevThresh, prevThresh + int(target - prevThresh) / 8);
            }
        }

        return TryWaitResult::SUCCESS;
    }

    void completeTurn(const uint32_t turn) noexcept {
        uint32_t state = state_.load(std::memory_order_acquire);
        while (true) {
            assert(state == encode(turn << kTurnShift, decodeMaxWaitersDelta(state)));
            uint32_t max_waiter_delta = decodeMaxWaitersDelta(state);
            uint32_t new_state = encode((turn + 1) << kTurnShift, max_waiter_delta == 0 ? 0 : max_waiter_delta - 1);

            if (state_.compare_exchange_strong(state, new_state)) {
                if (max_waiter_delta != 0) {
                    detail::futexWake(&state_, std::numeric_limits<int>::max(), futexChannel(turn + 1));
                }
                break;
            }
        }
    }

    uint8_t uncompletedTurnLSB() const noexcept {
        return uint8_t(state_.load(std::memory_order_acquire) >> kTurnShift);
    }

private:
    static constexpr bool kSpinUsingHardwareClock = 1;
    static constexpr uint32_t kCyclesPerSpinLimit = kSpinUsingHardwareClock ? 1 : 10;

    static constexpr uint32_t kTurnShift = 6;
    static constexpr uint32_t kWaitersMask = (1 << kTurnShift) - 1;

    static constexpr uint32_t kMinSpinLimit = 200 / kCyclesPerSpinLimit;

    static constexpr uint32_t kMaxSpinLimit = 20000 / kCyclesPerSpinLimit;

    std::atomic<std::uint32_t> state_;

private:
    uint32_t futexChannel(uint32_t turn) const noexcept {
        return 1u << (turn & 31);
    }

    uint32_t decodeCurrentSturn(uint32_t state) const noexcept {
        return state & ~kWaitersMask;
    }

    uint32_t decodeMaxWaitersDelta(uint32_t state) const noexcept {
        return state & kWaitersMask;
    }

    uint32_t encode(uint32_t currentSturn, uint32_t maxWaiterD) const noexcept {
        return currentSturn | std::min(uint32_t{kWaitersMask}, maxWaiterD);
    }
};