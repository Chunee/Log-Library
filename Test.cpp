#include "Log.h"
#include <thread>

int main() {
	Log log;
	std::string debug = "debug";
	std::string error = "error";
	std::string info = "info";

	log.debug("Hello, this is {} message", debug);
	log.error("Hello, this is {} message", error);
	log.info("Hello, this is {} message", info);

	std::thread([&log, error]{ log.error("Hello, this is {} message", error); }).join();
	
	std::thread([&log, info]{ log.info("Hello, this is {} message", info); }).join();
    log.error("Hello, this is {} message", error);
	return 0;
}