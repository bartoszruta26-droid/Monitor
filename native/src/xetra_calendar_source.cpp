#include "monitor/xetra_calendar_source.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace monitor::market {
namespace {
std::atomic<std::uint64_t> archive_counter{0};

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
struct DownloadedCalendar {
    VenueSessionCalendar calendar;
    std::string raw_html;
    std::string content_type;
};
DownloadedCalendar download(HttpTransport& transport,SourceCalendarDocument source) {
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
    const auto response=transport.get(request);
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
    auto calendar=parse_xetra_html_calendar(response.body,source);
    return DownloadedCalendar{std::move(calendar),response.body,it->second};
}
void archive_exact_bytes(const std::filesystem::path& path,const std::string& bytes) {
    std::error_code ec;
    if (std::filesystem::exists(path,ec)) {
        if (ec) throw std::runtime_error("Nie można sprawdzić istniejącego archiwum HTML.");
        std::ifstream existing(path,std::ios::binary);
        const std::string stored((std::istreambuf_iterator<char>(existing)),{});
        if (existing.bad() || stored!=bytes)
            throw std::runtime_error("Konflikt: archiwum SHA-256 istnieje, ale bajty są różne.");
        return;
    }
    std::filesystem::create_directories(path.parent_path());
    auto temporary=path;
    temporary += ".tmp."+std::to_string(archive_counter.fetch_add(1));
    try {
        {
            std::ofstream output(temporary,std::ios::binary|std::ios::trunc);
            if (!output) throw std::runtime_error("Nie można utworzyć tymczasowego archiwum HTML.");
            output.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));
            output.flush();
            if (!output) throw std::runtime_error("Nie udało się zapisać kompletnych bajtów HTML.");
        }
        std::filesystem::rename(temporary,path,ec);
        if (ec) {
            if (std::filesystem::exists(path)) {
                std::filesystem::remove(temporary);
                std::ifstream existing(path,std::ios::binary);
                const std::string stored((std::istreambuf_iterator<char>(existing)),{});
                if (!existing.bad() && stored==bytes) return;
            }
            throw std::runtime_error("Nie udało się atomowo opublikować archiwum HTML: "+ec.message());
        }
    } catch (...) {
        std::filesystem::remove(temporary,ec);
        throw;
    }
}
}

VenueSessionCalendar XetraCalendarSourceAdapter::fetch(SourceCalendarDocument source) const {
    return download(transport_,std::move(source)).calendar;
}
std::uint64_t XetraCalendarSourceAdapter::import(
    JsonMarketDataRepository& repository,SourceCalendarDocument source) const {
    auto downloaded=download(transport_,std::move(source));
    auto& metadata=downloaded.calendar.source;
    const auto hash=*metadata.content_sha256;
    const auto archive_root=repository.file_path().parent_path()/
        (repository.file_path().filename().string()+".sources")/"objects";
    const auto archive_path=archive_root/(hash+".html");
    // Archive by content identity, independently of calendar revision. An
    // interrupted import may leave an unreferenced object, never a dangling
    // calendar reference.
    archive_exact_bytes(archive_path,downloaded.raw_html);
    const auto relative_archive=std::filesystem::relative(archive_path,
        repository.file_path().parent_path().empty() ? std::filesystem::path(".")
                                                     : repository.file_path().parent_path());

    SourceArchiveObject source_object;
    source_object.source_object_id="sha256:"+hash;
    source_object.content_sha256=hash;
    source_object.archive_relative_path=relative_archive.generic_string();
    source_object.content_length_bytes=static_cast<std::uint64_t>(downloaded.raw_html.size());
    source_object.content_type=downloaded.content_type;
    metadata.source_object_id=source_object.source_object_id;
    // New records store byte metadata only in SourceArchiveObject. The
    // optional fields on SourceCalendarDocument remain for legacy records.
    metadata.archive_relative_path.reset();
    metadata.content_length_bytes.reset();
    metadata.content_type.reset();

    auto snapshot=repository.load();
    const auto duplicate=std::find_if(snapshot.venue_calendars.begin(),
        snapshot.venue_calendars.end(),[&](const VenueSessionCalendar& item) {
            return item.source.document_id==metadata.document_id &&
                   item.source.content_sha256==metadata.content_sha256;
        });
    if (duplicate!=snapshot.venue_calendars.end()) {
        const auto existing_object=std::find_if(snapshot.source_objects.begin(),
            snapshot.source_objects.end(),[&](const SourceArchiveObject& item) {
                return item.source_object_id==source_object.source_object_id;
            });
        if(existing_object==snapshot.source_objects.end())
            (void)repository.upsert_source_object(source_object);
        return snapshot.revision;
    }
    MarketDataBatch batch;
    batch.source_objects.push_back(std::move(source_object));
    batch.venue_calendars.push_back(std::move(downloaded.calendar));
    return repository.upsert_batch(batch);
}
} // namespace monitor::market
