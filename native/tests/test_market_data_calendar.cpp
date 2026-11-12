#include "monitor/market_data_calendar.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
using namespace monitor::market;
void require(bool value,const char* message) {
    if(!value) { std::cerr<<"FAIL: "<<message<<'\n'; std::exit(EXIT_FAILURE); }
}
template<class F> void throws(F&& fn,const char* message) {
    try { fn(); } catch(const std::exception&) { return; }
    std::cerr<<"FAIL: expected exception: "<<message<<'\n'; std::exit(EXIT_FAILURE);
}
LocalSessionSchedule base() {
    LocalSessionSchedule s;
    s.calendar_id="warsaw-weekdays";
    s.version=3;
    s.instrument_id="FX:EUR/PLN";
    s.provider_id="nbp-a";
    s.source_frequency=Frequency::Daily;
    s.timezone="Europe/Warsaw";
    s.start_date=parse_date("2024-03-29");
    s.end_date_exclusive=parse_date("2024-04-03");
    s.weekdays_iso={1,2,3,4,5};
    s.local_times={{12,0,0}};
    return s;
}
}
int main() {
    auto s=base();
    s.excluded_dates={parse_date("2024-04-01")};
    const auto generated=generate_session_calendar(s);
    require(generated.calendar_id==s.calendar_id&&generated.version==3,"identyfikator i wersja są zachowane");
    require(generated.expected_observation_times.size()==3,"weekend i data wyłączona są pomijane");
    require(format_utc_timestamp(generated.expected_observation_times[0])=="2024-03-29T11:00:00Z",
            "przed zmianą czasu Warszawa ma UTC+1");
    require(format_utc_timestamp(generated.expected_observation_times[1])=="2024-04-02T10:00:00Z",
            "po zmianie czasu Warszawa ma UTC+2");
    require(format_utc_timestamp(generated.expected_observation_times[2])=="2024-04-03T10:00:00Z",
            "kolejny dzień zachowuje lokalną godzinę");

    auto spring=base();
    spring.start_date=parse_date("2024-03-31");
    spring.end_date_exclusive=parse_date("2024-04-01");
    spring.weekdays_iso={7};
    spring.local_times={{2,30,0}};
    throws([&]{(void)generate_session_calendar(spring);},"nieistniejący czas domyślnie jest odrzucany");
    spring.nonexistent_time_policy=NonexistentLocalTimePolicy::NextValidTime;
    const auto shifted=generate_session_calendar(spring);
    require(format_utc_timestamp(shifted.expected_observation_times.front())=="2024-03-31T01:00:00Z",
            "polityka NextValidTime przesuwa slot do pierwszej poprawnej chwili");

    auto autumn=base();
    autumn.start_date=parse_date("2024-10-27");
    autumn.end_date_exclusive=parse_date("2024-10-28");
    autumn.weekdays_iso={7};
    autumn.local_times={{2,30,0}};
    throws([&]{(void)generate_session_calendar(autumn);},"powtórzony czas domyślnie wymaga decyzji");
    autumn.ambiguous_time_policy=AmbiguousLocalTimePolicy::EarlierInstant;
    const auto earlier=generate_session_calendar(autumn);
    autumn.ambiguous_time_policy=AmbiguousLocalTimePolicy::LaterInstant;
    const auto later=generate_session_calendar(autumn);
    require(later.expected_observation_times.front().unix_seconds-
            earlier.expected_observation_times.front().unix_seconds==3600,
            "wybór wcześniejszego i późniejszego czasu różni się godziną");

    auto bad=base();
    bad.timezone="Mars/Phobos";
    throws([&]{(void)generate_session_calendar(bad);},"nieznana strefa IANA jest odrzucana");
    bad=base();
    bad.weekdays_iso={1,1};
    throws([&]{(void)generate_session_calendar(bad);},"powtórzone dni tygodnia są odrzucane");
    bad=base();
    bad.local_times={{25,0,0}};
    throws([&]{(void)generate_session_calendar(bad);},"godzina poza zakresem jest odrzucana");
    bad=base();
    bad.end_date_exclusive=bad.start_date;
    throws([&]{(void)generate_session_calendar(bad);},"pusty zakres dat jest odrzucany");
    std::cout<<"All IANA session calendar tests passed.\n";
}
