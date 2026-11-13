#include "monitor/portfolio_service.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

namespace {
void require(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(EXIT_FAILURE);}}
template<class F>void throws(F&& fn,const char* message){try{fn();}catch(const std::exception&){return;}std::cerr<<"FAIL: expected exception: "<<message<<'\n';std::exit(EXIT_FAILURE);}
bool near(double a,double b,double tolerance=1e-8){return std::abs(a-b)<=tolerance;}
monitor::market::MarketObservation obs(std::string id,std::string instrument,
    std::int64_t time,double value,monitor::market::PriceKind kind){
    using namespace monitor::market;
    MarketObservation o;o.id=std::move(id);o.instrument_id=std::move(instrument);o.provider_id="provider";
    o.observed_at={time};o.received_at={time+1};o.value=value;o.price_kind=kind;
    o.quality=QualityStatus::Validated;o.source_uri="https://example.test/source";return o;
}
}
int main(){
    using namespace monitor::market;
    const auto dir=std::filesystem::temp_directory_path()/("monitor-service-"+std::to_string(::getpid()));
    std::filesystem::remove_all(dir);
    JsonPortfolioRepository repo(dir/"portfolio.json","PLN");
    require(repo.append(PortfolioTransaction{"eur-buy","EUR",PortfolioAssetKind::Currency,
        TradeSide::Buy,100,1,4.2,0,10,"bank"},0)==1,"persist EUR acquisition");
    require(repo.append(PortfolioTransaction{"gold-buy","XAU",PortfolioAssetKind::Gold,
        TradeSide::Buy,2,0.585,300,0,20,"dealer"},1)==2,"persist gold acquisition");

    MarketDataSnapshot market;
    market.instruments={
        {"FX:EUR/PLN","EUR/PLN","Euro/złoty",AssetClass::Currency,"EUR","PLN",Unit::CurrencyUnit,false},
        {"METAL:XAU:PLN","XAU","Złoto PLN",AssetClass::PreciousMetal,std::nullopt,"PLN",Unit::Gram,false}
    };
    market.observations={
        obs("eur-bid","FX:EUR/PLN",100,4.3,PriceKind::Bid),
        obs("eur-ask","FX:EUR/PLN",100,4.35,PriceKind::Ask),
        obs("gold-ref","METAL:XAU:PLN",110,300,PriceKind::Reference)
    };
    const auto report=value_saved_portfolio(repo,market,{200},1000);
    require(report.portfolio.revision==2&&report.portfolio.transactions.size()==2,
            "service loads persisted portfolio revision");
    require(report.summary.positions.size()==2,"service calculates both open positions");
    require(near(report.summary.total_cost_basis_reporting,771),"combined acquisition cost");
    require(near(report.summary.total_market_value_reporting,781),"combined current valuation");
    require(near(report.summary.total_unrealized_pnl_reporting,10),"combined unrealized result");
    require(report.price_book.provenance.size()==2,"combined report retains source provenance");

    throws([&]{(void)value_saved_portfolio(repo,market,{200},50);},
           "stale market observations do not produce a portfolio valuation");
    auto suspect=market;
    suspect.observations.front().quality=QualityStatus::Suspect;
    throws([&]{(void)value_saved_portfolio(repo,suspect,{200},1000);},
           "suspect FX quote cannot silently value an open currency position");
    std::filesystem::remove_all(dir);
    std::cout<<"All integrated portfolio service tests passed.\n";
}
