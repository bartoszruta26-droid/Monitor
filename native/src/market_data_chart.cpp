#include "monitor/market_data_chart.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <set>
#include <stdexcept>
#include <tuple>

namespace monitor::market {
namespace {
void validate_single_series(const std::vector<MarketObservation>& observations) {
    if (observations.empty()) return;
    const auto& first = observations.front();
    std::set<std::string> ids;
    for (const auto& item : observations) {
        if (item.instrument_id != first.instrument_id || item.provider_id != first.provider_id ||
            item.price_kind != first.price_kind || item.frequency != first.frequency)
            throw std::invalid_argument("Wykres wymaga jednego, jednorodnego szeregu danych.");
        if (item.id.empty() || !ids.insert(item.id).second)
            throw std::invalid_argument("Identyfikatory punktów wykresu muszą być unikalne.");
        if (!std::isfinite(item.value))
            throw std::invalid_argument("Punkt wykresu zawiera cenę niefinitywną.");
    }
}
bool same_candle(const OhlcCandle& a, const OhlcCandle& b) {
    return a.semantics == b.semantics && a.has_intraperiod_range == b.has_intraperiod_range &&
        a.instrument_id == b.instrument_id && a.provider_id == b.provider_id &&
        a.price_kind == b.price_kind && a.source_frequency == b.source_frequency &&
        a.frequency == b.frequency && a.period_start.unix_seconds == b.period_start.unix_seconds &&
        a.period_end.unix_seconds == b.period_end.unix_seconds &&
        a.open == b.open && a.high == b.high && a.low == b.low && a.close == b.close &&
        a.observation_count == b.observation_count && a.period_status == b.period_status &&
        a.quality == b.quality && a.quality_flags == b.quality_flags &&
        a.source_observation_ids == b.source_observation_ids;
}
bool same_gap(const MarketDataGap& a, const MarketDataGap& b) {
    return a.period_start.unix_seconds == b.period_start.unix_seconds &&
        a.period_end.unix_seconds == b.period_end.unix_seconds && a.frequency == b.frequency;
}
}
ChartSeries build_point_chart(const std::vector<MarketObservation>& observations,
                              ChartKind kind, std::size_t max_points) {
    if (kind != ChartKind::Line && kind != ChartKind::Area)
        throw std::invalid_argument("Nieznany rodzaj wykresu punktowego.");
    if (max_points == 0U) throw std::invalid_argument("Limit punktów musi być dodatni.");
    validate_single_series(observations);
    ChartSeries result;
    result.kind = kind;
    if (observations.empty()) return result;
    const auto& first = observations.front();
    result.instrument_id = first.instrument_id;
    result.provider_id = first.provider_id;
    result.price_kind = first.price_kind;
    result.source_frequency = first.frequency;
    result.display_frequency = first.frequency;
    if (observations.size() > max_points)
        throw std::length_error("Liczba punktów przekracza limit wykresu.");
    std::vector<const MarketObservation*> ordered;
    ordered.reserve(observations.size());
    for (const auto& item : observations) ordered.push_back(&item);
    std::stable_sort(ordered.begin(), ordered.end(), [](const auto* a, const auto* b) {
        return std::tie(a->observed_at.unix_seconds, a->id) <
               std::tie(b->observed_at.unix_seconds, b->id);
    });
    result.points.reserve(ordered.size());
    if (first.frequency != Frequency::Tick && first.frequency != Frequency::Irregular) {
        const auto periods = aggregate_ohlc(observations, first.frequency, max_points);
        result.gaps.reserve(periods.gaps.size());
        for (const auto& gap : periods.gaps)
            result.gaps.push_back({gap.period_start, gap.period_end, gap.frequency});
    }
    for (const auto* item : ordered)
        result.points.push_back({item->observed_at, item->value, item->quality,
                                 item->period_status, item->id});
    return result;
}
ChartSeries build_candlestick_chart(const std::vector<MarketObservation>& observations,
                                    const AggregationResult& aggregation,
                                    std::size_t max_candles) {
    if (max_candles == 0U) throw std::invalid_argument("Limit świec musi być dodatni.");
    validate_single_series(observations);
    ChartSeries result;
    result.kind = ChartKind::Candlestick;
    if (observations.empty()) {
        if (!aggregation.candles.empty() || !aggregation.gaps.empty())
            throw std::invalid_argument("Agregacja bez szeregu źródłowego.");
        return result;
    }
    if (aggregation.candles.empty())
        throw std::invalid_argument("Brak świec dla niepustego szeregu źródłowego.");
    const auto& first = observations.front();
    result.instrument_id = first.instrument_id;
    result.provider_id = first.provider_id;
    result.price_kind = first.price_kind;
    result.source_frequency = first.frequency;
    if (aggregation.candles.size() > max_candles)
        throw std::length_error("Liczba świec przekracza limit wykresu.");

    const Frequency target_frequency = aggregation.candles.front().frequency;
    const auto expected = aggregate_ohlc(observations, target_frequency, max_candles);
    if (expected.candles.size() != aggregation.candles.size() ||
        expected.gaps.size() != aggregation.gaps.size())
        throw std::invalid_argument("Agregacja nie odpowiada szeregowi źródłowemu.");
    for (std::size_t i = 0; i < expected.candles.size(); ++i) {
        if (!same_candle(expected.candles[i], aggregation.candles[i]))
            throw std::invalid_argument("Świeca nie odpowiada obserwacjom źródłowym.");
    }
    for (std::size_t i = 0; i < expected.gaps.size(); ++i) {
        if (!same_gap(expected.gaps[i], aggregation.gaps[i]))
            throw std::invalid_argument("Luka wykresu nie odpowiada szeregowi źródłowemu.");
    }
    result.candles = aggregation.candles;
    result.gaps.reserve(aggregation.gaps.size());
    for (const auto& gap : aggregation.gaps)
        result.gaps.push_back({gap.period_start, gap.period_end, gap.frequency});
    result.display_frequency = target_frequency;
    return result;
}
} // namespace monitor::market
