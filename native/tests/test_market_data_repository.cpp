#include "monitor/market_data_repository.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
using namespace monitor::market;
void require(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(EXIT_FAILURE);}}
template<class F>void throws(F&& f,const char* message){try{f();}catch(const std::exception&){return;}std::cerr<<"FAIL: expected exception: "<<message<<'\n';std::exit(EXIT_FAILURE);}
std::filesystem::path temp_dir() {
    auto path=std::filesystem::temp_directory_path()/("monitor-json-test-"+std::to_string(::getpid()));
    std::filesystem::remove_all(path);std::filesystem::create_directories(path);return path;
}
MarketObservation observation(double value,std::int64_t received) {
    MarketObservation x;x.id="nbp:eurpln:2026-11-12";x.instrument_id="FX:EUR/PLN";x.provider_id="nbp-a";
    x.observed_at=parse_utc_timestamp("2026-11-12T12:00:00Z");
    x.received_at=UtcTimestamp{x.observed_at.unix_seconds+received};
    x.provider_published_on=parse_date("2026-11-12");x.frequency=Frequency::Daily;x.price_kind=PriceKind::Average;
    x.value=value;x.quality=QualityStatus::Validated;x.provider_record_id="A/220/A/NBP/2026";
    x.source_uri="https://api.nbp.pl/api/exchangerates/rates/a/eur/2026-11-12/";return x;
}
void child_write(const std::string& file,int value) {
    JsonMarketDataRepository repo(file);
    const Instrument instrument{"FX:EUR/PLN","EUR/PLN","Euro do złotego",AssetClass::Currency,"EUR","PLN",Unit::CurrencyUnit,false};
    const Provider provider{"nbp-a","NBP tabela A","https://api.nbp.pl/","public data"};
    (void)repo.upsert_instrument(instrument);(void)repo.upsert_provider(provider);
    auto o=observation(4.0+static_cast<double>(value)/100.0,1000+value);
    o.observed_at.unix_seconds+=value;
    o.id="test-"+std::to_string(value);(void)repo.upsert_observation(o);
}
}
int main(){
    using namespace monitor::market;
    const auto dir=temp_dir();const auto file=dir/"market-data.json";
    JsonMarketDataRepository repo(file);
    require(!repo.exists(),"new repository starts without primary document");
    require(repo.load().revision==0,"missing document loads as empty snapshot");
    const Instrument fx{"FX:EUR/PLN","EUR/PLN","Euro do złotego",AssetClass::Currency,"EUR","PLN",Unit::CurrencyUnit,false};
    const Provider nbp{"nbp-a","NBP tabela A","https://api.nbp.pl/","public data"};
    require(repo.upsert_instrument(fx)==1,"first write revision");
    require(repo.upsert_provider(nbp)==2,"second write revision");
    require(repo.upsert_observation(observation(4.25,2000))==3,"third write revision");
    auto loaded=repo.load();
    require(loaded.instruments.size()==1&&loaded.providers.size()==1&&loaded.observations.size()==1,"round-trip document");
    require(loaded.observations.front().value==4.25,"round-trip preserves numeric value");
    require(loaded.observations.front().source_uri==observation(4.25,2000).source_uri,"round-trip preserves provenance");
    require(loaded.schema_version==json_document_schema_version&&loaded.revision==3,"document schema and revision");
    require(std::filesystem::exists(repo.backup_path()),"backup created after subsequent write");

    // Re-fetching the same value updates receipt time without inventing a correction.
    (void)repo.upsert_observation(observation(4.25,2100));
    loaded=repo.load();
    require(loaded.observations.size()==1&&loaded.observation_history.empty(),"identical data is deduplicated");
    require(loaded.observations.front().received_at.unix_seconds==observation(4.25,2100).received_at.unix_seconds,"receipt timestamp refreshed");

    // A changed value under the same stable key becomes a correction with preserved history.
    (void)repo.upsert_observation(observation(4.31,2200));
    loaded=repo.load();
    require(loaded.observations.size()==1&&loaded.observation_history.size()==1,"correction history retained");
    require(loaded.observation_history.front().previous.value==4.25,"previous value retained");
    require(loaded.observations.front().value==4.31,"latest correction becomes current");

    // A provider response can be ingested in one atomic commit.
    JsonMarketDataRepository batch_repo(dir/"batch.json");
    MarketDataBatch batch;
    batch.instruments.push_back(fx);
    batch.providers.push_back(nbp);
    auto batch_first=observation(4.20,3000);
    batch_first.id="batch-1";
    auto batch_second=observation(4.21,90000);
    batch_second.id="batch-2";
    batch_second.observed_at.unix_seconds+=86400;
    batch.observations={batch_first,batch_second};

    MarketSessionCalendar session_calendar;
    session_calendar.calendar_id="NBP-EURPLN";
    session_calendar.version=1U;
    session_calendar.instrument_id=fx.id;
    session_calendar.provider_id=nbp.id;
    session_calendar.source_frequency=Frequency::Daily;
    session_calendar.timezone="Europe/Warsaw";
    session_calendar.expected_observation_times={
        parse_utc_timestamp("2026-11-12T11:00:00Z"),
        parse_utc_timestamp("2026-11-13T11:00:00Z")
    };
    batch.session_calendars.push_back(session_calendar);

    batch.series_metadata.push_back(SeriesMetadata{fx.id,nbp.id,Frequency::Daily,
        batch_first.observed_at,batch_second.observed_at,2,2,SeriesCompleteness::Complete});
    require(batch_repo.upsert_batch(batch)==1,"provider batch is committed as one revision");
    auto batch_loaded=batch_repo.load();
    require(batch_loaded.instruments.size()==1&&batch_loaded.providers.size()==1&&
            batch_loaded.observations.size()==2&&batch_loaded.series_metadata.size()==1,
            "atomic batch persists all adapter result collections");
    require(batch_repo.upsert_batch(batch)==2,"replaying a batch is safe and revisioned");
    batch_loaded=batch_repo.load();
    require(batch_loaded.observations.size()==2&&batch_loaded.observation_history.empty()&&batch_loaded.session_calendars.size()==1,
            "powtórna partia nie duplikuje kalendarza");
    auto changed_calendar=session_calendar;
    changed_calendar.expected_observation_times.push_back(parse_utc_timestamp("2026-11-16T11:00:00Z"));
    throws([&]{(void)batch_repo.upsert_session_calendar(changed_calendar);},"opublikowana wersja kalendarza jest niezmienna");
    require(batch_repo.load().revision==2,"konflikt wersji nie zmienia rewizji");
    auto next_calendar=changed_calendar; next_calendar.version=2U;
    require(batch_repo.upsert_session_calendar(next_calendar)==3,"nowa wersja kalendarza jest zapisywana");
    batch_loaded=batch_repo.load();
    require(batch_loaded.session_calendars.size()==2,"starsza wersja pozostaje zachowana");
    auto invalid_calendar=next_calendar; invalid_calendar.calendar_id="bad-frequency"; invalid_calendar.version=1U;
    invalid_calendar.source_frequency=Frequency::Irregular;
    throws([&]{(void)batch_repo.upsert_session_calendar(invalid_calendar);},"odrzucana jest nieregularna częstotliwość");
    invalid_calendar=next_calendar; invalid_calendar.calendar_id="bad-reference"; invalid_calendar.version=1U;
    invalid_calendar.provider_id="missing-provider";
    throws([&]{(void)batch_repo.upsert_session_calendar(invalid_calendar);},"odrzucany jest nieznany dostawca");
    require(batch_repo.load().revision==3,"błędne kalendarze nie zmieniają rewizji");
    auto attempted_delete=batch_repo.load();
    attempted_delete.session_calendars.pop_back();
    throws([&]{(void)batch_repo.replace(attempted_delete);},
           "replace nie może usuwać opublikowanej wersji kalendarza");
    auto attempted_edit=batch_repo.load();
    attempted_edit.session_calendars.front().timezone="UTC";
    throws([&]{(void)batch_repo.replace(attempted_edit);},
           "replace nie może modyfikować opublikowanej wersji kalendarza");
    require(batch_repo.load().revision==3,"replace konflikt nie zmienia rewizji");
    MarketDataBatch invalid_batch;
    auto orphan=observation(4.5,4000);
    orphan.provider_id="missing-provider";
    invalid_batch.observations.push_back(orphan);
    throws([&]{(void)batch_repo.upsert_batch(invalid_batch);},"invalid batch rejected as a whole");
    require(batch_repo.load().revision==3,"failed batch leaves committed revision unchanged");

    const auto raw_before=std::filesystem::file_size(file);
    throws([&]{auto bad=loaded;bad.instruments.front().quote_currency="PL";(void)repo.replace(bad);},"invalid snapshot rejected");
    require(std::filesystem::file_size(file)==raw_before,"failed validation leaves primary file unchanged");

    // Explicit backup recovery restores the previous valid revision.
    {std::ofstream out(file,std::ios::binary|std::ios::trunc);out<<"{broken";}
    throws([&]{(void)repo.load();},"corrupt primary is not silently hidden");
    auto recovered=repo.restore_backup();
    require(recovered.revision==5,"backup recovery advances the document revision");
    require(recovered.observations.front().value==4.25,"backup content is previous valid state");

    const auto concurrent_file=(dir/"concurrent.json").string();
    std::vector<pid_t> children;
    for(int i=0;i<4;++i){const pid_t pid=::fork();if(pid==0){try{child_write(concurrent_file,i);::_exit(0);}catch(...){::_exit(2);}}if(pid<0)throw std::runtime_error("fork failed");children.push_back(pid);}
    for(const auto pid:children){int status{};if(::waitpid(pid,&status,0)<0||!WIFEXITED(status)||WEXITSTATUS(status)!=0)throw std::runtime_error("concurrent writer failed");}
    JsonMarketDataRepository concurrent(concurrent_file);
    const auto concurrent_snapshot=concurrent.load();
    require(concurrent_snapshot.instruments.size()==1&&concurrent_snapshot.providers.size()==1,"concurrent writers preserve shared dictionaries");
    require(concurrent_snapshot.observations.size()==4,"concurrent writes do not lose observations");
    require(concurrent_snapshot.revision==12,"serialized writers produce monotonic revisions");

    // Unknown document versions are rejected instead of being guessed or downgraded.
    const auto empty_file=dir/"empty.json";
    {std::ofstream out(empty_file,std::ios::binary);}
    JsonMarketDataRepository empty_repo(empty_file);
    throws([&]{(void)empty_repo.load();},"empty existing document is corruption, not a new repository");
    throws([&]{(void)empty_repo.replace(MarketDataSnapshot{});},"replace does not silently overwrite an empty corrupt file");

    const auto version_file=dir/"future.json";
    {std::ofstream out(version_file);out<<R"({"document_type":"monitor.market_data","schema_version":99})";}
    JsonMarketDataRepository future(version_file);
    throws([&]{(void)future.load();},"unknown future schema rejected");


    const auto legacy_file=dir/"legacy-v1.json";
    {std::ofstream out(legacy_file);
     out<<R"({"document_type":"monitor.market_data","schema_version":1,"revision":0,"updated_at":"2026-11-12T00:00:00Z","instruments":[],"providers":[],"observations":[],"series_metadata":[],"observation_history":[]})";}
    JsonMarketDataRepository legacy(legacy_file);
    require(legacy.load().session_calendars.empty(),"stary dokument schema v1 bez kalendarzy jest odczytywany");

    require(legacy.load().venue_calendars.empty(),
            "stary dokument schema v1 bez kalendarzy venue pozostaje zgodny");
    JsonMarketDataRepository venue_repo(dir/"venue-calendars.json");
    SourceCalendarDocument venue_source;
    venue_source.document_id="deutsche-boerse-xetra-2026";
    venue_source.venue="XETRA";
    venue_source.version=1U;
    venue_source.valid_from=parse_date("2026-01-01");
    venue_source.valid_to_exclusive=parse_date("2027-01-01");
    venue_source.timezone="Europe/Berlin";
    venue_source.source_uri="https://www.cashmarket.deutsche-boerse.com/cash-en/trading/trading-calendar-and-trading-hours";
    venue_source.attribution="Deutsche Börse Cash Market";
    auto venue_calendar=build_xetra_2026_calendar(venue_source);
    require(venue_repo.upsert_venue_calendar(venue_calendar)==1U,
            "first venue calendar version is persisted");
    auto venue_loaded=venue_repo.load();
    require(venue_loaded.venue_calendars.size()==1U &&
            venue_loaded.venue_calendars.front().days.size()==365U &&
            venue_loaded.venue_calendars.front().source.venue=="XETRA",
            "venue calendar source metadata and all daily sessions round-trip");
    require(venue_repo.upsert_venue_calendar(venue_calendar)==2U,
            "identical venue calendar version can be replayed idempotently");
    auto edited_venue=venue_calendar;
    edited_venue.source.attribution="Changed attribution";
    throws([&]{(void)venue_repo.upsert_venue_calendar(edited_venue);},
           "published venue calendar version cannot be modified");
    require(venue_repo.load().revision==2U,
            "rejected venue calendar edit leaves repository revision unchanged");
    auto venue_v2=venue_calendar;
    venue_v2.revision=2U;
    venue_v2.source.retrieved_at=parse_utc_timestamp("2026-11-12T22:00:00Z");
    require(venue_repo.upsert_venue_calendar(venue_v2)==3U,
            "new venue calendar version is appended without replacing v1");
    venue_loaded=venue_repo.load();
    require(venue_loaded.venue_calendars.size()==2U &&
            venue_loaded.venue_calendars[0].revision==1U &&
            venue_loaded.venue_calendars[1].revision==2U &&
            venue_loaded.venue_calendars[0].source.version==1U &&
            venue_loaded.venue_calendars[1].source.version==1U,
            "both immutable venue calendar versions remain available");
    auto venue_delete=venue_repo.load();
    venue_delete.venue_calendars.erase(venue_delete.venue_calendars.begin());
    throws([&]{(void)venue_repo.replace(venue_delete);},
           "replace cannot delete a published venue calendar version");
    auto venue_edit=venue_repo.load();
    venue_edit.venue_calendars.front().days.front().status=VenueDayStatus::Trading;
    throws([&]{(void)venue_repo.replace(venue_edit);},
           "replace cannot edit a published venue calendar day");
    require(venue_repo.load().revision==3U,
            "rejected venue calendar replace operations do not mutate repository");

    // Content-addressed source objects are immutable and can be shared by calendar revisions.
    const std::string source_bytes="<html>official source fixture</html>";
    const auto source_hash=sha256_hex(source_bytes);
    const auto source_object_id="sha256:"+source_hash;
    const auto source_relative_path=std::string("venue-calendars.json.sources/objects/")+source_hash+".html";
    const auto source_archive=venue_repo.file_path().parent_path()/source_relative_path;
    std::filesystem::create_directories(source_archive.parent_path());
    { std::ofstream output(source_archive,std::ios::binary); output<<source_bytes; }
    SourceArchiveObject source_object;
    source_object.source_object_id=source_object_id;
    source_object.content_sha256=source_hash;
    source_object.archive_relative_path=source_relative_path;
    source_object.content_length_bytes=source_bytes.size();
    source_object.content_type="text/html";
    MarketDataBatch source_batch;
    source_batch.source_objects.push_back(source_object);
    auto venue_v3=venue_calendar;
    venue_v3.revision=3U;
    venue_v3.source.content_sha256=source_hash;
    venue_v3.source.source_object_id=source_object_id;
    venue_v3.source.retrieved_at=parse_utc_timestamp("2026-11-12T22:30:00Z");
    source_batch.venue_calendars.push_back(venue_v3);
    require(venue_repo.upsert_batch(source_batch)==4U,
            "source object and its first calendar reference commit atomically");
    auto source_loaded=venue_repo.load();
    require(source_loaded.source_objects.size()==1U &&
            source_loaded.source_objects.front().source_object_id==source_object_id &&
            source_loaded.venue_calendars.back().source.source_object_id==source_object_id,
            "content-addressed source object and calendar reference round-trip");
    auto venue_v4=venue_v3;
    venue_v4.revision=4U;
    require(venue_repo.upsert_venue_calendar(venue_v4)==5U,
            "multiple calendar revisions can reuse one immutable source object");
    require(venue_repo.load().source_objects.size()==1U,
            "reusing source bytes does not duplicate source object records");
    require(find_orphaned_source_objects(venue_repo.load()).empty(),
            "all currently referenced source objects are absent from orphan report");
    auto orphan_bytes=std::string("<html>unreferenced fixture</html>");
    auto orphan_hash=sha256_hex(orphan_bytes);
    SourceArchiveObject orphan_object;
    orphan_object.source_object_id="sha256:"+orphan_hash;
    orphan_object.content_sha256=orphan_hash;
    orphan_object.archive_relative_path="venue-calendars.json.sources/objects/"+orphan_hash+".html";
    orphan_object.content_length_bytes=orphan_bytes.size();
    orphan_object.content_type="text/html";
    const auto orphan_archive=venue_repo.file_path().parent_path()/orphan_object.archive_relative_path;
    { std::ofstream output(orphan_archive,std::ios::binary); output<<orphan_bytes; }
    require(venue_repo.upsert_source_object(orphan_object)==6U,
            "unreferenced source object can be retained for diagnostic reporting");
    const auto orphan_report=find_orphaned_source_objects(venue_repo.load());
    require(orphan_report.size()==1U &&
            orphan_report.front().source_object_id==orphan_object.source_object_id,
            "orphan report identifies source objects without calendar references");
    auto venue_v5=venue_v4;
    venue_v5.revision=5U;
    venue_v5.source.document_id="xetra-parser-fixture";
    venue_v5.source.version=1U;
    venue_v5.source.content_sha256=orphan_hash;
    venue_v5.source.source_object_id=orphan_object.source_object_id;
    venue_v5.source.retrieved_at=parse_utc_timestamp("2026-11-12T22:40:00Z");
    require(venue_repo.upsert_venue_calendar(venue_v5)==7U,
            "new calendar revision can reference previously orphaned source object");
    require(find_orphaned_source_objects(venue_repo.load()).empty(),
            "source object disappears from orphan report after a calendar references it");

    VenueSessionNotice informational_notice;
    informational_notice.notice_id="xetra-2026-dec30-circular";
    informational_notice.revision=1U;
    informational_notice.calendar_id="deutsche-boerse-xetra-2026";
    informational_notice.venue="XETRA";
    informational_notice.effective_date=parse_date("2026-12-30");
    informational_notice.source_object_id=orphan_object.source_object_id;
    informational_notice.source_uri=venue_source.source_uri;
    informational_notice.attribution="Deutsche Börse Cash Market";
    informational_notice.kind=VenueSessionNoticeKind::Informational;
    require(venue_repo.upsert_venue_session_notice(informational_notice)==8U,
            "informational official notice is persisted separately from the base calendar");
    require(venue_repo.load().venue_session_notices.size()==1U,
            "notice JSON round-trip retains its source reference");
    const auto effective_informational=resolve_effective_venue_calendar_day(
        venue_repo.load(),"deutsche-boerse-xetra-2026",parse_date("2026-12-30"));
    require(effective_informational.day.product_sessions.front().closes_at.hour==17U &&
            effective_informational.requires_official_notice_unresolved &&
            std::find(effective_informational.applied_notice_ids.begin(),
                      effective_informational.applied_notice_ids.end(),
                      informational_notice.notice_id)!=effective_informational.applied_notice_ids.end(),
            "informational notice leaves the base schedule unchanged and unresolved");
    const auto unresolved_after_info=find_unresolved_venue_notice_dates(
        venue_repo.load(),"deutsche-boerse-xetra-2026");
    require(unresolved_after_info.size()==1U &&
            format_date(unresolved_after_info.front())=="2026-12-30",
            "unresolved notice report includes the date that still requires an official circular");
    require(venue_repo.upsert_venue_session_notice(informational_notice)==9U &&
            venue_repo.load().venue_session_notices.size()==1U,
            "replaying the same notice does not duplicate its revision");
    auto edited_notice=informational_notice;
    edited_notice.attribution="Changed attribution";
    throws([&]{(void)venue_repo.upsert_venue_session_notice(edited_notice);},
           "published notice revision is immutable");
    auto override_notice=informational_notice;
    override_notice.revision=2U;
    override_notice.kind=VenueSessionNoticeKind::SessionOverride;
    override_notice.day_status=VenueDayStatus::Trading;
    override_notice.product_sessions.push_back({"xetra_regular",{9,0,0},{13,0,0}});
    require(venue_repo.upsert_venue_session_notice(override_notice)==10U,
            "new notice revision may publish an explicit product session override");
    const auto effective_override=resolve_effective_venue_calendar_day(
        venue_repo.load(),"deutsche-boerse-xetra-2026",parse_date("2026-12-30"));
    require(effective_override.day.status==VenueDayStatus::Trading &&
            effective_override.day.product_sessions.size()==1U &&
            effective_override.day.product_sessions.front().closes_at.hour==13U &&
            !effective_override.requires_official_notice_unresolved,
            "active session override is applied to a copy of the base calendar day");
    require(find_unresolved_venue_notice_dates(
        venue_repo.load(),"deutsche-boerse-xetra-2026").empty(),
        "an active explicit session override resolves the official-notice requirement");
    auto incomplete_override=override_notice;
    incomplete_override.revision=3U;
    incomplete_override.product_sessions.clear();
    throws([&]{(void)venue_repo.upsert_venue_session_notice(incomplete_override);},
           "trading override without explicit hours is rejected");

    const std::string cancellation_bytes="<html>official cancellation notice fixture</html>";
    const auto cancellation_hash=sha256_hex(cancellation_bytes);
    SourceArchiveObject cancellation_object;
    cancellation_object.source_object_id="sha256:"+cancellation_hash;
    cancellation_object.content_sha256=cancellation_hash;
    cancellation_object.archive_relative_path="venue-calendars.json.sources/objects/"+cancellation_hash+".html";
    cancellation_object.content_length_bytes=cancellation_bytes.size();
    cancellation_object.content_type="text/html";
    const auto cancellation_archive=venue_repo.file_path().parent_path()/cancellation_object.archive_relative_path;
    { std::ofstream output(cancellation_archive,std::ios::binary); output<<cancellation_bytes; }
    VenueSessionNotice cancellation_notice;
    cancellation_notice.notice_id="xetra-2026-dec30-cancellation";
    cancellation_notice.revision=1U;
    cancellation_notice.calendar_id="deutsche-boerse-xetra-2026";
    cancellation_notice.venue="XETRA";
    cancellation_notice.effective_date=parse_date("2026-12-30");
    cancellation_notice.source_object_id=cancellation_object.source_object_id;
    cancellation_notice.source_uri=venue_source.source_uri;
    cancellation_notice.attribution="Deutsche Börse Cash Market";
    cancellation_notice.kind=VenueSessionNoticeKind::Cancellation;
    cancellation_notice.cancels_notice_id=override_notice.notice_id;
    cancellation_notice.cancels_revision=2U;
    MarketDataBatch cancellation_batch;
    cancellation_batch.source_objects.push_back(cancellation_object);
    cancellation_batch.venue_session_notices.push_back(cancellation_notice);
    require(venue_repo.upsert_batch(cancellation_batch)==11U,
            "cancellation notice and its immutable source object commit atomically");
    const auto effective_cancelled=resolve_effective_venue_calendar_day(
        venue_repo.load(),"deutsche-boerse-xetra-2026",parse_date("2026-12-30"));
    require(effective_cancelled.day.product_sessions.front().closes_at.hour==17U &&
            effective_cancelled.requires_official_notice_unresolved &&
            std::find(effective_cancelled.applied_notice_ids.begin(),
                      effective_cancelled.applied_notice_ids.end(),
                      cancellation_notice.notice_id)!=effective_cancelled.applied_notice_ids.end() &&
            std::find(effective_cancelled.applied_notice_ids.begin(),
                      effective_cancelled.applied_notice_ids.end(),
                      override_notice.notice_id)==effective_cancelled.applied_notice_ids.end(),
            "cancellation removes the override and restores the unresolved base schedule");
    const auto unresolved_after_cancellation=find_unresolved_venue_notice_dates(
        venue_repo.load(),"deutsche-boerse-xetra-2026");
    require(unresolved_after_cancellation.size()==1U &&
            format_date(unresolved_after_cancellation.front())=="2026-12-30",
            "unresolved notice report restores the date after cancellation");
    auto newer_revision_snapshot=venue_repo.load();
    auto newer_override=override_notice;
    newer_override.revision=3U;
    newer_override.product_sessions={{"xetra_regular",{9,0,0},{14,0,0}}};
    newer_revision_snapshot.venue_session_notices.push_back(newer_override);
    const auto effective_newer_revision=resolve_effective_venue_calendar_day(
        newer_revision_snapshot,"deutsche-boerse-xetra-2026",parse_date("2026-12-30"));
    require(effective_newer_revision.day.product_sessions.front().closes_at.hour==14U &&
            !effective_newer_revision.requires_official_notice_unresolved,
            "cancellation of an older notice revision does not cancel a later replacement revision");
    auto conflicting_snapshot=venue_repo.load();
    conflicting_snapshot.venue_session_notices.erase(
        std::remove_if(conflicting_snapshot.venue_session_notices.begin(),
            conflicting_snapshot.venue_session_notices.end(),[](const VenueSessionNotice& item) {
                return item.kind==VenueSessionNoticeKind::Cancellation;
            }),conflicting_snapshot.venue_session_notices.end());
    auto conflicting_override=override_notice;
    conflicting_override.notice_id="xetra-2026-dec30-other-override";
    conflicting_override.revision=1U;
    conflicting_override.product_sessions={{"shares_and_etps",{9,0,0},{15,0,0}}};
    conflicting_snapshot.venue_session_notices.push_back(conflicting_override);
    throws([&]{(void)resolve_effective_venue_calendar_day(
        conflicting_snapshot,"deutsche-boerse-xetra-2026",parse_date("2026-12-30"));},
        "two active session overrides for one date are rejected as a conflict");
    auto missing_cancellation= cancellation_notice;
    missing_cancellation.notice_id="bad-cancellation";
    missing_cancellation.cancels_notice_id="missing-notice";
    throws([&]{(void)venue_repo.upsert_venue_session_notice(missing_cancellation);},
           "cancellation must reference an existing notice revision");
    auto out_of_range_notice=informational_notice;
    out_of_range_notice.notice_id="out-of-range";
    out_of_range_notice.effective_date=parse_date("2027-01-01");
    throws([&]{(void)venue_repo.upsert_venue_session_notice(out_of_range_notice);},
           "notice date outside the calendar range is rejected");
    auto notice_delete=venue_repo.load();
    notice_delete.venue_session_notices.clear();
    throws([&]{(void)venue_repo.replace(notice_delete);},
           "replace cannot delete published notice history");

    auto conflicting_object=source_object;
    conflicting_object.content_type="application/pdf";
    throws([&]{(void)venue_repo.upsert_source_object(conflicting_object);},
           "same SHA-256 with conflicting object metadata is rejected");
    auto misplaced_object=source_object;
    misplaced_object.archive_relative_path="venue-calendars.json.sources/objects/not-the-hash.html";
    throws([&]{(void)venue_repo.upsert_source_object(misplaced_object);},
           "source archive filename must be content-addressed by its SHA-256");
    auto missing_source_calendar=venue_v4;
    missing_source_calendar.revision=5U;
    missing_source_calendar.source.source_object_id="sha256:"+std::string(64U,'0');
    missing_source_calendar.source.content_sha256=std::string(64U,'0');
    throws([&]{(void)venue_repo.upsert_venue_calendar(missing_source_calendar);},
           "calendar revision cannot reference a missing source object");
    auto source_delete=venue_repo.load();
    source_delete.source_objects.clear();
    throws([&]{(void)venue_repo.replace(source_delete);},
           "replace cannot delete an immutable source object");
    require(venue_repo.load().revision==11U,
            "rejected source object and notice operations leave repository unchanged");
    std::filesystem::remove(source_archive);
    throws([&]{(void)venue_repo.load();},
           "missing content-addressed source archive blocks repository load");

    std::filesystem::remove_all(dir);
    std::cout<<"All JSON repository tests passed.\n";
}
