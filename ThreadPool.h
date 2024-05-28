#pragma once 

#include <functional>
#include <thread>
#include <semaphore>
#include <type_traits>
#include <concepts>
#include <deque>
#include "Queue.h"

template <typename FunctionType = std::function<void()>>
requires std::invocable<FunctionType> && std::is_same_v<void, std::invoke_result_t<FunctionType>>
class ThreadPool {
public:
    explicit ThreadPool(const int num_threads) : tasks_(num_threads) , priority_queue_(num_threads) {
        for (std::size_t i = 0; i < num_threads; ++i) {
            priority_queue_.push(i);

            threads_.emplace_back([id = i, this](const std::stop_token &stop_tok) {
                do {
                    tasks_[id].signal.acquire();

                    do {
                        auto temp_task = tasks_[id].tasks.front();

                        unassigned_tasks_.fetch_sub(1, std::memory_order_release);
                        std::invoke(temp_task);
                        completed_tasks_.fetch_sub(1, std::memory_order_release);

                        tasks_[id].tasks.pop();

                    } while (unassigned_tasks_.load(std::memory_order_acquire) > 0);

                    if (completed_tasks_.load(std::memory_order_acquire) == 0) {
                        threads_done_.release();
                    }
                } while (!stop_tok.stop_requested());
            });
        }
    }

    ThreadPool(const ThreadPool & ) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    ~ThreadPool() {
        wait_for_tasks();

        for (std::size_t i = 0; i < threads_.size(); ++i) {
            threads_[i].request_stop();
            tasks_[i].signal.release();
            threads_[i].join();
        }
    }

    template <typename Function, typename... Args>
    requires std::invocable<Function, Args...> && std::is_same_v<void, std::invoke_result_t<Function &&, Args &&...>>
    void enqueue_detach(Function&& f, Args&&... args) {
        enqueue_task(std::move([f = std::forward<Function>(f), ... largs = std::forward<Args>(args)]() mutable -> decltype(auto) {
            std::invoke(f, largs...);
        }));
    }

private:
    template <typename Function>
    void enqueue_task(Function&& f) {
        current_id_.load(std::memory_order_acquire);
        unassigned_tasks_.fetch_add(1, std::memory_order_release);
        completed_tasks_.fetch_add(1, std::memory_order_release);
        tasks_[current_id_].tasks.push(std::move(std::forward<Function>(f)));
        tasks_[current_id_].signal.release();
        current_id_.fetch_add(1, std::memory_order_release);
    }

    void wait_for_tasks() {
        if (completed_tasks_.load(std::memory_order_acquire) > 0) {
            threads_done_.acquire();
        }
    }

    struct task_item {
        Queue<FunctionType> tasks{5};
        std::binary_semaphore signal{0};
    };

    std::vector<std::jthread> threads_;
    std::deque<task_item> tasks_;
    Queue<std::size_t> priority_queue_;
    std::atomic_int_fast64_t unassigned_tasks_{}, completed_tasks_{}, current_id_{};
    std::binary_semaphore threads_done_{0};
};