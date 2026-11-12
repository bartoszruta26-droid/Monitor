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
    require(sample.sse > 0, "SSE dodatnie");
    require(sample.residual_width > population.residual_width, "DGT n-1 a LTB n");
    const auto channel = monitor::regression_channel(noisy, monitor::WidthMethod::PopulationRms, 2.41);
    require(channel.upper > channel.regression.fitted_last, "górna granica");
    require(channel.lower < channel.regression.fitted_last, "dolna granica");
    bool rejected = false;
    try { const std::vector<double> short_data{1}; (void)monitor::linear_regression(short_data, monitor::WidthMethod::SampleSd); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "odrzucenie krótkiej serii");
    rejected = false;
    try { const std::vector<double> bad{1, std::numeric_limits<double>::quiet_NaN()}; (void)monitor::linear_regression(bad, monitor::WidthMethod::SampleSd); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "odrzucenie NaN");
    std::cout << "Wszystkie testy rdzenia Monitor zakończone powodzeniem.\n";
    return EXIT_SUCCESS;
}
