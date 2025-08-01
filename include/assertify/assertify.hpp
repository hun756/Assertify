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

    template <typename T = std::byte>
    [[nodiscard]] std::pmr::polymorphic_allocator<T> get_allocator() noexcept
    {
        if constexpr (std::is_same_v<T, std::byte>)
        {
            return allocator_;
        }
        else
        {
            return std::pmr::polymorphic_allocator<T>(&buffer_);
        }
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
    static double mean(R&& data)
    {
        if (std::ranges::empty(data))
            return 0.0;
        return std::accumulate(std::ranges::begin(data), std::ranges::end(data),
                               0.0) /
               static_cast<double>(std::ranges::distance(data));
    }

    template <std::ranges::range R>
    static double variance(R&& data)
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
        return sum_sq_diff /
               static_cast<double>(std::ranges::distance(data) - 1);
    }

    template <std::ranges::range R>
    static double standard_deviation(R&& data)
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

class string_algorithms
{
public:
    static std::size_t edit_distance(std::string_view s1, std::string_view s2)
    {
        std::vector<std::vector<std::size_t>> dp(
            s1.size() + 1, std::vector<std::size_t>(s2.size() + 1));

        for (std::size_t i = 0; i <= s1.size(); ++i)
            dp[i][0] = i;
        for (std::size_t j = 0; j <= s2.size(); ++j)
            dp[0][j] = j;

        for (std::size_t i = 1; i <= s1.size(); ++i)
        {
            for (std::size_t j = 1; j <= s2.size(); ++j)
            {
                if (s1[i - 1] == s2[j - 1])
                {
                    dp[i][j] = dp[i - 1][j - 1];
                }
                else
                {
                    dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1],
                                             dp[i - 1][j - 1]});
                }
            }
        }

        return dp[s1.size()][s2.size()];
    }

    static std::size_t hamming_distance(std::string_view s1,
                                        std::string_view s2)
    {
        if (s1.size() != s2.size())
            return std::numeric_limits<std::size_t>::max();

        std::size_t distance = 0;
        for (std::size_t i = 0; i < s1.size(); ++i)
        {
            if (s1[i] != s2[i])
                ++distance;
        }
        return distance;
    }

    static double fuzzy_match_ratio(std::string_view s1, std::string_view s2)
    {
        if (s1.empty() && s2.empty())
            return 1.0;
        if (s1.empty() || s2.empty())
            return 0.0;

        std::size_t max_len = std::max(s1.size(), s2.size());
        std::size_t edit_dist = edit_distance(s1, s2);
        return 1.0 -
               static_cast<double>(edit_dist) / static_cast<double>(max_len);
    }

    static std::vector<std::string_view> tokenize(std::string_view str,
                                                  char delimiter = ' ')
    {
        std::vector<std::string_view> tokens;
        std::size_t start = 0;
        std::size_t pos = 0;

        while ((pos = str.find(delimiter, start)) != std::string_view::npos)
        {
            if (pos > start)
            {
                tokens.emplace_back(str.substr(start, pos - start));
            }
            start = pos + 1;
        }

        if (start < str.size())
        {
            tokens.emplace_back(str.substr(start));
        }

        return tokens;
    }
};

template <typename T>
class value_formatter
{
public:
    static fast_string<char> format(const T& value)
    {
        if constexpr (string_like<T>)
        {
            return fast_string<char>(std::format("\"{}\"", value),
                                     tl_pool.get_allocator<char>());
        }
        else if constexpr (complex_numeric<T>)
        {
            return fast_string<char>(
                std::format("({} + {}i)", value.real(), value.imag()),
                tl_pool.get_allocator<char>());
        }
        else if constexpr (std::is_arithmetic_v<T>)
        {
            if constexpr (std::is_floating_point_v<T>)
            {
                return fast_string<char>(std::format("{:.6g}", value),
                                         tl_pool.get_allocator<char>());
            }
            else if constexpr (std::is_same_v<T, char> ||
                               std::is_same_v<T, signed char> ||
                               std::is_same_v<T, unsigned char>)
            {
                return fast_string<char>(
                    std::format("{}", static_cast<char>(value)),
                    tl_pool.get_allocator<char>());
            }
            else if constexpr (std::is_same_v<T, wchar_t>)
            {
                if (value <= 127 && value >= 0)
                {
                    return fast_string<char>(
                        std::format("{}", static_cast<char>(value)),
                        tl_pool.get_allocator<char>());
                }
                else
                {
                    return fast_string<char>(
                        std::format("U+{:04X}",
                                    static_cast<unsigned int>(value)),
                        tl_pool.get_allocator<char>());
                }
            }
            else
            {
                return fast_string<char>(std::format("{}", value),
                                         tl_pool.get_allocator<char>());
            }
        }
        else if constexpr (std::is_pointer_v<T>)
        {
            return fast_string<char>(
                value
                    ? std::format("0x{:x}", reinterpret_cast<uintptr_t>(value))
                    : "nullptr",
                tl_pool.get_allocator<char>());
        }
        else if constexpr (optional_like<T>)
        {
            return value.has_value()
                       ? fast_string<char>(
                             std::format("some({})", value_formatter<typename T::value_type>::format(*value)),
                             tl_pool.get_allocator<char>())
                       : fast_string<char>("none",
                                           tl_pool.get_allocator<char>());
        }
        else if constexpr (container_type<T>)
        {
            fast_string<char> result("[", tl_pool.get_allocator<char>());
            bool first = true;
            std::size_t count = 0;
            constexpr std::size_t max_display = 10;

            for (const auto& item : value)
            {
                if (count >= max_display)
                {
                    result += ", ...";
                    break;
                }
                if (!first)
                    result += ", ";
                result += value_formatter<std::decay_t<decltype(item)>>::format(item);
                first = false;
                ++count;
            }
            result += "]";
            return result;
        }
        else if constexpr (variant_like<T>)
        {
            return fast_string<char>(
                std::format("variant<index:{}>", value.index()),
                tl_pool.get_allocator<char>());
        }
        else if constexpr (std::is_enum_v<T>)
        {
            return fast_string<char>(
                std::format("enum({})",
                            static_cast<std::underlying_type_t<T>>(value)),
                tl_pool.get_allocator<char>());
        }
        else if constexpr (streamable<T>)
        {
            std::ostringstream oss;
            oss << value;
            return fast_string<char>(oss.str(), tl_pool.get_allocator<char>());
        }
        else
        {
            return fast_string<char>(
                std::format("object<{}>", typeid(T).name()),
                tl_pool.get_allocator<char>());
        }
    }
};

template <typename T>
[[nodiscard]] inline fast_string<char> format_value(const T& value) noexcept
{
    try
    {
        return value_formatter<T>::format(value);
    }
    catch (...)
    {
        return fast_string<char>("unprintable", tl_pool.get_allocator<char>());
    }
}

} // namespace detail

} // namespace assertify

#endif // end of include guard: LIB_ASSERTIFY_HPP_p06vza
