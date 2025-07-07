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
