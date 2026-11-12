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

    const std::vector<MarketObservation> monthly_quarters{
        observation("q1", "2023-10-15T10:00:00Z", 10.0, Frequency::Monthly),
        observation("q2", "2023-11-15T10:00:00Z", 12.0, Frequency::Monthly),
        observation("q3", "2024-01-15T10:00:00Z", 15.0, Frequency::Monthly),
        observation("q4", "2024-04-15T10:00:00Z", 18.0, Frequency::Monthly),
        observation("q5", "2024-07-15T10:00:00Z", 21.0, Frequency::Monthly),
        observation("q6", "2025-01-15T10:00:00Z", 24.0, Frequency::Monthly)
    };
    const auto quarterly = time_aware_rolling_regression(
        monthly_quarters, Frequency::Quarterly, 2U, WidthMethod::SampleSd);
    require(quarterly.size() == 3U, "kwartały sąsiadujące tworzą okna do luki w Q4 2024");
    require(quarterly[0].window_start.unix_seconds ==
                parse_utc_timestamp("2023-10-01T00:00:00Z").unix_seconds &&
            quarterly[0].window_end.unix_seconds ==
                parse_utc_timestamp("2024-04-01T00:00:00Z").unix_seconds,
            "kwartał IV i I prawidłowo przechodzą przez granicę roku");
    require(quarterly[2].segment_index == 0U,
            "segment kwartalny pozostaje ciągły do Q3 2024");

    const std::vector<MarketObservation> monthly_halves{
        observation("h1", "2024-01-15T10:00:00Z", 2.0, Frequency::Monthly),
        observation("h2", "2024-06-15T10:00:00Z", 4.0, Frequency::Monthly),
        observation("h3", "2024-07-15T10:00:00Z", 6.0, Frequency::Monthly),
        observation("h4", "2024-12-15T10:00:00Z", 8.0, Frequency::Monthly),
        observation("h5", "2025-01-15T10:00:00Z", 10.0, Frequency::Monthly)
    };
    const auto half_yearly = time_aware_rolling_regression(
        monthly_halves, Frequency::SemiAnnual, 2U, WidthMethod::SampleSd);
    require(half_yearly.size() == 2U, "półrocza sąsiadujące tworzą dwa okna");
    require(half_yearly[1].window_start.unix_seconds ==
                parse_utc_timestamp("2024-07-01T00:00:00Z").unix_seconds &&
            half_yearly[1].window_end.unix_seconds ==
                parse_utc_timestamp("2025-07-01T00:00:00Z").unix_seconds,
            "granica półrocza i roku jest poprawna");

    const std::vector<MarketObservation> annual_data{
        observation("y1", "2023-02-28T10:00:00Z", 1.0, Frequency::Monthly),
        observation("y2", "2024-02-29T10:00:00Z", 2.0, Frequency::Monthly),
        observation("y3", "2025-03-01T10:00:00Z", 4.0, Frequency::Monthly)
    };
    const auto annual = time_aware_rolling_regression(
        annual_data, Frequency::Annual, 2U, WidthMethod::SampleSd);
    require(annual.size() == 2U, "lata kalendarzowe tworzą dwa sąsiadujące okna");
    require(annual[0].window_start.unix_seconds ==
                parse_utc_timestamp("2023-01-01T00:00:00Z").unix_seconds &&
            annual[0].window_end.unix_seconds ==
                parse_utc_timestamp("2025-01-01T00:00:00Z").unix_seconds,
            "roczne okno zachowuje granice kalendarzowe w roku przestępnym");

    const std::vector<MarketObservation> method_fixture{
        observation("r1", "2024-02-26T12:00:00Z", 10.0),
        observation("r2", "2024-02-27T12:00:00Z", 11.0),
        observation("r3", "2024-03-04T12:00:00Z", 13.0),
        observation("r4", "2024-03-05T12:00:00Z", 14.0),
        observation("r5", "2024-03-11T12:00:00Z", 17.0),
        observation("r6", "2024-03-12T12:00:00Z", 18.0)
    };
    const std::vector<double> expected_values{11.0, 14.0, 18.0};
    for (const auto method : {WidthMethod::PopulationRms, WidthMethod::SampleSd,
                              WidthMethod::ResidualStandardErrorNMinus2}) {
        const auto htf = time_aware_rolling_regression(
            method_fixture, Frequency::Weekly, 3U, method);
        const auto expected = linear_regression(expected_values, method);
        require(htf.size() == 1U, "trzy okresy HTF tworzą jedno okno");
        require(near(htf[0].regression.slope, expected.slope) &&
                near(htf[0].regression.intercept, expected.intercept) &&
                near(htf[0].regression.residual_width, expected.residual_width),
                "każda metoda HTF odpowiada wspólnemu silnikowi regresji");
    }


    // Harmonogram sesji pomija weekendy i święto 2024-01-09.
    MarketSessionCalendar session_calendar;
    session_calendar.calendar_id = "XAU-UTC-weekday-fixture-v1";
    session_calendar.version = 4U;
    session_calendar.timezone = "Europe/Warsaw";
    session_calendar.instrument_id = "XAU";
    session_calendar.provider_id = "fixture";
    session_calendar.source_frequency = Frequency::Daily;
    const std::vector<std::string> session_dates{
        "2024-01-01T12:00:00Z", "2024-01-02T12:00:00Z",
        "2024-01-03T12:00:00Z", "2024-01-04T12:00:00Z",
        "2024-01-05T12:00:00Z",
        "2024-01-08T12:00:00Z", "2024-01-10T12:00:00Z",
        "2024-01-11T12:00:00Z", "2024-01-12T12:00:00Z",
        "2024-01-15T12:00:00Z", "2024-01-16T12:00:00Z",
        "2024-01-17T12:00:00Z", "2024-01-18T12:00:00Z",
        "2024-01-19T12:00:00Z"
    };
    std::vector<MarketObservation> session_data;
    for (std::size_t i = 0; i < session_dates.size(); ++i) {
        session_calendar.expected_observation_times.push_back(parse_utc_timestamp(session_dates[i]));
        session_data.push_back(observation("session-" + std::to_string(i),
                                           session_dates[i].c_str(),
                                           static_cast<double>(i + 1U)));
    }
    const auto session_windows = session_aware_rolling_regression(
        session_data, session_calendar, Frequency::Weekly, 2U, WidthMethod::SampleSd);
    require(session_windows.size() == 2U,
            "weekendy i święto wyłączone z harmonogramu nie przerywają serii");
    require(session_windows[0].calendar_id == session_calendar.calendar_id &&
            session_windows[0].calendar_version == session_calendar.version,
            "wynik regresji zachowuje identyfikator i wersję kalendarza");
    require(session_windows[0].source_observation_ids.size() == 9U,
            "pochodzenie zawiera wszystkie sesje dwóch tygodni");

    auto missing_session_data = session_data;
    missing_session_data.erase(missing_session_data.begin() + 6);
    const auto missing_session_windows = session_aware_rolling_regression(
        missing_session_data, session_calendar, Frequency::Weekly, 2U, WidthMethod::SampleSd);
    require(missing_session_windows.empty(),
            "brak zaplanowanej sesji rozdziela segmenty");

    auto suspect_session_data = session_data;
    suspect_session_data[6].quality = QualityStatus::Suspect;
    const auto suspect_windows = session_aware_rolling_regression(
        suspect_session_data, session_calendar, Frequency::Weekly, 2U, WidthMethod::SampleSd);
    require(suspect_windows.empty(),
            "podejrzana obserwacja rozdziela segmenty");

    auto outside_calendar = session_data;
    outside_calendar.push_back(observation("outside", "2024-01-06T12:00:00Z", 99.0));
    throws([&] { (void)session_aware_rolling_regression(
        outside_calendar, session_calendar, Frequency::Weekly, 2U, WidthMethod::SampleSd); },
        "obserwacja spoza harmonogramu jest odrzucana");

    auto unsorted_calendar = session_calendar;
    std::swap(unsorted_calendar.expected_observation_times[0],
              unsorted_calendar.expected_observation_times[1]);
    throws([&] { (void)session_aware_rolling_regression(
        session_data, unsorted_calendar, Frequency::Weekly, 2U, WidthMethod::SampleSd); },
        "nieuporządkowany harmonogram jest odrzucany");

    std::cout << "Wszystkie testy regresji czasowej HTF zakończone powodzeniem.\n";
    return EXIT_SUCCESS;
}
