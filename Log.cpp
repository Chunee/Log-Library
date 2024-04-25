#include "Log.h"

// thread_local StagingBuffer Log::staging_buffer_{};
// thread_local Queue<const char *> Log::queue_{100};

std::string Log::getPrefix() {
	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	auto time = std::chrono::system_clock::to_time_t(now);

	std::thread::id this_id = std::this_thread::get_id();

	std::stringstream ss;
	ss << std::put_time(std::localtime(&time), "%Y-%m-%d %X.");
	ss << std::setfill('0') << std::setw(3) << (ms % 1000) << ' ' << this_id << ' ';

	return ss.str();
}

std::string Log::logLevelToString(LogLevel level) {
	switch (level) {
		case LogLevel::DEBUG:
			return "DEBUG ";
		case LogLevel::INFO:
			return "INFO ";
		case LogLevel::ERROR:
			return "ERROR ";
		case LogLevel::FATAL:
			return "FATAL ";
		default:
			return "UNKNOWN ";
	}
}