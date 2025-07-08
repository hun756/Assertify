#include <assertify/assertify.hpp>
#include <chrono>
#include <gtest/gtest.h>
#include <random>
#include <string>
#include <vector>

namespace testing_utils
{

class StringAlgorithmsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        empty_string_ = "";
        single_char_ = "a";
        short_string_ = "cat";
        medium_string_ = "kitten";
        long_string_ = "The quick brown fox jumps over the lazy dog";
        unicode_string_ = "café München 北京";
        repeated_chars_ = "aaaaaaa";
        numeric_string_ = "12345";
        special_chars_ = "!@#$%^&*()";
        mixed_case_ = "Hello World";

        generate_random_strings();
    }

    void generate_random_strings()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> char_dist('a', 'z');

        for (size_t size : {10u, 100u, 1000u})
        {
            std::string random_str;
            random_str.reserve(size);
            for (size_t i = 0; i < size; ++i)
            {
                random_str += static_cast<char>(char_dist(gen));
            }
            random_strings_.push_back(std::move(random_str));
        }
    }

    template <typename Func>
    std::chrono::microseconds measure_time(Func&& func)
    {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                     start);
    }

    std::string empty_string_;
    std::string single_char_;
    std::string short_string_;
    std::string medium_string_;
    std::string long_string_;
    std::string unicode_string_;
    std::string repeated_chars_;
    std::string numeric_string_;
    std::string special_chars_;
    std::string mixed_case_;
    std::vector<std::string> random_strings_;
};

class StringPairTest
    : public StringAlgorithmsTest,
      public ::testing::WithParamInterface<std::pair<std::string, std::string>>
{
};

class ParamTokenizationTest
    : public StringAlgorithmsTest,
      public ::testing::WithParamInterface<
          std::tuple<std::string, char, std::vector<std::string>>>
{
};
} // namespace testing_utils

using namespace testing_utils;
using namespace assertify;
using namespace assertify::detail;

class EditDistanceTest : public StringAlgorithmsTest
{
};

TEST_F(EditDistanceTest, IdenticalStrings)
{
    EXPECT_EQ(string_algorithms::edit_distance("", ""), 0);
    EXPECT_EQ(string_algorithms::edit_distance("a", "a"), 0);
    EXPECT_EQ(string_algorithms::edit_distance("hello", "hello"), 0);
    EXPECT_EQ(string_algorithms::edit_distance("identical", "identical"), 0);
    EXPECT_EQ(string_algorithms::edit_distance(long_string_, long_string_), 0);
}

TEST_F(EditDistanceTest, EmptyStrings)
{
    EXPECT_EQ(string_algorithms::edit_distance("", ""), 0);
    EXPECT_EQ(string_algorithms::edit_distance("", "a"), 1);
    EXPECT_EQ(string_algorithms::edit_distance("a", ""), 1);
    EXPECT_EQ(string_algorithms::edit_distance("", "hello"), 5);
    EXPECT_EQ(string_algorithms::edit_distance("world", ""), 5);
}

TEST_F(EditDistanceTest, SingleCharacterOperations)
{

    EXPECT_EQ(string_algorithms::edit_distance("a", "b"), 1);
    EXPECT_EQ(string_algorithms::edit_distance("x", "y"), 1);

    EXPECT_EQ(string_algorithms::edit_distance("", "a"), 1);
    EXPECT_EQ(string_algorithms::edit_distance("a", "ab"), 1);

    EXPECT_EQ(string_algorithms::edit_distance("a", ""), 1);
    EXPECT_EQ(string_algorithms::edit_distance("ab", "a"), 1);
}

TEST_F(EditDistanceTest, ClassicExamples)
{

    EXPECT_EQ(string_algorithms::edit_distance("kitten", "sitting"), 3);
    EXPECT_EQ(string_algorithms::edit_distance("saturday", "sunday"), 3);
    EXPECT_EQ(string_algorithms::edit_distance("cat", "dog"), 3);
    EXPECT_EQ(string_algorithms::edit_distance("intention", "execution"), 5);
}

TEST_F(EditDistanceTest, CompletelyDifferentStrings)
{
    EXPECT_EQ(string_algorithms::edit_distance("abc", "def"), 3);
    EXPECT_EQ(string_algorithms::edit_distance("hello", "world"), 4);
    EXPECT_EQ(string_algorithms::edit_distance("12345", "abcde"), 5);
}

TEST_F(EditDistanceTest, DifferentLengths)
{
    EXPECT_EQ(string_algorithms::edit_distance("a", "abc"), 2);
    EXPECT_EQ(string_algorithms::edit_distance("abc", "a"), 2);
    EXPECT_EQ(string_algorithms::edit_distance("short", "very long string"),
              14);
    EXPECT_EQ(string_algorithms::edit_distance("very long string", "short"),
              14);
}

TEST_F(EditDistanceTest, RepeatedCharacters)
{
    EXPECT_EQ(string_algorithms::edit_distance("aaa", "aa"), 1);
    EXPECT_EQ(string_algorithms::edit_distance("aa", "aaa"), 1);
    EXPECT_EQ(string_algorithms::edit_distance("aaaa", "bbbb"), 4);
    EXPECT_EQ(string_algorithms::edit_distance("ababab", "bababa"), 2);
}

TEST_F(EditDistanceTest, UnicodeStrings)
{

    EXPECT_EQ(string_algorithms::edit_distance("café", "cafe"), 2);
    EXPECT_EQ(string_algorithms::edit_distance("München", "Munchen"), 2);
    EXPECT_EQ(string_algorithms::edit_distance("北京", "东京"), 3);
}

TEST_F(EditDistanceTest, SpecialCharacters)
{
    EXPECT_EQ(string_algorithms::edit_distance("!@#", "$%^"), 3);
    EXPECT_EQ(string_algorithms::edit_distance("a!b@c#", "a$b%c^"), 3);
    EXPECT_EQ(string_algorithms::edit_distance("()", "[]"), 2);
}

TEST_F(EditDistanceTest, CaseSensitivity)
{
    EXPECT_EQ(string_algorithms::edit_distance("Hello", "hello"), 1);
    EXPECT_EQ(string_algorithms::edit_distance("WORLD", "world"), 5);
    EXPECT_EQ(string_algorithms::edit_distance("MiXeD", "mixed"), 3);
}

TEST_F(EditDistanceTest, Whitespace)
{
    EXPECT_EQ(string_algorithms::edit_distance("hello world", "hello  world"),
              1);
    EXPECT_EQ(string_algorithms::edit_distance("no spaces", "nospaces"), 1);
    EXPECT_EQ(string_algorithms::edit_distance("  trim  ", "trim"), 4);
}

class HammingDistanceTest : public StringAlgorithmsTest
{
};

TEST_F(HammingDistanceTest, IdenticalStrings)
{
    EXPECT_EQ(string_algorithms::hamming_distance("", ""), 0);
    EXPECT_EQ(string_algorithms::hamming_distance("a", "a"), 0);
    EXPECT_EQ(string_algorithms::hamming_distance("hello", "hello"), 0);
    EXPECT_EQ(string_algorithms::hamming_distance("identical", "identical"), 0);
}

TEST_F(HammingDistanceTest, UnequalLengths)
{

    EXPECT_EQ(string_algorithms::hamming_distance("a", "ab"),
              std::numeric_limits<std::size_t>::max());
    EXPECT_EQ(string_algorithms::hamming_distance("short", "longer"),
              std::numeric_limits<std::size_t>::max());
    EXPECT_EQ(string_algorithms::hamming_distance("", "a"),
              std::numeric_limits<std::size_t>::max());
    EXPECT_EQ(string_algorithms::hamming_distance("hello", "hi"),
              std::numeric_limits<std::size_t>::max());
}

TEST_F(HammingDistanceTest, SingleDifferences)
{
    EXPECT_EQ(string_algorithms::hamming_distance("a", "b"), 1);
    EXPECT_EQ(string_algorithms::hamming_distance("cat", "bat"), 1);
    EXPECT_EQ(string_algorithms::hamming_distance("hello", "hallo"), 1);
    EXPECT_EQ(string_algorithms::hamming_distance("world", "worle"), 1);
}

TEST_F(HammingDistanceTest, MultipleDifferences)
{
    EXPECT_EQ(string_algorithms::hamming_distance("abc", "def"), 3);
    EXPECT_EQ(string_algorithms::hamming_distance("hello", "world"), 4);
    EXPECT_EQ(string_algorithms::hamming_distance("12345", "54321"), 4);
    EXPECT_EQ(string_algorithms::hamming_distance("aaaaa", "bbbbb"), 5);
}

TEST_F(HammingDistanceTest, BinaryStrings)
{
    EXPECT_EQ(string_algorithms::hamming_distance("1011101", "1001001"), 2);
    EXPECT_EQ(string_algorithms::hamming_distance("0000", "1111"), 4);
    EXPECT_EQ(string_algorithms::hamming_distance("101010", "010101"), 6);
}

TEST_F(HammingDistanceTest, NumericStrings)
{
    EXPECT_EQ(string_algorithms::hamming_distance("123", "124"), 1);
    EXPECT_EQ(string_algorithms::hamming_distance("999", "000"), 3);
    EXPECT_EQ(string_algorithms::hamming_distance("12345", "67890"), 5);
}

TEST_F(HammingDistanceTest, CaseSensitive)
{
    EXPECT_EQ(string_algorithms::hamming_distance("Hello", "hello"), 1);
    EXPECT_EQ(string_algorithms::hamming_distance("WORLD", "world"), 5);
    EXPECT_EQ(string_algorithms::hamming_distance("MiXeD", "mIxEd"), 5);
}

TEST_F(HammingDistanceTest, SpecialCharacters)
{
    EXPECT_EQ(string_algorithms::hamming_distance("!@#", "$%^"), 3);
    EXPECT_EQ(string_algorithms::hamming_distance("a!b", "a@b"), 1);
    EXPECT_EQ(string_algorithms::hamming_distance("()[]", "{}||"), 4);
}

class FuzzyMatchRatioTest : public StringAlgorithmsTest
{
};

TEST_F(FuzzyMatchRatioTest, IdenticalStrings)
{
    EXPECT_DOUBLE_EQ(string_algorithms::fuzzy_match_ratio("", ""), 1.0);
    EXPECT_DOUBLE_EQ(string_algorithms::fuzzy_match_ratio("a", "a"), 1.0);
    EXPECT_DOUBLE_EQ(string_algorithms::fuzzy_match_ratio("hello", "hello"),
                     1.0);
    EXPECT_DOUBLE_EQ(
        string_algorithms::fuzzy_match_ratio("identical", "identical"), 1.0);
}

TEST_F(FuzzyMatchRatioTest, EmptyStrings)
{
    EXPECT_DOUBLE_EQ(string_algorithms::fuzzy_match_ratio("", ""), 1.0);
    EXPECT_DOUBLE_EQ(string_algorithms::fuzzy_match_ratio("", "a"), 0.0);
    EXPECT_DOUBLE_EQ(string_algorithms::fuzzy_match_ratio("a", ""), 0.0);
    EXPECT_DOUBLE_EQ(string_algorithms::fuzzy_match_ratio("", "hello"), 0.0);
    EXPECT_DOUBLE_EQ(string_algorithms::fuzzy_match_ratio("world", ""), 0.0);
}

TEST_F(FuzzyMatchRatioTest, RatioCalculation)
{

    EXPECT_NEAR(string_algorithms::fuzzy_match_ratio("cat", "bat"), 2.0 / 3.0,
                1e-10);

    EXPECT_NEAR(string_algorithms::fuzzy_match_ratio("kitten", "sitting"),
                4.0 / 7.0, 1e-10);

    EXPECT_NEAR(string_algorithms::fuzzy_match_ratio("hello", "world"),
                1.0 / 5.0, 1e-10);

    EXPECT_NEAR(string_algorithms::fuzzy_match_ratio("MiXeD", "mixed"),
                2.0 / 5.0, 1e-10);
}

TEST_F(FuzzyMatchRatioTest, CompletelyDifferentStrings)
{

    EXPECT_NEAR(string_algorithms::fuzzy_match_ratio("abc", "def"), 0.0, 1e-10);
    EXPECT_NEAR(string_algorithms::fuzzy_match_ratio("12345", "abcde"), 0.0,
                1e-10);
}

TEST_F(FuzzyMatchRatioTest, PartialMatches)
{

    auto ratio1 = string_algorithms::fuzzy_match_ratio("test", "best");
    auto ratio2 = string_algorithms::fuzzy_match_ratio("test", "fest");
    auto ratio3 = string_algorithms::fuzzy_match_ratio("test", "rest");

    EXPECT_GT(ratio1, 0.5);
    EXPECT_GT(ratio2, 0.5);
    EXPECT_GT(ratio3, 0.5);

    EXPECT_DOUBLE_EQ(ratio1, ratio2);
    EXPECT_DOUBLE_EQ(ratio2, ratio3);
}

TEST_F(FuzzyMatchRatioTest, DifferentLengths)
{

    auto ratio1 = string_algorithms::fuzzy_match_ratio("a", "abc");
    auto ratio2 = string_algorithms::fuzzy_match_ratio("abc", "a");

    EXPECT_DOUBLE_EQ(ratio1, ratio2);
    EXPECT_NEAR(ratio1, 1.0 / 3.0, 1e-10);
}

TEST_F(FuzzyMatchRatioTest, RangeValidation)
{

    std::vector<std::pair<std::string, std::string>> test_pairs = {
        {"", "test"},
        {"test", ""},
        {"hello", "world"},
        {"abc", "def"},
        {"similar", "similiar"},
        {"longer string", "short"},
        {"123", "abc"}};

    for (const auto& [s1, s2] : test_pairs)
    {
        double ratio = string_algorithms::fuzzy_match_ratio(s1, s2);
        EXPECT_GE(ratio, 0.0)
            << "Ratio should be >= 0 for '" << s1 << "' and '" << s2 << "'";
        EXPECT_LE(ratio, 1.0)
            << "Ratio should be <= 1 for '" << s1 << "' and '" << s2 << "'";
    }
}

TEST_F(FuzzyMatchRatioTest, Symmetry)
{

    std::vector<std::pair<std::string, std::string>> test_pairs = {
        {"hello", "world"},
        {"kitten", "sitting"},
        {"cat", "dog"},
        {"short", "longer string"},
        {"abc", "123"}};

    for (const auto& [s1, s2] : test_pairs)
    {
        double ratio1 = string_algorithms::fuzzy_match_ratio(s1, s2);
        double ratio2 = string_algorithms::fuzzy_match_ratio(s2, s1);
        EXPECT_DOUBLE_EQ(ratio1, ratio2) << "Ratio should be symmetric for '"
                                         << s1 << "' and '" << s2 << "'";
    }
}

class TokenizationTest : public StringAlgorithmsTest
{
};

TEST_F(TokenizationTest, EmptyString)
{
    auto tokens = string_algorithms::tokenize("");
    EXPECT_TRUE(tokens.empty());

    auto tokens_comma = string_algorithms::tokenize("", ',');
    EXPECT_TRUE(tokens_comma.empty());
}

TEST_F(TokenizationTest, SingleToken)
{
    auto tokens = string_algorithms::tokenize("hello");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0], "hello");

    auto tokens_comma = string_algorithms::tokenize("world", ',');
    ASSERT_EQ(tokens_comma.size(), 1);
    EXPECT_EQ(tokens_comma[0], "world");
}

TEST_F(TokenizationTest, MultipleTokensSpace)
{
    auto tokens = string_algorithms::tokenize("hello world test");
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
    EXPECT_EQ(tokens[2], "test");
}

TEST_F(TokenizationTest, MultipleTokensComma)
{
    auto tokens = string_algorithms::tokenize("apple,banana,cherry", ',');
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "apple");
    EXPECT_EQ(tokens[1], "banana");
    EXPECT_EQ(tokens[2], "cherry");
}

TEST_F(TokenizationTest, LeadingTrailingDelimiters)
{
    auto tokens_leading = string_algorithms::tokenize(" hello world");
    ASSERT_EQ(tokens_leading.size(), 2);
    EXPECT_EQ(tokens_leading[0], "hello");
    EXPECT_EQ(tokens_leading[1], "world");

    auto tokens_trailing = string_algorithms::tokenize("hello world ");
    ASSERT_EQ(tokens_trailing.size(), 2);
    EXPECT_EQ(tokens_trailing[0], "hello");
    EXPECT_EQ(tokens_trailing[1], "world");

    auto tokens_both = string_algorithms::tokenize(" hello world ");
    ASSERT_EQ(tokens_both.size(), 2);
    EXPECT_EQ(tokens_both[0], "hello");
    EXPECT_EQ(tokens_both[1], "world");
}

TEST_F(TokenizationTest, ConsecutiveDelimiters)
{
    auto tokens_spaces = string_algorithms::tokenize("hello  world   test");
    ASSERT_EQ(tokens_spaces.size(), 3);
    EXPECT_EQ(tokens_spaces[0], "hello");
    EXPECT_EQ(tokens_spaces[1], "world");
    EXPECT_EQ(tokens_spaces[2], "test");

    auto tokens_commas = string_algorithms::tokenize("a,,b,,,c", ',');
    ASSERT_EQ(tokens_commas.size(), 3);
    EXPECT_EQ(tokens_commas[0], "a");
    EXPECT_EQ(tokens_commas[1], "b");
    EXPECT_EQ(tokens_commas[2], "c");
}

TEST_F(TokenizationTest, OnlyDelimiters)
{
    auto tokens_spaces = string_algorithms::tokenize("   ");
    EXPECT_TRUE(tokens_spaces.empty());

    auto tokens_commas = string_algorithms::tokenize(",,,", ',');
    EXPECT_TRUE(tokens_commas.empty());
}

TEST_F(TokenizationTest, DifferentDelimiters)
{
    auto tokens_semicolon = string_algorithms::tokenize("a;b;c", ';');
    ASSERT_EQ(tokens_semicolon.size(), 3);
    EXPECT_EQ(tokens_semicolon[0], "a");
    EXPECT_EQ(tokens_semicolon[1], "b");
    EXPECT_EQ(tokens_semicolon[2], "c");

    auto tokens_pipe = string_algorithms::tokenize("x|y|z", '|');
    ASSERT_EQ(tokens_pipe.size(), 3);
    EXPECT_EQ(tokens_pipe[0], "x");
    EXPECT_EQ(tokens_pipe[1], "y");
    EXPECT_EQ(tokens_pipe[2], "z");

    auto tokens_tab = string_algorithms::tokenize("1\t2\t3", '\t');
    ASSERT_EQ(tokens_tab.size(), 3);
    EXPECT_EQ(tokens_tab[0], "1");
    EXPECT_EQ(tokens_tab[1], "2");
    EXPECT_EQ(tokens_tab[2], "3");
}

TEST_F(TokenizationTest, ComplexStrings)
{
    auto tokens = string_algorithms::tokenize("The quick brown fox jumps");
    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0], "The");
    EXPECT_EQ(tokens[1], "quick");
    EXPECT_EQ(tokens[2], "brown");
    EXPECT_EQ(tokens[3], "fox");
    EXPECT_EQ(tokens[4], "jumps");

    auto csv_tokens = string_algorithms::tokenize("name,age,city,country", ',');
    ASSERT_EQ(csv_tokens.size(), 4);
    EXPECT_EQ(csv_tokens[0], "name");
    EXPECT_EQ(csv_tokens[1], "age");
    EXPECT_EQ(csv_tokens[2], "city");
    EXPECT_EQ(csv_tokens[3], "country");
}

TEST_F(TokenizationTest, UnicodeStrings)
{
    auto tokens = string_algorithms::tokenize("café München 北京");
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "café");
    EXPECT_EQ(tokens[1], "München");
    EXPECT_EQ(tokens[2], "北京");
}

TEST_F(TokenizationTest, SpecialCharactersInTokens)
{
    auto tokens = string_algorithms::tokenize("hello@world #test $money", ' ');
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "hello@world");
    EXPECT_EQ(tokens[1], "#test");
    EXPECT_EQ(tokens[2], "$money");
}

TEST_F(TokenizationTest, StringViewValidity)
{

    std::string source = "apple,banana,cherry";
    auto tokens = string_algorithms::tokenize(source, ',');

    ASSERT_EQ(tokens.size(), 3);

    EXPECT_EQ(tokens[0].data(), source.data());
    EXPECT_EQ(tokens[1].data(), source.data() + 6);
    EXPECT_EQ(tokens[2].data(), source.data() + 13);

    EXPECT_EQ(tokens[0].length(), 5);
    EXPECT_EQ(tokens[1].length(), 6);
    EXPECT_EQ(tokens[2].length(), 6);
}
