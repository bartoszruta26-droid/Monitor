#include "monitor/portfolio.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
#include <set>
#include <utility>

namespace monitor::market {
namespace {
struct Lot {
    double gross_remaining{};
    double fine_remaining{};
    double cost_remaining{};
};
bool currency_code(const std::string& value) {
    if (value.size() != 3U) return false;
    for (const char c : value) if (c < 'A' || c > 'Z') return false;
    return true;
}
void validate_transaction(const PortfolioTransaction& tx) {
    if (tx.id.empty() || tx.asset_code.empty() ||
        !std::isfinite(tx.quantity) || tx.quantity <= 0.0 ||
        !std::isfinite(tx.purity) || tx.purity <= 0.0 || tx.purity > 1.0 ||
        !std::isfinite(tx.unit_price_reporting) || tx.unit_price_reporting <= 0.0 ||
        !std::isfinite(tx.fee_reporting) || tx.fee_reporting < 0.0 ||
        tx.executed_at_unix < 0) {
        throw std::invalid_argument("Nieprawidłowa transakcja portfela.");
    }
    if (tx.asset_kind == PortfolioAssetKind::Currency) {
        if (!currency_code(tx.asset_code) || tx.asset_code == "XAU" || tx.purity != 1.0)
            throw std::invalid_argument("Waluta wymaga kodu ISO 4217 i próby 1.");
    } else if (tx.asset_kind == PortfolioAssetKind::Gold) {
        if (tx.asset_code != "XAU")
            throw std::invalid_argument("Aktywo złota musi mieć kod XAU.");
    } else {
        throw std::invalid_argument("Nieznany rodzaj aktywa portfela.");
    }
    if (tx.side != TradeSide::Buy && tx.side != TradeSide::Sell)
        throw std::invalid_argument("Nieznana strona transakcji.");
}
}

PortfolioSummary calculate_portfolio(
    const std::string& reporting_currency,
    const std::vector<PortfolioTransaction>& transactions,
    const std::vector<std::pair<std::string, double>>& current_prices) {
    if (!currency_code(reporting_currency) || reporting_currency == "XAU")
        throw std::invalid_argument("Nieprawidłowa waluta raportowania.");

    std::map<std::string, double> prices;
    for (const auto& [asset, price] : current_prices) {
        if ((asset != "XAU" && !currency_code(asset)) ||
            !std::isfinite(price) || price <= 0.0 ||
            !prices.emplace(asset, price).second) {
            throw std::invalid_argument("Nieprawidłowa lub powtórzona cena portfela.");
        }
    }

    std::map<std::string, std::vector<Lot>> lots;
    std::map<std::string, PortfolioAssetKind> kinds;
    std::map<std::string, double> realized;
    std::map<std::string, double> fees;
    std::map<std::string, std::int64_t> last_execution;
    std::set<std::string> transaction_ids;
    std::map<std::string, std::vector<const PortfolioTransaction*>> operations;
    double total_fees{};

    for (const auto& tx : transactions) {
        validate_transaction(tx);
        if(!tx.operation_id.empty()) operations[tx.operation_id].push_back(&tx);
        const auto kind = kinds.find(tx.asset_code);
        if (kind != kinds.end() && kind->second != tx.asset_kind)
            throw std::invalid_argument("Kod aktywa użyty z różnymi rodzajami.");
        kinds[tx.asset_code] = tx.asset_kind;
        if (last_execution.contains(tx.asset_code) &&
            tx.executed_at_unix < last_execution[tx.asset_code])
            throw std::invalid_argument("Transakcje aktywa muszą być chronologiczne.");
        if (!transaction_ids.insert(tx.id).second)
            throw std::invalid_argument("Powtórzony identyfikator transakcji.");
        last_execution[tx.asset_code] = tx.executed_at_unix;
        fees[tx.asset_code] += tx.fee_reporting;
        total_fees += tx.fee_reporting;
        if (!std::isfinite(fees[tx.asset_code]) || !std::isfinite(total_fees))
            throw std::overflow_error("Suma opłat przekracza zakres liczbowy.");

        const double fine_quantity = tx.quantity * tx.purity;
        const double gross_value = fine_quantity * tx.unit_price_reporting;
        if (!std::isfinite(fine_quantity) || !std::isfinite(gross_value))
            throw std::overflow_error("Wartość transakcji przekracza zakres liczbowy.");

        auto& asset_lots = lots[tx.asset_code];
        if (tx.side == TradeSide::Buy) {
            const double cost = gross_value + tx.fee_reporting;
            if (!std::isfinite(cost)) throw std::overflow_error("Koszt nabycia przekracza zakres.");
            asset_lots.push_back({tx.quantity, fine_quantity, cost});
            continue;
        }

        double remaining_fine = fine_quantity;
        double allocated_cost{};
        for (auto& lot : asset_lots) {
            if (remaining_fine <= 0.0) break;
            if (lot.fine_remaining <= 0.0) continue;
            const double take = std::min(remaining_fine, lot.fine_remaining);
            const double fraction = take / lot.fine_remaining;
            allocated_cost += lot.cost_remaining * fraction;
            lot.cost_remaining -= lot.cost_remaining * fraction;
            lot.gross_remaining -= lot.gross_remaining * fraction;
            lot.fine_remaining -= take;
            remaining_fine -= take;
        }
        if (remaining_fine > 0.0)
            throw std::invalid_argument("Sprzedaż przekracza stan posiadania aktywa.");
        const double proceeds = gross_value - tx.fee_reporting;
        if (!std::isfinite(proceeds) || proceeds < 0.0)
            throw std::invalid_argument("Opłaty sprzedaży przekraczają przychód.");
        realized[tx.asset_code] += proceeds - allocated_cost;
        if (!std::isfinite(realized[tx.asset_code]))
            throw std::overflow_error("Wynik zrealizowany przekracza zakres.");
    }

    for(const auto& [operation_id,legs]:operations) {
        (void)operation_id;
        if(legs.size()!=2U)
            throw std::invalid_argument("Operacja wymiany FX musi zawierać dokładnie dwie nogi.");
        const PortfolioTransaction* debit=nullptr;
        const PortfolioTransaction* credit=nullptr;
        for(const auto* leg:legs) {
            if(leg->asset_kind!=PortfolioAssetKind::Currency)
                throw std::invalid_argument("Sparowana operacja FX może obejmować wyłącznie waluty.");
            if(leg->side==TradeSide::Sell) debit=leg;
            else if(leg->side==TradeSide::Buy) credit=leg;
        }
        if(!debit||!credit||debit->asset_code==credit->asset_code||
           debit->executed_at_unix!=credit->executed_at_unix||
           debit->source_id!=credit->source_id||debit->fee_reporting!=0.0||
           debit->fx_audit.has_value())
            throw std::invalid_argument("Niespójne nogi operacji wymiany FX.");
        const double debit_value=debit->quantity*debit->unit_price_reporting;
        const double credit_value=credit->quantity*credit->unit_price_reporting+credit->fee_reporting;
        const double tolerance=1e-8*std::max({1.0,std::abs(debit_value),std::abs(credit_value)});
        if(!std::isfinite(debit_value)||!std::isfinite(credit_value)||
           std::abs(debit_value-credit_value)>tolerance)
            throw std::invalid_argument("Nogi wymiany FX nie bilansują się w walucie raportowania.");
        if (credit->fx_audit) {
            const auto& audit=*credit->fx_audit;
            const bool direct=audit.base_currency==debit->asset_code &&
                              audit.quote_currency==credit->asset_code;
            const bool inverse=audit.base_currency==credit->asset_code &&
                               audit.quote_currency==debit->asset_code;
            const double expected_mid=audit.bid+(audit.ask-audit.bid)/2.0;
            const double expected_spread=(audit.ask-audit.bid)/expected_mid*10000.0;
            const double expected_gross=direct?audit.bid:(inverse?1.0/audit.ask:0.0);
            const double expected_effective=credit->quantity/debit->quantity;
            const auto close=[](double a,double b) {
                return std::isfinite(a)&&std::isfinite(b)&&
                    std::abs(a-b)<=1e-8*std::max({1.0,std::abs(a),std::abs(b)});
            };
            if ((!direct&&!inverse) || !std::isfinite(audit.bid) ||
                !std::isfinite(audit.ask) || audit.bid<=0.0 || audit.ask<audit.bid ||
                !std::isfinite(audit.mid) || audit.mid<=0.0 ||
                !close(audit.mid,expected_mid) ||
                !close(audit.spread_basis_points,expected_spread) ||
                !close(audit.gross_execution_rate,expected_gross) ||
                !close(audit.effective_execution_rate,expected_effective) ||
                !std::isfinite(audit.fee_percent) || audit.fee_percent<0.0 ||
                audit.fee_percent>100.0 || !std::isfinite(audit.fixed_fee_output) ||
                audit.fixed_fee_output<0.0 || audit.fee_currency!=credit->asset_code ||
                audit.provider_id.empty() || audit.observation_id.empty() ||
                audit.source_uri.empty() || audit.observed_at_unix<0 ||
                audit.observed_at_unix>credit->executed_at_unix)
                throw std::invalid_argument("Nieprawidłowe dane audytowe wykonania FX.");
        }
    }

    PortfolioSummary result;
    result.reporting_currency = reporting_currency;
    result.total_fees_reporting = total_fees;
    for (auto& [asset, asset_lots] : lots) {
        double gross_quantity{};
        double fine_quantity{};
        double cost_basis{};
        for (const auto& lot : asset_lots) {
            gross_quantity += lot.gross_remaining;
            fine_quantity += lot.fine_remaining;
            cost_basis += lot.cost_remaining;
        }
        if (fine_quantity <= 0.0) continue;
        const auto price = prices.find(asset);
        if (price == prices.end())
            throw std::invalid_argument("Brak bieżącej ceny dla aktywa " + asset + ".");
        const double market_value = fine_quantity * price->second;
        if (!std::isfinite(gross_quantity) || !std::isfinite(fine_quantity) ||
            !std::isfinite(market_value) || !std::isfinite(cost_basis))
            throw std::overflow_error("Wycena portfela przekracza zakres liczbowy.");
        const double unrealized = market_value - cost_basis;
        result.positions.push_back({asset, kinds.at(asset), gross_quantity,
            fine_quantity, cost_basis, cost_basis / fine_quantity,
            market_value, unrealized, realized[asset], fees[asset]});
        result.total_cost_basis_reporting += cost_basis;
        result.total_market_value_reporting += market_value;
        result.total_unrealized_pnl_reporting += unrealized;
    }
    for (const auto& [asset, value] : realized) {
        (void)asset;
        result.total_realized_pnl_reporting += value;
    }
    if (!std::isfinite(result.total_cost_basis_reporting) ||
        !std::isfinite(result.total_market_value_reporting) ||
        !std::isfinite(result.total_unrealized_pnl_reporting) ||
        !std::isfinite(result.total_realized_pnl_reporting))
        throw std::overflow_error("Podsumowanie portfela przekracza zakres liczbowy.");
    return result;
}

std::vector<PortfolioTransaction> make_fx_exchange_transactions(
    const FxExchangeExecution& execution) {
    if(execution.operation_id.empty() ||
       !currency_code(execution.from_currency) || !currency_code(execution.to_currency) ||
       execution.from_currency==execution.to_currency ||
       !std::isfinite(execution.from_amount) || execution.from_amount<=0.0 ||
       !std::isfinite(execution.to_amount) || execution.to_amount<=0.0 ||
       !std::isfinite(execution.to_unit_price_reporting) || execution.to_unit_price_reporting<=0.0 ||
       !std::isfinite(execution.fee_reporting) || execution.fee_reporting<0.0 ||
       execution.executed_at_unix<0)
        throw std::invalid_argument("Nieprawidłowe dane wykonanej wymiany FX.");
    const double gross_reporting=execution.to_amount*execution.to_unit_price_reporting+
                                 execution.fee_reporting;
    const double from_unit_price=gross_reporting/execution.from_amount;
    if(!std::isfinite(gross_reporting)||gross_reporting<=0.0||
       !std::isfinite(from_unit_price)||from_unit_price<=0.0)
        throw std::overflow_error("Wartość wymiany FX przekracza zakres liczbowy.");
    PortfolioTransaction debit{execution.operation_id+":debit",execution.from_currency,
        PortfolioAssetKind::Currency,TradeSide::Sell,execution.from_amount,1.0,
        from_unit_price,0.0,execution.executed_at_unix,execution.source_id,execution.operation_id};
    PortfolioTransaction credit{execution.operation_id+":credit",execution.to_currency,
        PortfolioAssetKind::Currency,TradeSide::Buy,execution.to_amount,1.0,
        execution.to_unit_price_reporting,execution.fee_reporting,execution.executed_at_unix,
        execution.source_id,execution.operation_id,execution.audit};
    return {std::move(debit),std::move(credit)};
}

} // namespace monitor::market
