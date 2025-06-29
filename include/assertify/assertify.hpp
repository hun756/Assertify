#ifndef LIB_ASSERTIFY_HPP_p06vza
#define LIB_ASSERTIFY_HPP_p06vza

#include <chrono>
#include <memory_resource>
#include <shared_mutex>
#include <source_location>
#include <stacktrace>
#include <stdexcept>
#include <unordered_map>

namespace assertify
{

class assertion_error final : public std::logic_error
{
private:
    std::source_location location_;
    std::stacktrace trace_;
    std::string context_;
    std::chrono::time_point<std::chrono::high_resolution_clock> timestamp_;

public:
    explicit assertion_error(
        std::string_view message,
        std::source_location location = std::source_location::current(),
        std::string_view context = "")
        : std::logic_error(std::string(message)), location_(location),
          trace_(std::stacktrace::current()), context_(context),
          timestamp_(std::chrono::high_resolution_clock::now())
    {
    }

    template <typename... Args>
    explicit assertion_error(std::source_location location,
                             std::string_view context, Args&&... args)
        : std::logic_error(std::format(std::forward<Args>(args)...)),
          location_(location), trace_(std::stacktrace::current()),
          context_(context),
          timestamp_(std::chrono::high_resolution_clock::now())
    {
    }

    [[nodiscard]] const std::source_location& where() const noexcept
    {
        return location_;
    }

    [[nodiscard]] const std::stacktrace& stack_trace() const noexcept
    {
        return trace_;
    }

    [[nodiscard]] const std::string& context() const noexcept
    {
        return context_;
    }

    [[nodiscard]] const auto& timestamp() const noexcept { return timestamp_; }

    [[nodiscard]] std::string detailed_message() const
    {
        return std::format(
            "{}\nContext: {}\nLocation: {}:{}\nTimestamp: {}", what(), context_,
            location_.file_name(), location_.line(),
            std::chrono::duration_cast<std::chrono::milliseconds>(
                timestamp_.time_since_epoch())
                .count());
    }
};

namespace detail
{
template <typename T>
class thread_safe_counter
{
    mutable std::atomic<T> value_{0};

public:
    void increment() noexcept
    {
        value_.fetch_add(1, std::memory_order_relaxed);
    }

    void add(T val) noexcept
    {
        value_.fetch_add(val, std::memory_order_relaxed);
    }

    [[nodiscard]] T get() const noexcept
    {
        return value_.load(std::memory_order_relaxed);
    }

    void reset() noexcept { value_.store(0, std::memory_order_relaxed); }
};

class basic_memory_pool
{
private:
    struct block_header
    {
        std::size_t size;
        bool allocated;
        block_header* next;
        std::chrono::time_point<std::chrono::high_resolution_clock>
            allocation_time;
    };

    std::pmr::monotonic_buffer_resource buffer_;
    std::pmr::polymorphic_allocator<std::byte> allocator_;
    mutable std::shared_mutex mutex_;
    thread_safe_counter<std::size_t> allocation_count_;
    thread_safe_counter<std::size_t> total_allocated_;
    std::unordered_map<void*, block_header*> active_allocations_;

public:
    basic_memory_pool(std::size_t initial_size = 1024 * 1024)
        : buffer_(initial_size), allocator_(&buffer_)
    {
    }

    template <typename T>
    [[nodiscard]] T* allocate(std::size_t count = 1)
    {
        std::unique_lock lock(mutex_);
        auto size = sizeof(T) * count;
        auto* ptr =
            static_cast<T*>(allocator_.allocate_bytes(size, alignof(T)));

        auto* header = static_cast<block_header*>(allocator_.allocate_bytes(
            sizeof(block_header), alignof(block_header)));
        header->size = size;
        header->allocated = true;
        header->allocation_time = std::chrono::high_resolution_clock::now();
        active_allocations_[ptr] = header;

        allocation_count_.increment();
        total_allocated_.add(size);
        return ptr;
    }

    template <typename T>
    void deallocate(T* ptr)
    {
        std::unique_lock lock(mutex_);
        if (auto it = active_allocations_.find(ptr);
            it != active_allocations_.end())
        {
            it->second->allocated = false;
            active_allocations_.erase(it);
        }
    }

    [[nodiscard]] std::size_t active_allocation_count() const
    {
        std::shared_lock lock(mutex_);
        return active_allocations_.size();
    }

    [[nodiscard]] bool has_memory_leaks() const
    {
        return active_allocation_count() > 0;
    }

    void reset() noexcept
    {
        std::unique_lock lock(mutex_);
        buffer_.release();
        active_allocations_.clear();
        allocation_count_.reset();
        total_allocated_.reset();
    }

    [[nodiscard]] std::vector<std::pair<void*, std::chrono::duration<double>>>
    get_leak_report() const
    {
        std::shared_lock lock(mutex_);
        std::vector<std::pair<void*, std::chrono::duration<double>>> leaks;
        auto now = std::chrono::high_resolution_clock::now();

        for (const auto& [ptr, header] : active_allocations_)
        {
            auto duration =
                std::chrono::duration_cast<std::chrono::duration<double>>(
                    now - header->allocation_time);
            leaks.emplace_back(ptr, duration);
        }
        return leaks;
    }
};

thread_local basic_memory_pool tl_pool;

} // namespace detail

} // namespace assertify

#endif // end of include guard: LIB_ASSERTIFY_HPP_p06vza