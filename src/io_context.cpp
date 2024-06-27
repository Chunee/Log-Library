#include "log/io_context.h"
#include <iostream>

logging::IoContext::IoContext() :running_(true) {
    if (const int result = io_uring_queue_init(QUEUE_DEPTH, &io_uring_, 0); result != 0) {
        throw std::runtime_error("Failed to invoke 'io_uring_queue_init'");
    }
}

logging::IoContext::~IoContext() {
    stop_event_loop();
    io_uring_queue_exit(&io_uring_);
}

void logging::IoContext::prepare_write_request(int fd, const char* message, size_t size, off_t offset) {
    struct io_uring_sqe* sqe = io_uring_get_sqe(&io_uring_);
    if (!sqe) {
        fprintf(stderr, "Failed to get submission queue entry\n");
        return;
    }

    // memcpy(buffer, message, size);
    io_uring_prep_write(sqe, fd, message, size, offset);
}

void logging::IoContext::submit_request() {
    int ret = io_uring_submit(&io_uring_);
    if (ret < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-ret));
    }
    struct io_uring_cqe* cqe;
    int ret_wait = io_uring_wait_cqe(&io_uring_, &cqe);  // Block until a completion is available
    if (ret_wait < 0) {
        fprintf(stderr, "io_uring_wait_cqe failed: %s\n", strerror(-ret_wait));
        return;
    }
    if (cqe->res < 0) {
        fprintf(stderr, "Write failed: %s\n", strerror(-cqe->res));
    } else {
        printf("Write completed successfully: %d bytes\n", cqe->res);
    }
    io_uring_cqe_seen(&io_uring_, cqe);
}

void logging::IoContext::handle_completion() {
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
        running_ = false;
        event_loop_thread_.join();
    }
    io_uring_cqe_seen(&io_uring_, cqe);
}

void logging::IoContext::run_event_loop() {
    while (running_) {
        handle_completion();
    }
}

void logging::IoContext::stop_event_loop() {
    running_ = false;
}

void logging::IoContext::write(int fd, const char* message, size_t len) {
    prepare_write_request(fd, message, len, 0);
    submit_request();
    if (!running_) {
        running_ = true;
        event_loop_thread_ = std::thread(&logging::IoContext::run_event_loop, this);
    }
}
