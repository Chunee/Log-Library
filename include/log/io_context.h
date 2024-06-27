#include <liburing.h>
#include <vector>
#include <thread>
#include <atomic>
#include <stdexcept>
#include <cstring>
#include <string>

#define QUEUE_DEPTH 8
#define BUFFER_SIZE 4096
#define BUFFER_GROUP_ID 0

struct io_uring_buf_ring;

namespace logging {
    class IoContext {
    public:
        IoContext();

        IoContext(const IoContext&) = delete;

        IoContext(IoContext&&) = delete;

        IoContext& operator=(const IoContext&) = delete;

        IoContext& operator=(IoContext&&) = delete;

        ~IoContext();

        void write(int, const char*, size_t);

    private:
        void prepare_write_request(int, const char*, size_t, off_t);

        void submit_request();

        void handle_completion();

        void run_event_loop();

        void stop_event_loop();

    private:
        struct io_uring io_uring_;
        std::thread event_loop_thread_;
        std::atomic<bool> running_;
        char buffer[BUFFER_SIZE];
    };
}