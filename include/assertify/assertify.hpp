#ifndef LIB_ASSERTIFY_HPP_p06vza
#define LIB_ASSERTIFY_HPP_p06vza

#include <chrono>
#include <source_location>
#include <stacktrace>
#include <stdexcept>

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
} // namespace detail

} // namespace assertify

#endif // end of include guard: LIB_ASSERTIFY_HPP_p06vza