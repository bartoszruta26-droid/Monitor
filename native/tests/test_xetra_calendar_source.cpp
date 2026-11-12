#include "monitor/xetra_calendar_source.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

using namespace monitor::market;
namespace {
void require(bool condition,const char* message) {
    if (!condition) {std::cerr<<"FAIL: "<<message<<'\n';std::exit(EXIT_FAILURE);}
}
template<class F> void throws(F&& f,const char* message) {
    try {f();} catch(const std::exception&) {return;}
    require(false,message);
}
struct FakeTransport final : HttpTransport {
    HttpResponse response;
    HttpRequest last_request;
    unsigned calls{};
    HttpResponse get(const HttpRequest& request) override {
        ++calls;
        last_request=request;
        return response;
    }
};
SourceCalendarDocument source() {
    SourceCalendarDocument s;
    s.document_id="xetra-2026-official-html";
    s.venue="XETRA";
    s.version=1U;
    s.valid_from=parse_date("2026-01-01");
    s.valid_to_exclusive=parse_date("2027-01-01");
    s.timezone="Europe/Berlin";
    s.source_uri="https://www.cashmarket.deutsche-boerse.com/cash-en/trading/trading-calendar-and-trading-hours";
    s.attribution="Deutsche Börse Cash Market";
    return s;
}
}
int main() {
    const auto path=std::filesystem::path(MONITOR_TEST_SOURCE_DIR)/"fixtures/xetra_2026_calendar.html";
    std::ifstream file(path,std::ios::binary);
    require(file.good(),"fixture exists");
    const std::string html((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    FakeTransport transport;
    transport.response={200,{{"content-type","text/html; charset=utf-8"}},html};
    XetraCalendarSourceAdapter adapter(transport);
    const auto parsed=adapter.fetch(source());
    require(parsed.days.size()==365U,"all days imported");
    require(parsed.source.content_sha256==sha256_hex(html),"SHA-256 computed from exact response bytes");
    require(parsed.source.retrieved_at.has_value(),"retrieval time recorded");
    require(transport.calls==1U && transport.last_request.timeout.count()==15000 &&
            transport.last_request.max_response_bytes==4U*1024U*1024U,
            "bounded HTTPS transport request");
    auto invalid=source();
    invalid.source_uri="http://www.cashmarket.deutsche-boerse.com/calendar";
    throws([&]{(void)adapter.fetch(invalid);},"reject plain HTTP");
    invalid=source();
    invalid.source_uri="https://attacker.example/calendar";
    throws([&]{(void)adapter.fetch(invalid);},"reject unknown host");
    invalid=source();
    invalid.content_sha256=sha256_hex(html);
    throws([&]{(void)adapter.fetch(invalid);},"reject caller-supplied hash");
    transport.response.status_code=302;
    throws([&]{(void)adapter.fetch(source());},"reject redirects");
    transport.response.status_code=503;
    throws([&]{(void)adapter.fetch(source());},"reject server failure");
    transport.response.status_code=200;
    transport.response.headers["content-type"]="application/pdf";
    throws([&]{(void)adapter.fetch(source());},"reject non-HTML content type");
    transport.response.headers.clear();
    throws([&]{(void)adapter.fetch(source());},"reject missing content type");
    transport.response.headers["content-type"]="text/html";
    transport.response.body=std::string(4U*1024U*1024U+1U,'a');
    throws([&]{(void)adapter.fetch(source());},"reject oversized body even for fake transport");
    transport.response.body="<html>changed layout</html>";
    throws([&]{(void)adapter.fetch(source());},"reject unrecognized semantic HTML");
    transport.response.body=html;
    const auto folder=std::filesystem::temp_directory_path()/"monitor-xetra-import-tests";
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    JsonMarketDataRepository repository(folder/"calendar.json");
    require(adapter.import(repository,source())==1U,"first import commits atomically");
    const auto saved=repository.load();
    require(saved.venue_calendars.size()==1U &&
            saved.venue_calendars.front().source.content_sha256==sha256_hex(html),
            "persist exact response SHA-256");
    const auto archive_root=folder/"calendar.json.sources"/"xetra-2026-official-html";
    const auto archive_v1=archive_root/("1-"+sha256_hex(html)+".html");
    require(std::filesystem::exists(archive_v1),"archive exact original HTML bytes");
    {
        std::ifstream archived(archive_v1,std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(archived)),{});
        require(bytes==html,"archived bytes are byte-for-byte identical");
    }
    auto next=source();
    next.version=2U;
    transport.response.body="<html>invalid</html>";
    throws([&]{(void)adapter.import(repository,next);},"invalid next version cannot commit");
    require(repository.load().revision==1U &&
            repository.load().venue_calendars.size()==1U,
            "failed import leaves repository unchanged");
    transport.response.body=html;
    require(adapter.import(repository,next)==1U,"identical content hash is not imported as a new version");
    require(repository.load().venue_calendars.size()==1U,
            "identical content is deduplicated across requested versions");
    require(std::filesystem::exists(archive_root/("2-"+sha256_hex(html)+".html")),
            "each source version request archives its exact response");
    transport.response.body=html+"\\n";
    require(adapter.import(repository,next)==2U,"changed exact bytes append a new version");
    const auto final_snapshot=repository.load();
    require(final_snapshot.venue_calendars.size()==2U &&
            final_snapshot.venue_calendars.back().source.version==2U &&
            final_snapshot.venue_calendars.back().source.content_sha256==sha256_hex(html+"\\n"),
            "new byte hash creates a new immutable version while preserving v1");
    require(std::filesystem::exists(archive_root/("2-"+sha256_hex(html+"\\n")+".html")),
            "new source version is archived under its content hash");
    std::filesystem::remove_all(folder);
    std::cout<<"Xetra HTTPS source adapter tests passed\n";
}
