#include "monitor/xetra_calendar_source.hpp"
#include "monitor/xetra_calendar_importer.hpp"

#include <nlohmann/json.hpp>

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
    require(parsed.parser_version=="xetra-html-2026-v1","parser ruleset is stamped with source calendar year");
    require(parsed.days.at(1).product_sessions.front().opens_at.hour==9U &&
            parsed.days.at(1).product_sessions.front().closes_at.hour==17U &&
            parsed.days.at(1).product_sessions.front().closes_at.minute==30U,
            "regular session hours are read from source text rather than fixed constants");
    const auto date_is=[](const CivilDate& date,int year,unsigned month,unsigned day) {
        return date.year==year && date.month==month && date.day==day;
    };
    auto source_2027=source();
    source_2027.document_id="xetra-2027-synthetic-html";
    source_2027.version=1U;
    source_2027.valid_from=parse_date("2027-01-01");
    source_2027.valid_to_exclusive=parse_date("2028-01-01");
    const auto parsed_2027=parse_xetra_html_calendar(html,source_2027);
    require(parsed_2027.days.size()==365U &&
            parsed_2027.parser_version=="xetra-html-2027-v1",
            "same parser supports the next calendar year from the requested range");
    require(date_is(parsed_2027.days.at(84).date,2027,3U,26U) &&
            parsed_2027.days.at(84).status==VenueDayStatus::Closed,
            "dynamic year parser reads 2027 Good Friday from the selected year column");
    require(date_is(parsed_2027.days.at(125).date,2027,5U,6U) &&
            parsed_2027.days.at(125).product_sessions.size()==2U &&
            parsed_2027.days.at(125).product_sessions.front().closes_at.hour==20U,
            "dynamic year parser reads 2027 holiday trading dates");
    require(date_is(parsed_2027.days.at(136).date,2027,5U,17U) &&
            date_is(parsed_2027.days.at(146).date,2027,5U,27U),
            "all named 2027 holiday trading dates are parsed without 2026 constants");
    require(date_is(parsed_2027.days.at(363).date,2027,12U,30U) &&
            parsed_2027.days.at(363).requires_official_notice,
            "extraordinary-hours marker is derived from an explicit year-specific notice");
    auto source_2028=source_2027;
    source_2028.document_id="xetra-2028-leap-fixture";
    source_2028.valid_from=parse_date("2028-01-01");
    source_2028.valid_to_exclusive=parse_date("2029-01-01");
    const auto parsed_2028=parse_xetra_html_calendar(html,source_2028);
    require(parsed_2028.days.size()==366U &&
            date_is(parsed_2028.days.at(59).date,2028,2U,29U) &&
            parsed_2028.parser_version=="xetra-html-2028-v1",
            "dynamic year parser handles leap-year ranges and February 29");
    auto wrong_year_html=html;
    const auto wrong_year_date=wrong_year_html.find("Friday Mar 26, 2027");
    require(wrong_year_date!=std::string::npos,"2027 date fixture marker exists");
    wrong_year_html.replace(wrong_year_date,std::string("Friday Mar 26, 2027").size(),"Friday Mar 26, 2026");
    throws([&]{(void)parse_xetra_html_calendar(wrong_year_html,source_2027);},
           "reject a table date whose year differs from the selected column");
    auto missing_year_html=html;
    const auto missing_year_date=missing_year_html.find("Friday Mar 26, 2027");
    missing_year_html.replace(missing_year_date,std::string("Friday Mar 26, 2027").size(),"Friday Mar 26");
    throws([&]{(void)parse_xetra_html_calendar(missing_year_html,source_2027);},
           "reject a table date without an explicit year");
    auto unsupported_year=source_2027;
    unsupported_year.valid_to_exclusive=parse_date("2027-12-31");
    throws([&]{(void)parse_xetra_html_calendar(html,unsupported_year);},
           "reject a source range that is not exactly one calendar year");
    auto missing_hours_html=html;
    const auto regular_hours_text=std::string("<p>Regular trading hours are from 09:00 to 17:30.</p>");
    missing_hours_html.erase(missing_hours_html.find(regular_hours_text),regular_hours_text.size());
    throws([&]{(void)parse_xetra_html_calendar(missing_hours_html,source_2027);},
           "reject a calendar fixture with missing regular session hours");
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
    invalid=source();
    invalid.source_object_id="sha256:"+sha256_hex(html);
    throws([&]{(void)adapter.fetch(invalid);},"reject caller-supplied source object reference");
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
    const auto archive_root=folder/"calendar.json.sources"/"objects";
    const auto archive_v1=archive_root/(sha256_hex(html)+".html");
    require(std::filesystem::exists(archive_v1),"archive exact original HTML bytes");
    require(adapter.import(repository,source())==1U,"repeated import verifies existing archive and remains idempotent");
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
    require(std::filesystem::exists(archive_v1) &&
            std::distance(std::filesystem::directory_iterator(archive_root),std::filesystem::directory_iterator{})==1,
            "identical bytes reuse one content-addressed archive object");
    transport.response.body=html+"\\n";
    require(adapter.import(repository,next)==2U,"changed exact bytes append a new version");
    const auto final_snapshot=repository.load();
    require(final_snapshot.venue_calendars.size()==2U &&
            final_snapshot.venue_calendars.back().source.version==2U &&
            final_snapshot.venue_calendars.back().source.content_sha256==sha256_hex(html+"\\n"),
            "new byte hash creates a new immutable version while preserving v1");
    require(final_snapshot.source_objects.size()==2U,
            "changed bytes create a second immutable source object");
    const auto manifest=final_snapshot.source_objects.back();
    require(manifest.archive_relative_path==std::string("calendar.json.sources/objects/")+sha256_hex(html+"\\n")+".html" &&
            manifest.content_length_bytes==static_cast<std::uint64_t>((html+"\\n").size()) &&
            manifest.content_type=="text/html",
            "source object stores relative archive path, exact byte length and original content type");
    const auto latest_archive=archive_root/(sha256_hex(html+"\\n")+".html");
    const auto original_json=[&] {
        std::ifstream input(folder/"calendar.json",std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(input)),{});
    }();
    const auto restore_json=[&] {
        std::ofstream output(folder/"calendar.json",std::ios::binary|std::ios::trunc);
        output<<original_json;
    };
    {
        auto legacy_json=nlohmann::json::parse(original_json);
        auto& legacy_calendar=legacy_json.at("venue_calendars").at(1);
        legacy_calendar.erase("calendar_id");
        legacy_calendar.erase("revision");
        auto& legacy_source=legacy_calendar.at("source");
        legacy_source.erase("archive_relative_path");
        legacy_source.erase("content_length_bytes");
        legacy_source.erase("content_type");
        legacy_source.erase("source_object_id");
        std::ofstream output(folder/"calendar.json",std::ios::binary|std::ios::trunc);
        output<<legacy_json.dump(2);
    }
    const auto legacy_loaded=repository.load();
    require(legacy_loaded.venue_calendars.size()==2U &&
            !legacy_loaded.venue_calendars.back().source.archive_relative_path &&
            !legacy_loaded.venue_calendars.back().source.content_length_bytes &&
            !legacy_loaded.venue_calendars.back().source.content_type &&
            legacy_loaded.venue_calendars.back().calendar_id=="xetra-2026-official-html" &&
            legacy_loaded.venue_calendars.back().revision==2U,
            "legacy schema-v1 calendar key migrates from source document ID/version");
    restore_json();
    {
        std::ofstream damaged(latest_archive,std::ios::binary|std::ios::trunc);
        damaged<<"truncated";
    }
    throws([&]{(void)repository.load();},"reject archive with mismatched length or SHA-256");
    {
        std::ofstream restored(latest_archive,std::ios::binary|std::ios::trunc);
        restored<<(html+"\\n");
    }
    std::filesystem::remove(latest_archive);
    throws([&]{(void)repository.load();},"reject missing archive referenced by manifest");
    {
        std::ofstream restored(latest_archive,std::ios::binary|std::ios::trunc);
        restored<<(html+"\\n");
    }
    std::filesystem::remove(latest_archive);
    {
        std::ofstream link_target(folder/"outside.html",std::ios::binary);
        link_target<<(html+"\\n");
    }
    std::filesystem::create_symlink(folder/"outside.html",latest_archive);
    throws([&]{(void)repository.load();},"reject symlink archive traversal");
    std::filesystem::remove(latest_archive);
    {
        std::ofstream restored(latest_archive,std::ios::binary|std::ios::trunc);
        restored<<(html+"\\n");
    }
    {
        auto tampered=original_json;
        const auto marker=manifest.archive_relative_path;
        const auto at=tampered.find(marker);
        require(at!=std::string::npos,"manifest path exists in persisted JSON");
        tampered.replace(at,marker.size(),"../escape.html");
        std::ofstream output(folder/"calendar.json",std::ios::binary|std::ios::trunc);
        output<<tampered;
    }
    throws([&]{(void)repository.load();},"reject manifest path traversal");
    restore_json();
    require(repository.load().venue_calendars.size()==2U,"valid archive manifest loads after tamper recovery");
    const auto archive_directory=folder/"calendar.json.sources"/"objects";
    const auto real_archive_directory=folder/"calendar.json.sources"/"objects.real";
    std::filesystem::rename(archive_directory,real_archive_directory);
    std::filesystem::create_directory_symlink(real_archive_directory,archive_directory);
    throws([&]{(void)repository.load();},"reject symlinked parent directory in archive path");
    std::filesystem::remove(archive_directory);
    std::filesystem::rename(real_archive_directory,archive_directory);
    require(repository.load().venue_calendars.size()==2U &&
            repository.load().source_objects.size()==2U,
            "archive verifies after symlinked parent directory is restored");

    // New API: source document version is independent from semantic calendar revision.
    const auto revision_folder=folder/"independent-revisions";
    std::filesystem::create_directories(revision_folder);
    JsonMarketDataRepository revision_repository(revision_folder/"calendar.json");
    transport.response.body=html;
    auto source_v1=source();
    source_v1.version=1U;
    require(adapter.import(revision_repository,source_v1,"XETRA-2026",10U)==1U,
            "publish calendar revision 10 from source document version 1");
    const auto requests_before_offline_revision=transport.calls;
    require(adapter.publish_revision_from_archive(revision_repository,source_v1,"XETRA-2026",11U,
                "sha256:"+sha256_hex(html))==2U,
            "publish calendar revision 11 by re-parsing archived bytes");
    require(transport.calls==requests_before_offline_revision,
            "offline calendar revision does not issue another HTTP request");
    auto source_v2=source();
    source_v2.version=2U;
    require(adapter.import(revision_repository,source_v2,"XETRA-2026",12U)==3U,
            "publish calendar revision 12 from source document version 2");
    const auto revision_snapshot=revision_repository.load();
    require(revision_snapshot.venue_calendars.size()==3U &&
            revision_snapshot.venue_calendars[0].calendar_id=="XETRA-2026" &&
            revision_snapshot.venue_calendars[0].revision==10U &&
            revision_snapshot.venue_calendars[0].parser_version=="xetra-html-2026-v1" &&
            revision_snapshot.venue_calendars[1].revision==11U &&
            revision_snapshot.venue_calendars[1].parser_version=="xetra-html-2026-v1" &&
            revision_snapshot.venue_calendars[2].revision==12U &&
            revision_snapshot.venue_calendars[0].source.version==1U &&
            revision_snapshot.venue_calendars[1].source.version==1U &&
            revision_snapshot.venue_calendars[2].source.version==2U,
            "source document versions and semantic calendar revisions remain independent");
    require(revision_snapshot.source_objects.size()==1U &&
            revision_snapshot.venue_calendars[0].source.source_object_id==
                revision_snapshot.venue_calendars[2].source.source_object_id,
            "multiple semantic revisions share the same content-addressed source object");
    require(adapter.publish_revision_from_archive(revision_repository,source_v1,"XETRA-2026",11U,
                "sha256:"+sha256_hex(html))==3U,
            "re-publishing an existing calendar revision is idempotent");

    transport.response.body=html+"\\n";
    auto source_v3=source();
    source_v3.version=3U;
    throws([&]{(void)adapter.import(revision_repository,source_v3,"XETRA-2026",12U);},
           "changed source bytes cannot overwrite an existing semantic revision");
    require(revision_repository.load().revision==3U &&
            revision_repository.load().source_objects.size()==1U,
            "rejected source conflict does not publish a dangling object reference");
    require(adapter.import(revision_repository,source_v3,"XETRA-2026",13U)==4U,
            "changed source bytes publish only under a new semantic revision");
    const auto changed_revision_snapshot=revision_repository.load();
    require(changed_revision_snapshot.venue_calendars.size()==4U &&
            changed_revision_snapshot.source_objects.size()==2U &&
            changed_revision_snapshot.venue_calendars.back().source.version==3U &&
            changed_revision_snapshot.venue_calendars.back().revision==13U,
            "new source document version and new calendar revision are independently recorded");
    transport.response.body=html+"\\n";
    throws([&]{(void)adapter.import(revision_repository,source_v1,"XETRA-2026",14U);},
           "one source document version cannot identify different downloaded bytes");
    require(revision_repository.load().revision==4U &&
            revision_repository.load().source_objects.size()==2U,
            "conflicting source version is rejected without repository mutation");

    const auto read_fixture=[](const std::string& name) {
        std::ifstream input(std::filesystem::path(MONITOR_TEST_SOURCE_DIR)/"fixtures"/name,std::ios::binary);
        if(!input) throw std::runtime_error("Missing synthetic notice fixture: "+name);
        return std::string(std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>());
    };
    const auto override_html=read_fixture("xetra_notice_2026_dec30_override.synthetic.html");
    VenueSessionNotice notice_metadata;
    notice_metadata.notice_id="xetra-2026-dec30-circular";
    notice_metadata.revision=1U;
    notice_metadata.calendar_id="XETRA-2026";
    notice_metadata.venue="XETRA";
    notice_metadata.source_object_id="sha256:"+sha256_hex(override_html);
    notice_metadata.source_uri=source().source_uri;
    notice_metadata.attribution="Deutsche Börse Cash Market";
    const auto parsed_notice=parse_xetra_session_notice_html(override_html,notice_metadata);
    require(parsed_notice.kind==VenueSessionNoticeKind::SessionOverride &&
            format_date(parsed_notice.effective_date)=="2026-12-30" &&
            parsed_notice.day_status==VenueDayStatus::Trading &&
            parsed_notice.product_sessions.size()==1U &&
            parsed_notice.product_sessions.front().product_group=="xetra_regular" &&
            parsed_notice.product_sessions.front().closes_at.hour==13U,
            "notice parser reads explicit date, trading status and shortened product session");

    const auto info_html=read_fixture("xetra_notice_2026_dec30_info.synthetic.html");
    auto info_metadata=notice_metadata;
    info_metadata.notice_id="xetra-2026-dec30-info";
    info_metadata.source_object_id="sha256:"+sha256_hex(info_html);
    const auto parsed_info=parse_xetra_session_notice_html(info_html,info_metadata);
    require(parsed_info.kind==VenueSessionNoticeKind::Informational &&
            !parsed_info.day_status && parsed_info.product_sessions.empty(),
            "notice without explicit hours remains informational and does not invent sessions");

    const auto cancel_html=read_fixture("xetra_notice_2026_dec30_cancel.synthetic.html");
    auto cancel_metadata=notice_metadata;
    cancel_metadata.notice_id="xetra-2026-dec30-cancel";
    cancel_metadata.source_object_id="sha256:"+sha256_hex(cancel_html);
    const auto parsed_cancel=parse_xetra_session_notice_html(cancel_html,cancel_metadata);
    require(parsed_cancel.kind==VenueSessionNoticeKind::Cancellation &&
            parsed_cancel.cancels_notice_id=="xetra-2026-dec30-circular" &&
            parsed_cancel.cancels_revision==2U,
            "notice parser extracts an explicit cancellation target and revision");

    auto wrong_venue_html=override_html;
    wrong_venue_html.replace(wrong_venue_html.find("Venue: XETRA"),13U,"Venue: EUREX");
    throws([&]{(void)parse_xetra_session_notice_html(wrong_venue_html,notice_metadata);},
           "reject a notice whose venue differs from the selected calendar");
    auto missing_hours_html=override_html;
    const auto session_row=missing_hours_html.find("<tr><td>xetra_regular</td><td>09:00</td><td>13:00</td></tr>");
    require(session_row!=std::string::npos,"override fixture has explicit session row");
    missing_hours_html.erase(session_row,std::string("<tr><td>xetra_regular</td><td>09:00</td><td>13:00</td></tr>").size());
    throws([&]{(void)parse_xetra_session_notice_html(missing_hours_html,notice_metadata);},
           "reject a trading override without explicit product hours");
    auto unknown_product_html=override_html;
    const auto product_at=unknown_product_html.find("xetra_regular");
    unknown_product_html.replace(product_at,std::string("xetra_regular").size(),"unknown_group");
    throws([&]{(void)parse_xetra_session_notice_html(unknown_product_html,notice_metadata);},
           "reject unknown product groups in official session notices");
    auto invalid_time_html=override_html;
    const auto close_at=invalid_time_html.find("13:00");
    invalid_time_html.replace(close_at,5U,"26:00");
    throws([&]{(void)parse_xetra_session_notice_html(invalid_time_html,notice_metadata);},
           "reject session times outside the local day");

    const auto notice_import_folder=folder/"notice-import";
    std::filesystem::create_directories(notice_import_folder);
    JsonMarketDataRepository notice_repository(notice_import_folder/"calendar.json");
    transport.response.status_code=200;
    transport.response.headers={{"content-type","text/html"}};
    transport.response.body=html;
    require(adapter.import(notice_repository,source(),"XETRA-2026",1U)==1U,
            "notice import test starts from a persisted base calendar");
    transport.response.body=override_html;
    VenueSessionNotice notice_to_import;
    notice_to_import.notice_id="xetra-2026-dec30-imported";
    notice_to_import.revision=1U;
    notice_to_import.calendar_id="XETRA-2026";
    notice_to_import.venue="XETRA";
    notice_to_import.attribution="Deutsche Börse Cash Market";
    const auto notice_url=source().source_uri;
    require(adapter.import_notice(notice_repository,notice_to_import,notice_url)==2U,
            "notice adapter archives exact HTML bytes and atomically publishes the parsed notice");
    const auto notice_snapshot=notice_repository.load();
    require(notice_snapshot.venue_session_notices.size()==1U &&
            notice_snapshot.source_objects.size()==2U &&
            notice_snapshot.venue_session_notices.front().source_object_id==
                "sha256:"+sha256_hex(override_html),
            "imported notice references its content-addressed archive object");
    const auto imported_notice_object=std::find_if(notice_snapshot.source_objects.begin(),
        notice_snapshot.source_objects.end(),[&](const SourceArchiveObject& object) {
            return object.source_object_id=="sha256:"+sha256_hex(override_html);
        });
    require(imported_notice_object!=notice_snapshot.source_objects.end() &&
            std::filesystem::exists(notice_import_folder/"calendar.json.sources"/"objects"/
                (sha256_hex(override_html)+".html")),
            "notice importer persists the exact source object before publication");
    const auto revision_before_notice_replay=notice_repository.load().revision;
    require(adapter.import_notice(notice_repository,notice_to_import,notice_url)==revision_before_notice_replay &&
            notice_repository.load().venue_session_notices.size()==1U,
            "re-importing the same notice revision is idempotent");
    transport.response.body=override_html+"\n";
    throws([&]{(void)adapter.import_notice(notice_repository,notice_to_import,notice_url);},
           "changed bytes cannot overwrite an already published notice revision");
    require(notice_repository.load().revision==revision_before_notice_replay &&
            notice_repository.load().source_objects.size()==2U,
            "rejected notice source conflict does not publish a dangling object");
    auto invalid_notice_metadata=notice_to_import;
    invalid_notice_metadata.notice_id="xetra-2026-dec30-invalid";
    transport.response.body=missing_hours_html;
    throws([&]{(void)adapter.import_notice(notice_repository,invalid_notice_metadata,notice_url);},
           "invalid notice HTML is rejected before archive publication");
    require(notice_repository.load().revision==revision_before_notice_replay,
            "invalid notice does not change repository revision");

    std::filesystem::remove_all(folder);
    std::cout<<"Xetra HTTPS source adapter tests passed\n";
}
