#include "monitor/market_data_calendar.hpp"

#include <unicode/calendar.h>
#include <unicode/gregocal.h>
#include <unicode/timezone.h>
#include <unicode/unistr.h>
#include <unicode/ucal.h>
#include <unicode/utypes.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>

namespace monitor::market {
namespace {
void validate_frequency(Frequency f) {
    switch(f) {
    case Frequency::Minute: case Frequency::Hour: case Frequency::Daily:
    case Frequency::Weekly: case Frequency::Monthly: case Frequency::Quarterly:
    case Frequency::SemiAnnual: case Frequency::Annual: return;
    case Frequency::Tick: case Frequency::Irregular: break;
    }
    throw std::invalid_argument("Generator wymaga regularnej częstotliwości źródłowej.");
}
void check_icu(UErrorCode status,const char* operation) {
    if(U_FAILURE(status))
        throw std::runtime_error(std::string(operation)+": "+u_errorName(status));
}
std::int64_t local_to_utc(const LocalSessionSchedule& schedule,
                          const CivilDate& date,const LocalTimeOfDay& time) {
    UErrorCode status=U_ZERO_ERROR;
    UBool is_system_id=false;
    icu::UnicodeString canonical;
    const auto requested=icu::UnicodeString::fromUTF8(schedule.timezone);
    icu::TimeZone::getCanonicalID(requested,canonical,is_system_id,status);
    check_icu(status,"Nie można rozpoznać strefy IANA");
    if(!is_system_id)
        throw std::invalid_argument("Nieznana strefa IANA: "+schedule.timezone);
    std::unique_ptr<icu::TimeZone> zone(icu::TimeZone::createTimeZone(canonical));
    if(!zone) throw std::runtime_error("Nie można utworzyć strefy czasowej ICU.");
    icu::GregorianCalendar calendar(zone.release(),status);
    check_icu(status,"Nie można utworzyć kalendarza gregoriańskiego");
    calendar.setLenient(false);
    switch(schedule.ambiguous_time_policy) {
    case AmbiguousLocalTimePolicy::Reject:
        calendar.setRepeatedWallTimeOption(UCAL_WALLTIME_FIRST);
        break;
    case AmbiguousLocalTimePolicy::EarlierInstant:
        calendar.setRepeatedWallTimeOption(UCAL_WALLTIME_FIRST);
        break;
    case AmbiguousLocalTimePolicy::LaterInstant:
        calendar.setRepeatedWallTimeOption(UCAL_WALLTIME_LAST);
        break;
    default: throw std::invalid_argument("Nieznana polityka powtórzonego czasu lokalnego.");
    }
    switch(schedule.nonexistent_time_policy) {
    case NonexistentLocalTimePolicy::Reject:
    case NonexistentLocalTimePolicy::NextValidTime:
        calendar.setSkippedWallTimeOption(UCAL_WALLTIME_NEXT_VALID);
        break;
    default: throw std::invalid_argument("Nieznana polityka nieistniejącego czasu lokalnego.");
    }
    calendar.clear();
    calendar.set(UCAL_YEAR,date.year);
    calendar.set(UCAL_MONTH,static_cast<int32_t>(date.month)-1);
    calendar.set(UCAL_DATE,static_cast<int32_t>(date.day));
    calendar.set(UCAL_HOUR_OF_DAY,static_cast<int32_t>(time.hour));
    calendar.set(UCAL_MINUTE,static_cast<int32_t>(time.minute));
    calendar.set(UCAL_SECOND,static_cast<int32_t>(time.second));
    calendar.set(UCAL_MILLISECOND,0);
    const UDate milliseconds=calendar.getTime(status);
    check_icu(status,"Nie można przeliczyć lokalnego czasu na UTC");
    if(schedule.ambiguous_time_policy==AmbiguousLocalTimePolicy::Reject) {
        calendar.setRepeatedWallTimeOption(UCAL_WALLTIME_LAST);
        calendar.clear();
        calendar.set(UCAL_YEAR,date.year);
        calendar.set(UCAL_MONTH,static_cast<int32_t>(date.month)-1);
        calendar.set(UCAL_DATE,static_cast<int32_t>(date.day));
        calendar.set(UCAL_HOUR_OF_DAY,static_cast<int32_t>(time.hour));
        calendar.set(UCAL_MINUTE,static_cast<int32_t>(time.minute));
        calendar.set(UCAL_SECOND,static_cast<int32_t>(time.second));
        calendar.set(UCAL_MILLISECOND,0);
        const UDate later_milliseconds=calendar.getTime(status);
        check_icu(status,"Nie można sprawdzić powtórzonego czasu lokalnego");
        if(later_milliseconds!=milliseconds)
            throw std::invalid_argument("Podany czas lokalny występuje dwukrotnie z powodu zmiany czasu; wymagana jest jawna polityka.");
    }
    if(schedule.nonexistent_time_policy==NonexistentLocalTimePolicy::Reject) {
        const auto year=calendar.get(UCAL_YEAR,status);
        const auto month=calendar.get(UCAL_MONTH,status)+1;
        const auto day=calendar.get(UCAL_DATE,status);
        const auto hour=calendar.get(UCAL_HOUR_OF_DAY,status);
        const auto minute=calendar.get(UCAL_MINUTE,status);
        const auto second=calendar.get(UCAL_SECOND,status);
        check_icu(status,"Nie można zweryfikować lokalnego czasu");
        if(year!=date.year||month!=static_cast<int32_t>(date.month)||
           day!=static_cast<int32_t>(date.day)||hour!=static_cast<int32_t>(time.hour)||
           minute!=static_cast<int32_t>(time.minute)||second!=static_cast<int32_t>(time.second))
            throw std::invalid_argument("Podany czas lokalny nie istnieje z powodu zmiany czasu.");
    }
    const long double seconds=static_cast<long double>(milliseconds)/1000.0L;
    if(!std::isfinite(milliseconds)||seconds<static_cast<long double>(std::numeric_limits<std::int64_t>::min())||
       seconds>static_cast<long double>(std::numeric_limits<std::int64_t>::max()))
        throw std::overflow_error("Wynik konwersji czasu przekracza zakres UTC.");
    return static_cast<std::int64_t>(std::llround(seconds));
}
}
MarketSessionCalendar generate_session_calendar(const LocalSessionSchedule& s) {
    if(s.calendar_id.empty()||s.version==0U||s.instrument_id.empty()||
       s.provider_id.empty()||s.timezone.empty())
        throw std::invalid_argument("Harmonogram wymaga identyfikatora, wersji, szeregu i strefy.");
    validate_frequency(s.source_frequency);
    if(!is_valid_date(s.start_date)||!is_valid_date(s.end_date_exclusive)||
       !(std::chrono::year{s.start_date.year}/std::chrono::month{s.start_date.month}/
         std::chrono::day{s.start_date.day} <
         std::chrono::year{s.end_date_exclusive.year}/std::chrono::month{s.end_date_exclusive.month}/
         std::chrono::day{s.end_date_exclusive.day}))
        throw std::invalid_argument("Zakres dat harmonogramu jest niepoprawny lub pusty.");
    if(s.weekdays_iso.empty()||s.local_times.empty())
        throw std::invalid_argument("Harmonogram wymaga dni tygodnia i co najmniej jednej godziny.");
    std::set<unsigned> weekdays;
    for(const auto day:s.weekdays_iso)
        if(day<1U||day>7U||!weekdays.insert(day).second)
            throw std::invalid_argument("Dni tygodnia muszą być unikalne w zakresie 1–7.");
    std::set<std::int64_t> excluded;
    for(const auto& date:s.excluded_dates) {
        if(!is_valid_date(date)) throw std::invalid_argument("Niepoprawna data wyłączenia.");
        using namespace std::chrono;
        const sys_days point{year{date.year}/month{date.month}/day{date.day}};
        if(!excluded.insert(point.time_since_epoch().count()).second)
            throw std::invalid_argument("Powtórzona data wyłączenia.");
    }
    std::set<std::tuple<unsigned,unsigned,unsigned>> times;
    for(const auto& time:s.local_times) {
        if(time.hour>23U||time.minute>59U||time.second>59U||
           !times.emplace(time.hour,time.minute,time.second).second)
            throw std::invalid_argument("Godziny muszą być poprawne i unikalne.");
    }
    using namespace std::chrono;
    const sys_days begin{year{s.start_date.year}/month{s.start_date.month}/day{s.start_date.day}};
    const sys_days end{year{s.end_date_exclusive.year}/month{s.end_date_exclusive.month}/day{s.end_date_exclusive.day}};
    if((end-begin).count()>36600)
        throw std::invalid_argument("Zakres generatora przekracza 100 lat.");
    MarketSessionCalendar result;
    result.calendar_id=s.calendar_id;
    result.version=s.version;
    result.instrument_id=s.instrument_id;
    result.provider_id=s.provider_id;
    result.source_frequency=s.source_frequency;
    result.timezone=s.timezone;
    for(auto date=begin;date<end;date+=days{1}) {
        const weekday weekday_value{date};
        if(!weekdays.contains(weekday_value.iso_encoding())) continue;
        if(excluded.contains(date.time_since_epoch().count())) continue;
        const year_month_day ymd{date};
        const CivilDate civil{static_cast<int>(ymd.year()),static_cast<unsigned>(ymd.month()),
                              static_cast<unsigned>(ymd.day())};
        for(const auto& time:s.local_times)
            result.expected_observation_times.push_back(UtcTimestamp{local_to_utc(s,civil,time)});
    }
    std::sort(result.expected_observation_times.begin(),result.expected_observation_times.end(),
        [](const auto& a,const auto& b){return a.unix_seconds<b.unix_seconds;});
    if(result.expected_observation_times.empty())
        throw std::invalid_argument("Reguły nie wygenerowały żadnego slotu sesyjnego.");
    for(std::size_t i=1;i<result.expected_observation_times.size();++i)
        if(result.expected_observation_times[i-1].unix_seconds>=result.expected_observation_times[i].unix_seconds)
            throw std::invalid_argument("Reguły wygenerowały kolidujące znaczniki UTC.");
    return result;
}
} // namespace monitor::market
