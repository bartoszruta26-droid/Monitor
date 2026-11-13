#include "monitor/regression.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
namespace {
void require(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}
bool near(double a, double b, double tolerance = 1e-10) { return std::abs(a - b) <= tolerance; }
template<class F> void throws(F&& fn, const char* message) {
    try { fn(); } catch (const std::exception&) { return; }
    std::cerr << "FAIL: expected exception: " << message << '\n'; std::exit(EXIT_FAILURE);
}
}
int main() {
    const std::vector<double> exact{2,4,6,8,10};
    const auto fit = monitor::linear_regression(exact, monitor::WidthMethod::SampleSd);
    require(near(fit.slope, 2), "nachylenie");
    require(near(fit.intercept, 2), "wyraz wolny");
    require(near(fit.fitted_last, 10), "wartość dopasowana");
    require(near(fit.sse, 0), "SSE linii idealnej");
    const std::vector<double> noisy{1,2,2,4,5};
    // Translating a series by a large constant must not change its residual
    // statistics. This catches cancellation in sum-of-squares implementations.
    const std::vector<double> low_level{0.0, 0.02, -0.01, 0.03, -0.02, 0.015, -0.005};
    std::vector<double> high_level;
    high_level.reserve(low_level.size());
    for (double value : low_level) high_level.push_back(1.0e12 + value);
    std::vector<double> recovered;
    recovered.reserve(high_level.size());
    for (double value : high_level) recovered.push_back(value - 1.0e12);
    const auto low_fit = monitor::linear_regression(recovered, monitor::WidthMethod::PopulationRms);
    const auto high_fit = monitor::linear_regression(high_level, monitor::WidthMethod::PopulationRms);
    require(near(high_fit.slope, low_fit.slope, 1e-9),
            "nachylenie nie zależy od dużego przesunięcia poziomu cen");
    require(near(high_fit.sse, low_fit.sse, 1e-9),
            "SSE zachowuje precyzję po dużym przesunięciu poziomu cen");
    const auto sample = monitor::linear_regression(noisy, monitor::WidthMethod::SampleSd);
    const auto population = monitor::linear_regression(noisy, monitor::WidthMethod::PopulationRms);
    const auto n_minus_2 = monitor::linear_regression(noisy, monitor::WidthMethod::ResidualStandardErrorNMinus2);
    require(sample.sse > 0, "SSE dodatnie");
    require(sample.residual_width > population.residual_width, "DGT n-1 a LTB n");
    require(n_minus_2.residual_width > sample.residual_width,
            "klasyczny błąd standardowy n-2 jest osobną metodą");
    const auto channel = monitor::regression_channel(noisy, monitor::WidthMethod::PopulationRms, 2.41);
    require(channel.upper > channel.regression.fitted_last, "górna granica");
    require(channel.lower < channel.regression.fitted_last, "dolna granica");
    const auto rolling = monitor::rolling_linear_regression(noisy, 3, monitor::WidthMethod::SampleSd);
    require(rolling.size() == 3 && rolling.front().start_index == 0 &&
            rolling.front().end_index == 2 && rolling.back().start_index == 2 &&
            rolling.back().end_index == 4, "okna kroczące mają poprawne indeksy");
    const auto last = monitor::linear_regression(std::span<const double>(noisy).subspan(2,3),
                                                  monitor::WidthMethod::SampleSd);
    require(near(rolling.back().regression.slope, last.slope),
            "ostatnie okno kroczące odpowiada regresji samodzielnej");
    throws([&] { (void)monitor::rolling_linear_regression(noisy, 1, monitor::WidthMethod::SampleSd); },
           "okno mniejsze od dwóch");
    throws([&] { (void)monitor::rolling_linear_regression(noisy, 6, monitor::WidthMethod::SampleSd); },
           "okno większe od danych");
    throws([&] { (void)monitor::linear_regression(std::span<const double>(noisy).first(2),
                       monitor::WidthMethod::ResidualStandardErrorNMinus2); }, "n-2 wymaga n>=3");
    throws([&] { (void)monitor::linear_regression(noisy, static_cast<monitor::WidthMethod>(99)); },
           "nieznana metoda regresji");
    throws([&] { const std::vector<double> short_data{1}; (void)monitor::linear_regression(short_data, monitor::WidthMethod::SampleSd); },
           "odrzucenie krótkiej serii");
    throws([&] { const std::vector<double> bad{1, std::numeric_limits<double>::quiet_NaN()}; (void)monitor::linear_regression(bad, monitor::WidthMethod::SampleSd); },
           "odrzucenie NaN");
    std::cout << "Wszystkie testy rdzenia Monitor zakończone powodzeniem.\n";
    return EXIT_SUCCESS;
}
