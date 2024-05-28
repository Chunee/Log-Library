#include "Log.h"
#include "simple_logger.hpp"

using namespace std::chrono_literals;

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

    // using juzzlin::L;
    // L::init("myLog.txt");
    // L::enableEchoMode(false);

	// Start measuring time using RDTSC
    uint64_t start = rdtsc();

    log.error("Hello, this is {} message", error);
    log.info("Hello, this is {} message", info);
    // std::thread([&log, error]{ log.error("Hello, this is {} message", error); }).join();
    // std::thread([&log, info]{ log.info("Hello, this is {} message", info); }).join();

    // L().info() << "Something happened";
    // L().error() << "Something happened";
    // std::thread([&]{ L().info() << "Something happened"; }).join();
    // std::thread([&]{ L().error() << "Something happened"; }).join();

    // End measuring time using RDTSC
    uint64_t end = rdtsc();

    // Calculate the elapsed time
    uint64_t elapsedTime = end - start;

    // Output the elapsed time
    std::cout << "Elapsed time: " << elapsedTime << " CPU cycles" << std::endl;

	return 0;
}