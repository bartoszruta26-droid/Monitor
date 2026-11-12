#pragma once

#include "monitor/market_data.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace monitor::market {

inline constexpr std::uint32_t json_document_schema_version = 1;

struct MarketObservationRevision {
    MarketObservation previous;
    UtcTimestamp superseded_at;
    std::string superseded_by_id;
};

struct MarketDataSnapshot {
    std::uint32_t schema_version{json_document_schema_version};
    std::uint64_t revision{};
    UtcTimestamp updated_at{};
    std::vector<Instrument> instruments;
    std::vector<Provider> providers;
    std::vector<MarketObservation> observations;
    std::vector<SeriesMetadata> series_metadata;
    std::vector<MarketObservationRevision> observation_history;
};

struct JsonRepositoryConfig {
    std::size_t max_document_bytes{256U * 1024U * 1024U};
    bool backup_on_write{true};
    bool create_parent_directories{true};
};

class MarketDataRepository {
public:
    virtual ~MarketDataRepository() = default;
    [[nodiscard]] virtual MarketDataSnapshot load() const = 0;
    [[nodiscard]] virtual std::uint64_t replace(const MarketDataSnapshot& snapshot) = 0;
    [[nodiscard]] virtual std::uint64_t upsert_instrument(const Instrument& instrument) = 0;
    [[nodiscard]] virtual std::uint64_t upsert_provider(const Provider& provider) = 0;
    [[nodiscard]] virtual std::uint64_t upsert_observation(const MarketObservation& observation) = 0;
    [[nodiscard]] virtual std::uint64_t upsert_series_metadata(const SeriesMetadata& metadata) = 0;
};

class JsonMarketDataRepository final : public MarketDataRepository {
public:
    explicit JsonMarketDataRepository(std::filesystem::path file_path,
                                      JsonRepositoryConfig config = {});

    [[nodiscard]] MarketDataSnapshot load() const override;
    [[nodiscard]] std::uint64_t replace(const MarketDataSnapshot& snapshot) override;
    [[nodiscard]] std::uint64_t upsert_instrument(const Instrument& instrument) override;
    [[nodiscard]] std::uint64_t upsert_provider(const Provider& provider) override;
    [[nodiscard]] std::uint64_t upsert_observation(const MarketObservation& observation) override;
    [[nodiscard]] std::uint64_t upsert_series_metadata(const SeriesMetadata& metadata) override;

    // Explicit recovery only. A corrupt primary file is never silently replaced.
    [[nodiscard]] MarketDataSnapshot restore_backup();

    [[nodiscard]] const std::filesystem::path& file_path() const noexcept { return file_path_; }
    [[nodiscard]] std::filesystem::path backup_path() const;
    [[nodiscard]] bool exists() const;

private:
    [[nodiscard]] MarketDataSnapshot load_unlocked() const;
    void write_unlocked(const MarketDataSnapshot& snapshot, bool rotate_backup) const;

    std::filesystem::path file_path_;
    JsonRepositoryConfig config_;
};

} // namespace monitor::market
