#pragma once

#include "monitor/market_data.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace monitor::market {

struct OhlcCandle {
    std::string instrument_id;
    std::string provider_id;
    PriceKind price_kind{PriceKind::Reference};
    Frequency source_frequency{Frequency::Daily};
    Frequency frequency{Frequency::Daily};
    UtcTimestamp period_start{};
    UtcTimestamp period_end{}; // granica wyłączna
    double open{};
    double high{};
    double low{};
    double close{};
    std::size_t observation_count{};
    PeriodStatus period_status{PeriodStatus::Complete};
    QualityStatus quality{QualityStatus::Unreviewed};
    std::vector<std::string> quality_flags;
    std::vector<std::string> source_observation_ids;
};

struct MarketDataGap {
    UtcTimestamp period_start{};
    UtcTimestamp period_end{}; // granica wyłączna
    Frequency frequency{Frequency::Daily};
};

struct AggregationResult {
    std::vector<OhlcCandle> candles;
    std::vector<MarketDataGap> gaps;
};

// Agreguje jeden szereg: instrument + dostawca + rodzaj ceny + częstotliwość źródłowa.
// Nie interpoluje, nie uzupełnia cen i nie zwiększa częstotliwości danych.
[[nodiscard]] AggregationResult aggregate_ohlc(
    const std::vector<MarketObservation>& observations,
    Frequency target_frequency,
    std::size_t max_periods = 100000);

} // namespace monitor::market
