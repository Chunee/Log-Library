#include <benchmark/benchmark.h>
#include <iostream>
#include <thread>
#include "Log.h"

static void BM_TestLog(benchmark::State& state) {
	Log log;
	for (auto _ : state) {
//		std::thread t1([&] { log.debug("Hello, {0}, Your money is {2} and {1}", "Chun", 45, 5.12); });
//		std::thread t2([&] { log.debug("Hello, {0}, Your money is {2} and {1}", "Chun", 50, 5.13); });
//
//    	t1.detach();
//    	t2.detach();
//
    	log.debug("Hello, {0}, Your money is {2} and {1}", "Chun", 60, 9.12);
		log.debug("Hello, {0}, Your money is {2} and {1}", "Chun", 65, 8.12);
		log.debug("Hello, {0}, Your money is {2} and {1}", "Chun", 55, 7.12);

    	// Check if the benchmark should exit
    	// if (state.iterations() >= 1000) {
    	//    	state.SetIterationTime(0);  // Set iteration time to 0 to indicate completion
    	//    	break;  // Exit the loop
    	// }
	}
}

BENCHMARK(BM_TestLog);

BENCHMARK_MAIN();