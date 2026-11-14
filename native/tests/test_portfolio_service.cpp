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
    JsonPortfolioRepository history_repo(dir/"history.json","PLN");
    require(history_repo.append(PortfolioTransaction{"history-eur-buy","EUR",PortfolioAssetKind::Currency,
        TradeSide::Buy,100,1,4.2,0,10,"bank"},0)==1,"seed history EUR acquisition");
    require(history_repo.append(PortfolioTransaction{"history-gold-buy","XAU",PortfolioAssetKind::Gold,
        TradeSide::Buy,2,0.585,300,0,20,"dealer"},1)==2,"seed history XAU acquisition");
    require(history_repo.append(PortfolioTransaction{"history-usd-buy","USD",PortfolioAssetKind::Currency,
        TradeSide::Buy,100,1,3.68,0,25,"bank"},2)==3,"seed history USD acquisition");
    const FxExecutionAudit execution_audit{
        "USD","EUR",0.92,0.94,0.93,(0.94-0.92)/0.93*10000.0,
        0.92,0.92,0.0,0.0,"EUR","bank-provider",
        "usd-eur-quote-1","https://bank.example/quotes/1",29};
    const auto fx_legs=make_fx_exchange_transactions(
        {"fx-history-1","USD","EUR",100,92,4.0,0.0,30,"bank-provider",execution_audit});
    require(history_repo.append_batch(fx_legs,3)==4,"persist audited FX exchange atomically");

    const auto history_snapshot=history_repo.load();
    const auto all_history=build_portfolio_history(history_snapshot);
    require(all_history.transaction_count==5&&all_history.entries.size()==5,
            "history returns all persisted FX and XAU ledger entries");
    require(all_history.audited_fx_execution_count==1,
            "history counts a quote-audited execution once per operation");
    require(all_history.entries[3].transaction.id=="fx-history-1:debit" &&
            all_history.entries[4].transaction.id=="fx-history-1:credit" &&
            all_history.entries[4].price_evidence==PortfolioPriceEvidence::ExecutedWithQuoteAudit,
            "history is chronological and distinguishes executed quote evidence");
    PortfolioHistoryFilter pair_filter;pair_filter.fx_pair="USD/EUR";
    const auto pair_history=build_portfolio_history(history_snapshot,pair_filter);
    require(pair_history.transaction_count==2&&pair_history.entries[0].fx_pair=="USD/EUR",
            "FX pair filter returns both linked exchange legs");
    pair_filter.fx_pair="EUR/USD";
    require(build_portfolio_history(history_snapshot,pair_filter).transaction_count==2,
            "FX pair filter accepts inverse pair orientation");
    PortfolioHistoryFilter source_filter;source_filter.source_id="bank-provider";
    source_filter.executed_from_unix=30;source_filter.executed_to_unix=30;
    const auto source_history=build_portfolio_history(history_snapshot,source_filter);
    require(source_history.transaction_count==2&&source_history.total_fees_reporting==0.0,
            "source and inclusive execution-time filters compose");
    PortfolioHistoryFilter asset_filter;asset_filter.asset_code="EUR";
    require(build_portfolio_history(history_snapshot,asset_filter).transaction_count==2,
            "asset filter returns EUR acquisitions and exchange credits");
    PortfolioHistoryPageRequest page_request;
    page_request.page_size=2;
    const auto page_one=get_portfolio_history_page(history_snapshot,page_request);
    require(page_one.api_version==1&&page_one.snapshot_revision==4&&
            page_one.total_matching==5&&page_one.entries.size()==2&&
            page_one.next_cursor.has_value(),
            "versioned history API returns first bounded page and continuation cursor");
    page_request.cursor=page_one.next_cursor;
    const auto page_two=get_portfolio_history_page(history_snapshot,page_request);
    require(page_two.entries.size()==2&&page_two.next_cursor.has_value()&&
            page_two.entries.front().transaction.id=="history-usd-buy",
            "history cursor continues without repeating earlier entries");
    page_request.cursor=page_two.next_cursor;
    const auto page_three=get_portfolio_history_page(history_snapshot,page_request);
    require(page_three.entries.size()==1&&!page_three.next_cursor&&
            page_three.entries.front().transaction.fx_audit.has_value(),
            "last page retains FX execution audit metadata");
    PortfolioHistoryPageRequest unsupported_version;unsupported_version.api_version=2;
    throws([&]{(void)get_portfolio_history_page(history_snapshot,unsupported_version);},
           "unsupported history API version rejected");
    PortfolioHistoryPageRequest zero_page;zero_page.page_size=0;
    throws([&]{(void)get_portfolio_history_page(history_snapshot,zero_page);},
           "zero history page size rejected");
    PortfolioHistoryPageRequest oversized_page;oversized_page.page_size=501;
    throws([&]{(void)get_portfolio_history_page(history_snapshot,oversized_page);},
           "history page size above maximum rejected");
    PortfolioHistoryPageRequest malformed_cursor;malformed_cursor.cursor="broken";
    throws([&]{(void)get_portfolio_history_page(history_snapshot,malformed_cursor);},
           "malformed history cursor rejected");
    auto newer_snapshot=history_snapshot;++newer_snapshot.revision;
    page_request.cursor=page_one.next_cursor;
    throws([&]{(void)get_portfolio_history_page(newer_snapshot,page_request);},
           "cursor from older portfolio revision rejected");
    PortfolioHistoryPageRequest filtered_cursor;
    filtered_cursor.filter.asset_code="XAU";
    filtered_cursor.cursor=page_one.next_cursor;
    throws([&]{(void)get_portfolio_history_page(history_snapshot,filtered_cursor);},
           "cursor outside the active filter rejected");

    PortfolioHistoryFilter invalid_time;invalid_time.executed_from_unix=31;invalid_time.executed_to_unix=30;
    throws([&]{(void)build_portfolio_history(history_snapshot,invalid_time);},
           "reversed history time range rejected");

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
