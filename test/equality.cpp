#include <gtest/gtest.h>
#include <complex>
#include <limits>
#include <cassert>
#include <cmath>
#include <assertify/assertify.hpp>

using namespace assertify;
using namespace assertify::detail;

class AlmostEqualTest : public ::testing::Test {
protected:
};

TEST_F(AlmostEqualTest, FloatExactEquality) {
    float a = 1.0f, b = 1.0f;
    EXPECT_TRUE(almost_equal(a, b));
}

TEST_F(AlmostEqualTest, DoubleExactEquality) {
    double a = 42.0, b = 42.0;
    EXPECT_TRUE(almost_equal(a, b));
}

TEST_F(AlmostEqualTest, FloatRelativeTolerance) {
    float a = 1.0f, b = 1.0f + 1e-10f;
    EXPECT_TRUE(almost_equal(a, b));
}

TEST_F(AlmostEqualTest, DoubleAbsoluteTolerance) {
    double a = 1e-13, b = 0.0;
    detail::epsilon_config config;
    config.absolute_epsilon = 1e-12;
    config.relative_epsilon = 1e-9;
    EXPECT_TRUE(almost_equal(a, b, config));
}

TEST_F(AlmostEqualTest, DoubleNotEqual) {
    double a = 1.0, b = 1.1;
    EXPECT_FALSE(almost_equal(a, b));
}

TEST_F(AlmostEqualTest, FloatULPComparison) {
    float a = 1.0f;
    float b = std::nextafter(a, 2.0f); // 1 ULP away
    detail::epsilon_config config;
    config.use_ulp_comparison = true;
    config.max_ulp_difference = 2;
    EXPECT_TRUE(almost_equal(a, b, config));
}

TEST_F(AlmostEqualTest, FloatULPComparisonFail) {
    float a = 1.0f;
    float b = std::nextafter(std::nextafter(a, 2.0f), 2.0f); // 2 ULP away
    detail::epsilon_config config;
    config.use_ulp_comparison = true;
    config.max_ulp_difference = 1;
    EXPECT_FALSE(almost_equal(a, b, config));
}

TEST_F(AlmostEqualTest, ComplexExactEquality) {
    std::complex<double> a{1.0, 2.0}, b{1.0, 2.0};
    EXPECT_TRUE(almost_equal(a, b));
}

TEST_F(AlmostEqualTest, ComplexAlmostEqual) {
    std::complex<double> a{1.0, 2.0};
    std::complex<double> b{1.0 + 1e-10, 2.0 - 1e-10};
    EXPECT_TRUE(almost_equal(a, b));
}

TEST_F(AlmostEqualTest, ComplexNotEqual) {
    std::complex<double> a{1.0, 2.0};
    std::complex<double> b{1.1, 2.0};
    EXPECT_FALSE(almost_equal(a, b));
}

TEST_F(AlmostEqualTest, HandlesInfinityAndNaN) {
    double inf = std::numeric_limits<double>::infinity();
    double nan = std::numeric_limits<double>::quiet_NaN();
    EXPECT_FALSE(almost_equal(inf, 1.0));
    EXPECT_FALSE(almost_equal(nan, 1.0));
    EXPECT_TRUE(almost_equal(inf, inf));
    EXPECT_FALSE(almost_equal(nan, nan));
}