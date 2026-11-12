#pragma once

#include "monitor/market_data_calendar.hpp"

#include <string_view>

namespace monitor::market {

// Parses a standalone, explicitly structured Xetra HTML notice. The caller
// supplies immutable provenance and an archive object reference; the parser
// fills the effective date, kind, status, cancellation target and sessions.
[[nodiscard]] VenueSessionNotice parse_xetra_session_notice_html(
    std::string_view html, VenueSessionNotice notice);

// Parses text extracted from an official PDF announcement. The effective date
// is supplied by the caller and must be explicitly present in the extracted text.
[[nodiscard]] VenueSessionNotice parse_xetra_session_notice_text(
    std::string_view extracted_text, VenueSessionNotice notice);

} // namespace monitor::market
