#pragma once

#include "monitor/portfolio_repository.hpp"
#include "monitor/portfolio_valuation.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace monitor::market {

struct PortfolioValuationReport {
    PortfolioSnapshot portfolio;
    PortfolioPriceBook price_book;
    PortfolioSummary summary;
};

enum class PortfolioPriceEvidence { RecordedTransaction, ExecutedWithQuoteAudit };

struct PortfolioHistoryFilter {
    std::optional<std::string> asset_code;
    std::optional<std::string> fx_pair; // canonical BASE/QUOTE pair, either orientation accepted
    std::optional<std::string> source_id;
    std::optional<std::string> operation_id;
    std::optional<std::int64_t> executed_from_unix; // inclusive
    std::optional<std::int64_t> executed_to_unix;   // inclusive
};

struct PortfolioHistoryEntry {
    PortfolioTransaction transaction;
    std::string fx_pair;
    PortfolioPriceEvidence price_evidence{PortfolioPriceEvidence::RecordedTransaction};
};

struct PortfolioAssetActivity {
    std::string asset_code;
    std::size_t transaction_count{};
    double bought_quantity{};
    double sold_quantity{};
    double fees_reporting{};
};

struct PortfolioHistoryReport {
    std::string reporting_currency;
    std::vector<PortfolioHistoryEntry> entries;
    std::vector<PortfolioAssetActivity> activity;
    std::size_t transaction_count{};
    std::size_t audited_fx_execution_count{};
    double total_fees_reporting{};
};

// Filters the persisted ledger without changing it. Execution quote evidence is
// kept distinct from indicative/reference market observations.
[[nodiscard]] PortfolioHistoryReport build_portfolio_history(
    const PortfolioSnapshot& portfolio,
    const PortfolioHistoryFilter& filter = {});

inline constexpr std::uint32_t portfolio_history_api_version = 1;

struct PortfolioHistoryPageRequest {
    std::uint32_t api_version{portfolio_history_api_version};
    PortfolioHistoryFilter filter;
    std::size_t page_size{100};
    std::optional<std::string> cursor;
};

struct PortfolioHistoryPage {
    std::uint32_t api_version{portfolio_history_api_version};
    std::uint64_t snapshot_revision{};
    std::size_t total_matching{};
    std::vector<PortfolioHistoryEntry> entries;
    std::optional<std::string> next_cursor;
};

// Versioned, transport-neutral read contract. Cursors are bound to the
// snapshot revision and remain stable across pages of the same snapshot.
[[nodiscard]] PortfolioHistoryPage get_portfolio_history_page(
    const PortfolioSnapshot& portfolio,
    const PortfolioHistoryPageRequest& request = {});

// Loads the persisted ledger, constructs a provenance-preserving price book,
// and calculates the portfolio as one deterministic domain operation.
[[nodiscard]] PortfolioValuationReport value_saved_portfolio(
    const JsonPortfolioRepository& portfolio_repository,
    const MarketDataSnapshot& market_data,
    UtcTimestamp as_of,
    std::int64_t max_age_seconds);

} // namespace monitor::market
