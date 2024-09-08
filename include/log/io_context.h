#include <liburing.h>
#include <vector>
#include <thread>
#include <atomic>
#include <stdexcept>
#include <cstring>
#include <string>
#include <string_view>
#include <mutex>
#include "turn_sequencer.h"

#define QUEUE_DEPTH 100
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

        int register_file(std::string_view);
        void write(const char*, size_t);

    private:
        struct io_uring io_uring_;
        int fds[2];
        std::atomic<uint32_t> count_{0};
        std::atomic<uint32_t> turn_{0};
        TurnSequencer<std::atomic> turn_sequencer_;
        alignas(64) std::atomic<uint32_t> spinCutoff_;
    };
}