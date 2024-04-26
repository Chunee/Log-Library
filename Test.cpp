#include "Log.h"
#include <thread>

// Function to read the RDTSC register
inline uint64_t rdtsc() {
    uint32_t low, high;
    asm volatile ("rdtsc" : "=a" (low), "=d" (high));
    return ((uint64_t)high << 32) | low;
}

int main() {
	Log log;
	std::string debug = "debug";
	std::string error = "error";
	std::string info = "info";

	// Start measuring time using RDTSC
    uint64_t start = rdtsc();

	log.debug("Hello, this is {} message", debug);
	log.error("Hello, this is {} message", error);
	log.info("Hello, this is {} message", info);
	std::thread([&log, error]{ log.error("Hello, this is {} message", error); }).join();
	std::thread([&log, info]{ log.info("Hello, this is {} message", info); }).join();
    log.error("Hello, this is {} message", error);

    // End measuring time using RDTSC
    uint64_t end = rdtsc();

    // Calculate the elapsed time
    uint64_t elapsedTime = end - start;

    // Output the elapsed time
    std::cout << "Elapsed time: " << elapsedTime << " CPU cycles" << std::endl;

	return 0;
}