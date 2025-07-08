#include <assertify/assertify.hpp>
#include <deque>
#include <gtest/gtest.h>
#include <random>
#include <vector>

namespace testing_utils
{

class StatisticalAnalyzerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        small_data_ = {1.0, 2.0, 3.0, 4.0, 5.0};
        large_data_ = generate_large_dataset(1000);
        normal_data_ = generate_normal_distribution(100, 50.0, 10.0);
        integer_data_ = {10, 20, 30, 40, 50};
        mixed_data_ = {-5.5, 0.0, 2.3, 7.8, 15.2};
        single_element_ = {42.0};
        identical_elements_ = {5.0, 5.0, 5.0, 5.0, 5.0};
    }

    bool almost_equal(double a, double b, double tolerance = 1e-9)
    {
        return std::abs(a - b) < tolerance;
    }

    std::vector<double> generate_large_dataset(size_t size)
    {
        std::vector<double> data;
        data.reserve(size);
        for (size_t i = 0; i < size; ++i)
        {
            data.push_back(static_cast<double>(i + 1));
        }
        return data;
    }

    std::vector<double> generate_normal_distribution(size_t size, double mean,
                                                     double stddev)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<double> dist(mean, stddev);

        std::vector<double> data;
        data.reserve(size);
        for (size_t i = 0; i < size; ++i)
        {
            data.push_back(dist(gen));
        }
        return data;
    }

    std::vector<double> small_data_;
    std::vector<double> large_data_;
    std::vector<double> normal_data_;
    std::vector<int> integer_data_;
    std::vector<double> mixed_data_;
    std::vector<double> single_element_;
    std::vector<double> identical_elements_;
    std::vector<double> empty_data_;
};

template <typename T>
class StatisticalAnalyzerContainerTest : public StatisticalAnalyzerTest
{
public:
    using container_type = T;
};

using ContainerTypes =
    ::testing::Types<std::vector<double>, std::list<double>, std::deque<double>,
                     std::array<double, 5>>;
} // namespace testing_utils

using namespace testing_utils;
using namespace assertify;
using namespace assertify::detail;

class MeanTest : public StatisticalAnalyzerTest
{
};

TEST_F(MeanTest, BasicMeanCalculation)
{
    double result = statistical_analyzer::mean(small_data_);
    EXPECT_DOUBLE_EQ(result, 3.0);
}

TEST_F(MeanTest, EmptyContainerMean)
{
    double result = statistical_analyzer::mean(empty_data_);
    EXPECT_DOUBLE_EQ(result, 0.0);
}

TEST_F(MeanTest, SingleElementMean)
{
    double result = statistical_analyzer::mean(single_element_);
    EXPECT_DOUBLE_EQ(result, 42.0);
}

TEST_F(MeanTest, IdenticalElementsMean)
{
    double result = statistical_analyzer::mean(identical_elements_);
    EXPECT_DOUBLE_EQ(result, 5.0);
}

TEST_F(MeanTest, IntegerDataMean)
{
    double result = statistical_analyzer::mean(integer_data_);
    EXPECT_DOUBLE_EQ(result, 30.0);
}

TEST_F(MeanTest, MixedPositiveNegativeMean)
{
    double result = statistical_analyzer::mean(mixed_data_);
    double expected = (-5.5 + 0.0 + 2.3 + 7.8 + 15.2) / 5.0;
    EXPECT_DOUBLE_EQ(result, expected);
}

TEST_F(MeanTest, LargeDatasetMean)
{
    double result = statistical_analyzer::mean(large_data_);
    double expected = 500.5;
    EXPECT_DOUBLE_EQ(result, expected);
}

TEST_F(MeanTest, DifferentContainerTypes)
{
    std::list<double> list_data(small_data_.begin(), small_data_.end());
    std::deque<double> deque_data(small_data_.begin(), small_data_.end());
    std::array<double, 5> array_data = {1.0, 2.0, 3.0, 4.0, 5.0};

    EXPECT_DOUBLE_EQ(statistical_analyzer::mean(list_data), 3.0);
    EXPECT_DOUBLE_EQ(statistical_analyzer::mean(deque_data), 3.0);
    EXPECT_DOUBLE_EQ(statistical_analyzer::mean(array_data), 3.0);
}

TEST_F(MeanTest, RangeViewsMean)
{
    auto even_numbers =
        small_data_ |
        std::views::filter([](double x)
                           { return static_cast<int>(x) % 2 == 0; });
    double result = statistical_analyzer::mean(even_numbers);
    EXPECT_DOUBLE_EQ(result, 3.0);
}

TEST_F(MeanTest, ExtremeValues)
{
    std::vector<double> extreme_data = {std::numeric_limits<double>::max(),
                                        std::numeric_limits<double>::lowest(),
                                        0.0};

    double result = statistical_analyzer::mean(extreme_data);
    EXPECT_FALSE(std::isnan(result));
    EXPECT_FALSE(std::isinf(result));
}

class VarianceTest : public StatisticalAnalyzerTest
{
};

TEST_F(VarianceTest, BasicVarianceCalculation)
{
    double result = statistical_analyzer::variance(small_data_);
    EXPECT_DOUBLE_EQ(result, 2.5);
}

TEST_F(VarianceTest, EmptyContainerVariance)
{
    double result = statistical_analyzer::variance(empty_data_);
    EXPECT_DOUBLE_EQ(result, 0.0);
}

TEST_F(VarianceTest, SingleElementVariance)
{
    double result = statistical_analyzer::variance(single_element_);
    EXPECT_DOUBLE_EQ(result, 0.0);
}

TEST_F(VarianceTest, IdenticalElementsVariance)
{
    double result = statistical_analyzer::variance(identical_elements_);
    EXPECT_DOUBLE_EQ(result, 0.0);
}

TEST_F(VarianceTest, TwoElementsVariance)
{
    std::vector<double> two_elements = {1.0, 3.0};
    double result = statistical_analyzer::variance(two_elements);
    EXPECT_DOUBLE_EQ(result, 2.0);
}

TEST_F(VarianceTest, IntegerDataVariance)
{
    double result = statistical_analyzer::variance(integer_data_);
    EXPECT_DOUBLE_EQ(result, 250.0);
}

TEST_F(VarianceTest, MixedDataVariance)
{
    double result = statistical_analyzer::variance(mixed_data_);
    EXPECT_GT(result, 0.0);
    EXPECT_FALSE(std::isnan(result));
}

TEST_F(VarianceTest, VarianceProperties)
{
    double result = statistical_analyzer::variance(normal_data_);
    EXPECT_GE(result, 0.0);
    EXPECT_FALSE(std::isnan(result));
    EXPECT_FALSE(std::isinf(result));
}

class StandardDeviationTest : public StatisticalAnalyzerTest
{
};

TEST_F(StandardDeviationTest, BasicStandardDeviationCalculation)
{
    double result = statistical_analyzer::standard_deviation(small_data_);
    double expected_variance = 2.5;
    double expected_stddev = std::sqrt(expected_variance);
    EXPECT_DOUBLE_EQ(result, expected_stddev);
}

TEST_F(StandardDeviationTest, EmptyContainerStandardDeviation)
{
    double result = statistical_analyzer::standard_deviation(empty_data_);
    EXPECT_DOUBLE_EQ(result, 0.0);
}

TEST_F(StandardDeviationTest, SingleElementStandardDeviation)
{
    double result = statistical_analyzer::standard_deviation(single_element_);
    EXPECT_DOUBLE_EQ(result, 0.0);
}

TEST_F(StandardDeviationTest, IdenticalElementsStandardDeviation)
{
    double result =
        statistical_analyzer::standard_deviation(identical_elements_);
    EXPECT_DOUBLE_EQ(result, 0.0);
}

TEST_F(StandardDeviationTest, StandardDeviationVarianceConsistency)
{
    double variance = statistical_analyzer::variance(small_data_);
    double stddev = statistical_analyzer::standard_deviation(small_data_);
    EXPECT_DOUBLE_EQ(stddev * stddev, variance);
}

TEST_F(StandardDeviationTest, StandardDeviationProperties)
{
    double result = statistical_analyzer::standard_deviation(normal_data_);
    EXPECT_GE(result, 0.0);
    EXPECT_FALSE(std::isnan(result));
    EXPECT_FALSE(std::isinf(result));
}

TEST_F(StandardDeviationTest, KnownDistributionStandardDeviation)
{
    std::vector<double> precise_normal =
        generate_normal_distribution(10000, 0.0, 1.0);
    double result = statistical_analyzer::standard_deviation(precise_normal);

    EXPECT_NEAR(result, 1.0, 0.1);
}

class MedianTest : public StatisticalAnalyzerTest
{
};

TEST_F(MedianTest, OddNumberElementsMedian)
{
    double result = statistical_analyzer::median(small_data_);
    EXPECT_DOUBLE_EQ(result, 3.0);
}

TEST_F(MedianTest, EvenNumberElementsMedian)
{
    std::vector<double> even_data = {1.0, 2.0, 3.0, 4.0};
    double result = statistical_analyzer::median(even_data);
    EXPECT_DOUBLE_EQ(result, 2.5);
}

TEST_F(MedianTest, SingleElementMedian)
{
    double result = statistical_analyzer::median(single_element_);
    EXPECT_DOUBLE_EQ(result, 42.0);
}

TEST_F(MedianTest, UnsortedDataMedian)
{
    std::vector<double> unsorted = {5.0, 1.0, 3.0, 2.0, 4.0};
    double result = statistical_analyzer::median(unsorted);
    EXPECT_DOUBLE_EQ(result, 3.0);
}

TEST_F(MedianTest, IdenticalElementsMedian)
{
    double result = statistical_analyzer::median(identical_elements_);
    EXPECT_DOUBLE_EQ(result, 5.0);
}

TEST_F(MedianTest, IntegerDataMedian)
{
    auto int_copy = integer_data_;
    double result = statistical_analyzer::median(int_copy);
    EXPECT_DOUBLE_EQ(result, 30.0);
}

TEST_F(MedianTest, NegativeValuesMedian)
{
    std::vector<double> negative_data = {-5.0, -2.0, -1.0, 0.0, 1.0};
    double result = statistical_analyzer::median(negative_data);
    EXPECT_DOUBLE_EQ(result, -1.0);
}

TEST_F(MedianTest, LargeDatasetMedian)
{
    auto large_copy = large_data_;
    double result = statistical_analyzer::median(large_copy);
    EXPECT_DOUBLE_EQ(result, 500.5);
}

TEST_F(MedianTest, MedianWithDuplicates)
{
    std::vector<double> duplicate_data = {1.0, 2.0, 2.0, 3.0, 3.0, 3.0, 4.0};
    double result = statistical_analyzer::median(duplicate_data);
    EXPECT_DOUBLE_EQ(result, 3.0);
}

TEST_F(MedianTest, MedianDoesNotModifyInput)
{
    std::vector<double> original = {5.0, 1.0, 3.0, 2.0, 4.0};
    std::vector<double> copy = original;

    double result = statistical_analyzer::median(copy);

    EXPECT_EQ(original, copy);
    EXPECT_DOUBLE_EQ(result, 3.0);
    EXPECT_FALSE(std::is_sorted(copy.begin(), copy.end()));
}