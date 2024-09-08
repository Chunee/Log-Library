#include <new>
#include <cassert>

#include "turn_sequencer.h"

template <typename T, template <typename> class Atom>
struct SingleElementQueue;

template <typename T>
class MPMCQueueBase;

template <typename T, template <typename> class Atom = std::atomic, bool Dynamic = false>
class MPMCQueue : public MPMCQueueBase<MPMCQueue<T, Atom, Dynamic>> {
    using Slot = SingleElementQueue<T, Atom>;

public:
    explicit MPMCQueue(size_t queueCapacity) : MPMCQueueBase<MPMCQueue<T, Atom, Dynamic>>(queueCapacity) {
        this->stride_ = this->computeStride(queueCapacity);
        this->slots_ = new Slot[queueCapacity + 2 * this->kSlotPadding];
    }

    MPMCQueue() noexcept {}
};

template <template <typename T, template <typename> class Atom, bool Dynamic> class Derived, 
            typename T, template <typename> class Atom, bool Dynamic>
class MPMCQueueBase<Derived<T, Atom, Dynamic>> {
    static_assert(std::is_nothrow_constructible<T, T&&>::value);

public:
    typedef T value_type;

    using Slot = SingleElementQueue<T, Atom>;

    explicit MPMCQueueBase(size_t queueCapacity) 
        : capacity_(queueCapacity), 
          pushTicket_(0),
          popTicket_(0),
          pushSpinCutoff_(0),
          popSpinCutoff_(0) 
    {
        if (queueCapacity == 0) {
            throw std::invalid_argument("MPMCQueue with explicit capacity 0 is impossible");
        }

        assert(alignof(MPMCQueue<T, Atom>) >= hardware_destructive_interference_size);
        assert(static_cast<uint8_t*>(static_cast<void*>(&popTicket_)) - 
                static_cast<uint8_t*>(static_cast<void*>(&pushTicket_)) >= 
                static_cast<ptrdiff_t>(hardware_destructive_interference_size));
    }

    MPMCQueueBase(const MPMCQueueBase&) = delete;
    MPMCQueueBase& operator=(const MPMCQueueBase&) = delete;

    ~MPMCQueueBase() { delete[] slots_; }

    ssize_t size() const noexcept {
        uint64_t pushes = pushTicket_.load(std::memory_order_acquire);
        uint64_t pops = popTicket_.load(std::memory_order_acquire);
        while (true) {
            uint64_t nextPushes = pushTicket_.load(std::memory_order_acquire);
            if (pushes == nextPushes) {
                return ssize_t(pushes - pops);
            } 
            pushes = nextPushes;
            uint64_t nextPops = popTicket_.load(std::memory_order_acquire);
            if (pops == nextPops) {
                return ssize_t(pushes - pops);
            }
            pops = nextPops;
        }
    }

    bool isEmpty() const noexcept { return size() <= 0; }

    bool isFull() const noexcept { return size() >= static_cast<ssize_t>(capacity_); }

    ssize_t sizeGuess() const noexcept { return writeCount() - readCount(); }

    size_t capacity() const noexcept { return capacity_; }

    uint64_t writeCount() const noexcept {
        return pushTicket_.load(std::memory_order_acquire);
    }

    uint64_t readCount() const noexcept {
        return popTicket_.load(std::memory_order_acquire);
    }

    template <typename... Args>
    void blockingWrite(Args&&... args) noexcept {
        enqueueWithTicketBase(pushTicket_++, slots_, capacity_, stride_, std::forward<Args>(args)...);
    }

    template <typename... Args>
    bool write(Args&&... args) noexcept {
        uint64_t ticket;
        Slot* slots;
        size_t cap;
        int stride;
        if (static_cast<Derived<T, Atom, Dynamic>*>(this)->tryObtainReadyPushTicket(ticket, slots, cap, stride)) {
            enqueueWithTicketBase(ticket, slots, cap, stride, std::forward<Args>(args)...);
            return true;
        } else {
            return false;
        }
    }

    template <class Clock, typename... Args>
    bool tryWriteUntil(const std::chrono::time_point<Clock>& when, Args&&... args) noexcept {
        uint64_t ticket;
        Slot* slots;
        size_t cap;
        int stride;
        if (tryObtainPromisedPushTicketUntil(ticket, slots, cap, stride, when)) {
            enqueueWithTicketBase(ticket, slots, cap, stride, std::forward<Args>(args)...);
            return true;
        } else {
            return false;
        }
    }

    template <typename... Args>
    bool writeIfNotFull(Args&&... args) noexcept {
        uint64_t ticket;
        Slot* slots;
        size_t cap;
        int stride;
        if (static_cast<Derived<T, Atom, Dynamic>*>(this)->tryObtainPromisedPushTicket(ticket, slots, cap, stride)) {
            enqueueWithTicketBase(ticket, slots, cap, stride, std::forward<Args>(args)...);
            return true;
        } else {
            return false;
        }
    }

    void blockingRead(T& elem) {
        uint64_t ticket;
        static_cast<Derived<T, Atom, Dynamic>*>(this)->blockingReadWithTicket(ticket, elem);
    }

    void blockingReadWithTicket(uint64_t& ticket, T& elem) noexcept {
        assert(capacity_ != 0);
        ticket = popTicket_++;
        dequeueWithTicketBase(ticket, slots_, capacity_, stride_, elem);
    }

    bool read(T& elem) noexcept {
        uint64_t ticket;
        return readAndGetTicket(ticket, elem);
    }

    bool readAndGetTicket(uint64_t& ticket, T& elem) noexcept {
        Slot* slots;
        size_t cap;
        int stride;
        if (static_cast<Derived<T, Atom, Dynamic>*>(this)->tryObtainReadyPopTicket(ticket, slots, cap, stride)) {
            dequeueWithTicketBase(ticket, slots, cap, stride, elem); 
            return true;
        } else {
            return false;
        }
    }

    template <class Clock, typename... Args>
    bool tryReadUntil(const std::chrono::time_point<Clock>& when, T& elem) noexcept {
        uint64_t ticket;
        Slot* slots;
        size_t cap;
        int stride;
        if (tryObtainPromisedPopTicketUntil(ticket, slots, cap, stride, when)) {
            dequeueWithTicketBase(ticket, slots, cap, stride, elem);
            return true;
        } else {
            return false;
        }
    }

   bool readIfNotEmpty(T& elem) noexcept {
        uint64_t ticket;
        Slot* slots;
        size_t cap;
        int stride;
        if (static_cast<Derived<T, Atom, Dynamic>*>(this)->tryObtainReadyPopTicket(ticket, slots, cap, stride)) {
            dequeueWithTicketBase(ticket, slots, cap, stride, elem);
            return true;
        } else {
            return false;
        }
    }

protected:
    static constexpr std::size_t hardware_destructive_interference_size = 64;

    enum {
        kAdaptationFreq = 128,

        kSlotPadding = (hardware_destructive_interference_size - 1) / sizeof(Slot) + 1
    };

    alignas(hardware_destructive_interference_size) size_t capacity_;

    Slot* slots_;

    int stride_;

    alignas(hardware_destructive_interference_size) Atom<uint64_t> pushTicket_;

    alignas(hardware_destructive_interference_size) Atom<uint64_t> popTicket_;

    alignas(hardware_destructive_interference_size) Atom<uint32_t> pushSpinCutoff_;

    alignas(hardware_destructive_interference_size) Atom<uint32_t> popSpinCutoff_;

    char pad_[hardware_destructive_interference_size - sizeof(Atom<uint32_t>)];

    static int computeStride(size_t capacity) noexcept {
        static const int smallPrimes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23};

        int bestStride = 1;
        size_t bestSep = 1;

        for (int stride : smallPrimes) {
            if ((stride % capacity) == 0 || (capacity % stride) == 0) {
                continue;
            } 
            size_t sep = stride % capacity;
            sep = std::min(sep, capacity - sep);
            if (sep > bestSep) {
                bestStride = stride;
                bestSep = sep;
            }
        }
        return bestStride;
    }

    size_t idx(uint64_t ticket, size_t cap, int stride) noexcept {
        return ((ticket * stride) % cap) + kSlotPadding;
    }

    uint32_t turn(uint64_t ticket, size_t cap) noexcept {
        assert(cap != 0);
        return uint32_t(ticket / cap);
    }

    bool tryObtainReadyPushTicket(uint64_t& ticket, Slot*& slots, size_t& cap, int& stride) noexcept {
        ticket = pushTicket_.load(std::memory_order_acquire);
        slots = slots_;
        cap = capacity_;
        stride = stride_;
        while (true) {
            if (!slots[idx(ticket, cap, stride)].mayEnqueue(turn(ticket, cap))) {
                auto prev = ticket;
                ticket = pushTicket_.load(std::memory_order_acquire);
                if (prev == ticket) {
                    return false;
                }
            } else {
                if (pushTicket_.compare_exchange_strong(ticket, ticket + 1)) {
                    return true;
                }
            }
        }
    }

    template <class Clock>
    bool tryObtainPromisedPushTicketUntil(uint64_t& ticket, Slot*& slots, size_t& cap, int& stride, const std::chrono::time_point<Clock>& when) noexcept {
        bool deadlineReached = false;
        while (!deadlineReached) {
            if (tryObtainPromisedPushTicket(ticket, slots, cap, stride)) {
                return true;
            }

            deadlineReached = !slots[idx(ticket, cap, stride)].tryWaitForEnqueueTurnUntil(turn(ticket, cap), pushSpinCutoff_, 
            (ticket % kAdaptationFreq) == 0, when);
        }
        return false;
    }

    bool tryObtainPromisedPushTicket(uint64_t& ticket, Slot*& slots, size_t& cap, int& stride) noexcept {
        auto numPushes = pushTicket_.load(std::memory_order_acquire);
        slots = slots_;
        cap = capacity_;
        stride = stride_;
        while (true) {
            ticket = numPushes;
            const auto numPops = popTicket_.load(std::memory_order_acquire);
            const int64_t n = int64_t(numPushes - numPops);
            if (n >= static_cast<ssize_t>(capacity_)) {
                return false;
            }
            if (pushTicket_.compare_exchange_strong(numPushes, numPushes + 1)) {
                return true;
            }
        }
    }

    bool tryObtainReadyPopTicket(uint64_t& ticket, Slot*& slots, size_t& cap, int& stride) noexcept {
        ticket = popTicket_.load(std::memory_order_acquire);
        slots = slots_;
        cap = capacity_;
        stride = stride_;
        while (true) {
            if (!slots[idx(ticket, cap, stride)].mayDequeue(turn(ticket, cap))) {
                auto prev = ticket;
                ticket = popTicket_.load(std::memory_order_acquire);
                if (prev == ticket) {
                    return false;
                }
            } else {
                if (popTicket_.compare_exchange_strong(ticket, ticket + 1)) {
                    return true;
                }
            }
        }
    }

    template <class Clock>
    bool tryObtainPromisedPopTicketUntil(uint64_t& ticket, Slot*& slots, size_t& cap, int& stride, const std::chrono::time_point<Clock>& when) noexcept {
        bool deadlineReached = false;
        while (!deadlineReached) {
            if (static_cast<Derived<T, Atom, Dynamic>>(this)->tryObtainPromisedPopTicket(ticket, slots, cap, stride)) {
                return true;
            }

            deadlineReached = !slots[idx(ticket, cap, stride)].tryWaitForDequeueTurnUntil(turn(ticket, cap), pushSpinCutoff_, 
            (ticket % kAdaptationFreq) == 0, when);
        }
        return false;
    }

    bool tryObtainPromisedPopTicket(uint64_t& ticket, Slot*& slots, size_t& cap, int& stride) noexcept {
        auto numPops = popTicket_.load(std::memory_order_acquire);
        slots = slots_;
        cap = capacity_;
        stride = stride_;
        while (true) {
            ticket = numPops;
            const auto numPushes = pushTicket_.load(std::memory_order_acquire);
            if (numPops >= numPushes) {
                return false;
            }
            if (popTicket_.compare_exchange_strong(numPops, numPops + 1)) {
                return true;
            }
        }
    }

    template <typename... Args>
    void enqueueWithTicketBase(uint64_t ticket, Slot* slots, size_t cap, int stride, Args&&... args) noexcept {
        slots[idx(ticket, cap, stride)].enqueue(turn(ticket, cap), pushSpinCutoff_, (ticket % kAdaptationFreq) == 0, std::forward<Args>(args)...);
    }

    void dequeueWithTicketBase(uint64_t ticket, Slot* slots, size_t cap, int stride, T& elem) noexcept {
        assert(cap != 0);
        slots[idx(ticket, cap, stride)].dequeue(turn(ticket, cap), popSpinCutoff_, (ticket % kAdaptationFreq) == 0, elem);
    }
};

template <typename T, template <typename> class Atom>
struct SingleElementQueue {
    ~SingleElementQueue() noexcept {
        if ((sequencer_.uncompletedTurnLSB() & 1) == 1) {
            destroyContents();
        }
    }

    template <typename = typename std::enable_if<std::is_nothrow_constructible<T>::value || std::is_nothrow_constructible<T, T&&>::value>::type>
    void enqueue(const uint32_t turn, Atom<uint32_t>& spinCutoff, const bool updateSpinCutoff, T&& goner) noexcept {
        enqueueImpl(turn, spinCutoff, updateSpinCutoff, std::move(goner), typename std::conditional<std::is_nothrow_constructible<T, T&&>::value, 
                    ImplByMove, ImplByRelocation>::type());
    }

    template <class Clock>
    bool tryWaitForEnqueueTurnUntil(const uint32_t turn, Atom<uint32_t>& spinCutoff, const bool updateSpinCutoff, 
                                    const std::chrono::time_point<Clock>& when) noexcept {
        return sequencer_.tryWaitForTurn(turn * 2, spinCutoff, updateSpinCutoff, &when) != TurnSequencer<Atom>::TryWaitResult::TIMEDOUT;
    }

    bool mayEnqueue(const uint32_t turn) const noexcept {
        return sequencer_.isTurn(turn * 2);
    }

    void dequeue(uint32_t turn, Atom<uint32_t>& spinCutoff, const bool updateSpinCutoff, T& elem) noexcept {
        dequeueImpl(turn, spinCutoff, updateSpinCutoff, elem, typename std::conditional<std::is_trivially_copyable<T>::value, ImplByRelocation, ImplByMove>::type());
    }

    template <class Clock>
    bool tryWaitForDequeueTurnUntil(const uint32_t turn, Atom<uint32_t>& spinCutoff, const bool updateSpinCutoff, 
                                    const std::chrono::time_point<Clock>& when) noexcept {
        return sequencer_.tryWaitForTurn(turn * 2 + 1, spinCutoff, updateSpinCutoff, &when) != TurnSequencer<Atom>::TryWaitResult::TIMEDOUT;
    }

    bool mayDequeue(const uint32_t turn) const noexcept {
        return sequencer_.isTurn(turn * 2 + 1);
    }

private:
    std::aligned_storage_t<sizeof(T), alignof(T)>::type contents_;

    TurnSequencer<Atom> sequencer_;

    T* ptr() noexcept { return static_cast<T*>(static_cast<void*>(&contents_)); }

    void destroyContents() noexcept {
        try {
            ptr()->~T();
        } catch (...) {

        }
        if (true) { // kIsDebug
            memset(&contents_, 'Q', sizeof(T));
        }
    }

    struct ImplByRelocation {};
    struct ImplByMove {};

    void enqueueImpl(const uint32_t turn, Atom<uint32_t>& spinCutoff, const bool updateSpinCutoff, T&& goner, ImplByMove) noexcept {
        sequencer_.waitForTurn(turn * 2, spinCutoff, updateSpinCutoff);
        new (&contents_) T(std::move(goner));
        sequencer_.completeTurn(turn * 2);
    }

    void enqueueImpl(const uint32_t turn, Atom<uint32_t>& spinCutoff, const bool updateSpinCutoff, T&& goner, ImplByRelocation) noexcept {
        sequencer_.waitForTurn(turn * 2, spinCutoff, updateSpinCutoff);
        memcpy(static_cast<void*>(&contents_), static_cast<void const*>(&goner), sizeof(T));
        sequencer_.completeTurn(turn * 2);
        new (&goner) T();
    }

    void dequeueImpl(uint32_t turn, Atom<uint32_t>& spinCutoff, const bool updateSpinCutoff, T& elem, ImplByRelocation) noexcept {
        try {
            elem.~T();
        } catch (...) {

        }
        sequencer_.waitForTurn(turn * 2 + 1, spinCutoff, updateSpinCutoff);
        memcpy(static_cast<void*>(&elem), static_cast<void const*>(&contents_), sizeof(T));
        sequencer_.completeTurn(turn * 2 + 1);
    } 

    void dequeueImpl(uint32_t turn, Atom<uint32_t>& spinCutoff, const bool updateSpinCutoff, T& elem, ImplByMove) noexcept {
        sequencer_.waitForTurn(turn * 2 + 1, spinCutoff, updateSpinCutoff);
        elem = std::move(*ptr());
        destroyContents();
        sequencer_.completeTurn(turn * 2 + 1);
    } 
};
