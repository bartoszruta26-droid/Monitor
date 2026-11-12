#include "monitor/market_data.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace {
void require(bool ok,const char* msg){if(!ok){std::cerr<<"FAIL: "<<msg<<'\n';std::exit(EXIT_FAILURE);}}
template<class F>void throws(F&& f,const char* msg){try{f();}catch(const std::exception&){return;}std::cerr<<"FAIL: expected exception: "<<msg<<'\n';std::exit(EXIT_FAILURE);}
bool near(double a,double b,double tol=1e-8){return std::abs(a-b)<=tol;}
}
int main(){
 using namespace monitor::market;
 const Instrument fx{"FX:EUR/PLN","EUR/PLN","Euro do złotego",AssetClass::Currency,"EUR","PLN",Unit::CurrencyUnit,false};
 const Instrument au{"METAL:XAU","XAU","Złoto",AssetClass::PreciousMetal,std::nullopt,"USD",Unit::TroyOunce,false};
 const Provider nbp{"nbp","Narodowy Bank Polski","https://api.nbp.pl/","public data"};
 validate_instrument(fx);validate_instrument(au);validate_provider(nbp);
 const auto t=parse_utc_timestamp("2026-11-12T14:30:00+01:00");
 require(format_utc_timestamp(t)=="2026-11-12T13:30:00Z","normalizacja offsetu UTC");
 require(format_utc_timestamp(parse_utc_timestamp("2024-02-29T23:59:59Z"))=="2024-02-29T23:59:59Z","data przestępna");
 require(format_date(parse_date("2026-11-12"))=="2026-11-12","data publikacji");
 throws([]{(void)parse_date("2025-02-29");},"data kalendarzowa");
 throws([]{(void)parse_utc_timestamp("2026-11-12T14:30:00");},"jawna strefa czasu");
 throws([]{(void)parse_utc_timestamp("2026-13-12T14:30:00Z");},"miesiąc poza zakresem");
 MarketObservation o;o.id="nbp:EURPLN:2026-11-12";o.instrument_id=fx.id;o.provider_id=nbp.id;o.observed_at=t;
 o.received_at=UtcTimestamp{t.unix_seconds+60};o.provider_published_on=parse_date("2026-11-12");
 o.frequency=Frequency::Daily;o.price_kind=PriceKind::Reference;o.value=4.25;o.quality=QualityStatus::Validated;
 o.source_uri="https://api.nbp.pl/api/exchangerates/rates/a/eur/2026-11-12/";o.provider_record_id="A/220/A/NBP/2026";
 validate_observation(fx,nbp,o);
 auto bad=o;bad.value=std::numeric_limits<double>::quiet_NaN();throws([&]{validate_observation(fx,nbp,bad);},"NaN");
 bad=o;bad.provider_id="frankfurter";throws([&]{validate_observation(fx,nbp,bad);},"dostawca");
 bad=o;bad.received_at=UtcTimestamp{t.unix_seconds-1};throws([&]{validate_observation(fx,nbp,bad);},"chronologia");
 bad=o;bad.value=-1;throws([&]{validate_observation(fx,nbp,bad);},"ujemna cena FX");
 bad=o;bad.schema=schema_version+1U;throws([&]{validate_observation(fx,nbp,bad);},"wersja schematu");
 auto bad_fx=fx;bad_fx.quote_currency="PL";throws([&]{validate_instrument(bad_fx);},"kod waluty");
 bad_fx=fx;bad_fx.quote_currency="EUR";throws([&]{validate_instrument(bad_fx);},"waluty pary");
 require(near(convert_quantity(1,Unit::TroyOunce,Unit::Gram),31.1034768),"uncja na gram");
 require(near(convert_quantity(1,Unit::Kilogram,Unit::Gram),1000),"kg na gram");
 require(near(convert_quantity(31.1034768,Unit::Gram,Unit::TroyOunce),1),"gram na uncję");
 throws([]{(void)convert_quantity(1,Unit::Barrel,Unit::Gram);},"niejawna konwersja");
 SeriesMetadata series{fx.id,nbp.id,Frequency::Daily,UtcTimestamp{100},UtcTimestamp{200},10,10,SeriesCompleteness::Complete};
 validate_series_metadata(series);series.completeness=SeriesCompleteness::Partial;series.observed_count=9;validate_series_metadata(series);
 series.observed_count=10;throws([&]{validate_series_metadata(series);},"sprzeczna kompletność");
 std::cout<<"Wszystkie testy modelu danych rynkowych zakończone powodzeniem.\n";
}
