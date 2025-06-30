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

class BasicMemoryPoolTest : public BaseTest
{
protected:
    void SetUp() override
    {
        BaseTest::SetUp();
        pool_ = std::make_unique<basic_memory_pool>(1024 * 1024); // 1MB
    }

    void TearDown() override { pool_->reset(); }

    std::unique_ptr<basic_memory_pool> pool_;
};

TEST_F(BasicMemoryPoolTest, InitialState)
{
    EXPECT_EQ(pool_->active_allocation_count(), 0);
    EXPECT_FALSE(pool_->has_memory_leaks());
    EXPECT_TRUE(pool_->get_leak_report().empty());
}

TEST_F(BasicMemoryPoolTest, SingleAllocation)
{
    int* ptr = pool_->allocate<int>();
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(pool_->active_allocation_count(), 1);
    EXPECT_TRUE(pool_->has_memory_leaks());

    pool_->deallocate(ptr);
    EXPECT_EQ(pool_->active_allocation_count(), 0);
    EXPECT_FALSE(pool_->has_memory_leaks());
}

TEST_F(BasicMemoryPoolTest, MultipleAllocations)
{
    constexpr int num_allocations = 10;
    std::vector<int*> ptrs;

    for (int i = 0; i < num_allocations; ++i)
    {
        ptrs.push_back(pool_->allocate<int>());
        EXPECT_NE(ptrs.back(), nullptr);
    }

    EXPECT_EQ(pool_->active_allocation_count(), num_allocations);
    EXPECT_TRUE(pool_->has_memory_leaks());

    for (auto* ptr : ptrs)
    {
        pool_->deallocate(ptr);
    }

    EXPECT_EQ(pool_->active_allocation_count(), 0);
    EXPECT_FALSE(pool_->has_memory_leaks());
}

TEST_F(BasicMemoryPoolTest, AllocateArray)
{
    constexpr std::size_t array_size = 100;
    int* array = pool_->allocate<int>(array_size);

    EXPECT_NE(array, nullptr);
    EXPECT_EQ(pool_->active_allocation_count(), 1);

    for (std::size_t i = 0; i < array_size; ++i)
    {
        array[i] = static_cast<int>(i);
    }

    for (std::size_t i = 0; i < array_size; ++i)
    {
        EXPECT_EQ(array[i], static_cast<int>(i));
    }

    pool_->deallocate(array);
    EXPECT_EQ(pool_->active_allocation_count(), 0);
}

TEST_F(BasicMemoryPoolTest, DifferentTypes)
{
    auto* int_ptr = pool_->allocate<int>();
    auto* double_ptr = pool_->allocate<double>();
    auto* char_ptr = pool_->allocate<char>(256);

    EXPECT_NE(int_ptr, nullptr);
    EXPECT_NE(double_ptr, nullptr);
    EXPECT_NE(char_ptr, nullptr);
    EXPECT_EQ(pool_->active_allocation_count(), 3);

    EXPECT_EQ(reinterpret_cast<uintptr_t>(int_ptr) % alignof(int), 0);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(double_ptr) % alignof(double), 0);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(char_ptr) % alignof(char), 0);

    pool_->deallocate(int_ptr);
    pool_->deallocate(double_ptr);
    pool_->deallocate(char_ptr);

    EXPECT_EQ(pool_->active_allocation_count(), 0);
}

TEST_F(BasicMemoryPoolTest, MemoryLeakDetection)
{
    auto* ptr1 = pool_->allocate<int>();
    auto* ptr2 = pool_->allocate<double>();

    wait_for(std::chrono::milliseconds(10));

    auto* ptr3 = pool_->allocate<char>();

    EXPECT_TRUE(pool_->has_memory_leaks());
    EXPECT_EQ(pool_->active_allocation_count(), 3);

    auto leak_report = pool_->get_leak_report();
    EXPECT_EQ(leak_report.size(), 3);

    for (const auto& [ptr, duration] : leak_report)
    {
        EXPECT_GT(duration.count(), 0);
        EXPECT_LT(duration.count(), 1.0);
    }

    pool_->deallocate(ptr1);
    pool_->deallocate(ptr2);
    pool_->deallocate(ptr3);
}

TEST_F(BasicMemoryPoolTest, ResetFunctionality)
{
    for (int i = 0; i < 5; ++i)
    {
        pool_->allocate<int>();
    }

    EXPECT_EQ(pool_->active_allocation_count(), 5);
    EXPECT_TRUE(pool_->has_memory_leaks());

    pool_->reset();

    EXPECT_EQ(pool_->active_allocation_count(), 0);
    EXPECT_FALSE(pool_->has_memory_leaks());
    EXPECT_TRUE(pool_->get_leak_report().empty());
}

TEST_F(BasicMemoryPoolTest, ThreadSafetyAllocations)
{
    constexpr int num_threads = 8;
    constexpr int allocations_per_thread = 50;
    std::vector<std::thread> threads;
    std::vector<std::vector<int*>> thread_ptrs(num_threads);

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back(
            [this, t, &thread_ptrs]()
            {
                thread_ptrs[static_cast<size_t>(t)].reserve(
                    allocations_per_thread);
                for (int i = 0; i < allocations_per_thread; ++i)
                {
                    auto* ptr = pool_->allocate<int>();
                    EXPECT_NE(ptr, nullptr);
                    thread_ptrs[t].push_back(ptr);
                    *ptr = t * 1000 + i;
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    EXPECT_EQ(pool_->active_allocation_count(),
              num_threads * allocations_per_thread);

    for (int t = 0; t < num_threads; ++t)
    {
        for (int i = 0; i < allocations_per_thread; ++i)
        {
            EXPECT_EQ(*thread_ptrs[t][i], t * 1000 + i);
        }
    }

    for (int t = 0; t < num_threads; ++t)
    {
        for (auto* ptr : thread_ptrs[t])
        {
            pool_->deallocate(ptr);
        }
    }

    EXPECT_EQ(pool_->active_allocation_count(), 0);
}

TEST_F(BasicMemoryPoolTest, ThreadSafetyConcurrentAllocDealloc)
{
    constexpr int duration_ms = 200;
    std::atomic<bool> stop_flag{false};
    std::atomic<int> total_allocations{0};
    std::atomic<int> total_deallocations{0};

    std::vector<std::thread> allocators;
    std::vector<std::vector<int*>> allocator_ptrs(4);

    for (int i = 0; i < 4; ++i)
    {
        allocators.emplace_back(
            [this, i, &allocator_ptrs, &stop_flag, &total_allocations]()
            {
                while (!stop_flag.load())
                {
                    auto* ptr = pool_->allocate<int>();
                    if (ptr)
                    {
                        allocator_ptrs[i].push_back(ptr);
                        total_allocations.fetch_add(1);
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            });
    }

    std::thread deallocator(
        [this, &allocator_ptrs, &stop_flag, &total_deallocations]()
        {
            while (!stop_flag.load())
            {
                for (auto& ptrs : allocator_ptrs)
                {
                    if (!ptrs.empty())
                    {
                        auto* ptr = ptrs.back();
                        ptrs.pop_back();
                        pool_->deallocate(ptr);
                        total_deallocations.fetch_add(1);
                    }
                }
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        });

    wait_for(std::chrono::milliseconds(duration_ms));
    stop_flag.store(true);

    for (auto& allocator : allocators)
    {
        allocator.join();
    }
    deallocator.join();

    for (auto& ptrs : allocator_ptrs)
    {
        for (auto* ptr : ptrs)
        {
            pool_->deallocate(ptr);
        }
    }

    EXPECT_GT(total_allocations.load(), 0);
    EXPECT_GT(total_deallocations.load(), 0);
    EXPECT_EQ(pool_->active_allocation_count(), 0);
}

TEST_F(BasicMemoryPoolTest, LargeAllocations)
{
    constexpr std::size_t large_size = 1024 * 64; // 64KB

    auto* large_ptr = pool_->allocate<char>(large_size);
    EXPECT_NE(large_ptr, nullptr);
    EXPECT_EQ(pool_->active_allocation_count(), 1);

    std::memset(large_ptr, 0xAA, large_size);

    for (std::size_t i = 0; i < large_size; ++i)
    {
        EXPECT_EQ(static_cast<unsigned char>(large_ptr[i]), 0xAA);
    }

    pool_->deallocate(large_ptr);
    EXPECT_EQ(pool_->active_allocation_count(), 0);
}

TEST_F(BasicMemoryPoolTest, AlignmentRequirements)
{
    struct alignas(32) AlignedStruct
    {
        double data[4];
    };

    auto* aligned_ptr = pool_->allocate<AlignedStruct>();
    EXPECT_NE(aligned_ptr, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(aligned_ptr) % 32, 0);

    pool_->deallocate(aligned_ptr);
}

TEST_F(BasicMemoryPoolTest, InvalidDeallocations)
{
    auto* valid_ptr = pool_->allocate<int>();
    EXPECT_EQ(pool_->active_allocation_count(), 1);

    int stack_var = 42;
    pool_->deallocate(&stack_var);
    EXPECT_EQ(pool_->active_allocation_count(), 1);

    pool_->deallocate(valid_ptr);
    EXPECT_EQ(pool_->active_allocation_count(), 0);

    pool_->deallocate(valid_ptr);
    EXPECT_EQ(pool_->active_allocation_count(), 0);
}

TEST_F(BasicMemoryPoolTest, ConstMemberFunctions)
{
    const auto* const_pool = pool_.get();

    [[maybe_unused]] auto count = const_pool->active_allocation_count();
    [[maybe_unused]] auto has_leaks = const_pool->has_memory_leaks();
    [[maybe_unused]] auto report = const_pool->get_leak_report();

    EXPECT_EQ(count, 0);
    EXPECT_FALSE(has_leaks);
    EXPECT_TRUE(report.empty());
}
