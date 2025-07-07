#ifndef LIB_ASSERTIFY_HPP_p06vza
#define LIB_ASSERTIFY_HPP_p06vza

#include <algorithm>
#include <atomic>
#include <bit>
#include <chrono>
#include <concepts>
#include <coroutine>
#include <cstdint>
#include <format>
#include <iostream>
#include <memory_resource>
#include <numeric>
#include <ranges>
#include <shared_mutex>
#include <source_location>
#include <stacktrace>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

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
                             std::string_view context,
                             std::format_string<Args...> fmt, Args&&... args)
        : std::logic_error(std::format(fmt, std::forward<Args>(args)...)),
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

extern thread_local basic_memory_pool tl_pool;

template <typename T>
using fast_allocator = std::pmr::polymorphic_allocator<T>;

template <typename T>
using fast_string =
    std::basic_string<char, std::char_traits<char>, fast_allocator<char>>;

template <typename T>
using fast_vector = std::pmr::vector<T>;

template <typename T>
using fast_unordered_map = std::pmr::unordered_map<T, T>;

template <typename T>
concept numeric_type = std::integral<T> || std::floating_point<T>;

template <typename T>
concept complex_numeric = requires(T t) {
    { t.real() } -> std::floating_point;
    { t.imag() } -> std::floating_point;
};

template <typename T>
concept container_type =
    std::ranges::range<T> && !std::convertible_to<T, std::string_view>;

template <typename T>
concept associative_container = requires(T t) {
    typename T::key_type;
    typename T::mapped_type;
    t.find(typename T::key_type{});
};

template <typename T>
concept sequence_container = container_type<T> && !associative_container<T>;

template <typename T>
concept string_like = std::convertible_to<T, std::string_view> ||
                      std::convertible_to<T, std::string>;

template <typename T>
concept pointer_like =
    std::is_pointer_v<std::remove_reference_t<T>> || requires(T&& t) {
        *std::forward<T>(t);
        { static_cast<bool>(std::forward<T>(t)) } -> std::convertible_to<bool>;
    };

template <typename T>
concept optional_like = requires(T t) {
    { t.has_value() } -> std::convertible_to<bool>;
    { *t } -> std::convertible_to<typename T::value_type>;
};

template <typename T>
concept variant_like = requires(T t) {
    { t.index() } -> std::convertible_to<std::size_t>;
    std::visit([](auto&&) {}, t);
};

template <typename T>
concept callable_type = std::invocable<T>;

template <typename T>
concept boolean_convertible = requires(T t) {
    { static_cast<bool>(t) } -> std::convertible_to<bool>;
};

template <typename T>
concept streamable = requires(std::ostream& os, const T& t) { os << t; };

template <typename T>
concept hashable = requires(T t) { std::hash<T>{}(t); };

template <typename T>
concept comparable = requires(T a, T b) {
    { a <=> b } -> std::convertible_to<std::partial_ordering>;
};

template <typename T>
concept equality_comparable = std::equality_comparable<T>;

template <typename T>
concept regular_type = std::regular<T>;

template <typename T, typename U>
concept comparable_with = requires(const T& t, const U& u) {
    { t == u } -> std::convertible_to<bool>;
    { t != u } -> std::convertible_to<bool>;
    { t < u } -> std::convertible_to<bool>;
    { t <= u } -> std::convertible_to<bool>;
    { t > u } -> std::convertible_to<bool>;
    { t >= u } -> std::convertible_to<bool>;
};

template <typename T>
concept serializable = requires(T t) {
    { t.serialize() } -> std::convertible_to<std::string>;
} || requires(T t, std::ostream& os) {
    { os << t } -> std::convertible_to<std::ostream&>;
};

template <typename T>
concept deserializable = requires(T t, const std::string& s) {
    { T::deserialize(s) } -> std::convertible_to<T>;
} || requires(T t, std::istream& is) {
    { is >> t } -> std::convertible_to<std::istream&>;
};

template <typename T>
concept network_testable = requires(T t) {
    { t.get_status_code() } -> std::convertible_to<int>;
    { t.get_headers() } -> container_type;
    { t.get_body() } -> string_like;
};

template <typename T>
concept coroutine_like = requires(T t) {
    { t.await_ready() } -> std::convertible_to<bool>;
    t.await_suspend(std::coroutine_handle<>{});
    t.await_resume();
};

struct epsilon_config
{
    double relative_epsilon = 1e-9;
    double absolute_epsilon = 1e-12;
    bool use_ulp_comparison = false;
    int max_ulp_difference = 4;
};

template <typename T>
struct same_size_signed;
template <>
struct same_size_signed<float>
{
    using type = int32_t;
};
template <>
struct same_size_signed<double>
{
    using type = int64_t;
};
#if defined(__SIZEOF_FLOAT128__)
template <>
struct same_size_signed<__float128>
{
    using type = __int128_t;
};
#endif

template <std::floating_point T>
constexpr bool almost_equal(T a, T b,
                            const epsilon_config& config = {}) noexcept
{
    if (std::isnan(a) || std::isnan(b))
        return false;
    if (std::isinf(a) || std::isinf(b))
        return a == b;

    if (a == b)
        return true;

    if (config.use_ulp_comparison)
    {
        using int_type = typename same_size_signed<T>::type;
        const auto a_int = std::bit_cast<int_type>(a);
        const auto b_int = std::bit_cast<int_type>(b);
        const auto ulp_diff = std::abs(a_int - b_int);
        return ulp_diff <= config.max_ulp_difference;
    }

    const T abs_eps = static_cast<T>(config.absolute_epsilon);
    const T rel_eps = static_cast<T>(config.relative_epsilon);

    const T diff = std::fabs(a - b);
    if (diff <= abs_eps)
        return true;

    const T largest = std::fmax(std::fabs(a), std::fabs(b));
    return diff <= largest * rel_eps;
}

template <complex_numeric T>
constexpr bool almost_equal(const T& a, const T& b,
                            const epsilon_config& config = {}) noexcept
{
    return almost_equal(a.real(), b.real(), config) &&
           almost_equal(a.imag(), b.imag(), config);
}

class statistical_analyzer
{
public:
    template <std::ranges::range R>
    static double mean(const R& data)
    {
        if (std::ranges::empty(data))
            return 0.0;
        return std::accumulate(std::ranges::begin(data), std::ranges::end(data),
                               0.0) /
               std::ranges::distance(data);
    }

    template <std::ranges::range R>
    static double variance(const R& data)
    {
        if (std::ranges::distance(data) < 2)
            return 0.0;
        double m = mean(data);
        double sum_sq_diff = 0.0;
        for (const auto& val : data)
        {
            double diff = val - m;
            sum_sq_diff += diff * diff;
        }
        return sum_sq_diff / (std::ranges::distance(data) - 1);
    }

    template <std::ranges::range R>
    static double standard_deviation(const R& data)
    {
        return std::sqrt(variance(data));
    }

    template <std::ranges::range R>
    static auto median(R data)
    {
        std::ranges::sort(data);
        auto size = std::ranges::distance(data);
        if (size % 2 == 0)
        {
            auto it1 = std::ranges::begin(data) + size / 2 - 1;
            auto it2 = std::ranges::begin(data) + size / 2;
            return (*it1 + *it2) / 2.0;
        }
        else
        {
            auto it = std::ranges::begin(data) + size / 2;
            return static_cast<double>(*it);
        }
    }

    template <std::ranges::range R1, std::ranges::range R2>
    static double correlation(const R1& x, const R2& y)
    {
        if (std::ranges::distance(x) != std::ranges::distance(y) ||
            std::ranges::empty(x))
        {
            return 0.0;
        }

        double mean_x = mean(x);
        double mean_y = mean(y);
        double numerator = 0.0;
        double sum_sq_x = 0.0;
        double sum_sq_y = 0.0;

        auto it_x = std::ranges::begin(x);
        auto it_y = std::ranges::begin(y);

        for (; it_x != std::ranges::end(x); ++it_x, ++it_y)
        {
            double diff_x = *it_x - mean_x;
            double diff_y = *it_y - mean_y;
            numerator += diff_x * diff_y;
            sum_sq_x += diff_x * diff_x;
            sum_sq_y += diff_y * diff_y;
        }

        double denominator = std::sqrt(sum_sq_x * sum_sq_y);
        return denominator != 0.0 ? numerator / denominator : 0.0;
    }
};

} // namespace detail

} // namespace assertify

#endif // end of include guard: LIB_ASSERTIFY_HPP_p06vza