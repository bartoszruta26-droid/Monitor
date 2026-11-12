#include "monitor/xetra_calendar_source.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>

namespace monitor::market {
namespace {
bool allowed_xetra_url(const std::string& url) {
    constexpr std::string_view prefix="https://";
    if (url.rfind(prefix,0)!=0) return false;
    const auto end=url.find_first_of("/?#",prefix.size());
    const auto host=url.substr(prefix.size(),end==std::string::npos
        ? std::string::npos : end-prefix.size());
    return (host=="www.cashmarket.deutsche-boerse.com" ||
            host=="www.eurexgroup.com") &&
           url.find('@')==std::string::npos &&
           url.find('#')==std::string::npos;
}
std::string lower(std::string value) {
    std::transform(value.begin(),value.end(),value.begin(),[](unsigned char c){
        return static_cast<char>(std::tolower(c));
    });
    return value;
}
}

VenueSessionCalendar XetraCalendarSourceAdapter::fetch(SourceCalendarDocument source) const {
    validate_source_calendar_document(source);
    if (!allowed_xetra_url(source.source_uri))
        throw std::invalid_argument("Adres kalendarza Xetra musi używać zatwierdzonego hosta HTTPS.");
    if (source.retrieved_at || source.content_sha256)
        throw std::invalid_argument("Metadane pobrania i SHA-256 ustala importer, nie wywołujący.");
    HttpRequest request;
    request.url=source.source_uri;
    request.timeout=std::chrono::milliseconds{15000};
    request.max_response_bytes=4U*1024U*1024U;
    request.headers={{"Accept","text/html"},{"User-Agent","Monitor-Rynkow/0.2 C++20"}};
    const auto response=transport_.get(request);
    if (response.status_code!=200)
        throw std::runtime_error("Oficjalne źródło Xetra zwróciło HTTP "+std::to_string(response.status_code)+".");
    if (response.body.empty() || response.body.size()>request.max_response_bytes)
        throw std::runtime_error("Odpowiedź HTML Xetra jest pusta lub przekracza limit.");
    const auto it=response.headers.find("content-type");
    if (it==response.headers.end())
        throw std::runtime_error("Odpowiedź HTML Xetra nie ma nagłówka Content-Type.");
    const auto type=lower(it->second);
    if (type!="text/html" && type.rfind("text/html;",0)!=0)
        throw std::runtime_error("Źródło Xetra zwróciło nieoczekiwany typ treści.");
    source.content_sha256=sha256_hex(response.body);
    const auto now=std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    source.retrieved_at=UtcTimestamp{now};
    return parse_xetra_html_calendar(response.body,source);
}
std::uint64_t XetraCalendarSourceAdapter::import(
    JsonMarketDataRepository& repository,SourceCalendarDocument source) const {
    auto calendar=fetch(std::move(source));
    return repository.upsert_venue_calendar(calendar);
}
} // namespace monitor::market
