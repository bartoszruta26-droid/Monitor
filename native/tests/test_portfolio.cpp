#include "monitor/portfolio.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
void require(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}
template<class F> void throws(F&& fn, const char* message) {
    try { fn(); } catch (const std::exception&) { return; }
    std::cerr << "FAIL: expected exception: " << message << '\n';
    std::exit(EXIT_FAILURE);
}
bool near(double a, double b, double tolerance = 1e-8) {
    return std::abs(a - b) <= tolerance;
}
}

int main() {
    using namespace monitor::market;
    const std::vector<PortfolioTransaction> tx{
        {"eur-buy-1","EUR",PortfolioAssetKind::Currency,TradeSide::Buy,100,1,4.0,2,100,"bank-a"},
        {"eur-buy-2","EUR",PortfolioAssetKind::Currency,TradeSide::Buy,100,1,4.2,2,200,"bank-b"},
        {"eur-sell","EUR",PortfolioAssetKind::Currency,TradeSide::Sell,150,1,4.5,1,300,"bank-a"},
        {"gold-buy","XAU",PortfolioAssetKind::Gold,TradeSide::Buy,10,0.585,300,10,400,"dealer-1"},
        {"gold-sell","XAU",PortfolioAssetKind::Gold,TradeSide::Sell,2,0.585,330,2,500,"dealer-1"}
    };
    const auto p = calculate_portfolio("PLN",tx,{{"EUR",4.4},{"XAU",320}});
    require(p.positions.size()==2,"two open asset positions");
    const auto& eur=p.positions[0];
    require(eur.asset_code=="EUR","positions are deterministic by asset code");
    require(near(eur.gross_quantity,50),"remaining EUR quantity");
    // FIFO: first 100 EUR cost 402 PLN; 150 EUR sale allocates 402 + 101.
    require(near(eur.cost_basis_reporting,211),"remaining EUR cost basis");
    require(near(eur.realized_pnl_reporting,61),"realized EUR profit after fee");
    require(near(eur.market_value_reporting,220),"EUR current value");
    require(near(eur.unrealized_pnl_reporting,9),"EUR unrealized profit");
    const auto& gold=p.positions[1];
    require(gold.asset_code=="XAU","gold position code");
    require(near(gold.gross_quantity,8),"remaining gross gold grams");
    require(near(gold.fine_quantity,4.68),"remaining fine gold grams");
    require(near(gold.cost_basis_reporting,1412),"gold cost basis after FIFO disposal");
    require(near(gold.market_value_reporting,1497.6),"gold current value");
    require(near(gold.realized_pnl_reporting,31.1),"gold realized result");
    require(near(p.total_realized_pnl_reporting,92.1),"portfolio realized result");
    require(near(p.total_unrealized_pnl_reporting,94.6),"portfolio unrealized result");
    require(near(p.total_fees_reporting,17),"all fees recorded");

    throws([&]{(void)calculate_portfolio("PLN",tx,{{"EUR",4.4}});},
           "missing current XAU price");
    auto oversell=tx;oversell.push_back({"eur-sell-2","EUR",PortfolioAssetKind::Currency,
        TradeSide::Sell,51,1,4.5,0,400,"bank-a"});
    throws([&]{(void)calculate_portfolio("PLN",oversell,{{"EUR",4.4},{"XAU",320}});},
           "overselling a position");
    auto wrong_purity=tx;wrong_purity[0].purity=0.9;
    throws([&]{(void)calculate_portfolio("PLN",wrong_purity,{{"EUR",4.4},{"XAU",320}});},
           "currency purity must be one");
    auto out_of_order=tx;out_of_order[1].executed_at_unix=50;
    throws([&]{(void)calculate_portfolio("PLN",out_of_order,{{"EUR",4.4},{"XAU",320}});},
           "chronological transactions");
    auto invalid_gold=tx;invalid_gold[3].asset_code="SILVER";
    throws([&]{(void)calculate_portfolio("PLN",invalid_gold,{{"EUR",4.4},{"SILVER",320}});},
           "gold asset must be XAU");
    throws([&]{(void)calculate_portfolio("PL",tx,{{"EUR",4.4},{"XAU",320}});},
           "invalid reporting currency");
    throws([&]{(void)calculate_portfolio("PLN",tx,{{"EUR",4.4},{"EUR",4.5},{"XAU",320}});},
           "duplicate price");
    std::cout << "All FX/XAU portfolio tests passed.\n";
}
