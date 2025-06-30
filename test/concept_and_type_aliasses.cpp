#include <algorithm>
#include <assertify/assertify.hpp>
#include <complex>
#include <gtest/gtest.h>
#include <ranges>

using namespace assertify;
using namespace assertify::detail;

namespace testing_utils
{
class ConceptsAndAliasesTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup test data if needed
    }

    template <typename T, template <typename> typename Concept>
    static constexpr bool satisfies_concept()
    {
        return Concept<T>::value;
    }
};

struct SerializableType
{
    std::string serialize() const { return "serialized"; }
    static SerializableType deserialize(const std::string&) { return {}; }
};

struct StreamableType
{
    friend std::ostream& operator<<(std::ostream& os, const StreamableType&)
    {
        return os << "streamable";
    }
    friend std::istream& operator>>(std::istream& is, StreamableType&)
    {
        return is;
    }
};

struct NetworkTestableType
{
    int get_status_code() const { return 200; }
    std::vector<std::string> get_headers() const
    {
        return {"header1", "header2"};
    }
    std::string get_body() const { return "response body"; }
};

struct CoroutineLikeType
{
    bool await_ready() const { return true; }
    void await_suspend(std::coroutine_handle<>) const {}
    int await_resume() const { return 42; }
};

struct PointerLikeType
{
    int value = 42;
    int& operator*() { return value; }
    const int& operator*() const { return value; }
    explicit operator bool() const { return true; }
};

struct ComparableType
{
    int value;
    auto operator<=>(const ComparableType& other) const = default;
    bool operator==(const ComparableType& other) const = default;
};
} // namespace testing_utils

using namespace testing_utils;

class TypeAliasesTest : public ConceptsAndAliasesTest
{
};

TEST_F(TypeAliasesTest, FastAllocatorBasicUsage)
{
    std::pmr::monotonic_buffer_resource buffer(1024);
    fast_allocator<int> alloc(&buffer);

    EXPECT_NE(alloc.resource(), nullptr);

    auto* ptr = alloc.allocate(10);
    EXPECT_NE(ptr, nullptr);

    alloc.deallocate(ptr, 10);

    static_assert(std::is_same_v<fast_allocator<int>,
                                 std::pmr::polymorphic_allocator<int>>);
}

TEST_F(TypeAliasesTest, FastStringCreationAndUsage)
{
    std::pmr::monotonic_buffer_resource buffer(1024);
    fast_allocator<char> alloc(&buffer);

    fast_string<char> str1(alloc);
    fast_string<char> str2("Hello, World!", alloc);
    fast_string<char> str3(str2, alloc);

    EXPECT_TRUE(str1.empty());
    EXPECT_EQ(str2, "Hello, World!");
    EXPECT_EQ(str3, str2);

    str1 = "Fast ";
    str1 += "String";
    EXPECT_EQ(str1, "Fast String");

    EXPECT_EQ(str1.get_allocator().resource(), &buffer);

    std::pmr::monotonic_buffer_resource buffer2(1024);
    fast_string<char> str4("Different allocator",
                           fast_allocator<char>(&buffer2));
    EXPECT_NE(str4.get_allocator().resource(), str1.get_allocator().resource());
}

TEST_F(TypeAliasesTest, FastVectorOperations)
{
    std::pmr::monotonic_buffer_resource buffer(1024);
    fast_allocator<int> alloc(&buffer);

    fast_vector<int> vec(alloc);

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 2);
    EXPECT_EQ(vec[2], 3);

    fast_vector<int> vec2({10, 20, 30}, alloc);
    EXPECT_EQ(vec2.size(), 3);
    EXPECT_EQ(vec2[0], 10);

    EXPECT_EQ(vec.get_allocator().resource(), &buffer);

    fast_vector<int> vec3 = std::move(vec2);
    EXPECT_EQ(vec3.size(), 3);
    EXPECT_EQ(vec2.size(), 0);
}

TEST_F(TypeAliasesTest, FastUnorderedMapOperations)
{
    std::pmr::monotonic_buffer_resource buffer(1024);
    fast_allocator<std::pair<const int, int>> alloc(&buffer);

    fast_unordered_map<int> map(alloc);

    map[1] = 10;
    map[2] = 20;
    map.insert({3, 30});

    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map[1], 10);
    EXPECT_EQ(map[2], 20);
    EXPECT_EQ(map[3], 30);

    auto it = map.find(2);
    EXPECT_NE(it, map.end());
    EXPECT_EQ(it->second, 20);

    EXPECT_EQ(map.get_allocator().resource(), &buffer);

    fast_unordered_map<int> map2(10, std::hash<int>{}, std::equal_to<int>{},
                                 alloc);
    map2[100] = 1000;
    EXPECT_EQ(map2[100], 1000);
}

TEST_F(TypeAliasesTest, TypeAliasCompatibility)
{
    std::pmr::monotonic_buffer_resource buffer(1024);
    fast_allocator<int> alloc(&buffer);

    fast_vector<int> vec({5, 2, 8, 1, 9}, alloc);
    std::sort(vec.begin(), vec.end());

    fast_vector<int> expected({1, 2, 5, 8, 9}, alloc);
    EXPECT_EQ(vec, expected);

    auto even_numbers =
        vec | std::views::filter([](int n) { return n % 2 == 0; });
    std::vector<int> evens(even_numbers.begin(), even_numbers.end());
    EXPECT_EQ(evens, std::vector<int>({2, 8}));
}

class NumericConceptsTest : public ConceptsAndAliasesTest
{
};

TEST_F(NumericConceptsTest, NumericTypeValidation)
{
    static_assert(numeric_type<int>);
    static_assert(numeric_type<long>);
    static_assert(numeric_type<short>);
    static_assert(numeric_type<char>);
    static_assert(numeric_type<unsigned int>);
    static_assert(numeric_type<std::size_t>);

    static_assert(numeric_type<float>);
    static_assert(numeric_type<double>);
    static_assert(numeric_type<long double>);

    static_assert(!numeric_type<std::string>);
    static_assert(!numeric_type<std::vector<int>>);
    static_assert(!numeric_type<void*>);
    static_assert(!numeric_type<ComparableType>);

    SUCCEED();
}

TEST_F(NumericConceptsTest, ComplexNumericValidation)
{
    static_assert(complex_numeric<std::complex<float>>);
    static_assert(complex_numeric<std::complex<double>>);
    static_assert(complex_numeric<std::complex<long double>>);

    static_assert(!complex_numeric<int>);
    static_assert(!complex_numeric<double>);
    static_assert(!complex_numeric<std::string>);

    std::complex<double> c(3.0, 4.0);
    EXPECT_DOUBLE_EQ(c.real(), 3.0);
    EXPECT_DOUBLE_EQ(c.imag(), 4.0);

    SUCCEED();
}