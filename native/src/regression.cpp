#include "monitor/regression.hpp"
#include <cmath>
#include <stdexcept>
namespace monitor {
RegressionResult linear_regression(std::span<const double> values, WidthMethod method) {
    const std::size_t n = values.size();
    if (n < 2) throw std::invalid_argument("Regresja wymaga co najmniej dwóch obserwacji.");
    long double sum_y = 0.0L;
    for (double value : values) {
        if (!std::isfinite(value)) throw std::invalid_argument("Dane regresji zawierają NaN lub nieskończoność.");
        sum_y += static_cast<long double>(value);
    }
    const long double count = static_cast<long double>(n);
    const long double mean_x = (count - 1.0L) / 2.0L;
    const long double mean_y = sum_y / count;
    const long double sxx = count * (count * count - 1.0L) / 12.0L;
    long double covariance = 0.0L;
    for (std::size_t i = 0; i < n; ++i) {
        covariance += (static_cast<long double>(i) - mean_x) *
                      (static_cast<long double>(values[i]) - mean_y);
    }
    const long double slope = covariance / sxx;
    const long double intercept = mean_y - slope * mean_x;
    // Direct residual summation avoids catastrophic cancellation in SSE.
    long double sse = 0.0L;
    for (std::size_t i = 0; i < n; ++i) {
        const long double predicted = intercept + slope * static_cast<long double>(i);
        const long double residual = static_cast<long double>(values[i]) - predicted;
        sse += residual * residual;
    }
    const long double divisor = method == WidthMethod::SampleSd ? count - 1.0L : count;
    const long double width = std::sqrt(sse / divisor);
    const long double fitted_last = intercept + slope * (count - 1.0L);
    return {n, static_cast<double>(intercept), static_cast<double>(slope),
            static_cast<double>(fitted_last), static_cast<double>(sse), static_cast<double>(width)};
}
RegressionChannel regression_channel(std::span<const double> values, WidthMethod method, double multiplier) {
    if (!std::isfinite(multiplier) || multiplier < 0.0)
        throw std::invalid_argument("Mnożnik kanału musi być nieujemną liczbą skończoną.");
    const auto result = linear_regression(values, method);
    const double radius = result.residual_width * multiplier;
    return {result, multiplier, result.fitted_last + radius, result.fitted_last - radius};
}
} // namespace monitor
