#include <assertify/assertify.hpp>
#include <chrono>
#include <gtest/gtest.h>
#include <source_location>
#include <stacktrace>
#include <string>

namespace testing_utils
{

class AssertionErrorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    void TearDown() override {}

    bool is_timestamp_valid(
        const std::chrono::time_point<std::chrono::high_resolution_clock>&
            timestamp)
    {
        auto now = std::chrono::high_resolution_clock::now();
        return timestamp >= start_time_ && timestamp <= now;
    }

    std::uint_least32_t get_current_line()
    {
        return std::source_location::current().line();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time_;
};

} // namespace testing_utils

using namespace testing_utils;
using namespace assertify;

TEST_F(AssertionErrorTest, BasicConstructorWithMessage)
{
    constexpr auto test_message = "Test assertion failed";

    const assertion_error error(test_message);

    EXPECT_STREQ(error.what(), test_message);
    EXPECT_TRUE(error.context().empty());
    EXPECT_TRUE(is_timestamp_valid(error.timestamp()));
    EXPECT_FALSE(error.stack_trace().empty());
}

TEST_F(AssertionErrorTest, ConstructorWithMessageAndContext)
{
    constexpr auto test_message = "Test assertion failed";
    constexpr auto test_context = "Unit test context";

    const assertion_error error(test_message, std::source_location::current(),
                                test_context);

    EXPECT_STREQ(error.what(), test_message);
    EXPECT_EQ(error.context(), test_context);
    EXPECT_TRUE(is_timestamp_valid(error.timestamp()));
    EXPECT_FALSE(error.stack_trace().empty());
}

TEST_F(AssertionErrorTest, ConstructorWithEmptyMessage)
{
    const assertion_error error("");

    EXPECT_STREQ(error.what(), "");
    EXPECT_TRUE(error.context().empty());
    EXPECT_TRUE(is_timestamp_valid(error.timestamp()));
}

TEST_F(AssertionErrorTest, ConstructorWithEmptyContext)
{
    constexpr auto test_message = "Test message";

    const assertion_error error(test_message, std::source_location::current(),
                                "");

    EXPECT_STREQ(error.what(), test_message);
    EXPECT_TRUE(error.context().empty());
}

TEST_F(AssertionErrorTest, TemplateConstructorWithFormatting)
{
    const auto expected_location = std::source_location::current();
    const assertion_error error(expected_location, "format test",
                                "Value {} is not equal to expected {}", 42,
                                100);

    EXPECT_STREQ(error.what(), "Value 42 is not equal to expected 100");
    EXPECT_EQ(error.context(), "format test");
    EXPECT_EQ(error.where().line(), expected_location.line());
    EXPECT_TRUE(is_timestamp_valid(error.timestamp()));
}

TEST_F(AssertionErrorTest, TemplateConstructorWithComplexFormatting)
{
    const assertion_error error(
        std::source_location::current(), "complex format",
        "Test failed: expected={:.2f}, actual={:.2f}, tolerance={:.2e}",
        3.14159, 3.14, 1e-3);

    std::string error_message = error.what();
    EXPECT_TRUE(error_message.find("expected=3.14") != std::string::npos);
    EXPECT_TRUE(error_message.find("actual=3.14") != std::string::npos);
    EXPECT_TRUE(error_message.find("tolerance=1.00e-03") != std::string::npos);
}

TEST_F(AssertionErrorTest, InheritsFromLogicError)
{
    const assertion_error error("test");
    EXPECT_TRUE((std::is_base_of_v<std::logic_error, assertion_error>));

    const std::logic_error& base_ref = error;
    EXPECT_STREQ(base_ref.what(), "test");
}

TEST_F(AssertionErrorTest, CanBeCaughtAsStdException)
{
    bool caught_as_exception = false;
    bool caught_as_logic_error = false;
    bool caught_as_assertion_error = false;

    try
    {
        throw assertion_error("test exception");
    }
    catch (const assertion_error&)
    {
        caught_as_assertion_error = true;
    }
    catch (...)
    {
        FAIL() << "Should not reach this catch block when catching as "
                  "assertion_error";
    }

    try
    {
        throw assertion_error("test exception");
    }
    catch (const std::logic_error&)
    {
        caught_as_logic_error = true;
    }
    catch (...)
    {
        FAIL()
            << "Should not reach this catch block when catching as logic_error";
    }

    try
    {
        throw assertion_error("test exception");
    }
    catch (const std::exception&)
    {
        caught_as_exception = true;
    }
    catch (...)
    {
        FAIL()
            << "Should not reach this catch block when catching as exception";
    }

    EXPECT_TRUE(caught_as_assertion_error);
    EXPECT_TRUE(caught_as_logic_error);
    EXPECT_TRUE(caught_as_exception);
}

TEST_F(AssertionErrorTest, SourceLocationIsAccurate)
{
    const auto expected_line = std::source_location::current().line() + 1;
    const assertion_error error("test");

    const auto& location = error.where();
    EXPECT_EQ(location.line(), expected_line);

    std::string file_name = location.file_name();
    EXPECT_TRUE(file_name.find("test") != std::string::npos);

    std::string function_name = location.function_name();
    EXPECT_TRUE(function_name.find("SourceLocationIsAccurate") !=
                std::string::npos);
}

TEST_F(AssertionErrorTest, SourceLocationWithExplicitLocation)
{
    const auto custom_location = std::source_location::current();
    const assertion_error error("test", custom_location);

    const auto& location = error.where();
    EXPECT_EQ(location.line(), custom_location.line());
    EXPECT_STREQ(location.file_name(), custom_location.file_name());
    EXPECT_STREQ(location.function_name(), custom_location.function_name());
}

TEST_F(AssertionErrorTest, StackTraceIsNotEmpty)
{
    const assertion_error error("test");

    const auto& trace = error.stack_trace();
    EXPECT_FALSE(trace.empty());
    EXPECT_GT(trace.size(), 0);
}

TEST_F(AssertionErrorTest, StackTraceContainsCurrentFunction)
{
    const assertion_error error("test");

    const auto& trace = error.stack_trace();
    bool found_test_function = false;

    for (const auto& entry : trace)
    {
        auto desc = std::to_string(entry);
        if (desc.find("StackTraceContainsCurrentFunction") != std::string::npos)
        {
            found_test_function = true;
            break;
        }
    }

    EXPECT_TRUE(found_test_function)
        << "Stack trace should contain current test function";
}

TEST_F(AssertionErrorTest, TimestampIsReasonable)
{
    const auto before = std::chrono::high_resolution_clock::now();
    const assertion_error error("test");
    const auto after = std::chrono::high_resolution_clock::now();

    const auto timestamp = error.timestamp();
    EXPECT_GE(timestamp, before);
    EXPECT_LE(timestamp, after);
}

TEST_F(AssertionErrorTest, TimestampProgression)
{
    const assertion_error error1("first");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    const assertion_error error2("second");

    EXPECT_LT(error1.timestamp(), error2.timestamp());
}

TEST_F(AssertionErrorTest, ConstCorrectnessAndNoexcept)
{
    const assertion_error error("test");

    static_assert(noexcept(error.where()));
    static_assert(noexcept(error.stack_trace()));
    static_assert(noexcept(error.context()));
    static_assert(noexcept(error.timestamp()));

    [[maybe_unused]] const auto& location = error.where();
    [[maybe_unused]] const auto& trace = error.stack_trace();
    [[maybe_unused]] const auto& context = error.context();
    [[maybe_unused]] const auto& timestamp = error.timestamp();
    [[maybe_unused]] const auto detailed = error.detailed_message();
}

TEST_F(AssertionErrorTest, VeryLongMessage)
{
    const std::string long_message(10000, 'A');
    const assertion_error error(long_message);

    EXPECT_EQ(std::string(error.what()), long_message);
    EXPECT_TRUE(is_timestamp_valid(error.timestamp()));
}

TEST_F(AssertionErrorTest, SpecialCharactersInMessage)
{
    constexpr auto special_message = "Error with special chars: \n\t\r\0\x1F";
    const assertion_error error(special_message);

    EXPECT_STREQ(error.what(), special_message);
}

TEST_F(AssertionErrorTest, UnicodeInMessage)
{
    constexpr auto unicode_message = "Error: 测试 🚀 Ελληνικά";
    const assertion_error error(unicode_message);

    EXPECT_STREQ(error.what(), unicode_message);
}

TEST_F(AssertionErrorTest, MoveSemantics)
{
    auto create_error = []()
    {
        return assertion_error("moveable error",
                               std::source_location::current(), "move context");
    };

    const auto error = create_error();

    EXPECT_STREQ(error.what(), "moveable error");
    EXPECT_EQ(error.context(), "move context");
    EXPECT_TRUE(is_timestamp_valid(error.timestamp()));
}
