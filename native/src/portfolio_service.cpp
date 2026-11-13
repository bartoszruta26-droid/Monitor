#include "monitor/portfolio_service.hpp"

#include <utility>

namespace monitor::market {

PortfolioValuationReport value_saved_portfolio(
    const JsonPortfolioRepository& portfolio_repository,
    const MarketDataSnapshot& market_data,
    UtcTimestamp as_of,
    std::int64_t max_age_seconds) {
    auto portfolio=portfolio_repository.load();
    auto price_book=build_portfolio_price_book(
        market_data,portfolio.reporting_currency,as_of,max_age_seconds);
    auto summary=calculate_portfolio(
        portfolio.reporting_currency,portfolio.transactions,price_book.prices);
    return {std::move(portfolio),std::move(price_book),std::move(summary)};
}

} // namespace monitor::market
