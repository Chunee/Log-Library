#include <iostream>
#include <utility>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <concepts>
#include <source_location>

#include "log_level.h"
#include "io_context.h"
#include "mpmc_queue.h"

#include <fcntl.h>
#include <unistd.h>
#include <liburing.h>

#include <fmt/format.h>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <fmt/chrono.h>

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
		~Log(); 

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
			std::string now = getPrefix();

			auto const& loc = fmt.location();

			auto log_msg = fmt::vformat(to_string_view(fmt.format()), fmt::make_format_args(args...));

			auto output_msg = fmt::format("{} {}:{} [{}] {}\n", now, loc.file_name(), loc.line(), logLevelToString(level), log_msg);

			auto deadline = std::chrono::steady_clock::now() + std::chrono::microseconds(250);

			mpmc_.write(std::move(output_msg));

			std::string pop_msg{};
			while (mpmc_.size() >= mpmc_.capacity() / 2) {
				mpmc_.read(pop_msg);

				io_context_.write(pop_msg.data(), pop_msg.size());
			} 
		}

		template <typename T, typename... Args>
		fmt::basic_string_view<T> to_string_view(fmt::basic_format_string<T, Args...> fmt) {
			return fmt;
		}
		
		std::string getPrefix();
		std::string logLevelToString(logging::LogLevel);

	private:
		logging::IoContext io_context_;
		std::string_view file_path_;
		MPMCQueue<std::string> mpmc_{100};
	};
}
