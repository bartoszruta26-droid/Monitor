#include "monitor/portfolio_repository.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

namespace {
void require(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(EXIT_FAILURE);}}
template<class F>void throws(F&& fn,const char* message){try{fn();}catch(const std::exception&){return;}std::cerr<<"FAIL: expected exception: "<<message<<'\n';std::exit(EXIT_FAILURE);}
}
int main(){
    using namespace monitor::market;
    const auto dir=std::filesystem::temp_directory_path()/("monitor-portfolio-"+std::to_string(::getpid()));
    std::filesystem::remove_all(dir);
    const auto file=dir/"portfolio.json";
    JsonPortfolioRepository repo(file,"PLN");
    require(!repo.exists(),"repository starts without a file");
    auto snapshot=repo.load();
    require(snapshot.revision==0&&snapshot.reporting_currency=="PLN"&&snapshot.transactions.empty(),
            "missing file returns empty configured snapshot");
    PortfolioTransaction buy{"eur-1","EUR",PortfolioAssetKind::Currency,TradeSide::Buy,
        100,1,4.2,2,1000,"dealer"};
    require(repo.append(buy,0)==1,"first append advances revision");
    snapshot=repo.load();
    require(snapshot.transactions.size()==1&&snapshot.transactions.front().id=="eur-1",
            "transaction JSON round trip");
    require(repo.append(PortfolioTransaction{"xau-1","XAU",PortfolioAssetKind::Gold,
        TradeSide::Buy,5,0.585,300,5,1100,"gold-dealer"},1)==2,
        "gold transaction append");
    require(std::filesystem::exists(repo.backup_path()),"previous revision backup created");
    snapshot=repo.load();
    require(snapshot.transactions.size()==2&&snapshot.revision==2,"both assets persist");
    throws([&]{(void)repo.append(buy,2);},"duplicate id rejected");
    require(repo.load().revision==2,"invalid append leaves revision unchanged");
    throws([&]{(void)repo.append(PortfolioTransaction{"eur-sell","EUR",PortfolioAssetKind::Currency,
        TradeSide::Sell,101,1,4.3,0,1200,"dealer"},2);},"oversell rejected atomically");
    require(repo.load().revision==2,"oversell leaves repository unchanged");
    throws([&]{(void)repo.append(PortfolioTransaction{"eur-2","EUR",PortfolioAssetKind::Currency,
        TradeSide::Buy,1,1,4.2,0,900,"dealer"},2);},"out-of-order transaction rejected");
    require(repo.load().revision==2,"chronology error leaves repository unchanged");
    throws([&]{(void)repo.replace(snapshot,0);},"stale expected revision rejected");
    auto replacement=snapshot;
    replacement.transactions.front().fee_reporting=3;
    require(repo.replace(replacement,2)==3,"replace advances revision");
    require(repo.load().transactions.front().fee_reporting==3,"replacement round trip");
    throws([&]{JsonPortfolioRepository wrong(file,"EUR");(void)wrong.load();},
           "reporting currency mismatch rejected");
    const auto future=dir/"future.json";
    {std::ofstream out(future);out<<R"({"document_type":"monitor.portfolio","schema_version":99,"revision":0,"reporting_currency":"PLN","transactions":[]})";}
    JsonPortfolioRepository future_repo(future,"PLN");
    throws([&]{(void)future_repo.load();},"unknown schema rejected");
    const auto corrupt=dir/"corrupt.json";
    {std::ofstream out(corrupt);out<<"{broken";}
    JsonPortfolioRepository corrupt_repo(corrupt,"PLN");
    throws([&]{(void)corrupt_repo.load();},"corrupt JSON is never treated as empty");
    const auto paired_file=dir/"paired.json";
    JsonPortfolioRepository paired_repo(paired_file,"PLN");
    require(paired_repo.append(PortfolioTransaction{"pln-cash","PLN",PortfolioAssetKind::Currency,
        TradeSide::Buy,1000,1,1,0,10,"cash"},0)==1,"opening reporting-currency balance");
    const auto pair=make_fx_exchange_transactions(
        {"repo-exchange-1","PLN","EUR",434,100,4.32,2,20,"kantor"});
    require(paired_repo.append_batch(pair,1)==2,"paired FX legs commit in one revision");
    const auto paired_loaded=paired_repo.load();
    require(paired_loaded.transactions.size()==3 &&
            paired_loaded.transactions[1].operation_id=="repo-exchange-1" &&
            paired_loaded.transactions[2].operation_id=="repo-exchange-1",
            "paired operation id survives JSON round trip");
    throws([&]{(void)paired_repo.append_batch({pair.front()},2);},
           "incomplete FX pair rejected atomically");
    require(paired_repo.load().revision==2,"failed FX batch leaves revision unchanged");
    std::filesystem::remove_all(dir);
    std::cout<<"All portfolio repository tests passed.\n";
}
