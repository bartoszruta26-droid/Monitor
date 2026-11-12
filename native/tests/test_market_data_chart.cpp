#include "monitor/market_data_chart.hpp"

#include <algorithm>
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
    std::cerr << "FAIL: expected exception: " << message << '\n'; std::exit(EXIT_FAILURE);
}
MarketObservation obs(std::string id, const char* time, double value) {
    MarketObservation x;
    x.id = std::move(id); x.instrument_id = "FX:EUR/PLN"; x.provider_id = "nbp";
    x.observed_at = parse_utc_timestamp(time);
    x.received_at = UtcTimestamp{x.observed_at.unix_seconds + 60};
    x.frequency = Frequency::Daily; x.price_kind = PriceKind::Average;
    x.value = value; x.quality = QualityStatus::Validated;
    return x;
}
}
int main() {
    using namespace monitor::market;
    const std::vector<MarketObservation> data{
        obs("p2", "2024-01-03T12:00:00Z", 4.2),
        obs("p1", "2024-01-01T12:00:00Z", 4.0),
        obs("p3", "2024-01-04T12:00:00Z", 4.4)
    };
    const auto line = build_point_chart(data);
    require(line.kind == ChartKind::Line && line.points.size() == 3,
            "wykres liniowy z kompletem punktów");
    require(line.points.front().source_observation_id == "p1" &&
            line.points.back().source_observation_id == "p3",
            "chronologiczne sortowanie punktów");
    require(line.gaps.size() == 1 &&
            line.gaps.front().period_start.unix_seconds ==
            parse_utc_timestamp("2024-01-02T00:00:00Z").unix_seconds,
            "luka na wykresie punktowym pozostaje jawna");
    const auto area = build_point_chart(data, ChartKind::Area);
    require(area.kind == ChartKind::Area && area.points.size() == 3,
            "wykres obszarowy");

    const auto aggregation = aggregate_ohlc(data, Frequency::Weekly);
    const auto candles = build_candlestick_chart(data, aggregation);
    require(candles.kind == ChartKind::Candlestick && candles.candles.size() == 1 &&
            candles.display_frequency == Frequency::Weekly, "wykres świecowy z agregacji");
    require(candles.candles.front().semantics == OhlcSemantics::SampledObservations &&
            candles.candles.front().has_intraperiod_range, "jawna semantyka świecy próbkowanej");
    require(candles.source_frequency == Frequency::Daily, "zachowanie częstotliwości źródłowej");

    auto wrong_candles = aggregation;
    wrong_candles.candles.front().provider_id = "other";
    throws([&] { (void)build_candlestick_chart(data, wrong_candles); },
           "świece muszą pochodzić z tego samego szeregu");
    auto forged_source_ids = aggregation;
    forged_source_ids.candles.front().source_observation_ids.front() = "not-a-source-id";
    throws([&] { (void)build_candlestick_chart(data, forged_source_ids); },
           "identyfikatory źródeł muszą być zweryfikowane");
    auto forged_gap = aggregation;
    forged_gap.gaps.push_back({parse_utc_timestamp("2024-01-02T00:00:00Z"),
                               parse_utc_timestamp("2024-01-03T00:00:00Z"), Frequency::Weekly});
    throws([&] { (void)build_candlestick_chart(data, forged_gap); },
           "luki muszą odpowiadać obserwacjom");
    throws([&] { (void)build_point_chart(data, ChartKind::Candlestick); },
           "wymagany jawny model świec");
    throws([&] { (void)build_point_chart(data, static_cast<ChartKind>(99)); },
           "nieznany rodzaj wykresu");
    throws([&] { (void)build_point_chart(data, ChartKind::Line, 1); },
           "limit punktów");
    throws([&] { (void)build_candlestick_chart(data, aggregation, 0); },
           "limit świec dodatni");

    const std::vector<MarketObservation> empty;
    const auto empty_chart = build_point_chart(empty);
    require(empty_chart.points.empty(), "pusty wykres bez sztucznego punktu zero");
    std::cout << "Wszystkie testy modelu wykresów zakończone powodzeniem.\n";
}
