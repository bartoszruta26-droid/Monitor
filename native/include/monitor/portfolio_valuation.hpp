#pragma once

#include "monitor/market_data_repository.hpp"
#include "monitor/portfolio.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace monitor::market {

struct PortfolioPriceProvenance {
    std::string asset_code;
    double price_per_unit_reporting{};
    std::int64_t oldest_observation_unix{};
    std::int64_t newest_observation_unix{};
    std::int64_t oldest_received_unix{};
    std::vector<std::string> instrument_ids;
    std::vector<std::string> observation_ids;
    std::vector<std::string> provider_ids;
    std::vector<std::string> source_uris;
    std::vector<PriceKind> price_kinds;
    std::vector<std::string> quality_flags;
};

struct PortfolioPriceBook {
    std::string reporting_currency;
    UtcTimestamp as_of;
    std::vector<std::pair<std::string,double>> prices;
    std::vector<PortfolioPriceProvenance> provenance;
};

// Builds liquidation-oriented currency prices (base sells at bid, inverse
// uses reciprocal ask) and fine-gold-gram prices. Only validated, sufficiently
// fresh observations are eligible. Cross-currency paths are explicit and
// preserve provenance for every edge.
[[nodiscard]] PortfolioPriceBook build_portfolio_price_book(
    const MarketDataSnapshot& snapshot,
    const std::string& reporting_currency,
    UtcTimestamp as_of,
    std::int64_t max_age_seconds);

} // namespace monitor::market
