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

// Uses ICU's IANA time-zone database. The end date is exclusive.
// A nonexistent local time is either rejected or moved to the first valid
// local time after the DST gap. Repeated local times require an explicit
// earlier/later policy; no implicit DST choice is made.
[[nodiscard]] MarketSessionCalendar generate_session_calendar(
    const LocalSessionSchedule& schedule);

} // namespace monitor::market
