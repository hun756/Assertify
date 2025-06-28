#include <gtest/gtest.h>
#include <assertify/assertify.hpp>

// dumy test case
TEST(AssertionTest, BasicAssertions) {
    int a = 5;
    int b = 10;

    ASSERT_EQ(a + b, 15) << "Sum of a and b should be 15";
    ASSERT_NE(a, b) << "a and b should not be equal";
    ASSERT_LT(a, b) << "a should be less than b";
    ASSERT_LE(a, 5) << "a should be less than or equal to 5";
    ASSERT_GT(b, a) << "b should be greater than a";
    ASSERT_GE(b, 10) << "b should be greater than or equal to 10";
}