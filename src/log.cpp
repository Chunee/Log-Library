#include "log/log.h"

logging::Log::~Log() {
	while (!mpmc_.isEmpty()) {
		std::string pop_msg{};
		mpmc_.read(pop_msg);

		io_context_.write(pop_msg.data(), pop_msg.size());
	}
}

void logging::Log::setOutputFile(std::string_view file_path) {
	file_path_ = file_path;
	io_context_.register_file(file_path_);
}

std::string logging::Log::getPrefix() {
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    auto time = std::chrono::system_clock::to_time_t(now);

    // Get thread ID
    std::thread::id this_id = std::this_thread::get_id();
	auto thread_id_hash = std::hash<std::thread::id>{}(this_id);

    // Use fmt for formatting the prefix string
    return fmt::format("{:%Y-%m-%d %H:%M:%S}.{:09d} {} ", *std::localtime(&time), (ns % 1000000000), thread_id_hash);
}

std::string logging::Log::logLevelToString(logging::LogLevel level) {
	switch(level) {
#define _FUNCTION(name) case logging::LogLevel::name: return #name;
	LOGGING_FOR_EACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION
	}
	return "UNKNOWN";
}
