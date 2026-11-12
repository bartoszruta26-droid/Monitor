#pragma once

#include "monitor/market_data.hpp"
#include "monitor/regression.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace monitor::market {

// Harmonogram oczekiwanych obserwacji jednego szeregu w UTC.
// Święta i zamknięcia rynku pomija dostawca kalendarza; nie są wywnioskowywane.
struct MarketSessionCalendar {
    std::string calendar_id;
    std::string instrument_id;
    std::string provider_id;
    Frequency source_frequency{Frequency::Daily};
    std::vector<UtcTimestamp> expected_observation_times;
};

enum class HtfSamplePolicy {
    FirstObservation,
    LastObservation
};

struct TimeAwareRollingRegressionPoint {
    std::size_t segment_index{};
    std::string calendar_id;
    std::uint32_t calendar_version{};
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

// Okres HTF wymaga wszystkich zaplanowanych obserwacji ze statusem Complete
// i jakością Validated. Nieplanowane dni zamknięcia rynku nie tworzą luk.
[[nodiscard]] std::vector<TimeAwareRollingRegressionPoint>
session_aware_rolling_regression(
    const std::vector<MarketObservation>& observations,
    const MarketSessionCalendar& calendar,
    Frequency target_frequency,
    std::size_t window_size,
    WidthMethod method,
    HtfSamplePolicy sample_policy = HtfSamplePolicy::LastObservation);

} // namespace monitor::market
