#pragma once

#include "monitor/market_data.hpp"

#include <vector>

namespace monitor::market {

enum class AmbiguousLocalTimePolicy {
    Reject,
    EarlierInstant,
    LaterInstant
};

enum class NonexistentLocalTimePolicy {
    Reject,
    NextValidTime
};

struct LocalTimeOfDay {
    unsigned hour{};
    unsigned minute{};
    unsigned second{};
};

struct LocalSessionSchedule {
    std::string calendar_id;
    std::uint32_t version{1};
    std::string instrument_id;
    std::string provider_id;
    Frequency source_frequency{Frequency::Daily};
    std::string timezone;
    CivilDate start_date;
    CivilDate end_date_exclusive;
    // ISO weekday: Monday=1 ... Sunday=7.
    std::vector<unsigned> weekdays_iso;
    std::vector<LocalTimeOfDay> local_times;
    std::vector<CivilDate> excluded_dates;
    AmbiguousLocalTimePolicy ambiguous_time_policy{AmbiguousLocalTimePolicy::Reject};
    NonexistentLocalTimePolicy nonexistent_time_policy{NonexistentLocalTimePolicy::Reject};
};

enum class NbpPublicationTable { A, B, C };

// Local publication interval, never an inferred observation timestamp.
struct NbpPublicationWindow {
    NbpPublicationTable table{NbpPublicationTable::A};
    CivilDate local_date;
    std::string timezone{"Europe/Warsaw"};
    LocalTimeOfDay starts_at;
    LocalTimeOfDay ends_at;
};

// Statutory Polish public holidays (excluding ordinary Sundays, which are
// already excluded by the weekday rule). Includes movable Easter holidays.
[[nodiscard]] std::vector<CivilDate> polish_statutory_holidays(int year);

// Uses the statutory Polish holiday dates calculated for every year touched
// by the half-open range. Extraordinary days off still require the explicit
// holiday-list overload below.
[[nodiscard]] std::vector<NbpPublicationWindow> generate_nbp_publication_windows(
    const CivilDate& start_date, const CivilDate& end_date_exclusive);

// The holiday list is explicit input from an authoritative, versioned source.
[[nodiscard]] std::vector<NbpPublicationWindow> generate_nbp_publication_windows(
    const CivilDate& start_date, const CivilDate& end_date_exclusive,
    const std::vector<CivilDate>& public_holidays);

// Uses ICU's IANA time-zone database. The end date is exclusive.
// A nonexistent local time is either rejected or moved to the first valid
// local time after the DST gap. Repeated local times require an explicit
// earlier/later policy; no implicit DST choice is made.
[[nodiscard]] MarketSessionCalendar generate_session_calendar(
    const LocalSessionSchedule& schedule);

} // namespace monitor::market
