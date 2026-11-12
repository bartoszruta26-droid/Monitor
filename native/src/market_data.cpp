#include "monitor/market_data.hpp"
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string_view>
namespace monitor::market {
namespace {
int parse_int(std::string_view s,const char* field) {
 int v{};const auto r=std::from_chars(s.data(),s.data()+s.size(),v);
 if(r.ec!=std::errc{}||r.ptr!=s.data()+s.size())throw std::invalid_argument(std::string("Nieprawidłowe pole: ")+field);
 return v;
}
unsigned parse_uint(std::string_view s,const char* field) {
 unsigned v{};const auto r=std::from_chars(s.data(),s.data()+s.size(),v);
 if(r.ec!=std::errc{}||r.ptr!=s.data()+s.size())throw std::invalid_argument(std::string("Nieprawidłowe pole: ")+field);
 return v;
}
bool valid_id(std::string_view s) noexcept {
 if(s.empty())return false;
 for(unsigned char c:s)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c==':'||c=='/'))return false;
 return true;
}
bool valid_currency(std::string_view s) noexcept {
 if(s.size()!=3)return false;
 for(char c:s)if(c<'A'||c>'Z')return false;
 return true;
}
bool https(std::string_view s) noexcept{return s.size()>8&&s.substr(0,8)=="https://";}
bool mass(Unit u) noexcept{return u==Unit::Gram||u==Unit::Kilogram||u==Unit::TroyOunce;}
long double grams(double v,Unit u) {
 if(u==Unit::Gram)return static_cast<long double>(v);
 if(u==Unit::Kilogram)return static_cast<long double>(v)*1000.0L;
 if(u==Unit::TroyOunce)return static_cast<long double>(v)*31.1034768L;
 throw std::invalid_argument("Jednostka nie jest jednostką masy.");
}
}
bool is_valid_date(const CivilDate& d) noexcept {
 using namespace std::chrono;
 if(d.year<1||d.year>9999||d.month<1U||d.month>12U||d.day<1U||d.day>31U)return false;
 return year_month_day{year{d.year},month{d.month},day{d.day}}.ok();
}
CivilDate parse_date(const std::string& s) {
 if(s.size()!=10||s[4]!='-'||s[7]!='-')throw std::invalid_argument("Data musi mieć format YYYY-MM-DD.");
 CivilDate d{parse_int(std::string_view{s}.substr(0,4),"rok"),parse_uint(std::string_view{s}.substr(5,2),"miesiąc"),parse_uint(std::string_view{s}.substr(8,2),"dzień")};
 if(!is_valid_date(d))throw std::invalid_argument("Niepoprawna data kalendarzowa.");
 return d;
}
std::string format_date(const CivilDate& d) {
 if(!is_valid_date(d))throw std::invalid_argument("Niepoprawna data.");
 std::array<char,11>b{};const int n=std::snprintf(b.data(),b.size(),"%04d-%02u-%02u",d.year,d.month,d.day);
 if(n!=10)throw std::runtime_error("Błąd formatowania daty.");
 return b.data();
}
UtcTimestamp parse_utc_timestamp(const std::string& s) {
 if(s.size()<20||s[10]!='T'||s[13]!=':'||s[16]!=':')throw std::invalid_argument("Wymagany znacznik ISO-8601 ze strefą.");
 const auto d=parse_date(s.substr(0,10));const unsigned h=parse_uint(std::string_view{s}.substr(11,2),"godzina");
 const unsigned m=parse_uint(std::string_view{s}.substr(14,2),"minuta");const unsigned sec=parse_uint(std::string_view{s}.substr(17,2),"sekunda");
 if(h>23U||m>59U||sec>59U)throw std::invalid_argument("Godzina poza zakresem.");
 std::size_t p=19;
 if(p<s.size()&&s[p]=='.'){++p;const auto start=p;while(p<s.size()&&s[p]>='0'&&s[p]<='9')++p;if(p==start)throw std::invalid_argument("Pusta część ułamkowa czasu.");}
 int offset=0;
 if(p<s.size()&&s[p]=='Z'&&p+1==s.size()){}
 else if(p<s.size()&&(s[p]=='+'||s[p]=='-')&&p+6==s.size()&&s[p+3]==':'){
  const unsigned oh=parse_uint(std::string_view{s}.substr(p+1,2),"przesunięcie godzin");const unsigned om=parse_uint(std::string_view{s}.substr(p+4,2),"przesunięcie minut");
  if(oh>14U||om>59U||(oh==14U&&om!=0U))throw std::invalid_argument("Przesunięcie UTC poza zakresem.");
  const int mag=static_cast<int>(oh*3600U+om*60U);offset=s[p]=='+'?mag:-mag;
 }else throw std::invalid_argument("Znacznik czasu wymaga końcówki Z lub ±HH:MM.");
 using namespace std::chrono;const sys_days day_point{year{d.year}/month{d.month}/std::chrono::day{d.day}};
 const auto count=duration_cast<seconds>(day_point.time_since_epoch()).count()+static_cast<std::int64_t>(h*3600U+m*60U+sec)-static_cast<std::int64_t>(offset);
 return UtcTimestamp{count};
}
std::string format_utc_timestamp(UtcTimestamp t) {
 using namespace std::chrono;const sys_seconds point{seconds{t.unix_seconds}};const sys_days day=floor<days>(point);
 const year_month_day d{day};const hh_mm_ss tod{point-day};std::array<char,21>b{};
 const int n=std::snprintf(b.data(),b.size(),"%04d-%02u-%02uT%02lld:%02lld:%02lldZ",
  static_cast<int>(d.year()),static_cast<unsigned>(d.month()),static_cast<unsigned>(d.day()),
  static_cast<long long>(tod.hours().count()),static_cast<long long>(tod.minutes().count()),static_cast<long long>(tod.seconds().count()));
 if(n!=20)throw std::runtime_error("Błąd formatowania UTC.");
 return b.data();
}
void validate_instrument(const Instrument& x) {
 if(!valid_id(x.id)||!valid_id(x.symbol)||x.name.empty())throw std::invalid_argument("Niepoprawny identyfikator, symbol lub nazwa instrumentu.");
 if(!valid_currency(x.quote_currency))throw std::invalid_argument("Waluta wyceny musi być trzyliterowym kodem wielkimi literami.");
 if(x.asset_class==AssetClass::Currency){
  if(!x.base_currency||!valid_currency(*x.base_currency)||*x.base_currency==x.quote_currency||x.quantity_unit!=Unit::CurrencyUnit)
   throw std::invalid_argument("Niepoprawna para walutowa lub jednostka.");
 }else{
  if(x.base_currency||x.quantity_unit==Unit::CurrencyUnit)throw std::invalid_argument("Metal/surowiec wymaga jednostki ilościowej i braku waluty bazowej.");
  if(x.asset_class==AssetClass::PreciousMetal&&!mass(x.quantity_unit))throw std::invalid_argument("Metal szlachetny wymaga jednostki masy.");
 }
}
void validate_provider(const Provider& x) {
 if(!valid_id(x.id)||x.name.empty()||!https(x.homepage_url))throw std::invalid_argument("Niepoprawne dane dostawcy; wymagany adres HTTPS.");
}
void validate_observation(const Instrument& i,const Provider& p,const MarketObservation& o) {
 validate_instrument(i);validate_provider(p);
 if(o.schema!=schema_version)throw std::invalid_argument("Nieobsługiwana wersja schematu.");
 if(!valid_id(o.id)||o.instrument_id!=i.id||o.provider_id!=p.id)throw std::invalid_argument("Niepoprawne identyfikatory notowania.");
 if(o.received_at.unix_seconds<o.observed_at.unix_seconds)throw std::invalid_argument("Czas pobrania poprzedza czas obserwacji.");
 if(!std::isfinite(o.value))throw std::invalid_argument("Cena musi być skończona.");
 if(!i.allow_negative_prices&&o.value<0.0)throw std::invalid_argument("Ujemna cena jest niedozwolona dla instrumentu.");
 if(o.provider_published_on&&!is_valid_date(*o.provider_published_on))throw std::invalid_argument("Niepoprawna data publikacji.");
 if(o.source_uri&&!https(*o.source_uri))throw std::invalid_argument("Adres źródła musi używać HTTPS.");
 for(const auto& f:o.quality_flags)if(f.empty())throw std::invalid_argument("Pusta flaga jakości.");
}
void validate_series_metadata(const SeriesMetadata& x) {
 if(!valid_id(x.instrument_id)||!valid_id(x.provider_id))throw std::invalid_argument("Brak identyfikatora szeregu.");
 if(x.last_observed_at.unix_seconds<x.first_observed_at.unix_seconds)throw std::invalid_argument("Koniec szeregu poprzedza początek.");
 if(x.expected_count!=0U&&x.observed_count>x.expected_count)throw std::invalid_argument("Liczba obserwacji przekracza oczekiwaną.");
 if(x.completeness==SeriesCompleteness::Complete&&(x.expected_count==0U||x.observed_count!=x.expected_count))
  throw std::invalid_argument("Kompletny szereg wymaga zgodnej, znanej liczności.");
 if((x.completeness==SeriesCompleteness::Partial||x.completeness==SeriesCompleteness::Sparse)&&
    (x.expected_count==0U||x.observed_count>=x.expected_count))
  throw std::invalid_argument("Niepełny szereg wymaga brakujących obserwacji.");
}
double convert_quantity(double value,Unit from,Unit to) {
 if(!std::isfinite(value))throw std::invalid_argument("Ilość musi być skończona.");
 if(from==to)return value;
 if(!mass(from)||!mass(to))throw std::invalid_argument("Brak jawnej reguły konwersji tych jednostek.");
 const long double g=grams(value,from);long double result{};
 if(to==Unit::Gram)result=g;else if(to==Unit::Kilogram)result=g/1000.0L;else result=g/31.1034768L;
 const double converted=static_cast<double>(result);if(!std::isfinite(converted))throw std::overflow_error("Przekroczono zakres konwersji.");
 return converted;
}
} // namespace monitor::market
