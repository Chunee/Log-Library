#include "io_context.h"
#include <iostream>

io_context::io_context() :running_(false) {
    if (const int result = io_uring_queue_init(QUEUE_DEPTH, &io_uring_, 0); result != 0) {
        throw std::runtime_error("Failed to invoke 'io_uring_queue_init'");
    }
    initialize_buffer(4, 4096);
    // event_loop_thread_ = std::thread(&io_context::run_event_loop, this);
}

io_context::~io_context() {
    stop_event_loop();
    event_loop_thread_.join();
    io_uring_queue_exit(&io_uring_);
}

void io_context::initialize_buffer(const unsigned int buffer_ring_size, const size_t buffer_size) {
    size_t ring_entries_size = buffer_ring_size * sizeof(io_uring_buf);
    size_t page_alignment = sysconf(_SC_PAGESIZE);

    std::cout << "Page alignment: " << page_alignment << "\n";
    std::cout << "Buffer ring entries size: " << ring_entries_size << "\n";

    // void *buffer_ring = nullptr;
    // if (posix_memalign(&buffer_ring, page_alignment, ring_entries_size) != 0) {
    //     throw std::runtime_error("posix_memalign failed");
    // }

    io_uring_buf_ring* buffer_ring;
    if (posix_memalign(reinterpret_cast<void**>(&buffer_ring), page_alignment, ring_entries_size) != 0) {
        throw std::runtime_error("posix_memalign failed");
    }

    std::cout << "Buffer ring allocated at address: " << buffer_ring << "\n";

    // buffer_ring_.reset(reinterpret_cast<io_uring_buf_ring *>(buffer_ring));

    buffer_list_.reserve(buffer_ring_size);
    for (unsigned int i = 0; i < buffer_ring_size; ++i) {
      buffer_list_.emplace_back(buffer_size);
    }

    struct io_uring_buf_reg io_uring_buf_reg{
        .ring_addr = reinterpret_cast<__uint64_t>(buffer_ring),
        .ring_entries = static_cast<unsigned int>(buffer_list_.size()),
        .bgid = BUFFER_GROUP_ID, 
    };

    std::cout << "Registering buffer ring with:\n";
    std::cout << "  ring_addr: " << io_uring_buf_reg.ring_addr << "\n";
    std::cout << "  ring_entries: " << io_uring_buf_reg.ring_entries << "\n";
    std::cout << "  bgid: " << io_uring_buf_reg.bgid << "\n";

    if (const int result = io_uring_register_buf_ring(&io_uring_, &io_uring_buf_reg, 0); result != 0) {
        throw std::runtime_error("Failed to invoke 'io_uring_register_buf_ring': " + std::string(strerror(-result)));
    }
    io_uring_buf_ring_init(buffer_ring);

    const unsigned int mask = io_uring_buf_ring_mask(buffer_list_.size());
    for (unsigned int buffer_id = 0; buffer_id < buffer_list_.size(); ++buffer_id) {
        io_uring_buf_ring_add(
          buffer_ring, buffer_list_[buffer_id].data(), buffer_list_[buffer_id].size(), buffer_id, mask,
          buffer_id);
    }
    io_uring_buf_ring_advance(buffer_ring, buffer_list_.size());
}

void io_context::register_buffer() {
    io_uring_buf_ring* buffer_ring = buffer_ring_.get();
    unsigned int buffer_ring_size = buffer_list_.size();

    struct io_uring_buf_reg io_uring_buf_reg{
        .ring_addr = reinterpret_cast<__uint64_t>(buffer_ring),
        .ring_entries = buffer_ring_size,
        .bgid = BUFFER_GROUP_ID, 
    };

    if (const int result = io_uring_register_buf_ring(&io_uring_, &io_uring_buf_reg, 0); result != 0) {
        throw std::runtime_error("Failed to invoke 'io_uring_register_buf_ring'");
    }
    io_uring_buf_ring_init(buffer_ring);

    const unsigned int mask = io_uring_buf_ring_mask(buffer_ring_size);
    for (unsigned int buffer_id = 0; buffer_id < buffer_ring_size; ++buffer_id) {
        io_uring_buf_ring_add(
          buffer_ring, buffer_list_[buffer_id].data(), buffer_list_[buffer_id].size(), buffer_id, mask,
          buffer_id);
    }
    io_uring_buf_ring_advance(buffer_ring, buffer_ring_size);
}

void io_context::unregister_buffer() {
    if (io_uring_unregister_buffers(&io_uring_) < 0) {
        perror("io_uring_unregister_buffers");
    }
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
    io_uring_prep_write_fixed(sqe, f, buffer, size, offset, 0);
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
    // prepare_write_request(fd, message, len, 0);
    // submit_request();
    // event_loop_thread_ = std::thread(&io_context::run_event_loop, this);

    // Open the file before getting the SQE
    int f = open("output.txt", O_WRONLY | O_CREAT | O_DIRECT, S_IRUSR | S_IWUSR);
    if (f == -1) {
        std::cerr << "Error opening file" << std::endl;
        return;
    }

    struct io_uring_sqe* sqe = io_uring_get_sqe(&io_uring_);
    if (!sqe) {
        fprintf(stderr, "Failed to get submission queue entry\n");
        close(f); // Close the file on error
        return;
    }

    // Prepare the buffer with the message
    memcpy(buffer, message, len);

    // Prepare the write request
    io_uring_prep_write_fixed(sqe, f, buffer, len, 0, 0);

    // Submit the request
    int ret_sub = io_uring_submit(&io_uring_);
    if (ret_sub < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-ret_sub));
        close(f); // Close the file on error
        return;
    }

    // Wait for the completion
    struct io_uring_cqe* cqe;
    int ret = io_uring_wait_cqe(&io_uring_, &cqe);
    if (ret < 0) {
        fprintf(stderr, "io_uring_wait_cqe failed: %s\n", strerror(-ret));
        close(f); // Close the file on error
        return;
    }

    // Check the result of the write operation
    if (cqe->res < 0) {
        fprintf(stderr, "Write failed: %s\n", strerror(-cqe->res));
    } else {
        printf("Write completed successfully: %d bytes\n", cqe->res);
    }

    // Mark the completion queue entry as seen
    io_uring_cqe_seen(&io_uring_, cqe);

    // Close the file after the write is done
    close(f);
}