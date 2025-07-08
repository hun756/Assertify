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
