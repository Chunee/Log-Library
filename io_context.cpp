#include "io_context.h"
#include <iostream>

io_context::io_context() :running_(false) {
    if (const int result = io_uring_queue_init(QUEUE_DEPTH, &io_uring_, 0); result != 0) {
        throw std::runtime_error("Failed to invoke 'io_uring_queue_init'");
    }
    // event_loop_thread_ = std::thread(&io_context::run_event_loop, this);
}

io_context::~io_context() {
    stop_event_loop();
    event_loop_thread_.join();
    io_uring_queue_exit(&io_uring_);
}

void io_context::prepare_write_request(int fd, const char* message, size_t size, off_t offset) {
    struct io_uring_sqe* sqe = io_uring_get_sqe(&io_uring_);
    if (!sqe) {
        fprintf(stderr, "Failed to get submission queue entry\n");
        return;
    }

    int f = open("output.txt", O_WRONLY | O_CREAT | O_DIRECT, S_IRUSR | S_IWUSR);
    if (f == -1) {
        std::cerr << "Error opening file" << std::endl;
        return;
    }

    memcpy(buffer, message, size);
    io_uring_prep_write(sqe, f, buffer, size, offset);
}

void io_context::submit_request() {
    int ret = io_uring_submit(&io_uring_);
    if (ret < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-ret));
    }
}

void io_context::handle_completion() {
    struct io_uring_cqe* cqe;
    int ret = io_uring_wait_cqe(&io_uring_, &cqe);  // Block until a completion is available
    if (ret < 0) {
        fprintf(stderr, "io_uring_wait_cqe failed: %s\n", strerror(-ret));
        return;
    }
    if (cqe->res < 0) {
        fprintf(stderr, "Write failed: %s\n", strerror(-cqe->res));
    } else {
        printf("Write completed successfully: %d bytes\n", cqe->res);
    }
    io_uring_cqe_seen(&io_uring_, cqe);
}

void io_context::run_event_loop() {
    while (running_) {
        handle_completion();
    }
}

void io_context::stop_event_loop() {
    running_ = false;
}

void io_context::write(int fd, const char* message, size_t len) {
    prepare_write_request(fd, message, len, 0);
    submit_request();
    event_loop_thread_ = std::thread(&io_context::run_event_loop, this);

    // Open the file before getting the SQE
    // int f = open("output.txt", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    // if (f == -1) {
    //     std::cerr << "Error opening file" << std::endl;
    //     return;
    // }

    // struct io_uring_sqe* sqe = io_uring_get_sqe(&io_uring_);
    // if (!sqe) {
    //     fprintf(stderr, "Failed to get submission queue entry\n");
    //     close(f); // Close the file on error
    //     return;
    // }

    // // Prepare the buffer with the message
    // memcpy(buffer, message, len);

    // // Prepare the write request
    // io_uring_prep_write(sqe, f, buffer, len, 0);

    // // Submit the request
    // int ret_sub = io_uring_submit(&io_uring_);
    // if (ret_sub < 0) {
    //     fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-ret_sub));
    //     close(f); // Close the file on error
    //     return;
    // }

    // // Wait for the completion
    // struct io_uring_cqe* cqe;
    // int ret = io_uring_wait_cqe(&io_uring_, &cqe);
    // if (ret < 0) {
    //     fprintf(stderr, "io_uring_wait_cqe failed: %s\n", strerror(-ret));
    //     close(f); // Close the file on error
    //     return;
    // }

    // // Check the result of the write operation
    // if (cqe->res < 0) {
    //     fprintf(stderr, "Write failed: %s\n", strerror(-cqe->res));
    // } else {
    //     printf("Write completed successfully: %d bytes\n", cqe->res);
    // }

    // // Mark the completion queue entry as seen
    // io_uring_cqe_seen(&io_uring_, cqe);

    // // Close the file after the write is done
    // close(f);
}