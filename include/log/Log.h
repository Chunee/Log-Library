#include <iostream>
#include <utility>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <concepts>
#include <source_location>
#include <tuple>

#include "LogLevel.h"
#include "Queue.h"
#include "IoContext.h"

#include <fcntl.h>
#include <unistd.h>
#include <liburing.h>

#include <fmt/format.h>
#include <fmt/core.h>

namespace logging {
	template<class T>
	struct source_location {
	private:
		T inner_;
		std::source_location loc_;
		
	public:
		template<class U> requires std::constructible_from<T, U>
		consteval source_location(U&& inner, std::source_location loc = std::source_location::current()) 
		: inner_(std::forward<U>(inner)), loc_(std::move(loc)) {}

		constexpr T const& format() const { return inner_; }
			
		constexpr std::source_location const& location() const { return loc_; }
	};

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
		void name(source_location<fmt::format_string<Args...>> fmt, Args&&... args) { \
			addLogMessage(logging::LogLevel::name, fmt, std::forward<Args>(args)...); \
		}
		LOGGING_FOR_EACH_LOG_LEVEL(_FUNCTION)
		#undef _FUNCTION

		void setOutputFile(std::string_view);

	private:
		template <typename... Args>
		void addLogMessage(logging::LogLevel level, source_location<fmt::format_string<Args...>> fmt, Args&&... args) {
			// log(level, to_string_view(fmt.format()), std::forward<Args>(args)...);
			
			std::string now = getPrefix();

			auto const& loc = fmt.location();

			auto log_msg = fmt::vformat(to_string_view(fmt.format()), fmt::make_format_args(args...));

			auto output_msg = fmt::format("{} {}:{} [{}] {}\n", now, loc.file_name(), loc.line(), logLevelToString(level), log_msg);

			queue_.push(output_msg.data(), output_msg.size());

			if ((queue_.size() + output_msg.size()) >= (queue_.capacity() / 2)) {
				char* pop_ptr = new char[450];
				queue_.pop(pop_ptr);

				int fd = open(file_path_.data(), O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	   	      	if (fd == -1) {
	   	      		std::cerr << "Error opening file" << std::endl;
	   	      		return;
	   	   		}

				io_context_.write(fd, pop_ptr, strlen(pop_ptr));

			}

		}

		// template <typename T, typename... Args>
		// void log(logging::LogLevel level, fmt::basic_string_view<T> fmt, Args&&... args) {
		// 	std::string prefix = getPrefix();

		// 	fmt::basic_memory_buffer<T> buf;

		// 	std::string msg = fmt::format("{} {}:{} [{}] ", prefix, loc.file_name(), loc.line(), logLevelToString(level));

		// 	// std::string level_string = logLevelToString(level);
		// 	// buf.append(level_string);

		// 	buf.append(msg);

		// 	fmt::vformat_to(fmt::appender(buf), fmt);
		// 	buf.push_back('\n');
		// 	buf.push_back('\0');

		// 	queue_.push(buf.data(), buf.size());

		// 	if ((queue_.size() + buf.size()) >= (queue_.capacity() / 2)) {
		// 		T* pop_ptr = new char[100];
		// 		queue_.pop(pop_ptr);

		// 		int fd = open(file_path_.data(), O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	 //   	      	if (fd == -1) {
	 //   	      		std::cerr << "Error opening file" << std::endl;
	 //   	      		return;
	 //   	   		}

		// 		io_context_.write(fd, pop_ptr, strlen(pop_ptr));

		// 		pop_ptr = nullptr;
		// 	}
		// }

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
