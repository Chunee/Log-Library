namespace logging {

#define LOGGING_FOR_EACH_LOG_LEVEL(f) \
	f(debug) \
	f(info) \
	f(error) \
	f(fatal) 

enum class LogLevel : std::uint8_t {
#define _FUNCTION(name) name,
	LOGGING_FOR_EACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION
};

}
