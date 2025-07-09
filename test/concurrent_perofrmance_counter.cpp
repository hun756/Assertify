#include <assertify/assertify.hpp>
#include <chrono>
#include <cstdint>
#include <gtest/gtest.h>

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

TEST_F(ConcurrentPerformanceCounterTest, InitialState)
{
    EXPECT_EQ(counter_.count(), 0);
    EXPECT_EQ(counter_.total_time_ns(), 0);
    EXPECT_EQ(counter_.min_time_ns(), 0);
    EXPECT_EQ(counter_.max_time_ns(), 0);
    EXPECT_DOUBLE_EQ(counter_.average_time_ns(), 0.0);
    EXPECT_EQ(counter_.percentile(50.0), 0);
}

TEST_F(ConcurrentPerformanceCounterTest, SingleMeasurement)
{
    constexpr auto delay = std::chrono::microseconds(100);

    {
        auto timer = counter_.time();
        controlled_delay(delay);
    }

    EXPECT_EQ(counter_.count(), 1);
    EXPECT_GT(counter_.total_time_ns(), 0);
    EXPECT_GT(counter_.min_time_ns(), 0);
    EXPECT_GT(counter_.max_time_ns(), 0);
    EXPECT_GT(counter_.average_time_ns(), 0.0);

    EXPECT_EQ(counter_.min_time_ns(), counter_.max_time_ns());
    EXPECT_EQ(counter_.min_time_ns(), counter_.total_time_ns());

    EXPECT_DOUBLE_EQ(counter_.average_time_ns(),
                     static_cast<double>(counter_.total_time_ns()));

    EXPECT_TRUE(is_timing_reasonable(counter_.min_time_ns(), delay));
}

TEST_F(ConcurrentPerformanceCounterTest, MultipleMeasurements)
{
    constexpr int num_measurements = 5;
    std::vector<std::chrono::microseconds> delays = {
        std::chrono::microseconds(50), std::chrono::microseconds(100),
        std::chrono::microseconds(150), std::chrono::microseconds(200),
        std::chrono::microseconds(250)};

    for (const auto& delay : delays)
    {
        auto timer = counter_.time();
        controlled_delay(delay);
    }

    EXPECT_EQ(counter_.count(), num_measurements);
    EXPECT_GT(counter_.total_time_ns(), 0);
    EXPECT_GT(counter_.min_time_ns(), 0);
    EXPECT_GT(counter_.max_time_ns(), 0);
    EXPECT_GT(counter_.average_time_ns(), 0.0);

    EXPECT_LT(counter_.min_time_ns(), counter_.max_time_ns());

    auto expected_average =
        static_cast<double>(counter_.total_time_ns()) / num_measurements;
    EXPECT_DOUBLE_EQ(counter_.average_time_ns(), expected_average);

    EXPECT_TRUE(is_timing_reasonable(counter_.min_time_ns(), delays[0]));

    EXPECT_TRUE(is_timing_reasonable(counter_.max_time_ns(), delays[4]));
}

TEST_F(ConcurrentPerformanceCounterTest, ResetFunctionality)
{
    for (int i = 0; i < 3; ++i)
    {
        auto timer = counter_.time();
        controlled_delay(std::chrono::microseconds(50));
    }

    EXPECT_GT(counter_.count(), 0);
    EXPECT_GT(counter_.total_time_ns(), 0);

    counter_.reset();

    EXPECT_EQ(counter_.count(), 0);
    EXPECT_EQ(counter_.total_time_ns(), 0);
    EXPECT_EQ(counter_.min_time_ns(), 0);
    EXPECT_EQ(counter_.max_time_ns(), 0);
    EXPECT_DOUBLE_EQ(counter_.average_time_ns(), 0.0);
    EXPECT_EQ(counter_.percentile(50.0), 0);
}

TEST_F(ConcurrentPerformanceCounterTest, ZeroTimeMeasurement)
{
    {
        auto timer = counter_.time();
    }

    EXPECT_EQ(counter_.count(), 1);
    EXPECT_GE(counter_.total_time_ns(), 0);
    EXPECT_GE(counter_.min_time_ns(), 0);
    EXPECT_GE(counter_.max_time_ns(), 0);
}
