#include "monitor/regression.hpp"
#include <cmath>
#include <stdexcept>
namespace monitor {
RegressionResult linear_regression(std::span<const double> values, WidthMethod method) {
    const std::size_t n = values.size();
    if (n < 2) throw std::invalid_argument("Regresja wymaga co najmniej dwóch obserwacji.");
    switch (method) {
    case WidthMethod::PopulationRms:
    case WidthMethod::SampleSd: break;
    case WidthMethod::ResidualStandardErrorNMinus2:
        if (n < 3) throw std::invalid_argument("Metoda n-2 wymaga co najmniej trzech obserwacji.");
        break;
    default: throw std::invalid_argument("Nieznana metoda szerokości regresji.");
    }
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
    long double sse = 0.0L;
    for (std::size_t i = 0; i < n; ++i) {
        const long double predicted = intercept + slope * static_cast<long double>(i);
        const long double residual = static_cast<long double>(values[i]) - predicted;
        sse += residual * residual;
    }
    long double divisor = count;
    if (method == WidthMethod::SampleSd) divisor = count - 1.0L;
    if (method == WidthMethod::ResidualStandardErrorNMinus2) divisor = count - 2.0L;
    const long double width = std::sqrt(sse / divisor);
    const long double fitted_last = intercept + slope * (count - 1.0L);
    const RegressionResult result{n, static_cast<double>(intercept), static_cast<double>(slope),
        static_cast<double>(fitted_last), static_cast<double>(sse), static_cast<double>(width)};
    if (!std::isfinite(result.intercept) || !std::isfinite(result.slope) ||
        !std::isfinite(result.fitted_last) || !std::isfinite(result.sse) ||
        !std::isfinite(result.residual_width))
        throw std::overflow_error("Wynik regresji przekracza zakres typu double.");
    return result;
}
RegressionChannel regression_channel(std::span<const double> values, WidthMethod method, double multiplier) {
    if (!std::isfinite(multiplier) || multiplier < 0.0)
        throw std::invalid_argument("Mnożnik kanału musi być nieujemną liczbą skończoną.");
    const auto result = linear_regression(values, method);
    const double radius = result.residual_width * multiplier;
    const double upper = result.fitted_last + radius;
    const double lower = result.fitted_last - radius;
    if (!std::isfinite(upper) || !std::isfinite(lower))
        throw std::overflow_error("Granice kanału regresji przekraczają zakres typu double.");
    return {result, multiplier, upper, lower};
}
std::vector<RollingRegressionPoint> rolling_linear_regression(
    std::span<const double> values, std::size_t window_size, WidthMethod method) {
    if (window_size < 2U || window_size > values.size())
        throw std::invalid_argument("Rozmiar okna regresji kroczącej musi mieścić się w zakresie [2, liczba danych].");
    if (method == WidthMethod::ResidualStandardErrorNMinus2 && window_size < 3U)
        throw std::invalid_argument("Okno regresji z metodą n-2 wymaga co najmniej trzech obserwacji.");
    std::vector<RollingRegressionPoint> result;
    result.reserve(values.size() - window_size + 1U);
    for (std::size_t start = 0; start <= values.size() - window_size; ++start) {
        const auto window = values.subspan(start, window_size);
        result.push_back({start, start + window_size - 1U, linear_regression(window, method)});
    }
    return result;
}
} // namespace monitor
