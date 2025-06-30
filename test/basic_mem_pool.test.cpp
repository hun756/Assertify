#include <assertify/assertify.hpp>
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace assertify;
using namespace assertify::detail;

namespace testing_utils
{

class BaseTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    bool is_timestamp_valid(
        const std::chrono::time_point<std::chrono::high_resolution_clock>&
            timestamp)
    {
        auto now = std::chrono::high_resolution_clock::now();
        return timestamp >= start_time_ && timestamp <= now;
    }

    template <typename Duration>
    void wait_for(Duration duration)
    {
        std::this_thread::sleep_for(duration);
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time_;
};

} // namespace testing_utils

using namespace testing_utils;

class ThreadSafeCounterTest : public BaseTest
{
protected:
    thread_safe_counter<int> int_counter_;
    thread_safe_counter<std::size_t> size_counter_;
    thread_safe_counter<long long> large_counter_;
};

TEST_F(ThreadSafeCounterTest, InitialState)
{
    EXPECT_EQ(int_counter_.get(), 0);
    EXPECT_EQ(size_counter_.get(), 0);
    EXPECT_EQ(large_counter_.get(), 0);
}

TEST_F(ThreadSafeCounterTest, SingleIncrement)
{
    int_counter_.increment();
    EXPECT_EQ(int_counter_.get(), 1);

    int_counter_.increment();
    EXPECT_EQ(int_counter_.get(), 2);
}

TEST_F(ThreadSafeCounterTest, MultipleIncrements)
{
    constexpr int iterations = 1000;

    for (int i = 0; i < iterations; ++i)
    {
        int_counter_.increment();
    }

    EXPECT_EQ(int_counter_.get(), iterations);
}

TEST_F(ThreadSafeCounterTest, AddOperation)
{
    int_counter_.add(5);
    EXPECT_EQ(int_counter_.get(), 5);

    int_counter_.add(10);
    EXPECT_EQ(int_counter_.get(), 15);

    int_counter_.add(-3);
    EXPECT_EQ(int_counter_.get(), 12);
}

TEST_F(ThreadSafeCounterTest, ResetOperation)
{
    int_counter_.add(100);
    EXPECT_EQ(int_counter_.get(), 100);

    int_counter_.reset();
    EXPECT_EQ(int_counter_.get(), 0);
}

TEST_F(ThreadSafeCounterTest, LargeNumbers)
{
    constexpr long long large_value = 1000000000LL;

    large_counter_.add(large_value);
    EXPECT_EQ(large_counter_.get(), large_value);

    large_counter_.add(large_value);
    EXPECT_EQ(large_counter_.get(), 2 * large_value);
}

TEST_F(ThreadSafeCounterTest, ThreadSafetyIncrement)
{
    constexpr int num_threads = 10;
    constexpr int increments_per_thread = 1000;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(
            [this]() -> void
            {
                for (int j = 0; j < increments_per_thread; ++j)
                {
                    int_counter_.increment();
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_EQ(int_counter_.get(), num_threads * increments_per_thread);
}

TEST_F(ThreadSafeCounterTest, ThreadSafetyAdd)
{
    constexpr int num_threads = 8;
    constexpr int value_per_thread = 100;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([this]() -> void
                             { int_counter_.add(value_per_thread); });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_EQ(int_counter_.get(), num_threads * value_per_thread);
}

TEST_F(ThreadSafeCounterTest, ConcurrentReadWrite)
{
    constexpr int duration_ms = 100;
    std::atomic<bool> stop_flag{false};
    std::atomic<int> read_count{0};

    std::thread writer(
        [this, &stop_flag]() -> void
        {
            while (!stop_flag.load())
            {
                int_counter_.increment();
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });

    std::vector<std::thread> readers;
    for (int i = 0; i < 4; ++i)
    {
        readers.emplace_back(
            [this, &stop_flag, &read_count]() -> void
            {
                while (!stop_flag.load())
                {
                    [[maybe_unused]] auto value = int_counter_.get();
                    read_count.fetch_add(1);
                    std::this_thread::sleep_for(std::chrono::microseconds(5));
                }
            });
    }

    wait_for(std::chrono::milliseconds(duration_ms));
    stop_flag.store(true);

    writer.join();
    for (auto& reader : readers)
    {
        reader.join();
    }

    EXPECT_GT(int_counter_.get(), 0);
    EXPECT_GT(read_count.load(), 0);
}

TEST_F(ThreadSafeCounterTest, NoexceptGuarantees)
{
    static_assert(noexcept(int_counter_.increment()));
    static_assert(noexcept(int_counter_.add(5)));
    static_assert(noexcept(int_counter_.get()));
    static_assert(noexcept(int_counter_.reset()));
}
