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
    // ICU musi działać w trybie lenient, aby zastosować jawne opcje wall-time
    // dla przerwy DST. Dla polityki Reject odtwarzamy pola lokalne poniżej
    // i odrzucamy każdą normalizację czasu nieistniejącego.
    calendar.setLenient(true);
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

std::vector<CivilDate> polish_statutory_holidays(int year_value) {
    using namespace std::chrono;
    if(year_value<1||year_value>9999)
        throw std::invalid_argument("Rok kalendarza świąt musi mieścić się w zakresie 1–9999.");

    // Meeus/Jones/Butcher Gregorian computus.
    const int a=year_value%19;
    const int b=year_value/100;
    const int c=year_value%100;
    const int d=b/4;
    const int e=b%4;
    const int f=(b+8)/25;
    const int g=(b-f+1)/3;
    const int h=(19*a+b-d-g+15)%30;
    const int i=c/4;
    const int k=c%4;
    const int l=(32+2*e+2*i-h-k)%7;
    const int m=(a+11*h+22*l)/451;
    const int easter_month=(h+l-7*m+114)/31;
    const int easter_day=(h+l-7*m+114)%31+1;
    const sys_days easter{year{year_value}/month{static_cast<unsigned>(easter_month)}/
                          day{static_cast<unsigned>(easter_day)}};
    const auto civil=[](sys_days date) {
        const year_month_day ymd{date};
        return CivilDate{static_cast<int>(ymd.year()),static_cast<unsigned>(ymd.month()),
                         static_cast<unsigned>(ymd.day())};
    };

    std::vector<CivilDate> result{
        {year_value,1U,1U},{year_value,1U,6U},
        {year_value,5U,1U},{year_value,5U,3U},
        {year_value,8U,15U},{year_value,11U,1U},{year_value,11U,11U},
        {year_value,12U,24U},{year_value,12U,25U},{year_value,12U,26U},
        civil(easter),civil(easter+days{1}),civil(easter+days{49}),civil(easter+days{60})
    };
    std::sort(result.begin(),result.end(),[](const auto& lhs,const auto& rhs) {
        return year{lhs.year}/month{lhs.month}/day{lhs.day}<
               year{rhs.year}/month{rhs.month}/day{rhs.day};
    });
    return result;
}

std::vector<NbpPublicationWindow> generate_nbp_publication_windows(
    const CivilDate& start_date, const CivilDate& end_date_exclusive,
    const std::vector<CivilDate>& public_holidays) {
    using namespace std::chrono;
    if(!is_valid_date(start_date)||!is_valid_date(end_date_exclusive))
        throw std::invalid_argument("Niepoprawna data zakresu kalendarza NBP.");
    const sys_days begin{year{start_date.year}/month{start_date.month}/day{start_date.day}};
    const sys_days end{year{end_date_exclusive.year}/month{end_date_exclusive.month}/day{end_date_exclusive.day}};
    if(!(begin<end)||(end-begin).count()>36600)
        throw std::invalid_argument("Zakres kalendarza NBP jest pusty lub przekracza 100 lat.");
    std::set<std::int64_t> holidays;
    for(const auto& date:public_holidays) {
        if(!is_valid_date(date)) throw std::invalid_argument("Niepoprawna data święta NBP.");
        const sys_days point{year{date.year}/month{date.month}/day{date.day}};
        if(!holidays.insert(point.time_since_epoch().count()).second)
            throw std::invalid_argument("Powtórzona data święta NBP.");
    }
    const auto business_day=[&](sys_days date) {
        const unsigned wd=weekday{date}.iso_encoding();
        return wd>=1U&&wd<=5U&&!holidays.contains(date.time_since_epoch().count());
    };
    const auto civil=[](sys_days date) {
        const year_month_day ymd{date};
        return CivilDate{static_cast<int>(ymd.year()),static_cast<unsigned>(ymd.month()),
                         static_cast<unsigned>(ymd.day())};
    };
    std::vector<NbpPublicationWindow> result;
    for(auto date=begin;date<end;date+=days{1}) {
        if(business_day(date)) {
            result.push_back({NbpPublicationTable::A,civil(date),"Europe/Warsaw",{11,45,0},{12,15,0}});
            result.push_back({NbpPublicationTable::C,civil(date),"Europe/Warsaw",{7,45,0},{8,15,0}});
        }
        if(weekday{date}.iso_encoding()==3U) {
            auto publication=date;
            if(!business_day(publication)) {
                do { publication-=days{1}; } while(!business_day(publication));
            }
            if(publication>=begin&&publication<end)
                result.push_back({NbpPublicationTable::B,civil(publication),"Europe/Warsaw",{11,45,0},{12,15,0}});
        }
    }
    const auto order=[](NbpPublicationTable t) {
        switch(t) { case NbpPublicationTable::A:return 0; case NbpPublicationTable::B:return 1;
                    case NbpPublicationTable::C:return 2; }
        return 3;
    };
    std::sort(result.begin(),result.end(),[&](const auto& a,const auto& b) {
        const auto da=year{a.local_date.year}/month{a.local_date.month}/day{a.local_date.day};
        const auto db=year{b.local_date.year}/month{b.local_date.month}/day{b.local_date.day};
        if(da!=db) return da<db;
        return order(a.table)<order(b.table);
    });
    return result;
}

std::vector<NbpPublicationWindow> generate_nbp_publication_windows(
    const CivilDate& start_date, const CivilDate& end_date_exclusive) {
    if(!is_valid_date(start_date)||!is_valid_date(end_date_exclusive))
        throw std::invalid_argument("Niepoprawna data zakresu kalendarza NBP.");
    std::vector<CivilDate> holidays;
    for(int year_value=start_date.year;year_value<=end_date_exclusive.year;++year_value) {
        auto year_holidays=polish_statutory_holidays(year_value);
        holidays.insert(holidays.end(),year_holidays.begin(),year_holidays.end());
    }
    return generate_nbp_publication_windows(start_date,end_date_exclusive,holidays);
}

std::vector<ObservedPublication> build_observed_publications(
    const std::vector<MarketObservation>& observations) {
    std::vector<ObservedPublication> result;
    for (const auto& observation : observations) {
        if (!observation.provider_published_on.has_value()) {
            continue;
        }
        if (observation.provider_id.empty() ||
            !is_valid_date(*observation.provider_published_on)) {
            throw std::invalid_argument(
                "Obserwowana publikacja wymaga dostawcy i poprawnej daty publikacji.");
        }
        const auto same_occurrence = [&](const ObservedPublication& item) {
            return item.provider_id == observation.provider_id &&
                   item.publication_date.year == observation.provider_published_on->year &&
                   item.publication_date.month == observation.provider_published_on->month &&
                   item.publication_date.day == observation.provider_published_on->day &&
                   item.received_at.unix_seconds == observation.received_at.unix_seconds &&
                   item.source_uri == observation.source_uri;
        };
        const auto found = std::find_if(result.begin(), result.end(), same_occurrence);
        if (found == result.end()) {
            result.push_back({observation.provider_id,
                              *observation.provider_published_on,
                              observation.received_at,
                              observation.source_uri,
                              1U});
        } else {
            ++found->observation_count;
        }
    }
    std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.publication_date.year != rhs.publication_date.year)
            return lhs.publication_date.year < rhs.publication_date.year;
        if (lhs.publication_date.month != rhs.publication_date.month)
            return lhs.publication_date.month < rhs.publication_date.month;
        if (lhs.publication_date.day != rhs.publication_date.day)
            return lhs.publication_date.day < rhs.publication_date.day;
        if (lhs.provider_id != rhs.provider_id)
            return lhs.provider_id < rhs.provider_id;
        if (lhs.received_at.unix_seconds != rhs.received_at.unix_seconds)
            return lhs.received_at.unix_seconds < rhs.received_at.unix_seconds;
        return lhs.source_uri < rhs.source_uri;
    });
    return result;
}

} // namespace monitor::market
