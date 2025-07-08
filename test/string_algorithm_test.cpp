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

class TokenizationTest
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
