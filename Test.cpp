#include "log/Log.h"
#include "simple_logger.hpp"

using namespace std::chrono_literals;

// Function to read the RDTSC register
inline uint64_t rdtsc() {
    uint32_t low, high;
    asm volatile ("rdtsc" : "=a" (low), "=d" (high));
    return ((uint64_t)high << 32) | low;
}

int main() {
	logging::Log log;
	std::string debug = "debug";
	std::string error = "error";
	std::string info = "info";
    std::string fatal = "fatal";

    // using juzzlin::L;
    // L::init("myLog.txt");
    // L::enableEchoMode(false);

    log.setOutputFile("output.txt");

	// Start measuring time using RDTSC
    uint64_t start = rdtsc();

    log.error("Hello, this is {} message", error);
    // log.info("Hello, this is {} message", info);
    // log.debug("Hello, this is {} message", debug);
    // log.fatal("Hello, this is {} message", fatal);
    // std::thread([&log, error]{ log.error("Hello, this is {} message", error); }).join();
    // std::thread([&log, info]{ log.info("Hello, this is {} message", info); }).join();

    std::vector<std::thread> threads;

    for (int i = 0; i < 3; ++i) {
        threads.push_back(std::thread([i,&log] {
            std::vector<std::thread> innerThreads;
            for (int j = 0; j < 5; ++j) {
                innerThreads.push_back(std::thread([i, j, &log] {
                    log.error("Hello, this is thread {}, error", std::to_string(i));
                }));
            }
            for (auto& t : innerThreads) {
                t.join();
            }
        }));
    }

    for (auto& t : threads) {
        t.join();
    }

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
