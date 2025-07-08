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
