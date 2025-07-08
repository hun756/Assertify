#include <assertify/assertify.hpp>
#include <complex>
#include <gtest/gtest.h>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>

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

class ComplexNumericFormatterTest : public ValueFormatterTest
{
};

TEST_F(ComplexNumericFormatterTest, ComplexDoubleFormatting)
{
    std::complex<double> c(3.14159, 2.71828);
    auto result = value_formatter<std::complex<double>>::format(c);

    EXPECT_EQ(result, "(3.14159 + 2.71828i)");
    EXPECT_TRUE(uses_thread_local_allocator(result));
}

TEST_F(ComplexNumericFormatterTest, ComplexFloatFormatting)
{
    std::complex<float> c(1.0f, -2.5f);
    auto result = value_formatter<std::complex<float>>::format(c);

    EXPECT_EQ(result, "(1 + -2.5i)");
}

TEST_F(ComplexNumericFormatterTest, ComplexWithZeroImaginary)
{
    std::complex<double> c(42.0, 0.0);
    auto result = value_formatter<std::complex<double>>::format(c);

    EXPECT_EQ(result, "(42 + 0i)");
}

TEST_F(ComplexNumericFormatterTest, ComplexWithZeroReal)
{
    std::complex<double> c(0.0, 5.0);
    auto result = value_formatter<std::complex<double>>::format(c);

    EXPECT_EQ(result, "(0 + 5i)");
}

TEST_F(ComplexNumericFormatterTest, ComplexWithVerySmallNumbers)
{
    std::complex<double> c(1e-10, -1e-15);
    auto result = value_formatter<std::complex<double>>::format(c);

    EXPECT_TRUE(result.find("1e-10") != std::string::npos ||
                result.find("1e-010") != std::string::npos);
    EXPECT_TRUE(result.find("-1e-15") != std::string::npos ||
                result.find("-1e-015") != std::string::npos);
}

class ArithmeticFormatterTest : public ValueFormatterTest
{
};

TEST_F(ArithmeticFormatterTest, IntegerFormatting)
{
    int value = 42;
    auto result = value_formatter<int>::format(value);

    EXPECT_EQ(result, "42");
    EXPECT_TRUE(uses_thread_local_allocator(result));
}

TEST_F(ArithmeticFormatterTest, NegativeIntegerFormatting)
{
    int value = -123;
    auto result = value_formatter<int>::format(value);

    EXPECT_EQ(result, "-123");
}

TEST_F(ArithmeticFormatterTest, LargeIntegerFormatting)
{
    long long value = 9223372036854775807LL;
    auto result = value_formatter<long long>::format(value);

    EXPECT_EQ(result, "9223372036854775807");
}

TEST_F(ArithmeticFormatterTest, UnsignedIntegerFormatting)
{
    unsigned int value = 4294967295U;
    auto result = value_formatter<unsigned int>::format(value);

    EXPECT_EQ(result, "4294967295");
}

TEST_F(ArithmeticFormatterTest, FloatingPointFormatting)
{
    double value = 3.14159265359;
    auto result = value_formatter<double>::format(value);

    EXPECT_EQ(result, "3.14159");
}

TEST_F(ArithmeticFormatterTest, ScientificNotationFormatting)
{
    double value = 1.23456789e15;
    auto result = value_formatter<double>::format(value);

    EXPECT_TRUE(result.find("1.23457e+15") != std::string::npos ||
                result.find("1.23457e+015") != std::string::npos);
}

TEST_F(ArithmeticFormatterTest, VerySmallFloatingPoint)
{
    double value = 1.23456e-10;
    auto result = value_formatter<double>::format(value);

    EXPECT_TRUE(result.find("1.23456e-10") != std::string::npos ||
                result.find("1.23456e-010") != std::string::npos);
}

TEST_F(ArithmeticFormatterTest, SpecialFloatingPointValues)
{

    double inf = std::numeric_limits<double>::infinity();
    auto inf_result = value_formatter<double>::format(inf);
    EXPECT_TRUE(inf_result.find("inf") != std::string::npos);

    double neg_inf = -std::numeric_limits<double>::infinity();
    auto neg_inf_result = value_formatter<double>::format(neg_inf);
    EXPECT_TRUE(neg_inf_result.find("-inf") != std::string::npos);

    double nan_val = std::numeric_limits<double>::quiet_NaN();
    auto nan_result = value_formatter<double>::format(nan_val);
    EXPECT_TRUE(nan_result.find("nan") != std::string::npos);
}

TEST_F(ArithmeticFormatterTest, CharacterFormatting)
{
    char c = 'A';
    auto result = value_formatter<char>::format(c);

    EXPECT_EQ(result, "A");
}

TEST_F(ArithmeticFormatterTest, SpecialCharacterFormatting)
{

    char newline = '\n';
    char tab = '\t';
    char null_char = '\0';
    char space = ' ';

    auto newline_result = value_formatter<char>::format(newline);
    auto tab_result = value_formatter<char>::format(tab);
    auto null_result = value_formatter<char>::format(null_char);
    auto space_result = value_formatter<char>::format(space);

    EXPECT_EQ(newline_result, "\n");
    EXPECT_EQ(tab_result, "\t");
    EXPECT_EQ(null_result, fast_string<char>(std::string(1, '\0'),
                                             tl_pool.get_allocator<char>()));
    EXPECT_EQ(space_result, " ");
}

TEST_F(ArithmeticFormatterTest, CharacterVariantsFormatting)
{

    unsigned char uc = 'B';
    signed char sc = 'C';

    auto uc_result = value_formatter<unsigned char>::format(uc);
    auto sc_result = value_formatter<signed char>::format(sc);

    EXPECT_EQ(uc_result, "B");
    EXPECT_EQ(sc_result, "C");

    unsigned char numeric_uc = 65;
    auto numeric_result = value_formatter<unsigned char>::format(numeric_uc);
    EXPECT_EQ(numeric_result, "A");
}

TEST_F(ArithmeticFormatterTest, WideCharacterFormatting)
{
    wchar_t ascii_wc = L'X';
    auto ascii_result = value_formatter<wchar_t>::format(ascii_wc);
    EXPECT_EQ(ascii_result, "X");
    EXPECT_TRUE(uses_thread_local_allocator(ascii_result));

    wchar_t unicode_wc = L'Ω';
    auto unicode_result = value_formatter<wchar_t>::format(unicode_wc);
    EXPECT_EQ(unicode_result, "U+03A9");
    EXPECT_TRUE(uses_thread_local_allocator(unicode_result));

    wchar_t digit_wc = L'5';
    auto digit_result = value_formatter<wchar_t>::format(digit_wc);
    EXPECT_EQ(digit_result, "5");
    EXPECT_TRUE(uses_thread_local_allocator(digit_result));

    wchar_t null_wc = L'\0';
    auto null_result = value_formatter<wchar_t>::format(null_wc);
    EXPECT_EQ(null_result, fast_string<char>(std::string(1, '\0'),
                                             tl_pool.get_allocator<char>()));

    char regular_char = 'X';
    auto char_result = value_formatter<char>::format(regular_char);
    EXPECT_EQ(char_result, "X");
    EXPECT_TRUE(uses_thread_local_allocator(char_result));
}

TEST_F(ArithmeticFormatterTest, BooleanFormatting)
{
    bool true_val = true;
    bool false_val = false;

    auto true_result = value_formatter<bool>::format(true_val);
    auto false_result = value_formatter<bool>::format(false_val);

    EXPECT_EQ(true_result, "true");
    EXPECT_EQ(false_result, "false");
}

class PointerFormatterTest : public ValueFormatterTest
{
};

TEST_F(PointerFormatterTest, ValidPointerFormatting)
{
    int value = 42;
    int* ptr = &value;
    auto result = value_formatter<int*>::format(ptr);

    EXPECT_TRUE(result.starts_with("0x"));
    EXPECT_NE(result, "nullptr");
    EXPECT_TRUE(uses_thread_local_allocator(result));
}

TEST_F(PointerFormatterTest, NullPointerFormatting)
{
    int* null_ptr = nullptr;
    auto result = value_formatter<int*>::format(null_ptr);

    EXPECT_EQ(result, "nullptr");
}

TEST_F(PointerFormatterTest, VoidPointerFormatting)
{
    int value = 123;
    void* void_ptr = &value;
    auto result = value_formatter<void*>::format(void_ptr);

    EXPECT_TRUE(result.starts_with("0x"));
    EXPECT_NE(result, "nullptr");
}

TEST_F(PointerFormatterTest, ConstPointerFormatting)
{
    const int value = 456;
    const int* const_ptr = &value;
    auto result = value_formatter<const int*>::format(const_ptr);

    EXPECT_TRUE(result.starts_with("0x"));
}

TEST_F(PointerFormatterTest, FunctionPointerFormatting)
{
    auto func_ptr = &std::strlen;
    auto result = value_formatter<decltype(func_ptr)>::format(func_ptr);

    EXPECT_TRUE(result.starts_with("0x"));
}

class OptionalFormatterTest : public ValueFormatterTest
{
};

TEST_F(OptionalFormatterTest, OptionalWithValueFormatting)
{
    std::optional<int> opt = 42;
    auto result = value_formatter<std::optional<int>>::format(opt);

    EXPECT_EQ(result, "some(42)");
    EXPECT_TRUE(uses_thread_local_allocator(result));
}

TEST_F(OptionalFormatterTest, EmptyOptionalFormatting)
{
    std::optional<int> opt;
    auto result = value_formatter<std::optional<int>>::format(opt);

    EXPECT_EQ(result, "none");
}

TEST_F(OptionalFormatterTest, OptionalWithStringFormatting)
{
    std::optional<std::string> opt = "test string";
    auto result = value_formatter<std::optional<std::string>>::format(opt);

    EXPECT_EQ(result, "some(\"test string\")");
}

TEST_F(OptionalFormatterTest, OptionalWithComplexTypeFormatting)
{
    std::optional<std::complex<double>> opt = std::complex<double>(1.0, 2.0);
    auto result =
        value_formatter<std::optional<std::complex<double>>>::format(opt);

    EXPECT_EQ(result, "some((1 + 2i))");
}

TEST_F(OptionalFormatterTest, NestedOptionalFormatting)
{
    std::optional<std::optional<int>> nested_opt = std::optional<int>(42);
    auto result =
        value_formatter<std::optional<std::optional<int>>>::format(nested_opt);

    EXPECT_EQ(result, "some(some(42))");
}
