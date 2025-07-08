#include <assertify/assertify.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <regex>

using namespace assertify;
using namespace detail;

namespace testing_utils
{

class ValueFormatterTest : public ::testing::Test
{
protected:
    void SetUp() override { tl_pool.reset(); }

    void TearDown() override { tl_pool.reset(); }

    bool matches_pattern(const std::string& text, const std::string& pattern)
    {
        std::regex regex_pattern(pattern);
        return std::regex_match(text, regex_pattern);
    }

    template <typename StringType>
    bool uses_thread_local_allocator(const StringType& str)
    {
        return str.get_allocator().resource() ==
               tl_pool.get_allocator<char>().resource();
    }
};

enum class TestEnum
{
    VALUE1 = 10,
    VALUE2 = 20,
    VALUE3 = 42
};

enum TestEnumClass : unsigned char
{
    A = 1,
    B = 2,
    C = 255
};

struct StreamableTestType
{
    int value;
    friend std::ostream& operator<<(std::ostream& os,
                                    const StreamableTestType& obj)
    {
        return os << "StreamableTestType{" << obj.value << "}";
    }
};

struct NonStreamableType
{
    int data = 123;
};

struct ThrowingStreamableType
{
    friend std::ostream& operator<<(std::ostream& os,
                                    const ThrowingStreamableType&)
    {
        throw std::runtime_error("Stream error");
        return os;
    }
};
} // namespace testing_utils

using namespace testing_utils;

class StringLikeFormatterTest : public ValueFormatterTest
{
};

TEST_F(StringLikeFormatterTest, StdStringFormatting)
{
    std::string test_str = "Hello, World!";
    auto result = value_formatter<std::string>::format(test_str);

    EXPECT_EQ(result, "\"Hello, World!\"");
    EXPECT_TRUE(uses_thread_local_allocator(result));
}

TEST_F(StringLikeFormatterTest, EmptyStringFormatting)
{
    std::string empty_str = "";
    auto result = value_formatter<std::string>::format(empty_str);

    EXPECT_EQ(result, "\"\"");
}

TEST_F(StringLikeFormatterTest, StringViewFormatting)
{
    std::string_view test_view = "string_view test";
    auto result = value_formatter<std::string_view>::format(test_view);

    EXPECT_EQ(result, "\"string_view test\"");
}

TEST_F(StringLikeFormatterTest, CStringFormatting)
{
    const char* test_cstr = "C-style string";
    auto result = value_formatter<const char*>::format(test_cstr);

    EXPECT_EQ(result, "\"C-style string\"");
}

TEST_F(StringLikeFormatterTest, StringWithSpecialCharacters)
{
    std::string special = "Line1\nLine2\tTabbed\"Quoted\"";
    auto result = value_formatter<std::string>::format(special);

    EXPECT_EQ(result, "\"Line1\nLine2\tTabbed\"Quoted\"\"");
}

TEST_F(StringLikeFormatterTest, FastStringFormatting)
{
    fast_string<char> fast_str("fast string test",
                               tl_pool.get_allocator<char>());
    auto result = value_formatter<fast_string<char>>::format(fast_str);

    EXPECT_EQ(result, "\"fast string test\"");
    EXPECT_TRUE(uses_thread_local_allocator(result));
}
