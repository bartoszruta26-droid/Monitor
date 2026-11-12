#pragma once
#include <cstddef>
#include <span>
namespace monitor {
enum class WidthMethod { PopulationRms, SampleSd };
struct RegressionResult {
    std::size_t observations{};
    double intercept{};
    double slope{};
    double fitted_last{};
    double sse{};
    double residual_width{};
};
struct RegressionChannel {
    RegressionResult regression;
    double multiplier{};
    double upper{};
    double lower{};
};
[[nodiscard]] RegressionResult linear_regression(std::span<const double> values, WidthMethod method);
[[nodiscard]] RegressionChannel regression_channel(std::span<const double> values, WidthMethod method, double multiplier);
} // namespace monitor
