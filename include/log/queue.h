#include <memory>
#include <cstring>

namespace logging {
	template<typename T, typename Alloc = std::allocator<T>>
	class Queue : private Alloc {
	public: 
		using value_type = T;
		using allocator_traits = std::allocator_traits<Alloc>;
		using size_type = typename allocator_traits::size_type;

		Queue(Alloc const& alloc = Alloc{})
			: Alloc{alloc}
			, capacity_{0}
			, ring_{allocator_traits::allocate(*this, capacity_)}
		{}

		explicit Queue(size_type capacity, Alloc const& alloc = Alloc{})
			: Alloc{alloc}
			, capacity_{capacity}
			, ring_{allocator_traits::allocate(*this, capacity)}
		{}

		~Queue() {
			allocator_traits::deallocate(*this, ring_, capacity_);
		}
		
		auto size() const noexcept {
			return pushCursor_ - popCursor_;
		}

		auto empty() const noexcept {
			return size() == 0;
		}

		auto full() const noexcept {
			return size() == capacity();
		}

		auto capacity() const noexcept {
			return capacity_;
		}

		void push(T* value, size_type len) {
			if (full(pushCursor_, popCursor_)) {
				return;
			}

			auto div_pushCursor  = pushCursor_ % capacity_;
			std::size_t len_to_add = capacity_ - div_pushCursor;
			if (len > len_to_add) {
				memcpy(element(pushCursor_), value, len_to_add);
				memcpy(element(0), value + len_to_add, len - len_to_add);
			} else {
				memcpy(element(pushCursor_), value, len);
			}

			pushCursor_ += len;
			return;
		}

		void pop(T* value) {
			if (empty(pushCursor_, popCursor_)) {
				return;
			}

			if (pushCursor_ < capacity_) {
				memcpy(value, element(popCursor_), pushCursor_ - popCursor_);
				memset(element(popCursor_), '0', pushCursor_ - popCursor_);
				popCursor_ += (pushCursor_ - popCursor_);
			} else {
				auto real_push = (popCursor_ > capacity_) ? pushCursor_ % capacity_ : pushCursor_;
				auto real_pop = popCursor_ % capacity_;
               			std::size_t len_to_add = (real_push > real_pop) ? real_push - real_pop : (capacity_ - real_pop + real_push);

				if (real_pop + len_to_add > capacity_) {
					memcpy(value, element(popCursor_), capacity_ - real_pop);
					memcpy(value + (capacity_ - real_pop), element(popCursor_ + capacity_ - (real_pop)), len_to_add - (capacity_ - (real_pop)));
					memset(element(popCursor_), '0', capacity_ - real_pop);
					memset(element(popCursor_ + capacity_ - real_pop), '0', len_to_add - (capacity_ - real_pop));
				} else {
					memcpy(value, element(popCursor_), len_to_add);
					memset(element(popCursor_), '0', len_to_add);
				}
				
				popCursor_ += len_to_add;
			}
			return;
		}
		
		T* flush() {
			T* pop_ptr = new T[size()];
			pop(pop_ptr);
			return pop_ptr;
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

		size_type pushCursor_{0};
		size_type popCursor_{0};
	};
}
					

				
