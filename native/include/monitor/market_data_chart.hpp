#pragma once

#include "monitor/market_data_aggregation.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace monitor::market {

enum class ChartKind { Line, Area, Candlestick };
struct ChartPoint {
    UtcTimestamp timestamp{};
    double value{};
    QualityStatus quality{QualityStatus::Unreviewed};
    PeriodStatus period_status{PeriodStatus::Complete};
    std::string source_observation_id;
};
struct ChartGap {
    UtcTimestamp period_start{};
    UtcTimestamp period_end{};
    Frequency frequency{Frequency::Daily};
};
struct ChartSeries {
    ChartKind kind{ChartKind::Line};
    std::string instrument_id;
    std::string provider_id;
    PriceKind price_kind{PriceKind::Reference};
    Frequency source_frequency{Frequency::Daily};
    Frequency display_frequency{Frequency::Daily};
    std::vector<ChartPoint> points;
    std::vector<OhlcCandle> candles;
    std::vector<ChartGap> gaps;
};
[[nodiscard]] ChartSeries build_point_chart(
    const std::vector<MarketObservation>& observations,
    ChartKind kind = ChartKind::Line,
    std::size_t max_points = 100000);
[[nodiscard]] ChartSeries build_candlestick_chart(
    const std::vector<MarketObservation>& observations,
    const AggregationResult& aggregation,
    std::size_t max_candles = 100000);

} // namespace monitor::market
