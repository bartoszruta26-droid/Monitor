#pragma once

#include "monitor/market_data.hpp"
#include "monitor/regression.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace monitor::market {

enum class HtfSamplePolicy {
    FirstObservation,
    LastObservation
};

struct TimeAwareRollingRegressionPoint {
    std::size_t segment_index{};
    Frequency target_frequency{Frequency::Daily};
    UtcTimestamp window_start{};
    UtcTimestamp window_end{}; // granica wyłączna
    UtcTimestamp last_observed_at{}; // rzeczywisty czas wybranej próbki końcowej
    double last_value{};
    std::vector<std::string> source_observation_ids;
    RegressionResult regression;
};

// Buduje jeden punkt na rzeczywisty okres HTF, wybierając pierwszą albo ostatnią
// obserwację z danego okresu. Nie interpoluje i nie uzupełnia braków.
// Okna są liczone osobno dla ciągłych, sąsiadujących okresów HTF; żadna regresja
// nie przekracza luki. Tick i Irregular nie są obsługiwane.
[[nodiscard]] std::vector<TimeAwareRollingRegressionPoint>
time_aware_rolling_regression(
    const std::vector<MarketObservation>& observations,
    Frequency target_frequency,
    std::size_t window_size,
    WidthMethod method,
    HtfSamplePolicy sample_policy = HtfSamplePolicy::LastObservation);

} // namespace monitor::market
