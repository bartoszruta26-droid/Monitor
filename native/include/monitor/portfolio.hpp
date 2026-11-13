#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace monitor::market {

enum class PortfolioAssetKind { Currency, Gold };
enum class TradeSide { Buy, Sell };

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
};

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
