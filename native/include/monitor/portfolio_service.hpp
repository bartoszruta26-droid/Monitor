#pragma once

#include "monitor/portfolio_repository.hpp"
#include "monitor/portfolio_valuation.hpp"

namespace monitor::market {

struct PortfolioValuationReport {
    PortfolioSnapshot portfolio;
    PortfolioPriceBook price_book;
    PortfolioSummary summary;
};

// Loads the persisted ledger, constructs a provenance-preserving price book,
// and calculates the portfolio as one deterministic domain operation.
[[nodiscard]] PortfolioValuationReport value_saved_portfolio(
    const JsonPortfolioRepository& portfolio_repository,
    const MarketDataSnapshot& market_data,
    UtcTimestamp as_of,
    std::int64_t max_age_seconds);

} // namespace monitor::market
