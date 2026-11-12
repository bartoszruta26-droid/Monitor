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
        if (item.instrument_id != first.instrument_id ||
            item.provider_id != first.provider_id ||
            item.price_kind != first.price_kind ||
            item.frequency != first.frequency)
            throw std::invalid_argument("Wykres wymaga jednego, jednorodnego szeregu danych.");
        if (item.id.empty() || !ids.insert(item.id).second)
            throw std::invalid_argument("Identyfikatory punktów wykresu muszą być unikalne.");
        if (!std::isfinite(item.value))
            throw std::invalid_argument("Punkt wykresu zawiera cenę niefinitywną.");
    }
}
}

ChartSeries build_point_chart(const std::vector<MarketObservation>& observations,
                              ChartKind kind, std::size_t max_points) {
    if (kind == ChartKind::Candlestick)
        throw std::invalid_argument("Wykres świecowy wymaga build_candlestick_chart.");
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
    const auto& first = observations.front();
    result.instrument_id = first.instrument_id;
    result.provider_id = first.provider_id;
    result.price_kind = first.price_kind;
    result.source_frequency = first.frequency;
    if (aggregation.candles.size() > max_candles)
        throw std::length_error("Liczba świec przekracza limit wykresu.");
    result.candles = aggregation.candles;
    std::int64_t previous_start = std::numeric_limits<std::int64_t>::min();
    for (const auto& candle : result.candles) {
        if (candle.instrument_id != result.instrument_id ||
            candle.provider_id != result.provider_id ||
            candle.price_kind != result.price_kind ||
            candle.source_frequency != result.source_frequency ||
            candle.period_start.unix_seconds <= previous_start ||
            candle.period_end.unix_seconds <= candle.period_start.unix_seconds ||
            candle.observation_count == 0U ||
            candle.source_observation_ids.size() != candle.observation_count ||
            !std::isfinite(candle.open) || !std::isfinite(candle.high) ||
            !std::isfinite(candle.low) || !std::isfinite(candle.close) ||
            candle.high < candle.low || candle.high < candle.open ||
            candle.high < candle.close || candle.low > candle.open ||
            candle.low > candle.close ||
            candle.has_intraperiod_range != (candle.observation_count > 1U))
            throw std::invalid_argument("Nieprawidłowa świeca lub niezgodna pochodność danych.");
        previous_start = candle.period_start.unix_seconds;
    }
    result.gaps.reserve(aggregation.gaps.size());
    for (const auto& gap : aggregation.gaps)
        result.gaps.push_back({gap.period_start, gap.period_end, gap.frequency});
    if (!result.candles.empty()) {
        result.display_frequency = result.candles.front().frequency;
        for (const auto& candle : result.candles) {
            if (candle.frequency != result.display_frequency)
                throw std::invalid_argument("Świece mają różne interwały wyświetlania.");
        }
    } else result.display_frequency = result.source_frequency;
    return result;
}

} // namespace monitor::market
