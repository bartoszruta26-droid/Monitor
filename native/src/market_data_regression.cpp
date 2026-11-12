#include "monitor/market_data_regression.hpp"

#include "monitor/market_data_aggregation.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace monitor::market {
namespace {

struct HtfSample {
    const OhlcCandle* candle{};
    const MarketObservation* selected{};
};

void validate_target_frequency(Frequency frequency) {
    switch (frequency) {
    case Frequency::Minute:
    case Frequency::Hour:
    case Frequency::Daily:
    case Frequency::Weekly:
    case Frequency::Monthly:
    case Frequency::Quarterly:
    case Frequency::SemiAnnual:
    case Frequency::Annual:
        return;
    case Frequency::Tick:
    case Frequency::Irregular:
    default:
        throw std::invalid_argument("Docelowa częstotliwość HTF musi być regularna.");
    }
}

void validate_width_method(WidthMethod method) {
    switch (method) {
    case WidthMethod::PopulationRms:
    case WidthMethod::SampleSd:
    case WidthMethod::ResidualStandardErrorNMinus2:
        return;
    default:
        throw std::invalid_argument("Nieznana metoda regresji HTF.");
    }
}

void validate_policy(HtfSamplePolicy policy) {
    switch (policy) {
    case HtfSamplePolicy::FirstObservation:
    case HtfSamplePolicy::LastObservation:
        return;
    default:
        throw std::invalid_argument("Nieznana polityka wyboru próbki HTF.");
    }
}

} // namespace

std::vector<TimeAwareRollingRegressionPoint> time_aware_rolling_regression(
    const std::vector<MarketObservation>& observations,
    Frequency target_frequency,
    std::size_t window_size,
    WidthMethod method,
    HtfSamplePolicy sample_policy) {
    validate_policy(sample_policy);
    validate_target_frequency(target_frequency);
    validate_width_method(method);
    if (window_size < 2U)
        throw std::invalid_argument("Okno regresji HTF musi zawierać co najmniej dwie próbki.");
    if (method == WidthMethod::ResidualStandardErrorNMinus2 && window_size < 3U)
        throw std::invalid_argument("Metoda n-2 wymaga okna HTF o rozmiarze co najmniej trzech.");
    if (observations.empty()) return {};

    std::set<std::int64_t> timestamps;
    std::map<std::string, const MarketObservation*> by_id;
    for (const auto& observation : observations) {
        if (!timestamps.insert(observation.observed_at.unix_seconds).second)
            throw std::invalid_argument("Szereg HTF zawiera powtórzony znacznik czasu obserwacji.");
        if (observation.id.empty() || !by_id.emplace(observation.id, &observation).second)
            throw std::invalid_argument("Identyfikatory obserwacji HTF muszą być niepuste i unikalne.");
    }

    const AggregationResult aggregated = aggregate_ohlc(observations, target_frequency);
    if (aggregated.candles.empty()) return {};

    std::vector<HtfSample> samples;
    samples.reserve(aggregated.candles.size());
    for (const auto& candle : aggregated.candles) {
        if (candle.source_observation_ids.empty())
            throw std::logic_error("Świeca HTF nie ma obserwacji źródłowej.");
        const std::string& selected_id =
            sample_policy == HtfSamplePolicy::FirstObservation
                ? candle.source_observation_ids.front()
                : candle.source_observation_ids.back();
        const auto found = by_id.find(selected_id);
        if (found == by_id.end())
            throw std::logic_error("Brak obserwacji wskazanej przez pochodzenie świecy HTF.");
        samples.push_back({&candle, found->second});
    }

    std::vector<TimeAwareRollingRegressionPoint> result;
    std::vector<double> segment_values;
    std::vector<HtfSample> segment_samples;
    std::size_t segment_index = 0U;

    const auto flush_segment = [&]() {
        if (segment_values.size() >= window_size) {
            const auto rolling = rolling_linear_regression(segment_values, window_size, method);
            for (const auto& point : rolling) {
                const std::size_t first = point.start_index;
                const std::size_t last = point.end_index;
                TimeAwareRollingRegressionPoint output;
                output.segment_index = segment_index;
                output.target_frequency = target_frequency;
                output.window_start = segment_samples[first].candle->period_start;
                output.window_end = segment_samples[last].candle->period_end;
                output.last_observed_at = segment_samples[last].selected->observed_at;
                output.last_value = segment_samples[last].selected->value;
                output.regression = point.regression;
                for (std::size_t i = first; i <= last; ++i) {
                    const auto& ids = segment_samples[i].candle->source_observation_ids;
                    output.source_observation_ids.insert(output.source_observation_ids.end(),
                                                          ids.begin(), ids.end());
                }
                result.push_back(std::move(output));
            }
        }
        segment_values.clear();
        segment_samples.clear();
        ++segment_index;
    };

    for (std::size_t i = 0; i < samples.size(); ++i) {
        if (!segment_samples.empty() &&
            samples[i].candle->period_start.unix_seconds !=
                segment_samples.back().candle->period_end.unix_seconds) {
            flush_segment();
        }
        segment_samples.push_back(samples[i]);
        segment_values.push_back(samples[i].selected->value);
    }
    flush_segment();
    return result;
}

} // namespace monitor::market
