#pragma once
#include "monitor/market_data_adapters.hpp"
#include "monitor/market_data_calendar.hpp"
#include "monitor/market_data_repository.hpp"

namespace monitor::market {

// Fetches exact HTML bytes through the existing HTTPS transport and commits
// only a successfully parsed, complete calendar version.
class XetraCalendarSourceAdapter final {
public:
    explicit XetraCalendarSourceAdapter(HttpTransport& transport) : transport_(transport) {}
    [[nodiscard]] VenueSessionCalendar fetch(SourceCalendarDocument source) const;
    [[nodiscard]] std::uint64_t import(JsonMarketDataRepository& repository,
                                        SourceCalendarDocument source) const;
    // New API: source.version identifies the source document; calendar_id
    // and calendar_revision independently identify the semantic calendar revision.
    [[nodiscard]] std::uint64_t import(JsonMarketDataRepository& repository,
                                        SourceCalendarDocument source,
                                        std::string calendar_id,
                                        std::uint32_t calendar_revision) const;
    // Re-parse already archived bytes without issuing another HTTP request.
    [[nodiscard]] std::uint64_t publish_revision_from_archive(
        JsonMarketDataRepository& repository,SourceCalendarDocument source,
        std::string calendar_id,std::uint32_t calendar_revision,
        std::string source_object_id) const;
private:
    HttpTransport& transport_;
};

} // namespace monitor::market
