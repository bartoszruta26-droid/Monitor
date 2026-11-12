#include "monitor/market_data_aggregation.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>

namespace monitor::market {
namespace {
using namespace std::chrono;

int rank(Frequency f) {
    switch (f) {
    case Frequency::Tick: return 0;
    case Frequency::Minute: return 1;
    case Frequency::Hour: return 2;
    case Frequency::Daily: return 3;
    case Frequency::Weekly: return 4;
    case Frequency::Monthly: return 5;
    case Frequency::Quarterly: return 6;
    case Frequency::SemiAnnual: return 7;
    case Frequency::Annual: return 8;
    case Frequency::Irregular: return -1;
    }
    return -1;
}

struct Bucket { std::int64_t start{}; std::int64_t end{}; };

Bucket bucket_for(UtcTimestamp timestamp, Frequency frequency) {
    const sys_seconds point{seconds{timestamp.unix_seconds}};
    if (frequency == Frequency::Minute) {
        const auto start = floor<minutes>(point);
        return {duration_cast<seconds>(start.time_since_epoch()).count(),
                duration_cast<seconds>((start + minutes{1}).time_since_epoch()).count()};
    }
    if (frequency == Frequency::Hour) {
        const auto start = floor<hours>(point);
        return {duration_cast<seconds>(start.time_since_epoch()).count(),
                duration_cast<seconds>((start + hours{1}).time_since_epoch()).count()};
    }
    const sys_days day = floor<days>(point);
    if (frequency == Frequency::Daily) {
        return {duration_cast<seconds>(day.time_since_epoch()).count(),
                duration_cast<seconds>((day + days{1}).time_since_epoch()).count()};
    }
    if (frequency == Frequency::Weekly) {
        const weekday wd{day};
        const sys_days start = day - days{static_cast<int>(wd.iso_encoding()) - 1};
        return {duration_cast<seconds>(start.time_since_epoch()).count(),
                duration_cast<seconds>((start + days{7}).time_since_epoch()).count()};
    }
    const year_month_day ymd{day};
    const int year_value = static_cast<int>(ymd.year());
    const unsigned month_value = static_cast<unsigned>(ymd.month());
    unsigned first_month = month_value;
    unsigned span = 1;
    if (frequency == Frequency::Monthly) first_month = month_value;
    else if (frequency == Frequency::Quarterly) {
        first_month = ((month_value - 1U) / 3U) * 3U + 1U; span = 3;
    } else if (frequency == Frequency::SemiAnnual) {
        first_month = month_value <= 6U ? 1U : 7U; span = 6;
    } else if (frequency == Frequency::Annual) {
        first_month = 1U; span = 12;
    } else throw std::invalid_argument("Nieobsługiwana częstotliwość docelowa agregacji.");
    const year_month first{year{year_value}, month{first_month}};
    const sys_days start{first / std::chrono::day{1}};
    const sys_days end{(first + months{span}) / std::chrono::day{1}};
    return {duration_cast<seconds>(start.time_since_epoch()).count(),
            duration_cast<seconds>(end.time_since_epoch()).count()};
}

Bucket next_bucket(std::int64_t start, Frequency frequency) {
    const Bucket current = bucket_for(UtcTimestamp{start}, frequency);
    if (current.start != start) throw std::logic_error("Granica okresu agregacji jest niespójna.");
    return bucket_for(UtcTimestamp{current.end}, frequency);
}
bool valid_frequency(Frequency f) { return rank(f) >= 0; }
}

AggregationResult aggregate_ohlc(const std::vector<MarketObservation>& observations,
                                 Frequency target_frequency, std::size_t max_periods) {
    if (observations.empty()) return {};
    if (max_periods == 0U) throw std::invalid_argument("Limit okresów musi być dodatni.");
    const auto& first = observations.front();
    const Frequency source_frequency = first.frequency;
    if (!valid_frequency(source_frequency) || !valid_frequency(target_frequency) ||
        source_frequency == Frequency::Tick || source_frequency == Frequency::Irregular ||
        target_frequency == Frequency::Tick || target_frequency == Frequency::Irregular)
        throw std::invalid_argument("Agregacja wymaga obsługiwanej częstotliwości regularnej.");
    if (rank(target_frequency) < rank(source_frequency))
        throw std::invalid_argument("Agregacja nie może zwiększać częstotliwości ani tworzyć danych.");
    if (first.instrument_id.empty() || first.provider_id.empty())
        throw std::invalid_argument("Szereg wymaga identyfikatora instrumentu i dostawcy.");

    std::vector<const MarketObservation*> ordered;
    ordered.reserve(observations.size());
    std::set<std::string> ids;
    for (const auto& observation : observations) {
        if (observation.instrument_id != first.instrument_id ||
            observation.provider_id != first.provider_id ||
            observation.price_kind != first.price_kind ||
            observation.frequency != source_frequency)
            throw std::invalid_argument("Wejście zawiera więcej niż jeden szereg danych.");
        if (observation.id.empty() || !ids.insert(observation.id).second)
            throw std::invalid_argument("Identyfikatory obserwacji muszą być niepuste i unikalne.");
        if (!std::isfinite(observation.value))
            throw std::invalid_argument("Obserwacja zawiera cenę niefinitywną.");
        switch (observation.price_kind) {
        case PriceKind::Reference: case PriceKind::Average: case PriceKind::Bid:
        case PriceKind::Ask: case PriceKind::Mid: case PriceKind::Spot:
        case PriceKind::Transaction: break;
        default: throw std::invalid_argument("Nieznany rodzaj ceny w szeregu.");
        }
        switch (observation.quality) {
        case QualityStatus::Unreviewed: case QualityStatus::Validated:
        case QualityStatus::Suspect: break;
        default: throw std::invalid_argument("Nieznany status jakości w szeregu.");
        }
        switch (observation.period_status) {
        case PeriodStatus::Complete: case PeriodStatus::Partial:
        case PeriodStatus::Provisional: break;
        default: throw std::invalid_argument("Nieznany status okresu w szeregu.");
        }
        ordered.push_back(&observation);
    }
    std::stable_sort(ordered.begin(), ordered.end(), [](const auto* a, const auto* b) {
        return std::tie(a->observed_at.unix_seconds, a->id) <
               std::tie(b->observed_at.unix_seconds, b->id);
    });

    std::map<std::int64_t, OhlcCandle> by_period;
    for (const auto* observation : ordered) {
        const Bucket bucket = bucket_for(observation->observed_at, target_frequency);
        auto [it, inserted] = by_period.try_emplace(bucket.start);
        auto& candle = it->second;
        if (inserted) {
            candle.instrument_id = first.instrument_id;
            candle.provider_id = first.provider_id;
            candle.price_kind = first.price_kind;
            candle.source_frequency = source_frequency;
            candle.frequency = target_frequency;
            candle.period_start = UtcTimestamp{bucket.start};
            candle.period_end = UtcTimestamp{bucket.end};
            candle.open = candle.high = candle.low = candle.close = observation->value;
            candle.period_status = observation->period_status;
            candle.quality = observation->quality;
        } else {
            candle.high = std::max(candle.high, observation->value);
            candle.low = std::min(candle.low, observation->value);
            candle.close = observation->value;
            if (observation->period_status == PeriodStatus::Provisional)
                candle.period_status = PeriodStatus::Provisional;
            else if (observation->period_status == PeriodStatus::Partial &&
                     candle.period_status == PeriodStatus::Complete)
                candle.period_status = PeriodStatus::Partial;
            if (observation->quality == QualityStatus::Suspect)
                candle.quality = QualityStatus::Suspect;
            else if (observation->quality == QualityStatus::Unreviewed &&
                     candle.quality == QualityStatus::Validated)
                candle.quality = QualityStatus::Unreviewed;
        }
        ++candle.observation_count;
        candle.has_intraperiod_range = candle.observation_count > 1U;
        candle.source_observation_ids.push_back(observation->id);
        candle.quality_flags.insert(candle.quality_flags.end(),
                                    observation->quality_flags.begin(),
                                    observation->quality_flags.end());
    }

    AggregationResult result;
    if (by_period.size() > max_periods)
        throw std::length_error("Liczba świec przekracza limit agregacji.");
    result.candles.reserve(by_period.size());
    for (auto& [start, candle] : by_period) {
        (void)start;
        std::sort(candle.quality_flags.begin(), candle.quality_flags.end());
        candle.quality_flags.erase(std::unique(candle.quality_flags.begin(),
                                                candle.quality_flags.end()),
                                   candle.quality_flags.end());
        result.candles.push_back(std::move(candle));
    }
    const std::int64_t first_start = result.candles.front().period_start.unix_seconds;
    const std::int64_t last_start = result.candles.back().period_start.unix_seconds;
    std::int64_t cursor = first_start;
    std::size_t traversed = 0;
    while (cursor < last_start) {
        const Bucket next = next_bucket(cursor, target_frequency);
        if (next.start <= cursor) throw std::logic_error("Okres agregacji nie przesuwa granicy.");
        if (++traversed > max_periods)
            throw std::length_error("Zakres luk przekracza limit agregacji.");
        if (!by_period.contains(next.start))
            result.gaps.push_back({UtcTimestamp{next.start}, UtcTimestamp{next.end}, target_frequency});
        cursor = next.start;
    }
    return result;
}

} // namespace monitor::market
