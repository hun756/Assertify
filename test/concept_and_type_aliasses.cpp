#include <algorithm>
#include <assertify/assertify.hpp>
#include <complex>
#include <gtest/gtest.h>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <memory_resource>
#include <unordered_map>
#include <optional>
#include <variant>
#include <functional>

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

class ContainerConceptsTest : public ConceptsAndAliasesTest
{
};

TEST_F(ContainerConceptsTest, ContainerTypeValidation)
{
    static_assert(container_type<std::vector<int>>);
    static_assert(container_type<std::list<int>>);
    static_assert(container_type<std::array<int, 5>>);
    static_assert(container_type<std::span<int>>);
    static_assert(container_type<fast_vector<int>>);

    static_assert(container_type<std::map<int, int>>);
    static_assert(container_type<std::unordered_map<int, int>>);
    static_assert(container_type<std::set<int>>);
    static_assert(container_type<fast_unordered_map<int>>);

    static_assert(!container_type<int>);
    static_assert(!container_type<std::string_view>);
    static_assert(!container_type<double>);

    SUCCEED();
}

TEST_F(ContainerConceptsTest, AssociativeContainerValidation)
{
    static_assert(associative_container<std::map<int, std::string>>);
    static_assert(associative_container<std::unordered_map<std::string, int>>);
    static_assert(associative_container<fast_unordered_map<int>>);

    static_assert(!associative_container<std::vector<int>>);
    static_assert(!associative_container<std::list<std::string>>);
    static_assert(!associative_container<std::array<int, 5>>);

    std::map<int, std::string> map;
    map[1] = "one";
    auto it = map.find(1);
    EXPECT_NE(it, map.end());
    EXPECT_EQ(it->second, "one");

    SUCCEED();
}

TEST_F(ContainerConceptsTest, SequenceContainerValidation)
{
    static_assert(sequence_container<std::vector<int>>);
    static_assert(sequence_container<std::list<int>>);
    static_assert(sequence_container<std::array<int, 5>>);
    static_assert(sequence_container<fast_vector<int>>);

    static_assert(!sequence_container<std::map<int, int>>);
    static_assert(!sequence_container<std::unordered_map<int, int>>);
    static_assert(!sequence_container<fast_unordered_map<int>>);

    static_assert(!sequence_container<int>);
    static_assert(!sequence_container<std::string_view>);

    SUCCEED();
}

class StringAndPointerConceptsTest : public ConceptsAndAliasesTest
{
};

TEST_F(StringAndPointerConceptsTest, StringLikeValidation)
{
    static_assert(string_like<std::string>);
    static_assert(string_like<std::string_view>);
    static_assert(string_like<const char*>);
    static_assert(string_like<char*>);
    static_assert(string_like<fast_string<char>>);

    static_assert(!string_like<int>);
    static_assert(!string_like<std::vector<char>>);
    static_assert(!string_like<std::complex<double>>);

    std::string str = "test";
    std::string_view sv = str;
    const char* cstr = str.c_str();

    EXPECT_EQ(std::string_view(str), sv);
    EXPECT_EQ(std::string(cstr), str);

    SUCCEED();
}

TEST_F(StringAndPointerConceptsTest, PointerLikeValidation)
{
    static_assert(pointer_like<int*>);
    static_assert(pointer_like<const char*>);
    static_assert(pointer_like<void*>);

    static_assert(pointer_like<std::unique_ptr<int>>);
    static_assert(pointer_like<std::shared_ptr<std::string>>);

    static_assert(pointer_like<PointerLikeType>);

    static_assert(!pointer_like<int>);
    static_assert(!pointer_like<std::string>);
    static_assert(!pointer_like<std::vector<int>>);

    PointerLikeType ptr_like;
    EXPECT_TRUE(static_cast<bool>(ptr_like));
    EXPECT_EQ(*ptr_like, 42);

    SUCCEED();
}

class OptionalVariantConceptsTest : public ConceptsAndAliasesTest {};

TEST_F(OptionalVariantConceptsTest, OptionalLikeValidation) {
    static_assert(optional_like<std::optional<int>>);
    static_assert(optional_like<std::optional<std::string>>);
    static_assert(optional_like<std::optional<ComparableType>>);
    
    static_assert(!optional_like<int>);
    static_assert(!optional_like<std::string>);
    static_assert(!optional_like<std::vector<int>>);
    
    std::optional<int> opt1;
    std::optional<int> opt2 = 42;
    
    EXPECT_FALSE(opt1.has_value());
    EXPECT_TRUE(opt2.has_value());
    EXPECT_EQ(*opt2, 42);
    
    SUCCEED();
}

TEST_F(OptionalVariantConceptsTest, VariantLikeValidation) {
    static_assert(variant_like<std::variant<int, std::string>>);
    static_assert(variant_like<std::variant<double, char, bool>>);
    
    static_assert(!variant_like<int>);
    static_assert(!variant_like<std::string>);
    static_assert(!variant_like<std::optional<int>>);
    
    std::variant<int, std::string> var = 42;
    EXPECT_EQ(var.index(), 0);
    
    var = std::string("hello");
    EXPECT_EQ(var.index(), 1);
    
    std::visit([](const auto& value) {
        if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>) {
            EXPECT_EQ(value, "hello");
        }
    }, var);
    
    SUCCEED();
}

class FunctionalConceptsTest : public ConceptsAndAliasesTest {};

TEST_F(FunctionalConceptsTest, CallableTypeValidation) {
    static_assert(callable_type<void(*)()>);
    static_assert(!callable_type<int(*)(int)>);
    
    auto lambda = []() { return 42; };
    static_assert(callable_type<decltype(lambda)>);
    
    static_assert(callable_type<std::function<void()>>);
    static_assert(!callable_type<std::function<int(double)>>);
    
    static_assert(!callable_type<int>);
    static_assert(!callable_type<std::string>);
    static_assert(!callable_type<std::vector<int>>);
    
    auto func = []() { return "called"; };
    EXPECT_EQ(func(), "called");
    
    SUCCEED();
}

TEST_F(FunctionalConceptsTest, BooleanConvertibleValidation) {
    static_assert(boolean_convertible<bool>);
    static_assert(boolean_convertible<int>);
    static_assert(boolean_convertible<double>);
    static_assert(boolean_convertible<char*>);
    static_assert(boolean_convertible<std::unique_ptr<int>>);
    
    static_assert(boolean_convertible<PointerLikeType>);
    
    int zero = 0;
    int non_zero = 42;
    std::unique_ptr<int> null_ptr;
    std::unique_ptr<int> valid_ptr = std::make_unique<int>(100);
    
    EXPECT_FALSE(static_cast<bool>(zero));
    EXPECT_TRUE(static_cast<bool>(non_zero));
    EXPECT_FALSE(static_cast<bool>(null_ptr));
    EXPECT_TRUE(static_cast<bool>(valid_ptr));
    
    SUCCEED();
}
