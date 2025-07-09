#include <assertify/assertify.hpp>
#include <gtest/gtest.h>
#include <chrono>
#include <cstdint>

using namespace assertify;
using namespace detail;

namespace testing_utils
{
class ConcurrentPerformanceCounterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        counter_.reset();
        global_perf_counter.reset();
    }

    void TearDown() override
    {
        counter_.reset();
        global_perf_counter.reset();
    }

    // Helper to create controlled delay
    void controlled_delay(std::chrono::microseconds duration)
    {
        auto start = std::chrono::high_resolution_clock::now();
        while (std::chrono::high_resolution_clock::now() - start < duration)
        {
            // busy wait for more precise timing in tests
        }
    }

    template <typename Func>
    std::chrono::nanoseconds measure_overhead(Func&& func,
                                              int iterations = 1000)
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i)
        {
            std::forward<Func>(func)();
        }
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end -
                                                                    start) /
               iterations;
    }

    bool is_timing_reasonable(std::uint64_t measured_ns,
                              std::chrono::microseconds expected)
    {
        auto expected_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(expected)
                .count();
        // Allow 20% tolerance for timing tests
        auto tolerance = static_cast<std::uint64_t>(expected_ns * 2 / 10);
        return measured_ns >=
                   (static_cast<std::uint64_t>(expected_ns) - tolerance) &&
               measured_ns <=
                   (static_cast<std::uint64_t>(expected_ns) + tolerance);
    }

    concurrent_performance_counter counter_;
};
} // namespace testing_utils

using namespace testing_utils;