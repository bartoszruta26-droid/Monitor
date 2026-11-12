#pragma once

#include "monitor/market_data.hpp"

#include <string_view>
#include <optional>
#include <string>
#include <vector>

namespace monitor::market {

enum class AmbiguousLocalTimePolicy {
    Reject,
    EarlierInstant,
    LaterInstant
};

enum class NonexistentLocalTimePolicy {
    Reject,
    NextValidTime
};

struct LocalTimeOfDay {
    unsigned hour{};
    unsigned minute{};
    unsigned second{};
};

struct LocalSessionSchedule {
    std::string calendar_id;
    // Source-document version; legacy records also used this as calendar revision.
    std::uint32_t version{1};
    std::string instrument_id;
    std::string provider_id;
    Frequency source_frequency{Frequency::Daily};
    std::string timezone;
    CivilDate start_date;
    CivilDate end_date_exclusive;
    // ISO weekday: Monday=1 ... Sunday=7.
    std::vector<unsigned> weekdays_iso;
    std::vector<LocalTimeOfDay> local_times;
    std::vector<CivilDate> excluded_dates;
    AmbiguousLocalTimePolicy ambiguous_time_policy{AmbiguousLocalTimePolicy::Reject};
    NonexistentLocalTimePolicy nonexistent_time_policy{NonexistentLocalTimePolicy::Reject};
};

enum class VenueDayStatus { Closed, Trading, SettlementOnly };
enum class VenueCalendarLicenseStatus { Unknown, AttributionRequired, OpenData, Restricted };

struct VenueProductSession {
    std::string product_group;
    LocalTimeOfDay opens_at;
    LocalTimeOfDay closes_at;
};

enum class VenueSessionPhaseEventKind {
    PreTradingStart,
    RetailEarlyStart,
    OpeningAuctionCall,
    IntradayAuctionCall,
    ClosingAuctionCall,
    ClosingAuctionPriceDeterminationNotBefore,
    TradeAtCloseEnd,
    PostTradingEnd,
    OffBookServicesEnd
};

struct VenueSessionPhaseEvent {
    VenueSessionPhaseEventKind kind{VenueSessionPhaseEventKind::PreTradingStart};
    LocalTimeOfDay at;
    // Optional product or service scope, e.g. "DAX, MDAX, SDAX".
    std::string scope;
};

struct VenueCalendarDay {
    CivilDate date;
    VenueDayStatus status{VenueDayStatus::Closed};
    std::vector<VenueProductSession> product_sessions;
    std::vector<VenueSessionPhaseEvent> phase_events;
    bool requires_official_notice{};
};

struct SourceCalendarDocument {
    std::string document_id;
    std::string venue;
    std::uint32_t version{1};
    CivilDate valid_from;
    CivilDate valid_to_exclusive;
    std::string timezone;
    std::string source_uri;
    std::optional<UtcTimestamp> retrieved_at;
    // SHA-256 of the exact downloaded bytes. Empty until those bytes are
    // actually acquired; a URL or parsed text is not a substitute for it.
    std::optional<std::string> content_sha256;
    // Stable reference to a content-addressed immutable source object.
    std::optional<std::string> source_object_id;
    // Relative path under the JSON repository directory; absent in legacy records.
    std::optional<std::string> archive_relative_path;
    std::optional<std::uint64_t> content_length_bytes;
    std::optional<std::string> content_type;
    std::string attribution;
    VenueCalendarLicenseStatus license_status{VenueCalendarLicenseStatus::Unknown};
};

struct SourceArchiveObject {
    // Canonical form: "sha256:" followed by 64 lowercase hexadecimal digits.
    std::string source_object_id;
    std::string content_sha256;
    std::string archive_relative_path;
    std::uint64_t content_length_bytes{};
    std::string content_type;
};

struct VenueSessionCalendar {
    // Stable calendar identity and semantic revision. Zero/empty means a
    // legacy record whose key is source.document_id + source.version.
    std::string calendar_id;
    std::uint32_t revision{};
    // Identifier of the parser/ruleset that produced this interpretation.
    std::string parser_version;
    SourceCalendarDocument source;
    std::vector<VenueCalendarDay> days;
};

enum class VenueSessionNoticeKind { Informational, SessionOverride, Cancellation };

// A separately versioned official notice. It never mutates the base calendar.
struct VenueSessionNotice {
    std::string notice_id;
    std::uint32_t revision{1};
    std::string calendar_id;
    std::string venue;
    CivilDate effective_date;
    std::string source_object_id;
    std::string source_uri;
    std::string parser_version{"manual-v1"};
    std::optional<UtcTimestamp> retrieved_at;
    std::string attribution;
    VenueCalendarLicenseStatus license_status{VenueCalendarLicenseStatus::Unknown};
    VenueSessionNoticeKind kind{VenueSessionNoticeKind::Informational};
    std::optional<VenueDayStatus> day_status;
    std::vector<VenueProductSession> product_sessions;
    std::vector<VenueSessionPhaseEvent> phase_events;
    std::string cancels_notice_id;
    std::uint32_t cancels_revision{};
};

// Computes lowercase SHA-256 over the exact byte sequence supplied by the caller.
[[nodiscard]] std::string sha256_hex(std::string_view bytes);

void validate_source_calendar_document(const SourceCalendarDocument& document);

// Reproducible 2026 Xetra calendar fixture transcribed from the official
// Deutsche Börse calendar page. It is versioned and explicitly marked with
// unknown redistribution terms until the source file's bytes and licence
// terms have been verified.
[[nodiscard]] VenueSessionCalendar build_xetra_2026_calendar(
    const SourceCalendarDocument& document);

// Parses the semantic non-trading-days table and holiday-trading notices from
// the official Deutsche Börse HTML page. Input is the exact downloaded byte
// sequence; the caller records sha256_hex(html) before decoding it.
[[nodiscard]] VenueSessionCalendar parse_xetra_html_calendar(
    std::string_view html, const SourceCalendarDocument& document);

enum class NbpPublicationTable { A, B, C };

// Local publication interval, never an inferred observation timestamp.
struct NbpPublicationWindow {
    NbpPublicationTable table{NbpPublicationTable::A};
    CivilDate local_date;
    std::string timezone{"Europe/Warsaw"};
    LocalTimeOfDay starts_at;
    LocalTimeOfDay ends_at;
};


// One publication occurrence derived only from provider metadata on returned
// observations. publication_date is a civil date; received_at is the actual
// retrieval time. Neither is interpreted as an exact provider publication
// timestamp.
struct ObservedPublication {
    std::string provider_id;
    CivilDate publication_date;
    UtcTimestamp received_at;
    std::optional<std::string> source_uri;
    std::uint64_t observation_count{};
};

// Groups persisted/provider observations by provider, effective publication
// date, retrieval time and source URI. Records without provider_published_on
// are excluded rather than assigned a synthetic date or timestamp.
[[nodiscard]] std::vector<ObservedPublication> build_observed_publications(
    const std::vector<MarketObservation>& observations);

// Statutory Polish public holidays (excluding ordinary Sundays, which are
// already excluded by the weekday rule). Includes movable Easter holidays.
[[nodiscard]] std::vector<CivilDate> polish_statutory_holidays(int year);

// Uses the statutory Polish holiday dates calculated for every year touched
// by the half-open range. Extraordinary days off still require the explicit
// holiday-list overload below.
[[nodiscard]] std::vector<NbpPublicationWindow> generate_nbp_publication_windows(
    const CivilDate& start_date, const CivilDate& end_date_exclusive);

// The holiday list is explicit input from an authoritative, versioned source.
[[nodiscard]] std::vector<NbpPublicationWindow> generate_nbp_publication_windows(
    const CivilDate& start_date, const CivilDate& end_date_exclusive,
    const std::vector<CivilDate>& public_holidays);

// Uses ICU's IANA time-zone database. The end date is exclusive.
// A nonexistent local time is either rejected or moved to the first valid
// local time after the DST gap. Repeated local times require an explicit
// earlier/later policy; no implicit DST choice is made.
[[nodiscard]] MarketSessionCalendar generate_session_calendar(
    const LocalSessionSchedule& schedule);

} // namespace monitor::market
