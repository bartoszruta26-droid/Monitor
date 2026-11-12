#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace monitor::market {
inline constexpr std::uint32_t schema_version = 1;
enum class AssetClass { Currency, PreciousMetal, Commodity };
enum class Unit { CurrencyUnit, Gram, Kilogram, TroyOunce, MetricTon, Barrel, MegawattHour };
enum class PriceKind { Reference, Average, Bid, Ask, Mid, Spot, Transaction };
enum class Frequency { Tick, Minute, Hour, Daily, Weekly, Monthly, Quarterly, SemiAnnual, Annual, Irregular };
enum class PeriodStatus { Complete, Partial, Provisional };
enum class QualityStatus { Unreviewed, Validated, Suspect };
enum class SeriesCompleteness { Complete, Partial, Sparse, Unknown };
struct CivilDate { int year{}; unsigned month{}; unsigned day{}; };
struct UtcTimestamp { std::int64_t unix_seconds{}; };
struct Instrument {
    std::string id;
    std::string symbol;
    std::string name;
    AssetClass asset_class{AssetClass::Commodity};
    std::optional<std::string> base_currency;
    std::string quote_currency;
    Unit quantity_unit{Unit::CurrencyUnit};
    bool allow_negative_prices{false};
};
struct Provider { std::string id; std::string name; std::string homepage_url; std::string license; };
struct MarketObservation {
    std::uint32_t schema{schema_version};
    std::string id;
    std::string instrument_id;
    std::string provider_id;
    UtcTimestamp observed_at;
    UtcTimestamp received_at;
    std::optional<CivilDate> provider_published_on;
    Frequency frequency{Frequency::Daily};
    PeriodStatus period_status{PeriodStatus::Complete};
    PriceKind price_kind{PriceKind::Reference};
    double value{};
    QualityStatus quality{QualityStatus::Unreviewed};
    std::vector<std::string> quality_flags;
    std::optional<std::string> provider_record_id;
    std::optional<std::string> source_uri;
};
struct SeriesMetadata {
    std::string instrument_id;
    std::string provider_id;
    Frequency frequency{Frequency::Daily};
    UtcTimestamp first_observed_at;
    UtcTimestamp last_observed_at;
    std::uint64_t expected_count{};
    std::uint64_t observed_count{};
    SeriesCompleteness completeness{SeriesCompleteness::Unknown};
};
[[nodiscard]] bool is_valid_date(const CivilDate& date) noexcept;
[[nodiscard]] CivilDate parse_date(const std::string& text);
[[nodiscard]] std::string format_date(const CivilDate& date);
[[nodiscard]] UtcTimestamp parse_utc_timestamp(const std::string& text);
[[nodiscard]] std::string format_utc_timestamp(UtcTimestamp timestamp);
void validate_instrument(const Instrument& instrument);
void validate_provider(const Provider& provider);
void validate_observation(const Instrument& instrument, const Provider& provider,
                          const MarketObservation& observation);
void validate_series_metadata(const SeriesMetadata& metadata);
[[nodiscard]] double convert_quantity(double value, Unit from, Unit to);
} // namespace monitor::market
