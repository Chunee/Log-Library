#include <benchmark/benchmark.h>
#include <iostream>

// Define another benchmark
static void BM_StringCopy(benchmark::State& state) {
  std::string x = "hello";
  for (auto _ : state) {
	std::string copy(x);
        state.PauseTiming();  // Pause timing to perform I/O
        // Check if the benchmark should exit
        if (state.iterations() >= 1000) {
            state.SetIterationTime(0);  // Set iteration time to 0 to indicate completion
            break;  // Exit the loop
        }
        state.ResumeTiming();  // Resume timing after I/O
  }
}

BENCHMARK(BM_StringCopy);

BENCHMARK_MAIN();