#include "monitor/market_data_aggregation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using namespace monitor::market;
void require(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}
template<class F> void throws(F&& fn, const char* message) {
    try { fn(); } catch (const std::exception&) { return; }
    std::cerr << "FAIL: expected exception: " << message << '\n';
    std::exit(EXIT_FAILURE);
}
MarketObservation obs(std::string id, const char* time, double value,
                      Frequency frequency = Frequency::Daily) {
    MarketObservation x;
    x.id = std::move(id); x.instrument_id = "FX:EUR/PLN"; x.provider_id = "nbp";
    x.observed_at = parse_utc_timestamp(time);
    x.received_at = UtcTimestamp{x.observed_at.unix_seconds + 60};
    x.frequency = frequency; x.price_kind = PriceKind::Average; x.value = value;
    x.quality = QualityStatus::Validated; x.period_status = PeriodStatus::Complete;
    return x;
}
}
int main() {
    using namespace monitor::market;
    const std::vector<MarketObservation> daily{
        obs("d1", "2024-01-01T12:00:00Z", 4.0),
        obs("d2", "2024-01-02T12:00:00Z", 4.4),
        obs("d3", "2024-01-04T12:00:00Z", 3.8),
        obs("d4", "2024-01-31T12:00:00Z", 4.2)
    };
    const auto monthly = aggregate_ohlc(daily, Frequency::Monthly);
    require(monthly.candles.size() == 1, "jeden kubełek miesięczny");
    const auto& jan = monthly.candles.front();
    require(jan.open == 4.0 && jan.high == 4.4 && jan.low == 3.8 && jan.close == 4.2,
            "OHLC zgodne z chronologią");
    require(jan.semantics == OhlcSemantics::SampledObservations && jan.has_intraperiod_range,
            "jawna semantyka próbkowanego OHLC");
    require(jan.observation_count == 4 && jan.source_observation_ids.size() == 4,
            "liczność i pochodzenie świecy");
    require(jan.period_start.unix_seconds == parse_utc_timestamp("2024-01-01T00:00:00Z").unix_seconds &&
            jan.period_end.unix_seconds == parse_utc_timestamp("2024-02-01T00:00:00Z").unix_seconds,
            "granice miesiąca z końcem wyłącznym");

    const std::vector<MarketObservation> gaps{
        obs("a", "2024-01-01T08:00:00Z", 1.0),
        obs("b", "2024-01-03T08:00:00Z", 2.0)
    };
    const auto daily_gaps = aggregate_ohlc(gaps, Frequency::Daily);
    require(daily_gaps.candles.size() == 2 && daily_gaps.gaps.size() == 1,
            "jawna luka pomiędzy okresami");
    require(daily_gaps.gaps.front().period_start.unix_seconds ==
            parse_utc_timestamp("2024-01-02T00:00:00Z").unix_seconds, "data luki");

    const std::vector<MarketObservation> weekly_input{
        obs("sun", "2024-01-07T23:59:00Z", 5.0),
        obs("mon", "2024-01-08T00:00:00Z", 6.0)
    };
    const auto weekly = aggregate_ohlc(weekly_input, Frequency::Weekly);
    require(weekly.candles.size() == 2, "tydzień rozpoczyna się w poniedziałek UTC");

    const std::vector<MarketObservation> monthly_source{
        obs("m1", "2024-01-15T00:00:00Z", 10.0, Frequency::Monthly),
        obs("m2", "2024-02-15T00:00:00Z", 11.0, Frequency::Monthly)
    };
    const auto retained = aggregate_ohlc(monthly_source, Frequency::Monthly);
    require(retained.candles.size() == 2 &&
            retained.candles.front().source_frequency == Frequency::Monthly &&
            !retained.candles.front().has_intraperiod_range,
            "dane miesięczne pozostają miesięczne");
    throws([&] { (void)aggregate_ohlc(monthly_source, Frequency::Daily); },
           "zakaz sztucznego zwiększania częstotliwości");

    auto partial = gaps;
    partial[1].period_status = PeriodStatus::Provisional;
    partial[1].quality = QualityStatus::Suspect;
    partial[1].quality_flags = {"provider_revision"};
    const auto quality = aggregate_ohlc(partial, Frequency::Monthly);
    require(quality.candles.front().period_status == PeriodStatus::Provisional &&
            quality.candles.front().quality == QualityStatus::Suspect &&
            quality.candles.front().quality_flags.front() == "provider_revision",
            "konserwatywne przenoszenie statusu i jakości");

    auto unordered = daily;
    std::reverse(unordered.begin(), unordered.end());
    const auto deterministic = aggregate_ohlc(unordered, Frequency::Monthly);
    require(deterministic.candles.front().open == jan.open &&
            deterministic.candles.front().close == jan.close,
            "wynik niezależny od kolejności wejścia");

    throws([&] { (void)aggregate_ohlc(daily, Frequency::Monthly, 0); },
           "limit okresów dodatni");
    auto mixed = daily;
    mixed.back().provider_id = "other";
    throws([&] { (void)aggregate_ohlc(mixed, Frequency::Monthly); },
           "zakaz łączenia różnych dostawców");

    const std::vector<MarketObservation> calendar_edges{
        obs("feb1", "2024-02-01T00:00:00Z", 2.0),
        obs("feb2", "2024-02-29T23:59:59Z", 3.0),
        obs("mar1", "2024-03-01T00:00:00Z", 4.0),
        obs("apr1", "2024-04-01T00:00:00Z", 5.0)
    };
    const auto quarters = aggregate_ohlc(calendar_edges, Frequency::Quarterly);
    require(quarters.candles.size() == 2, "granica kwartału");
    require(quarters.candles.front().period_end.unix_seconds ==
            parse_utc_timestamp("2024-04-01T00:00:00Z").unix_seconds, "koniec pierwszego kwartału");
    const auto halves = aggregate_ohlc(calendar_edges, Frequency::SemiAnnual);
    require(halves.candles.size() == 1 &&
            halves.candles.front().period_end.unix_seconds ==
            parse_utc_timestamp("2024-07-01T00:00:00Z").unix_seconds, "granica półrocza");
    const auto years = aggregate_ohlc(calendar_edges, Frequency::Annual);
    require(years.candles.size() == 1 &&
            years.candles.front().period_end.unix_seconds ==
            parse_utc_timestamp("2025-01-01T00:00:00Z").unix_seconds, "granica roku");

    const std::vector<MarketObservation> intraday{
        obs("i1", "2024-03-31T23:59:59Z", 1.0, Frequency::Hour),
        obs("i2", "2024-04-01T00:00:00Z", 2.0, Frequency::Hour)
    };
    const auto intraday_daily = aggregate_ohlc(intraday, Frequency::Daily);
    require(intraday_daily.candles.size() == 2,
            "granica doby UTC niezależna od czasu letniego");

    auto invalid_enum = daily;
    invalid_enum.front().quality = static_cast<QualityStatus>(255);
    throws([&] { (void)aggregate_ohlc(invalid_enum, Frequency::Monthly); },
           "odrzucanie nieznanego statusu jakości");

    auto duplicate_id = daily;
    duplicate_id.back().id = duplicate_id.front().id;
    throws([&] { (void)aggregate_ohlc(duplicate_id, Frequency::Monthly); },
           "unikalne identyfikatory obserwacji");
    throws([&] { (void)aggregate_ohlc(daily, Frequency::Daily, 1); },
           "limit liczby świec");

    std::cout << "Wszystkie testy agregacji danych rynkowych zakończone powodzeniem.\n";
}
