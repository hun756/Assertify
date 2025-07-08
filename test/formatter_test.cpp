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

class ContainerFormatterTest : public ValueFormatterTest
{
};

TEST_F(ContainerFormatterTest, VectorFormattingBasic)
{
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto result = value_formatter<std::vector<int>>::format(vec);

    EXPECT_EQ(result, "[1, 2, 3, 4, 5]");
    EXPECT_TRUE(uses_thread_local_allocator(result));
}

TEST_F(ContainerFormatterTest, EmptyVectorFormatting)
{
    std::vector<int> empty_vec;
    auto result = value_formatter<std::vector<int>>::format(empty_vec);

    EXPECT_EQ(result, "[]");
}

TEST_F(ContainerFormatterTest, SingleElementVectorFormatting)
{
    std::vector<int> single_vec = {42};
    auto result = value_formatter<std::vector<int>>::format(single_vec);

    EXPECT_EQ(result, "[42]");
}

TEST_F(ContainerFormatterTest, LargeVectorFormattingTruncation)
{
    std::vector<int> large_vec;
    for (int i = 1; i <= 15; ++i)
    {
        large_vec.push_back(i);
    }

    auto result = value_formatter<std::vector<int>>::format(large_vec);

    EXPECT_TRUE(result.find("1, 2, 3, 4, 5, 6, 7, 8, 9, 10, ...") !=
                std::string::npos);
    EXPECT_TRUE(result.ends_with("]"));
}

TEST_F(ContainerFormatterTest, VectorOfStringsFormatting)
{
    std::vector<std::string> vec_str = {"hello", "world", "test"};
    auto result = value_formatter<std::vector<std::string>>::format(vec_str);

    EXPECT_EQ(result, "[\"hello\", \"world\", \"test\"]");
}

TEST_F(ContainerFormatterTest, ArrayFormattingStd)
{
    std::array<int, 4> arr = {10, 20, 30, 40};
    auto result = value_formatter<std::array<int, 4>>::format(arr);

    EXPECT_EQ(result, "[10, 20, 30, 40]");
}

TEST_F(ContainerFormatterTest, ListFormatting)
{
    std::list<double> lst = {1.1, 2.2, 3.3};
    auto result = value_formatter<std::list<double>>::format(lst);

    EXPECT_EQ(result, "[1.1, 2.2, 3.3]");
}

TEST_F(ContainerFormatterTest, FastVectorFormatting)
{
    fast_vector<int> fast_vec(tl_pool.get_allocator<int>());
    fast_vec.push_back(100);
    fast_vec.push_back(200);
    fast_vec.push_back(300);

    auto result = value_formatter<fast_vector<int>>::format(fast_vec);

    EXPECT_EQ(result, "[100, 200, 300]");
    EXPECT_TRUE(uses_thread_local_allocator(result));
}

TEST_F(ContainerFormatterTest, NestedContainerFormatting)
{
    std::vector<std::vector<int>> nested = {{1, 2}, {3, 4}, {5, 6}};
    auto result =
        value_formatter<std::vector<std::vector<int>>>::format(nested);

    EXPECT_EQ(result, "[[1, 2], [3, 4], [5, 6]]");
}

TEST_F(ContainerFormatterTest, ContainerOfComplexTypesFormatting)
{
    std::vector<std::complex<double>> complex_vec = {
        {1.0, 2.0}, {3.0, -1.0}, {0.0, 5.0}};
    auto result =
        value_formatter<std::vector<std::complex<double>>>::format(complex_vec);

    EXPECT_EQ(result, "[(1 + 2i), (3 + -1i), (0 + 5i)]");
}

TEST_F(ContainerFormatterTest, SpanFormatting)
{
    std::array<int, 5> arr = {1, 2, 3, 4, 5};
    std::span<int> span_view(arr);
    auto result = value_formatter<std::span<int>>::format(span_view);

    EXPECT_EQ(result, "[1, 2, 3, 4, 5]");
}

class VariantFormatterTest : public ValueFormatterTest
{
};

TEST_F(VariantFormatterTest, VariantFormattingFirstType)
{
    std::variant<int, std::string> var = 42;
    auto result = value_formatter<std::variant<int, std::string>>::format(var);

    EXPECT_EQ(result, "variant<index:0>");
    EXPECT_TRUE(uses_thread_local_allocator(result));
}

TEST_F(VariantFormatterTest, VariantFormattingSecondType)
{
    std::variant<int, std::string> var = std::string("hello");
    auto result = value_formatter<std::variant<int, std::string>>::format(var);

    EXPECT_EQ(result, "variant<index:1>");
}

TEST_F(VariantFormatterTest, VariantFormattingComplexTypes)
{
    std::variant<int, double, std::string, std::vector<int>> var =
        std::vector<int>{1, 2, 3};
    auto result = value_formatter<
        std::variant<int, double, std::string, std::vector<int>>>::format(var);

    EXPECT_EQ(result, "variant<index:3>");
}

class EnumFormatterTest : public ValueFormatterTest
{
};

TEST_F(EnumFormatterTest, ScopedEnumFormatting)
{
    TestEnum enum_val = TestEnum::VALUE3;
    auto result = value_formatter<TestEnum>::format(enum_val);

    EXPECT_EQ(result, "enum(42)");
    EXPECT_TRUE(uses_thread_local_allocator(result));
}

TEST_F(EnumFormatterTest, UnscopedEnumFormatting)
{
    TestEnumClass enum_val = TestEnumClass::C;
    auto result = value_formatter<TestEnumClass>::format(enum_val);

    EXPECT_EQ(result, "enum(255)");
}

TEST_F(EnumFormatterTest, EnumWithDifferentUnderlyingTypes)
{
    TestEnum enum1 = TestEnum::VALUE1;
    TestEnumClass enum2 = TestEnumClass::A;

    auto result1 = value_formatter<TestEnum>::format(enum1);
    auto result2 = value_formatter<TestEnumClass>::format(enum2);

    EXPECT_EQ(result1, "enum(10)");
    EXPECT_EQ(result2, "enum(1)");
}

class StreamableFormatterTest : public ValueFormatterTest
{
};

TEST_F(StreamableFormatterTest, CustomStreamableTypeFormatting)
{
    StreamableTestType obj{42};
    auto result = value_formatter<StreamableTestType>::format(obj);

    EXPECT_EQ(result, "StreamableTestType{42}");
    EXPECT_TRUE(uses_thread_local_allocator(result));
}

TEST_F(StreamableFormatterTest, StreamableTypeWithSpecialCharacters)
{
    StreamableTestType obj{-123};
    auto result = value_formatter<StreamableTestType>::format(obj);

    EXPECT_EQ(result, "StreamableTestType{-123}");
}

class NonStreamableFormatterTest : public ValueFormatterTest
{
};

TEST_F(NonStreamableFormatterTest, NonStreamableTypeFormatting)
{
    NonStreamableType obj;
    auto result = value_formatter<NonStreamableType>::format(obj);

    EXPECT_TRUE(result.starts_with("object<"));
    EXPECT_TRUE(result.ends_with(">"));
    EXPECT_TRUE(uses_thread_local_allocator(result));
}

TEST_F(NonStreamableFormatterTest, TypeNameInclusion)
{
    NonStreamableType obj;
    auto result = value_formatter<NonStreamableType>::format(obj);

    EXPECT_GT(result.length(), 8);
}

class FormatValueFunctionTest : public ValueFormatterTest
{
};

TEST_F(FormatValueFunctionTest, BasicFormatValueUsage)
{
    int value = 42;
    auto result = format_value(value);

    EXPECT_EQ(result, "42");
    EXPECT_TRUE(uses_thread_local_allocator(result));
}

TEST_F(FormatValueFunctionTest, FormatValueWithString)
{
    std::string str = "test";
    auto result = format_value(str);

    EXPECT_EQ(result, "\"test\"");
}

TEST_F(FormatValueFunctionTest, FormatValueWithContainer)
{
    std::vector<int> vec = {1, 2, 3};
    auto result = format_value(vec);

    EXPECT_EQ(result, "[1, 2, 3]");
}

TEST_F(FormatValueFunctionTest, FormatValueExceptionHandling)
{
    ThrowingStreamableType throwing_obj;
    auto result = format_value(throwing_obj);

    EXPECT_EQ(result, "unprintable");
    EXPECT_TRUE(uses_thread_local_allocator(result));
}

TEST_F(FormatValueFunctionTest, NoexceptGuarantee)
{
    static_assert(noexcept(format_value(std::declval<int>())));

    int value = 123;
    auto result = format_value(value);
    EXPECT_NO_THROW((void)result);
}

class MemoryManagementFormatterTest : public ValueFormatterTest
{
};

TEST_F(MemoryManagementFormatterTest, ThreadLocalPoolUsage)
{
    std::string test_str = "memory test";
    auto result = format_value(test_str);

    EXPECT_TRUE(uses_thread_local_allocator(result));
    EXPECT_EQ(result, "\"memory test\"");
}

TEST_F(MemoryManagementFormatterTest, MemoryPoolResetBehavior)
{

    auto result1 = format_value(std::string("test1"));
    auto result2 = format_value(std::vector<int>{1, 2, 3});

    EXPECT_GT(tl_pool.active_allocation_count(), 0);

    tl_pool.reset();
    EXPECT_EQ(tl_pool.active_allocation_count(), 0);

    auto result3 = format_value(std::string("test3"));
    EXPECT_EQ(result3, "\"test3\"");
}

TEST_F(MemoryManagementFormatterTest, LargeDataFormatting)
{

    std::vector<int> large_vec(1000, 42);
    auto result = format_value(large_vec);

    EXPECT_TRUE(result.starts_with("["));
    EXPECT_TRUE(result.ends_with("]"));
    EXPECT_TRUE(result.find("...") != std::string::npos);
}
