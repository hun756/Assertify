#include <benchmark/benchmark.h>

// Dummy bench code : 
static void BM_Calculator(benchmark::State& state) {
    for (auto _ : state) {
        int a = 1, b = 2;
        int result = a + b;
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Calculator);
BENCHMARK_MAIN();