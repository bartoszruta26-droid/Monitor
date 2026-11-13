#pragma once

#include "monitor/portfolio.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace monitor::market {

inline constexpr std::uint32_t portfolio_json_schema_version = 1;

struct PortfolioSnapshot {
    std::uint32_t schema_version{portfolio_json_schema_version};
    std::uint64_t revision{};
    std::string reporting_currency{"PLN"};
    std::vector<PortfolioTransaction> transactions;
};

struct PortfolioRepositoryConfig {
    std::size_t max_document_bytes{32U * 1024U * 1024U};
    bool backup_on_write{true};
    bool create_parent_directories{true};
};

class JsonPortfolioRepository {
public:
    explicit JsonPortfolioRepository(std::filesystem::path file_path,
        std::string reporting_currency = "PLN",
        PortfolioRepositoryConfig config = {});

    [[nodiscard]] PortfolioSnapshot load() const;
    [[nodiscard]] std::uint64_t replace(
        const PortfolioSnapshot& snapshot, std::uint64_t expected_revision);
    [[nodiscard]] std::uint64_t append(
        const PortfolioTransaction& transaction, std::uint64_t expected_revision);
    [[nodiscard]] std::uint64_t append_batch(
        const std::vector<PortfolioTransaction>& transactions, std::uint64_t expected_revision);
    // Explicit recovery only; never invoked automatically after a corrupt read.
    [[nodiscard]] PortfolioSnapshot restore_backup();
    [[nodiscard]] const std::filesystem::path& file_path() const noexcept { return file_path_; }
    [[nodiscard]] std::filesystem::path backup_path() const;
    [[nodiscard]] bool exists() const;

private:
    [[nodiscard]] PortfolioSnapshot load_unlocked() const;
    void write_unlocked(const PortfolioSnapshot& snapshot, bool rotate_backup = true) const;

    std::filesystem::path file_path_;
    std::string initial_reporting_currency_;
    PortfolioRepositoryConfig config_;
};

} // namespace monitor::market
