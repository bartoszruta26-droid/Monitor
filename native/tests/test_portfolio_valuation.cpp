#include "monitor/portfolio_valuation.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(EXIT_FAILURE);}}
template<class F>void throws(F&& fn,const char* message){try{fn();}catch(const std::exception&){return;}std::cerr<<"FAIL: expected exception: "<<message<<'\n';std::exit(EXIT_FAILURE);}
bool near(double a,double b,double tol=1e-8){return std::abs(a-b)<=tol;}
monitor::market::MarketObservation obs(std::string id,std::string instrument,std::string provider,
    std::int64_t time,double value,monitor::market::PriceKind kind){
    using namespace monitor::market;
    MarketObservation o;o.id=std::move(id);o.instrument_id=std::move(instrument);o.provider_id=std::move(provider);
    o.observed_at={time};o.received_at={time+5};o.value=value;o.price_kind=kind;
    o.quality=QualityStatus::Validated;o.source_uri="https://example.test/quote";return o;
}
}
int main(){
    using namespace monitor::market;
    MarketDataSnapshot s;
    s.instruments={
        {"FX:EUR/PLN","EUR/PLN","Euro/złoty",AssetClass::Currency,"EUR","PLN",Unit::CurrencyUnit,false},
        {"FX:USD/EUR","USD/EUR","Dolar/euro",AssetClass::Currency,"USD","EUR",Unit::CurrencyUnit,false},
        {"METAL:XAU:USD","XAU","Złoto USD",AssetClass::PreciousMetal,std::nullopt,"USD",Unit::TroyOunce,false},
        {"METAL:XAU:PLN","XAU","Złoto PLN",AssetClass::PreciousMetal,std::nullopt,"PLN",Unit::Gram,false}
    };
    s.observations={
        obs("eur-bid","FX:EUR/PLN","nbp-c",1000,4.30,PriceKind::Bid),
        obs("eur-ask","FX:EUR/PLN","nbp-c",1000,4.34,PriceKind::Ask),
        obs("usd-eur-mid","FX:USD/EUR","frankfurter",1000,0.92,PriceKind::Mid),
        obs("gold-usd","METAL:XAU:USD","xaus",1010,2400,PriceKind::Spot),
        obs("gold-pln-old","METAL:XAU:PLN","nbp-gold",900,300,PriceKind::Reference)
    };
    const auto book=build_portfolio_price_book(s,"PLN",{1020},120);
    const auto find=[&](const std::string& code){
        for(const auto& p:book.prices)if(p.first==code)return p.second;
        return 0.0;
    };
    require(near(find("PLN"),1.0),"reporting currency has unit price");
    require(near(find("EUR"),4.30),"direct currency liquidation uses bid");
    require(near(find("USD"),0.92*4.30),"cross currency route multiplies mid and bid");
    require(near(find("XAU"),(2400.0/31.1034768)*0.92*4.30),
            "gold USD per troy ounce converts to PLN per fine gram");
    require(book.provenance.size()==3,"provenance is present for converted assets");
    const auto stale=build_portfolio_price_book(s,"PLN",{1200},100);
    require(stale.prices.size()==1,"stale market observations are excluded");
    auto suspect=s;s.observations.front().quality=QualityStatus::Suspect;
    const auto without_bid=build_portfolio_price_book(s,"PLN",{1020},120);
    require(without_bid.prices.size()>=2,"inverse or alternative routes remain when one quote is suspect");
    throws([&]{(void)build_portfolio_price_book(s,"PL",{1020},120);},"invalid reporting currency");
    throws([&]{(void)build_portfolio_price_book(s,"PLN",{1020},-1);},"negative max age");
    std::cout<<"All portfolio valuation tests passed.\n";
}
