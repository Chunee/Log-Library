#include "log/io_context.h"
#include <iostream>

logging::IoContext::IoContext() {
    if (const int result = io_uring_queue_init(QUEUE_DEPTH, &io_uring_, 0); result != 0) {
        throw std::runtime_error("Failed to invoke 'io_uring_queue_init'");
    }
}

logging::IoContext::~IoContext() {
    if (count_ != 0) {
        io_uring_submit(&io_uring_);
        
        struct io_uring_cqe* cqe;
        for (int i = 0; i < count_; ++i) {
            int ret_wait = io_uring_wait_cqe(&io_uring_, &cqe);  // Block until a completion is available

            // Free the buffer once the write is completed
            char* completed_buffer = reinterpret_cast<char*>(cqe->user_data);
            delete[] completed_buffer;

            io_uring_cqe_seen(&io_uring_, cqe);  // Mark CQE as seen
        }        
    }
    io_uring_queue_exit(&io_uring_);
}

void logging::IoContext::write(const char* message, size_t len) {
    turn_sequencer_.waitForTurn(turn_, spinCutoff_, true);
    struct io_uring_sqe* sqe = io_uring_get_sqe(&io_uring_);
    if (!sqe) {
        fprintf(stderr, "Failed to get submission queue entry\n");
        return;
    }

    // Allocate a new buffer for each message to avoid overwriting
    char* new_buffer = new char[len];
    memcpy(new_buffer, message, len);

    // Prepare the write operation using the unique buffer
    io_uring_prep_write(sqe, fds[0], new_buffer, len, 0);
    sqe->user_data = reinterpret_cast<uint64_t>(new_buffer);  // Store the buffer address in user_data

    // int count = count_.fetch_add(1, std::memory_order_acq_rel) + 1;  // Increment and get the new value
    int turn = turn_.fetch_add(1, std::memory_order_acq_rel);
    turn_sequencer_.completeTurn(turn);

    // Atomically increment count before the condition check
    int count = count_.fetch_add(1, std::memory_order_acq_rel) + 1;  // Increment and get the new value

    if (count == QUEUE_DEPTH / 2) {
        io_uring_submit(&io_uring_);
        
        // Reset the counter to 0 atomically
        count_.store(0, std::memory_order_release);

        struct io_uring_cqe* cqe;
        for (int i = 0; i < QUEUE_DEPTH / 2; ++i) {
            int ret_wait = io_uring_wait_cqe(&io_uring_, &cqe);  // Block until a completion is available

            // Free the buffer once the write is completed
            char* completed_buffer = reinterpret_cast<char*>(cqe->user_data);
            delete[] completed_buffer;

            io_uring_cqe_seen(&io_uring_, cqe);  // Mark CQE as seen
        }
    }
}

int logging::IoContext::register_file(std::string_view file_path) {
    fds[0] = open(file_path.data(), O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    if (fds[0] == -1) {
        std::cerr << "Error opening file" << std::endl;
        return 1;
    }

    return 0;
}
