#include "monitor/market_data_calendar.hpp"

#include <algorithm>
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
    s.end_date_exclusive=parse_date("2024-04-04");
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
    const auto nbp=generate_nbp_publication_windows(
        parse_date("2026-11-10"),parse_date("2026-11-13"),{parse_date("2026-11-11")});
    require(nbp.size()==5,"NBP A/B/C windows respect holiday and working days");
    require(nbp[0].table==NbpPublicationTable::A&&format_date(nbp[0].local_date)=="2026-11-10"&&
            nbp[0].starts_at.hour==11&&nbp[0].starts_at.minute==45&&
            nbp[0].ends_at.hour==12&&nbp[0].ends_at.minute==15,
            "NBP A window is a Warsaw local interval, not an observation instant");
    require(nbp[1].table==NbpPublicationTable::B&&format_date(nbp[1].local_date)=="2026-11-10",
            "holiday Wednesday table B moves to the preceding business day");
    require(nbp[2].table==NbpPublicationTable::C&&nbp[2].starts_at.hour==7&&
            nbp[2].starts_at.minute==45&&nbp[2].ends_at.hour==8&&nbp[2].ends_at.minute==15,
            "NBP C uses its distinct morning publication window");
    require(format_date(nbp[3].local_date)=="2026-11-12"&&
            format_date(nbp[4].local_date)=="2026-11-12",
            "NBP public holiday is excluded from daily A and C");
    const auto ordinary=generate_nbp_publication_windows(
        parse_date("2026-11-16"),parse_date("2026-11-20"),{});
    require(std::any_of(ordinary.begin(),ordinary.end(),[](const auto& w) {
        return w.table==NbpPublicationTable::B&&format_date(w.local_date)=="2026-11-18";
    }),"NBP table B normally publishes on Wednesday");
    throws([&]{(void)generate_nbp_publication_windows(
        parse_date("2026-11-10"),parse_date("2026-11-13"),
        {parse_date("2026-11-11"),parse_date("2026-11-11")});},"duplicate holidays are rejected");
    throws([&]{(void)generate_nbp_publication_windows(
        parse_date("2026-11-13"),parse_date("2026-11-13"),{});},"empty NBP range is rejected");
    const auto holidays_2026=polish_statutory_holidays(2026);
    const auto contains_date=[&](const char* date) {
        return std::any_of(holidays_2026.begin(),holidays_2026.end(),[&](const auto& d) {
            return format_date(d)==date;
        });
    };
    require(holidays_2026.size()==14,"Polish statutory holiday list contains 14 dates");
    require(contains_date("2026-04-05")&&contains_date("2026-04-06"),
            "Easter Sunday and Monday are calculated");
    require(contains_date("2026-05-24")&&contains_date("2026-06-04"),
            "Pentecost and Corpus Christi are calculated");
    require(contains_date("2026-11-11")&&contains_date("2026-12-24"),
            "fixed national holidays and Christmas Eve are included");
    const auto holidays_2025=polish_statutory_holidays(2025);
    require(std::any_of(holidays_2025.begin(),holidays_2025.end(),[](const auto& d) {
        return format_date(d)=="2025-12-24";
    }),"Christmas Eve is a public holiday from 2025");
    throws([&]{(void)polish_statutory_holidays(0);},"year zero is rejected");
    const auto automatic_nbp=generate_nbp_publication_windows(
        parse_date("2026-11-10"),parse_date("2026-11-13"));
    require(automatic_nbp.size()==5,"default NBP calendar applies statutory holidays automatically");
    require(std::none_of(automatic_nbp.begin(),automatic_nbp.end(),[](const auto& w) {
        return format_date(w.local_date)=="2026-11-11";
    }),"default NBP calendar excludes 11 November");
    require(std::any_of(automatic_nbp.begin(),automatic_nbp.end(),[](const auto& w) {
        return w.table==NbpPublicationTable::B&&format_date(w.local_date)=="2026-11-10";
    }),"default NBP calendar moves table B for a statutory Wednesday holiday");
    const auto christmas_eve=generate_nbp_publication_windows(
        parse_date("2025-12-23"),parse_date("2025-12-26"));
    require(std::none_of(christmas_eve.begin(),christmas_eve.end(),[](const auto& w) {
        return format_date(w.local_date)=="2025-12-24";
    }),"default NBP calendar excludes Christmas Eve from 2025");
    require(std::any_of(christmas_eve.begin(),christmas_eve.end(),[](const auto& w) {
        return w.table==NbpPublicationTable::B&&format_date(w.local_date)=="2025-12-23";
    }),"table B moves to 23 December when Christmas Eve is a holiday Wednesday");
    std::cout<<"All IANA and NBP publication calendar tests passed.\n";
}
