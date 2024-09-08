#include "log/log.h"
#include "simple_logger.hpp"
#include <fstream>
#include <vector>
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

    int machine_id = 1023;

    log.setOutputFile("output.txt");
    // log.error("Machine ID: {0} is encountering problems. Please repair this machine ID: {0} immediately", machine_id);

    using juzzlin::L;
    L::init("myLog.txt");
    L::enableEchoMode(false);
    
    // L().error() << "User ID: " << user_id << ", Error message: " << error_message;

    // log.setOutputFile("output.txt");
    for (int j = 0; j < 30; ++j) {    
        std::vector<std::thread> threads;
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < 100; ++i) {
            log.error("User ID: {}, Error message: {}", i, error_message);
            // L().error() << "User ID: " << i << ", Error message: " << error_message;
        }

        // for (int i = 0; i < 8; ++i) {
        //     threads.push_back(std::thread([i, user_id, error_message, &log] {
        //         log.error("User ID: {}, Error message: {}", i, error_message);
        //         // L().error() << "User ID: " << i << ", Error message: " << error_message;
        //     }));
        // }

        // for (auto&t : threads) {
        //     t.join();
        // }
        
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        // std::cout << "Time taken: " << duration.count() << " nanoseconds\n";

        // Open file to write the duration
        std::ofstream outFile("mylog_duration_tested.txt", std::ios::app);
        if (outFile.is_open()) {
            outFile << duration.count() << "\n";
            outFile.close();
        } else {
            std::cerr << "Unable to open file\n";
        }
    }

    // for (int i = 0; i < 10; ++i) {
    //     threads.push_back(std::thread([i,&user_id, &error_message, &log] {
    //         std::vector<std::thread> innerThreads;
    //         for (int j = 0; j < 15; ++j) {
    //             innerThreads.push_back(std::thread([i, j, &user_id, &error_message, &log] {
    //                 log.error("User ID: {}, Error message: {}", user_id, error_message);
    //                 // L().error() << "User ID: " << user_id << ", Error message: " << error_message;
    //                 // LOG_INFO << "User ID: " << user_id << ", Error message" << error_message;
    //             }));
    //         }
    //         for (auto& t : innerThreads) {
    //             t.join();
    //         }
    //     }));
    // }

    // for (auto& t : threads) {
    //     t.join();
    // }

    return 0;
}
