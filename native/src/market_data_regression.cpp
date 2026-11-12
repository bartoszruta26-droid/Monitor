#include "monitor/market_data_regression.hpp"

#include "monitor/market_data_aggregation.hpp"

#include <algorithm>
#include <map>
#include <optional>
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


std::vector<TimeAwareRollingRegressionPoint> session_aware_rolling_regression(
    const std::vector<MarketObservation>& observations,
    const MarketSessionCalendar& calendar,
    Frequency target_frequency,
    std::size_t window_size,
    WidthMethod method,
    HtfSamplePolicy sample_policy) {
    validate_policy(sample_policy);
    validate_target_frequency(target_frequency);
    validate_width_method(method);
    if (window_size < 2U)
        throw std::invalid_argument("Okno regresji sesyjnej wymaga co najmniej dwóch próbek.");
    if (method == WidthMethod::ResidualStandardErrorNMinus2 && window_size < 3U)
        throw std::invalid_argument("Metoda n-2 wymaga co najmniej trzech okresów HTF.");
    if (calendar.calendar_id.empty() || calendar.version==0U || calendar.instrument_id.empty() ||
        calendar.provider_id.empty() || calendar.timezone.empty() || calendar.expected_observation_times.empty())
        throw std::invalid_argument("Kalendarz sesyjny wymaga identyfikatora, szeregu i terminów.");
    switch (calendar.source_frequency) {
    case Frequency::Minute: case Frequency::Hour: case Frequency::Daily: case Frequency::Weekly:
    case Frequency::Monthly: case Frequency::Quarterly: case Frequency::SemiAnnual: case Frequency::Annual: break;
    case Frequency::Tick: case Frequency::Irregular:
    default: throw std::invalid_argument("Kalendarz regresji sesyjnej wymaga regularnej częstotliwości.");
    }
    if (observations.empty()) return {};

    std::set<std::int64_t> expected_times;
    bool first_time = true;
    std::int64_t previous_time = 0;
    for (const auto timestamp : calendar.expected_observation_times) {
        if ((!first_time && timestamp.unix_seconds <= previous_time) ||
            !expected_times.insert(timestamp.unix_seconds).second)
            throw std::invalid_argument("Terminy kalendarza muszą być unikalne i ściśle rosnące.");
        previous_time = timestamp.unix_seconds;
        first_time = false;
    }

    std::map<std::int64_t, const MarketObservation*> observed_by_time;
    for (const auto& observation : observations) {
        if (observation.instrument_id != calendar.instrument_id ||
            observation.provider_id != calendar.provider_id ||
            observation.frequency != calendar.source_frequency)
            throw std::invalid_argument("Obserwacje nie odpowiadają szeregowi kalendarza.");
        if (!expected_times.contains(observation.observed_at.unix_seconds))
            throw std::invalid_argument("Obserwacja występuje poza harmonogramem sesji.");
        if (!observed_by_time.emplace(observation.observed_at.unix_seconds, &observation).second)
            throw std::invalid_argument("Powtórzony czas obserwacji w szeregu sesyjnym.");
    }

    // Ten sam agregator przypisuje oczekiwane sloty do okresów HTF.
    // Techniczne wartości 0 nie trafiają do regresji ani wyniku.
    std::vector<MarketObservation> slots;
    std::map<std::string, UtcTimestamp> slot_times;
    slots.reserve(calendar.expected_observation_times.size());
    for (std::size_t i = 0; i < calendar.expected_observation_times.size(); ++i) {
        MarketObservation slot;
        slot.id = "calendar-slot-" + std::to_string(i);
        slot.instrument_id = calendar.instrument_id;
        slot.provider_id = calendar.provider_id;
        slot.observed_at = calendar.expected_observation_times[i];
        slot.received_at = slot.observed_at;
        slot.frequency = calendar.source_frequency;
        slot.value = 0.0;
        slot_times.emplace(slot.id, slot.observed_at);
        slots.push_back(std::move(slot));
    }
    const auto expected = aggregate_ohlc(slots, target_frequency);
    const auto actual = aggregate_ohlc(observations, target_frequency);
    std::map<std::int64_t, const OhlcCandle*> actual_by_period;
    for (const auto& candle : actual.candles)
        actual_by_period.emplace(candle.period_start.unix_seconds, &candle);

    std::vector<std::optional<HtfSample>> periods;
    periods.reserve(expected.candles.size());
    for (const auto& expected_candle : expected.candles) {
        const auto actual_it = actual_by_period.find(expected_candle.period_start.unix_seconds);
        if (actual_it == actual_by_period.end()) {
            periods.emplace_back(std::nullopt);
            continue;
        }
        const MarketObservation* selected = nullptr;
        bool complete = true;
        for (std::size_t slot_index = 0;
             slot_index < expected_candle.source_observation_ids.size(); ++slot_index) {
            const auto& slot_id = expected_candle.source_observation_ids[slot_index];
            const auto slot_it = slot_times.find(slot_id);
            if (slot_it == slot_times.end())
                throw std::logic_error("Nieznany slot harmonogramu sesyjnego.");
            const auto observed_it = observed_by_time.find(slot_it->second.unix_seconds);
            if (observed_it == observed_by_time.end()) {
                complete = false;
                break;
            }
            const auto* source = observed_it->second;
            if (source->period_status != PeriodStatus::Complete ||
                source->quality != QualityStatus::Validated) {
                complete = false;
                break;
            }
            const bool choose = sample_policy == HtfSamplePolicy::FirstObservation
                ? slot_index == 0U
                : slot_index + 1U == expected_candle.source_observation_ids.size();
            if (choose) selected = source;
        }
        if (!complete || selected == nullptr) periods.emplace_back(std::nullopt);
        else periods.emplace_back(HtfSample{actual_it->second, selected});
    }

    std::vector<TimeAwareRollingRegressionPoint> result;
    std::vector<double> values;
    std::vector<HtfSample> segment;
    std::size_t segment_index = 0U;
    const auto flush = [&]() {
        if (values.size() >= window_size) {
            const auto windows = rolling_linear_regression(values, window_size, method);
            for (const auto& window : windows) {
                const auto first = window.start_index;
                const auto last = window.end_index;
                TimeAwareRollingRegressionPoint point;
                point.segment_index = segment_index;
                point.calendar_id = calendar.calendar_id;
                point.calendar_version = calendar.version;
                point.target_frequency = target_frequency;
                point.window_start = segment[first].candle->period_start;
                point.window_end = segment[last].candle->period_end;
                point.last_observed_at = segment[last].selected->observed_at;
                point.last_value = segment[last].selected->value;
                point.regression = window.regression;
                for (std::size_t i = first; i <= last; ++i) {
                    const auto& ids = segment[i].candle->source_observation_ids;
                    point.source_observation_ids.insert(point.source_observation_ids.end(),
                                                        ids.begin(), ids.end());
                }
                result.push_back(std::move(point));
            }
        }
        values.clear();
        segment.clear();
        ++segment_index;
    };
    for (const auto& period : periods) {
        if (!period) {
            flush();
            continue;
        }
        segment.push_back(*period);
        values.push_back(period->selected->value);
    }
    flush();
    return result;
}

} // namespace monitor::market
