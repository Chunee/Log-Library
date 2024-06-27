#include "log/log.h"

thread_local logging::Queue<char> logging::Log::queue_{1000};

logging::Log::~Log() {
	char* pop_ptr = queue_.flush();
	int fd = open(file_path_.data(), O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		std::cerr << "Error opening file" << std::endl;
	   	return;
	}
	io_context_.write(fd, pop_ptr, strlen(pop_ptr));
}

void logging::Log::setOutputFile(std::string_view file_path) {
	file_path_ = file_path;
}

std::string logging::Log::getPrefix() {
	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	auto time = std::chrono::system_clock::to_time_t(now);

	std::thread::id this_id = std::this_thread::get_id();

	std::stringstream ss;
	ss << std::put_time(std::localtime(&time), "%Y-%m-%d %X.");
	ss << std::setfill('0') << std::setw(3) << (ms % 1000) << ' ' << this_id << ' ';

	return ss.str();
}

std::string logging::Log::logLevelToString(logging::LogLevel level) {
	switch(level) {
#define _FUNCTION(name) case logging::LogLevel::name: return #name;
	LOGGING_FOR_EACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION
	}
	return "UNKNOWN";
}
