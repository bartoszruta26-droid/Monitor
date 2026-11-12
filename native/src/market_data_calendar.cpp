#include "monitor/market_data_calendar.hpp"

#include <unicode/calendar.h>
#include <unicode/gregocal.h>
#include <unicode/timezone.h>
#include <unicode/unistr.h>
#include <unicode/ucal.h>
#include <unicode/utypes.h>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <iomanip>
#include <sstream>
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


std::string sha256_hex(std::string_view bytes) {
    if(bytes.size()>std::numeric_limits<std::uint64_t>::max()/8U)
        throw std::length_error("Dane przekraczają zakres długości SHA-256.");
    static constexpr std::array<std::uint32_t,64> constants{
        0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
        0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
        0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
        0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
        0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
        0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
        0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
        0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U
    };
    std::vector<std::uint8_t> padded;
    padded.reserve(bytes.size()+72U);
    for(const unsigned char ch:bytes) padded.push_back(ch);
    const std::uint64_t bit_length=static_cast<std::uint64_t>(bytes.size())*8U;
    padded.push_back(0x80U);
    while(padded.size()%64U!=56U) padded.push_back(0U);
    for(int shift=56;shift>=0;shift-=8)
        padded.push_back(static_cast<std::uint8_t>((bit_length>>shift)&0xffU));
    std::array<std::uint32_t,8> hash{
        0x6a09e667U,0xbb67ae85U,0x3c6ef372U,0xa54ff53aU,
        0x510e527fU,0x9b05688cU,0x1f83d9abU,0x5be0cd19U
    };
    for(std::size_t offset=0;offset<padded.size();offset+=64U) {
        std::array<std::uint32_t,64> words{};
        for(std::size_t i=0;i<16U;++i) {
            const std::size_t p=offset+i*4U;
            words[i]=(static_cast<std::uint32_t>(padded[p])<<24U)|
                     (static_cast<std::uint32_t>(padded[p+1U])<<16U)|
                     (static_cast<std::uint32_t>(padded[p+2U])<<8U)|
                      static_cast<std::uint32_t>(padded[p+3U]);
        }
        for(std::size_t i=16U;i<64U;++i) {
            const auto s0=std::rotr(words[i-15U],7)^std::rotr(words[i-15U],18)^(words[i-15U]>>3U);
            const auto s1=std::rotr(words[i-2U],17)^std::rotr(words[i-2U],19)^(words[i-2U]>>10U);
            words[i]=words[i-16U]+s0+words[i-7U]+s1;
        }
        auto a=hash[0],b=hash[1],cc=hash[2],d=hash[3];
        auto e=hash[4],f=hash[5],g=hash[6],h=hash[7];
        for(std::size_t i=0;i<64U;++i) {
            const auto s1=std::rotr(e,6)^std::rotr(e,11)^std::rotr(e,25);
            const auto choose=(e&f)^((~e)&g);
            const auto temp1=h+s1+choose+constants[i]+words[i];
            const auto s0=std::rotr(a,2)^std::rotr(a,13)^std::rotr(a,22);
            const auto majority=(a&b)^(a&cc)^(b&cc);
            const auto temp2=s0+majority;
            h=g;g=f;f=e;e=d+temp1;d=cc;cc=b;b=a;a=temp1+temp2;
        }
        hash[0]+=a;hash[1]+=b;hash[2]+=cc;hash[3]+=d;
        hash[4]+=e;hash[5]+=f;hash[6]+=g;hash[7]+=h;
    }
    std::ostringstream out;
    out<<std::hex<<std::setfill('0');
    for(const auto word:hash) out<<std::setw(8)<<word;
    return out.str();
}

void validate_source_calendar_document(const SourceCalendarDocument& document) {
    if(document.document_id.empty()||document.venue.empty()||document.version==0U||
       document.timezone.empty()||document.source_uri.rfind("https://",0)!=0||
       document.attribution.empty()||!is_valid_date(document.valid_from)||
       !is_valid_date(document.valid_to_exclusive))
        throw std::invalid_argument("Dokument kalendarza wymaga identyfikatora, źródła HTTPS, zakresu i atrybucji.");
    using namespace std::chrono;
    const sys_days first{year{document.valid_from.year}/month{document.valid_from.month}/day{document.valid_from.day}};
    const sys_days last{year{document.valid_to_exclusive.year}/month{document.valid_to_exclusive.month}/day{document.valid_to_exclusive.day}};
    if(!(first<last)||(last-first).count()>36600)
        throw std::invalid_argument("Zakres dokumentu kalendarza jest pusty lub przekracza 100 lat.");
    if(document.content_sha256) {
        if(document.content_sha256->size()!=64U||
           !std::all_of(document.content_sha256->begin(),document.content_sha256->end(),[](unsigned char ch) {
               return (ch>='0'&&ch<='9')||(ch>='a'&&ch<='f');
           }))
            throw std::invalid_argument("SHA-256 dokumentu musi zawierać dokładnie 64 małe znaki szesnastkowe.");
    }
}

VenueSessionCalendar build_xetra_2026_calendar(const SourceCalendarDocument& document) {
    validate_source_calendar_document(document);
    if(document.venue!="XETRA"||document.timezone!="Europe/Berlin"||
       format_date(document.valid_from)!="2026-01-01"||
       format_date(document.valid_to_exclusive)!="2027-01-01")
        throw std::invalid_argument("Fixture Xetra 2026 wymaga venue XETRA, strefy Europe/Berlin i zakresu roku 2026.");
    VenueSessionCalendar result;
    result.calendar_id=document.document_id;
    result.revision=document.version;
    result.parser_version="xetra-html-2026-v1";
    result.source=document;
    using namespace std::chrono;
    const sys_days begin{year{2026}/January/1};
    const sys_days end{year{2027}/January/1};
    const auto civil=[](sys_days point) {
        const year_month_day ymd{point};
        return CivilDate{static_cast<int>(ymd.year()),static_cast<unsigned>(ymd.month()),static_cast<unsigned>(ymd.day())};
    };
    const std::set<std::int64_t> closed{
        sys_days{year{2026}/January/1}.time_since_epoch().count(),
        sys_days{year{2026}/April/3}.time_since_epoch().count(),
        sys_days{year{2026}/April/6}.time_since_epoch().count(),
        sys_days{year{2026}/May/1}.time_since_epoch().count(),
        sys_days{year{2026}/December/25}.time_since_epoch().count()
    };
    const std::set<std::int64_t> settlement_only{
        sys_days{year{2026}/December/24}.time_since_epoch().count(),
        sys_days{year{2026}/December/31}.time_since_epoch().count()
    };
    const std::set<std::int64_t> extended_holidays{
        sys_days{year{2026}/May/14}.time_since_epoch().count(),
        sys_days{year{2026}/May/25}.time_since_epoch().count(),
        sys_days{year{2026}/June/4}.time_since_epoch().count()
    };
    for(auto date=begin;date<end;date+=days{1}) {
        VenueCalendarDay day;
        day.date=civil(date);
        const auto key=date.time_since_epoch().count();
        const unsigned weekday_number=weekday{date}.iso_encoding();
        if(weekday_number>=6U||closed.contains(key)) {
            day.status=VenueDayStatus::Closed;
        } else if(settlement_only.contains(key)) {
            day.status=VenueDayStatus::SettlementOnly;
        } else {
            day.status=VenueDayStatus::Trading;
            if(extended_holidays.contains(key)) {
                day.product_sessions.push_back({"shares_and_etps",{9,0,0},{20,0,0}});
                day.product_sessions.push_back({"structured_products",{9,0,0},{22,0,0}});
            } else {
                day.product_sessions.push_back({"xetra_regular",{9,0,0},{17,30,0}});
            }
            day.requires_official_notice=key==sys_days{year{2026}/December/30}.time_since_epoch().count();
        }
        result.days.push_back(std::move(day));
    }
    return result;
}

} // namespace monitor::market
