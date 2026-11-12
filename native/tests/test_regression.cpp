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
