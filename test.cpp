#include "log/log.h"
#include "simple_logger.hpp"
#include <fstream>
// #include "NanoLog.hpp"

using namespace std::chrono_literals;

// Function to read the RDTSC register
inline uint64_t rdtsc() {
    uint32_t low, high;
    asm volatile ("rdtsc" : "=a" (low), "=d" (high));
    return ((uint64_t)high << 32) | low;
}

int main() {
    logging::Log log;
    int user_id = 42;
    std::string error_message = "File not found";

    log.setOutputFile("output.txt");

    // log.error("User ID: {}, Error message: {}", user_id, error_message);

    // std::string debug = "debug";
    // std::string error = "error";
    // std::string info = "info";
    // std::string fatal = "fatal";

    // using juzzlin::L;
    // L::init("myLog.txt");
    // L::enableEchoMode(false);
    // L().error() << "User ID: " << user_id << ", Error message: " << error_message;

    // nanolog::initialize(nanolog::GuaranteedLogger(), "/tmp/", "nanolog", 1);
    // log.setOutputFile("output.txt");

    // Start measuring time using RDTSC
    // uint64_t start = rdtsc();

    // log.error("Hello, this is {} message", error);
    // log.info("Hello, this is {} message", info);
    // log.debug("Hello, this is {} message", debug);
    // log.info("Hello, this is {} message", info);
    // log.fatal("Hello, this is {} message", fatal);
    // log.info("Hello, this is {} message", info);
    // log.debug("Hello, this is {} message", debug);
    // log.error("Hello, this is {} message", error);
    // log.fatal("Hello, this is {} message", fatal);
    // log.info("Hello, this is {} message", info);

    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10; ++i) {
        threads.push_back(std::thread([i,&user_id, &error_message, &log] {
            std::vector<std::thread> innerThreads;
            for (int j = 0; j < 15; ++j) {
                innerThreads.push_back(std::thread([i, j, &user_id, &error_message, &log] {
                    log.error("User ID: {}, Error message: {}", user_id, error_message);
                    // L().error() << "User ID: " << user_id << ", Error message: " << error_message;
                    // LOG_INFO << "User ID: " << user_id << ", Error message" << error_message;
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

    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    std::cout << "Time taken: " << duration.count() << " nanoseconds\n";

    // L().info() << "Something happened";
    // L().error() << "Something happened";
    // L().debug() << "Something happened";
    // L().error() << "Something happened";
    // L().info() << "Something happened";
    // L().error() << "Something happened";
    // L().debug() << "Something happened";
    // L().error() << "Something happened";
    // L().info() << "Something happened";
    // L().debug() << "Something happened";
    // std::thread([&]{ L().info() << "Something happened"; }).join();
    // std::thread([&]{ L().error() << "Something happened"; }).join();

    // End measuring time using RDTSC
    // uint64_t end = rdtsc();

    // Calculate the elapsed time
    // uint64_t elapsedTime = end - start;

    // Output the elapsed time
    // std::cout << "Elapsed time: " << elapsedTime << " CPU cycles" << std::endl;

    return 0;
}
