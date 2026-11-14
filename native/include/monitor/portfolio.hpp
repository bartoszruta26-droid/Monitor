#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace monitor::market {

enum class PortfolioAssetKind { Currency, Gold };
enum class TradeSide { Buy, Sell };

struct FxExecutionAudit {
    std::string base_currency;
    std::string quote_currency;
    double bid{};
    double ask{};
    double mid{};
    double spread_basis_points{};
    double gross_execution_rate{};
    double effective_execution_rate{};
    double fee_percent{};
    double fixed_fee_output{};
    std::string fee_currency;
    std::string provider_id;
    std::string observation_id;
    std::string source_uri;
    std::int64_t observed_at_unix{};
};

struct PortfolioTransaction {
    std::string id;
    std::string asset_code; // ISO 4217 code for Currency; XAU for Gold
    PortfolioAssetKind asset_kind{PortfolioAssetKind::Currency};
    TradeSide side{TradeSide::Buy};
    double quantity{}; // currency units or gross gold grams
    double purity{1.0}; // applies to gold; currency must use 1.0
    double unit_price_reporting{}; // reporting currency per currency unit / fine gold gram
    double fee_reporting{}; // fee expressed in portfolio reporting currency
    std::int64_t executed_at_unix{};
    std::string source_id;
    std::string operation_id; // shared id for paired FX debit/credit legs
    std::optional<FxExecutionAudit> fx_audit; // present on acquired leg only

    PortfolioTransaction() = default;
    PortfolioTransaction(std::string transaction_id, std::string code,
        PortfolioAssetKind kind, TradeSide transaction_side, double amount,
        double asset_purity, double price_reporting, double fee,
        std::int64_t execution_time, std::string source,
        std::string operation = {},
        std::optional<FxExecutionAudit> audit = {})
        : id(std::move(transaction_id)), asset_code(std::move(code)),
          asset_kind(kind), side(transaction_side), quantity(amount),
          purity(asset_purity), unit_price_reporting(price_reporting),
          fee_reporting(fee), executed_at_unix(execution_time),
          source_id(std::move(source)), operation_id(std::move(operation)),
          fx_audit(std::move(audit)) {}
};

struct FxExchangeExecution {
    std::string operation_id;
    std::string from_currency;
    std::string to_currency;
    double from_amount{};
    double to_amount{}; // net amount received after withholding fees
    double to_unit_price_reporting{}; // reporting currency per received currency unit
    double fee_reporting{};
    std::int64_t executed_at_unix{};
    std::string source_id;
    std::optional<FxExecutionAudit> audit;
};

// Creates a balanced debit/credit pair. The fee is booked once on the
// acquired leg; both legs share operation_id and one execution timestamp.
[[nodiscard]] std::vector<PortfolioTransaction> make_fx_exchange_transactions(
    const FxExchangeExecution& execution);

struct PortfolioPosition {
    std::string asset_code;
    PortfolioAssetKind asset_kind{PortfolioAssetKind::Currency};
    double gross_quantity{};
    double fine_quantity{}; // currency units or fine gold grams
    double cost_basis_reporting{};
    double average_cost_per_fine_unit{};
    double market_value_reporting{};
    double unrealized_pnl_reporting{};
    double realized_pnl_reporting{};
    double fees_reporting{};
};

struct PortfolioSummary {
    std::string reporting_currency;
    std::vector<PortfolioPosition> positions;
    double total_cost_basis_reporting{};
    double total_market_value_reporting{};
    double total_unrealized_pnl_reporting{};
    double total_realized_pnl_reporting{};
    double total_fees_reporting{};
};

// Prices are expressed in reporting currency per currency unit or per fine
// gram of gold. A current price is required for every non-empty position.
[[nodiscard]] PortfolioSummary calculate_portfolio(
    const std::string& reporting_currency,
    const std::vector<PortfolioTransaction>& transactions,
    const std::vector<std::pair<std::string, double>>& current_prices);

} // namespace monitor::market
