#include "monitor/regression.hpp"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>
int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Użycie: monitor-regression <wartość1> <wartość2> [wartość3 ...]\n";
        return EXIT_FAILURE;
    }
    std::vector<double> values;
    try {
        for (int i = 1; i < argc; ++i) {
            char* end = nullptr;
            const double value = std::strtod(argv[i], &end);
            if (end == argv[i] || *end != '\0') throw std::invalid_argument("Nieprawidłowa wartość liczbowa.");
            values.push_back(value);
        }
        const auto dgt = monitor::regression_channel(values, monitor::WidthMethod::SampleSd, 1.0);
        const auto ltb = monitor::regression_channel(values, monitor::WidthMethod::PopulationRms, 2.41);
        std::cout << std::setprecision(12)
                  << "obserwacje=" << dgt.regression.observations << '\n'
                  << "nachylenie=" << dgt.regression.slope << '\n'
                  << "regresja_ostatnia=" << dgt.regression.fitted_last << '\n'
                  << "SSE=" << dgt.regression.sse << '\n'
                  << "DGT_SD_n_minus_1=" << dgt.regression.residual_width << '\n'
                  << "LTB_RMS_n=" << ltb.regression.residual_width << '\n'
                  << "LTB_gorna_2_41=" << ltb.upper << '\n'
                  << "LTB_dolna_2_41=" << ltb.lower << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Błąd: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
