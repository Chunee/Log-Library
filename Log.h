#include <string_view>
#include <iostream>
#include <utility>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include "LogLevel.h"
// #include "StagingBuffer.h"
#include "Queue.h"

#include <fmt/format.h>
#include <fmt/core.h>
#include <fcntl.h>
#include <unistd.h>

class Log {
public:
	Log() = default;
	~Log() = default;
	
	Log(const Log& other) = delete;
	Log& operator=(const Log& other) = delete;
	Log(Log&& other) noexcept  = delete;
	Log& operator=(Log&& other) noexcept = delete;

	template<typename... Args>
	void debug(fmt::format_string<Args...> fmt, Args&&...args) {
		addLogMessage(LogLevel::DEBUG, fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void info(fmt::format_string<Args...> fmt, Args&&...args) {
		addLogMessage(LogLevel::INFO, fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void error(fmt::format_string<Args...> fmt, Args&&...args) {
		addLogMessage(LogLevel::ERROR, fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void fatal(fmt::format_string<Args...> fmt, Args&&...args) {
		addLogMessage(LogLevel::FATAL, fmt, std::forward<Args>(args)...);
	}

private:
	// static thread_local StagingBuffer staging_buffer_;
	// static thread_local Queue<const char *> queue_;
	Queue<char> queue_{100};

private:
	template <typename... Args>
	void addLogMessage(LogLevel level, fmt::format_string<Args...> fmt, Args&&... args) {
		log(level, to_string_view(fmt), std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	void log(LogLevel level, fmt::basic_string_view<T> fmt, Args&&... args) {
		std::string prefix = getPrefix();

		fmt::basic_memory_buffer<T> buf;
		buf.append(prefix);

		std::string level_string = logLevelToString(level);
		buf.append(level_string);

		fmt::vformat_to(fmt::appender(buf), fmt, fmt::make_format_args(args...));
		buf.push_back('\n');

		if ((queue_.size() + buf.size()) >= (queue_.capacity() / 4)) {
			T* pop_ptr = new char[100];
			queue_.pop(pop_ptr);
            int fd = open("output.txt", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
               	if (fd == -1) {
               		std::cerr << "Error opening file" << std::endl;
               		return;
            }

            ssize_t bytes_written = write(fd, buf.data(), buf.size());
            if (bytes_written == -1) {
                std::cerr << "Error writing to file" << std::endl;
                close(fd);
                return;
            }

			delete[] pop_ptr;
		}

		// if ((queue_.size() + buf.size()) >= (queue_.capacity() / 4)) {
			// auto task = pool.enqueue([&buf] {
				// int fd = open("output.txt", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
				// if (fd == -1) {
					//std::cerr << "Error opening file" << std::endl;
					//return;
				// }

				// ssize_t bytes_written = write(fd, buf.data(), buf.size());
				// if (bytes_written == -1) {
					// std::cerr << "Error writing to file" << std::endl;
					// close(fd);
					// return;
				// }
			// });
			// return;
		// }

		queue_.push(buf.data());
		// staging_buffer_.addToBuffer(buf.data(), buf.size());
	}

	template <typename T, typename... Args>
	inline fmt::basic_string_view<T> to_string_view(fmt::basic_format_string<T, Args...> fmt) {
		return fmt;
	}

	std::string getPrefix();
	std::string logLevelToString(LogLevel);
};
