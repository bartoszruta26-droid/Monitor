#pragma once
#include <cstddef>
#include <span>
#include <vector>
namespace monitor {
enum class WidthMethod {
    PopulationRms,              // LonesomeTheBlue: SSE / n
    SampleSd,                   // DGT: SSE / (n - 1)
    ResidualStandardErrorNMinus2 // klasyczny błąd standardowy regresji: SSE / (n - 2)
};
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
struct RollingRegressionPoint {
    std::size_t start_index{};
    std::size_t end_index{}; // indeks włącznie
    RegressionResult regression;
};
[[nodiscard]] RegressionResult linear_regression(std::span<const double> values, WidthMethod method);
[[nodiscard]] RegressionChannel regression_channel(std::span<const double> values, WidthMethod method, double multiplier);
// Wejście musi być jednym ciągłym segmentem chronologicznym; luki należy rozdzielić
// przed wywołaniem, aby okno nie traktowało odległych próbek jako sąsiednich.
[[nodiscard]] std::vector<RollingRegressionPoint> rolling_linear_regression(
    std::span<const double> values, std::size_t window_size, WidthMethod method);
} // namespace monitor
