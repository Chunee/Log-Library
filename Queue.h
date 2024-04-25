#include <atomic>
#include <cassert>
#include <memory>
#include <new>
#include <cstring>

template<typename T, typename Alloc = std::allocator<T>>
class Queue : private Alloc {
public:
    using value_type = T;
    using allocator_traits = std::allocator_traits<Alloc>;
    using size_type = typename allocator_traits::size_type;

    explicit Queue(size_type capacity, Alloc const& alloc = Alloc{})
        : Alloc{alloc}
        , capacity_{capacity}
        , ring_{allocator_traits::allocate(*this, capacity)}
    {}

    ~Queue() {
        while(not empty()) {
            element(popCursor_)->~T();
            ++popCursor_;
        }
        allocator_traits::deallocate(*this, ring_, capacity_);
    }


    /// Returns the number of elements in the fifo
    auto size() const noexcept {
        auto pushCursor = pushCursor_.load(std::memory_order_relaxed);
        auto popCursor = popCursor_.load(std::memory_order_relaxed);

        assert(popCursor <= pushCursor);
        return pushCursor - popCursor;
    }

    /// Returns whether the container has no elements
    auto empty() const noexcept { return size() == 0; }

    /// Returns whether the container has capacity_() elements
    auto full() const noexcept { return size() == capacity(); }

    /// Returns the number of elements that can be held in the fifo
    auto capacity() const noexcept { return capacity_; }


    /// Push one object onto the fifo.
    /// @return `true` if the operation is successful; `false` if fifo is full.
    void push(T const& value) {
        auto pushCursor = pushCursor_.load(std::memory_order_relaxed);
        auto popCursor = popCursor_.load(std::memory_order_acquire);
        if (full(pushCursor, popCursor)) {
            return; 
        }
        new (element(pushCursor)) T(value);

        pushCursor_.store(pushCursor + 1, std::memory_order_release);
        return;
    }

    void push(T* value) {
	    auto pushCursor = pushCursor_.load(std::memory_order_relaxed);
        auto popCursor = popCursor_.load(std::memory_order_acquire);
        if (full(pushCursor, popCursor)) {
            return; 
        }

        std::size_t len = strlen(value) + 1; // Add 1 to include the null-terminator
        // if (len > capacity() - (pushCursor - popCursor)) {
        //     // Ensure that there's enough space in the buffer
        //     return;
        // }
	    // std::size_t len = strlen(value);

        std::size_t remaining_len = capacity_ - (pushCursor % capacity_);
        if (len > remaining_len) {
            memcpy(element(pushCursor), value, remaining_len);
            memcpy(element(0), value + remaining_len, len - remaining_len);
        } else {
            memcpy(element(pushCursor), value, len); 
        }

        pushCursor_.store(pushCursor + len, std::memory_order_release);
        return;
    }

    void pop(T* value) {
	    auto pushCursor = pushCursor_.load(std::memory_order_acquire);
        auto popCursor = popCursor_.load(std::memory_order_relaxed);
        if (empty(pushCursor, popCursor)) {
            return;
        }

        if (pushCursor < capacity_) {
            memcpy(value, element(popCursor), pushCursor % capacity_);
            memset(element(popCursor), '0', pushCursor % capacity_);
            popCursor_.store(popCursor + pushCursor, std::memory_order_release);
        } else {
            std::size_t remaining_len = capacity_ - (popCursor % capacity_);
            memcpy(value, element(popCursor), remaining_len);
            memcpy(value + remaining_len, element(popCursor + remaining_len), pushCursor % capacity_);
            memset(element(popCursor), '0', remaining_len);
            memset(element(popCursor + remaining_len), '0', pushCursor % capacity_);
            popCursor_.store(popCursor + (capacity_ - ((popCursor % capacity_) - (pushCursor % capacity_))));
        }

        return;
    }

    /// Pop one object from the fifo.
    /// @return `true` if the pop operation is successful; `false` if fifo is empty.
    void pop(T& value) {
        auto pushCursor = pushCursor_.load(std::memory_order_acquire);
        auto popCursor = popCursor_.load(std::memory_order_relaxed);
        if (empty(pushCursor, popCursor)) {
            return;
        }
        value = *element(popCursor);
        element(popCursor)->~T();
        popCursor_.store(popCursor + 1, std::memory_order_release);
        return;
    }

private:
    auto full(size_type pushCursor, size_type popCursor) const noexcept {
        return (pushCursor - popCursor) == capacity_;
    }

    static auto empty(size_type pushCursor, size_type popCursor) noexcept {
        return pushCursor == popCursor;
    }

    auto element(size_type cursor) noexcept {
        return &ring_[cursor % capacity_];
    }

private:
	size_type capacity_;
	T* ring_;
	
	using CursorType = std::atomic<size_type>; 
	static_assert(CursorType::is_always_lock_free);

	static constexpr auto hardware_destructive_interference_size = size_type{64};

	alignas(hardware_destructive_interference_size) CursorType pushCursor_{0};
	alignas(hardware_destructive_interference_size) CursorType popCursor_{0};

	char padding_[hardware_destructive_interference_size - sizeof(size_type)];
};


