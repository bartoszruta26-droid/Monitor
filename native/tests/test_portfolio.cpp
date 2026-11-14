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
    const FxExecutionAudit audit{
        "EUR","PLN",4.30,4.34,4.32,(4.34-4.30)/4.32*10000.0,
        1.0/4.34,100.0/434.0,0.0,0.0,"EUR","provider-x",
        "quote-20261113-001","https://provider.example/quotes/001",590};
    const auto fx_pair=make_fx_exchange_transactions(
        {"exchange-001","PLN","EUR",434,100,4.32,2,600,"kantor-1",audit});
    require(fx_pair.size()==2 && fx_pair[0].operation_id=="exchange-001" &&
            fx_pair[1].operation_id=="exchange-001","FX conversion creates linked debit and credit legs");
    require(fx_pair[0].side==TradeSide::Sell && fx_pair[1].side==TradeSide::Buy,
            "FX conversion records both sides");
    require(near(fx_pair[0].quantity*fx_pair[0].unit_price_reporting,434),
            "source leg carries the balanced gross reporting value");
    require(near(fx_pair[1].fee_reporting,2),"exchange fee is booked exactly once on acquired leg");
    require(fx_pair[1].fx_audit.has_value()&&!fx_pair[0].fx_audit.has_value(),
            "quote audit is stored exactly once on acquired FX leg");
    require(fx_pair[1].fx_audit->provider_id=="provider-x" &&
            fx_pair[1].fx_audit->observation_id=="quote-20261113-001" &&
            near(fx_pair[1].fx_audit->gross_execution_rate,1.0/4.34) &&
            near(fx_pair[1].fx_audit->effective_execution_rate,100.0/434.0),
            "audit preserves provider quote, gross and net effective rates");
    std::vector<PortfolioTransaction> paired_ledger{
        {"pln-opening","PLN",PortfolioAssetKind::Currency,TradeSide::Buy,1000,1,1,0,500,"cash"},
        fx_pair[0],fx_pair[1]
    };
    const auto paired=calculate_portfolio("PLN",paired_ledger,{{"PLN",1},{"EUR",4.32}});
    require(near(paired.positions[1].gross_quantity,566),"source currency balance decreases by exchanged amount");
    require(near(paired.positions[0].gross_quantity,100),"destination currency balance increases");
    require(near(paired.total_fees_reporting,2),"paired exchange fee is counted once");
    auto invalid_audit=paired_ledger;
    invalid_audit.back().fx_audit->effective_execution_rate=0.25;
    throws([&]{(void)calculate_portfolio("PLN",invalid_audit,{{"PLN",1},{"EUR",4.32}});},
           "inconsistent effective FX rate in audit rejected");
    auto unbalanced=paired_ledger;unbalanced.back().fee_reporting=3;
    throws([&]{(void)calculate_portfolio("PLN",unbalanced,{{"PLN",1},{"EUR",4.32}});},
           "unbalanced paired exchange rejected");
    auto incomplete=paired_ledger;incomplete.pop_back();
    throws([&]{(void)calculate_portfolio("PLN",incomplete,{{"PLN",1}});},
           "unpaired FX operation rejected");
    std::cout << "All FX/XAU portfolio tests passed.\n";
}
