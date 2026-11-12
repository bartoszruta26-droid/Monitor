#include "monitor/market_data_regression.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace monitor;
using namespace monitor::market;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

template<class F>
void throws(F&& fn, const char* message) {
    try { fn(); }
    catch (const std::exception&) { return; }
    std::cerr << "FAIL: expected exception: " << message << '\n';
    std::exit(EXIT_FAILURE);
}

MarketObservation observation(std::string id, const char* timestamp, double value,
                              Frequency frequency = Frequency::Daily) {
    MarketObservation item;
    item.id = std::move(id);
    item.instrument_id = "XAU";
    item.provider_id = "fixture";
    item.observed_at = parse_utc_timestamp(timestamp);
    item.received_at = item.observed_at;
    item.frequency = frequency;
    item.value = value;
    item.quality = QualityStatus::Validated;
    return item;
}

bool near(double a, double b) { return std::abs(a - b) < 1e-10; }
}

int main() {
    const std::vector<MarketObservation> february{
        observation("a", "2024-02-28T12:00:00Z", 10.0),
        observation("b", "2024-02-29T12:00:00Z", 11.0),
        observation("c", "2024-03-01T12:00:00Z", 12.0),
        observation("d", "2024-03-04T12:00:00Z", 20.0),
        observation("e", "2024-03-05T12:00:00Z", 21.0)
    };
    const auto weekly = time_aware_rolling_regression(
        february, Frequency::Weekly, 2U, WidthMethod::SampleSd);
    require(weekly.size() == 1U, "tygodnie sąsiadujące dają jedno pełne okno");
    require(weekly[0].window_start.unix_seconds == parse_utc_timestamp("2024-02-26T00:00:00Z").unix_seconds,
            "początek tygodnia ISO przypada w poniedziałek");
    require(weekly[0].window_end.unix_seconds == parse_utc_timestamp("2024-03-11T00:00:00Z").unix_seconds,
            "koniec okna tygodniowego jest wyłączny");
    require(weekly[0].last_observed_at.unix_seconds == parse_utc_timestamp("2024-03-05T12:00:00Z").unix_seconds,
            "czas próbki końcowej pochodzi z rzeczywistej obserwacji");
    require(near(weekly[0].last_value, 21.0), "polityka domyślna wybiera ostatnią obserwację");
    require(weekly[0].source_observation_ids.size() == 5U,
            "pochodzenie obejmuje wszystkie obserwacje w oknie HTF");

    const auto first_policy = time_aware_rolling_regression(
        february, Frequency::Weekly, 2U, WidthMethod::SampleSd,
        HtfSamplePolicy::FirstObservation);
    require(first_policy.size() == 1U && near(first_policy[0].last_value, 20.0),
            "polityka pierwszej obserwacji jest jawna");

    const std::vector<MarketObservation> with_gap{
        observation("g1", "2024-01-01T12:00:00Z", 1.0),
        observation("g2", "2024-01-08T12:00:00Z", 2.0),
        observation("g3", "2024-01-22T12:00:00Z", 4.0),
        observation("g4", "2024-01-29T12:00:00Z", 5.0)
    };
    const auto segmented = time_aware_rolling_regression(
        with_gap, Frequency::Weekly, 2U, WidthMethod::SampleSd);
    require(segmented.size() == 2U, "luka dzieli serię na dwa segmenty");
    require(segmented[0].segment_index == 0U && segmented[1].segment_index == 1U,
            "identyfikatory segmentów są deterministyczne");
    require(segmented[0].window_start.unix_seconds == parse_utc_timestamp("2024-01-01T00:00:00Z").unix_seconds &&
            segmented[1].window_start.unix_seconds == parse_utc_timestamp("2024-01-22T00:00:00Z").unix_seconds,
            "żadne okno nie przekracza brakującego tygodnia");

    const std::vector<MarketObservation> month_boundary{
        observation("m1", "2023-12-30T10:00:00Z", 2.0),
        observation("m2", "2024-01-02T10:00:00Z", 4.0),
        observation("m3", "2024-02-29T10:00:00Z", 6.0)
    };
    const auto monthly = time_aware_rolling_regression(
        month_boundary, Frequency::Monthly, 2U, WidthMethod::SampleSd);
    require(monthly.size() == 2U, "grudzień, styczeń i luty tworzą dwa okna miesięczne");
    require(monthly[0].window_start.unix_seconds == parse_utc_timestamp("2023-12-01T00:00:00Z").unix_seconds &&
            monthly[0].window_end.unix_seconds == parse_utc_timestamp("2024-02-01T00:00:00Z").unix_seconds,
            "granica roku jest poprawna dla HTF miesięcznego");

    const std::vector<MarketObservation> duplicate_time{
        observation("x1", "2024-01-01T00:00:00Z", 1.0),
        observation("x2", "2024-01-01T00:00:00Z", 2.0)
    };
    throws([&] { (void)time_aware_rolling_regression(
        duplicate_time, Frequency::Weekly, 2U, WidthMethod::SampleSd); },
        "duplikaty znaczników czasu są odrzucane");
    throws([&] { (void)time_aware_rolling_regression(
        february, Frequency::Weekly, 1U, WidthMethod::SampleSd); },
        "zbyt małe okno");
    throws([&] { (void)time_aware_rolling_regression(
        february, Frequency::Weekly, 2U, WidthMethod::ResidualStandardErrorNMinus2); },
        "n-2 wymaga co najmniej trzech próbek");
    throws([&] { (void)time_aware_rolling_regression(
        february, Frequency::Weekly, 2U, WidthMethod::SampleSd,
        static_cast<HtfSamplePolicy>(99)); },
        "nieznana polityka próbki");
    throws([&] { (void)time_aware_rolling_regression(
        february, Frequency::Minute, 2U, WidthMethod::SampleSd); },
        "regresja odrzuca próbę zwiększenia częstotliwości");
    throws([&] { (void)time_aware_rolling_regression(
        february, Frequency::Weekly, 2U, static_cast<WidthMethod>(99)); },
        "nieznana metoda jest odrzucana niezależnie od liczby okien");

    std::cout << "Wszystkie testy regresji czasowej HTF zakończone powodzeniem.\n";
    return EXIT_SUCCESS;
}
