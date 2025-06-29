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
        FAIL() << "Should not reach this catch block when catching as assertion_error";
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
        FAIL() << "Should not reach this catch block when catching as logic_error";
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
        FAIL() << "Should not reach this catch block when catching as exception";
    }

    EXPECT_TRUE(caught_as_assertion_error);
    EXPECT_TRUE(caught_as_logic_error);
    EXPECT_TRUE(caught_as_exception);
}