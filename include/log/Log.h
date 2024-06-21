#include <iostream>
#include <utility>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>

#include "LogLevel.h"
#include "Queue.h"
#include "IoContext.h"

#include <fcntl.h>
#include <unistd.h>
#include <liburing.h>

#include <fmt/format.h>
#include <fmt/core.h>

namespace logging {
	class Log {
	public:
		Log() = default;
		~Log() = default;

		Log(const Log& other) = delete;
		Log& operator=(const Log& other) = delete;
		Log(Log&& other) noexcept  = delete;
		Log& operator=(Log&& other) noexcept = delete;

		#define _FUNCTION(name) \
		template<typename... Args> \
		void name(fmt::format_string<Args...> fmt, Args&&... args) { \
			addLogMessage(logging::LogLevel::name, fmt, std::forward<Args>(args)...); \
		}
		LOGGING_FOR_EACH_LOG_LEVEL(_FUNCTION)
		#undef _FUNCTION

		void setOutputFile(std::string_view);

	private:
		template <typename... Args>
		void addLogMessage(logging::LogLevel level, fmt::format_string<Args...> fmt, Args&&... args) {
			log(level, to_string_view(fmt), std::forward<Args>(args)...);
		}

		template <typename T, typename... Args>
		void log(logging::LogLevel level, fmt::basic_string_view<T> fmt, Args&&... args) {
			std::string prefix = getPrefix();

			fmt::basic_memory_buffer<T> buf;
			buf.append(prefix);

			std::string level_string = logLevelToString(level);
			buf.append(level_string);

			fmt::vformat_to(fmt::appender(buf), fmt, fmt::make_format_args(args...));
			buf.push_back('\n');
			buf.push_back('\0');

			queue_.push(buf.data(), buf.size());

			if ((queue_.size() + buf.size()) >= (queue_.capacity() / 2)) {
				T* pop_ptr = new char[100];
				queue_.pop(pop_ptr);

				int fd = open(file_path_.data(), O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	   	      	if (fd == -1) {
	   	      		std::cerr << "Error opening file" << std::endl;
	   	      		return;
	   	   		}

				io_context_.write(fd, pop_ptr, strlen(pop_ptr));

				pop_ptr = nullptr;
			}
		}

		template <typename T, typename... Args>
		inline fmt::basic_string_view<T> to_string_view(fmt::basic_format_string<T, Args...> fmt) {
			return fmt;
		}

		std::string getPrefix();
		std::string logLevelToString(logging::LogLevel);

	private:
		static thread_local logging::Queue<char> queue_;
		logging::IoContext io_context_{};
		std::string_view file_path_;
	};
}
