#include "monitor/market_data_repository.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

namespace monitor::market {
namespace {
using json = nlohmann::json;
std::atomic<std::uint64_t> temp_counter{0};

std::string enum_name(AssetClass v) {
    switch(v) { case AssetClass::Currency:return "currency"; case AssetClass::PreciousMetal:return "precious_metal"; case AssetClass::Commodity:return "commodity"; }
    throw std::invalid_argument("Nieznana klasa aktywa.");
}
std::string enum_name(Unit v) {
    switch(v) { case Unit::CurrencyUnit:return "currency_unit"; case Unit::Gram:return "gram"; case Unit::Kilogram:return "kilogram"; case Unit::TroyOunce:return "troy_ounce"; case Unit::MetricTon:return "metric_ton"; case Unit::Barrel:return "barrel"; case Unit::MegawattHour:return "megawatt_hour"; }
    throw std::invalid_argument("Nieznana jednostka.");
}
std::string enum_name(PriceKind v) {
    switch(v) { case PriceKind::Reference:return "reference"; case PriceKind::Average:return "average"; case PriceKind::Bid:return "bid"; case PriceKind::Ask:return "ask"; case PriceKind::Mid:return "mid"; case PriceKind::Spot:return "spot"; case PriceKind::Transaction:return "transaction"; }
    throw std::invalid_argument("Nieznany rodzaj ceny.");
}
std::string enum_name(Frequency v) {
    switch(v) { case Frequency::Tick:return "tick"; case Frequency::Minute:return "minute"; case Frequency::Hour:return "hour"; case Frequency::Daily:return "daily"; case Frequency::Weekly:return "weekly"; case Frequency::Monthly:return "monthly"; case Frequency::Quarterly:return "quarterly"; case Frequency::SemiAnnual:return "semi_annual"; case Frequency::Annual:return "annual"; case Frequency::Irregular:return "irregular"; }
    throw std::invalid_argument("Nieznana częstotliwość.");
}
std::string enum_name(PeriodStatus v) {
    switch(v) { case PeriodStatus::Complete:return "complete"; case PeriodStatus::Partial:return "partial"; case PeriodStatus::Provisional:return "provisional"; }
    throw std::invalid_argument("Nieznany status okresu.");
}
std::string enum_name(QualityStatus v) {
    switch(v) { case QualityStatus::Unreviewed:return "unreviewed"; case QualityStatus::Validated:return "validated"; case QualityStatus::Suspect:return "suspect"; }
    throw std::invalid_argument("Nieznany status jakości.");
}
std::string enum_name(SeriesCompleteness v) {
    switch(v) { case SeriesCompleteness::Complete:return "complete"; case SeriesCompleteness::Partial:return "partial"; case SeriesCompleteness::Sparse:return "sparse"; case SeriesCompleteness::Unknown:return "unknown"; }
    throw std::invalid_argument("Nieznana kompletność szeregu.");
}
template<class E> E parse_enum(const std::string& value);
template<> AssetClass parse_enum(const std::string& s) {
    if(s=="currency") return AssetClass::Currency;
    if(s=="precious_metal") return AssetClass::PreciousMetal;
    if(s=="commodity") return AssetClass::Commodity;
    throw std::invalid_argument("Nieznana klasa aktywa w JSON.");
}
template<> Unit parse_enum(const std::string& s) {
    if(s=="currency_unit") return Unit::CurrencyUnit;
    if(s=="gram") return Unit::Gram;
    if(s=="kilogram") return Unit::Kilogram;
    if(s=="troy_ounce") return Unit::TroyOunce;
    if(s=="metric_ton") return Unit::MetricTon;
    if(s=="barrel") return Unit::Barrel;
    if(s=="megawatt_hour") return Unit::MegawattHour;
    throw std::invalid_argument("Nieznana jednostka w JSON.");
}
template<> PriceKind parse_enum(const std::string& s) {
    if(s=="reference") return PriceKind::Reference;
    if(s=="average") return PriceKind::Average;
    if(s=="bid") return PriceKind::Bid;
    if(s=="ask") return PriceKind::Ask;
    if(s=="mid") return PriceKind::Mid;
    if(s=="spot") return PriceKind::Spot;
    if(s=="transaction") return PriceKind::Transaction;
    throw std::invalid_argument("Nieznany rodzaj ceny w JSON.");
}
template<> Frequency parse_enum(const std::string& s) {
    if(s=="tick") return Frequency::Tick;
    if(s=="minute") return Frequency::Minute;
    if(s=="hour") return Frequency::Hour;
    if(s=="daily") return Frequency::Daily;
    if(s=="weekly") return Frequency::Weekly;
    if(s=="monthly") return Frequency::Monthly;
    if(s=="quarterly") return Frequency::Quarterly;
    if(s=="semi_annual") return Frequency::SemiAnnual;
    if(s=="annual") return Frequency::Annual;
    if(s=="irregular") return Frequency::Irregular;
    throw std::invalid_argument("Nieznana częstotliwość w JSON.");
}
template<> PeriodStatus parse_enum(const std::string& s) {
    if(s=="complete") return PeriodStatus::Complete;
    if(s=="partial") return PeriodStatus::Partial;
    if(s=="provisional") return PeriodStatus::Provisional;
    throw std::invalid_argument("Nieznany status okresu w JSON.");
}
template<> QualityStatus parse_enum(const std::string& s) {
    if(s=="unreviewed") return QualityStatus::Unreviewed;
    if(s=="validated") return QualityStatus::Validated;
    if(s=="suspect") return QualityStatus::Suspect;
    throw std::invalid_argument("Nieznany status jakości w JSON.");
}
template<> SeriesCompleteness parse_enum(const std::string& s) {
    if(s=="complete") return SeriesCompleteness::Complete;
    if(s=="partial") return SeriesCompleteness::Partial;
    if(s=="sparse") return SeriesCompleteness::Sparse;
    if(s=="unknown") return SeriesCompleteness::Unknown;
    throw std::invalid_argument("Nieznana kompletność szeregu w JSON.");
}

json encode(const Instrument& x) {
    return {{"id",x.id},{"symbol",x.symbol},{"name",x.name},{"asset_class",enum_name(x.asset_class)},
        {"base_currency",x.base_currency?json(*x.base_currency):json(nullptr)},{"quote_currency",x.quote_currency},
        {"quantity_unit",enum_name(x.quantity_unit)},{"allow_negative_prices",x.allow_negative_prices}};
}
Instrument decode_instrument(const json& j) {
    Instrument x;
    x.id=j.at("id").get<std::string>();x.symbol=j.at("symbol").get<std::string>();x.name=j.at("name").get<std::string>();
    x.asset_class=parse_enum<AssetClass>(j.at("asset_class").get<std::string>());
    if(!j.at("base_currency").is_null())x.base_currency=j.at("base_currency").get<std::string>();
    x.quote_currency=j.at("quote_currency").get<std::string>();x.quantity_unit=parse_enum<Unit>(j.at("quantity_unit").get<std::string>());
    x.allow_negative_prices=j.at("allow_negative_prices").get<bool>();return x;
}
json encode(const Provider& x) {
    return {{"id",x.id},{"name",x.name},{"homepage_url",x.homepage_url},{"license",x.license}};
}
Provider decode_provider(const json& j) {
    return Provider{j.at("id").get<std::string>(),j.at("name").get<std::string>(),
        j.at("homepage_url").get<std::string>(),j.at("license").get<std::string>()};
}
json encode(const MarketObservation& x) {
    return {{"schema",x.schema},{"id",x.id},{"instrument_id",x.instrument_id},{"provider_id",x.provider_id},
        {"observed_at",format_utc_timestamp(x.observed_at)},{"received_at",format_utc_timestamp(x.received_at)},
        {"provider_published_on",x.provider_published_on?json(format_date(*x.provider_published_on)):json(nullptr)},
        {"frequency",enum_name(x.frequency)},{"period_status",enum_name(x.period_status)},
        {"price_kind",enum_name(x.price_kind)},{"value",x.value},{"quality",enum_name(x.quality)},
        {"quality_flags",x.quality_flags},{"provider_record_id",x.provider_record_id?json(*x.provider_record_id):json(nullptr)},
        {"source_uri",x.source_uri?json(*x.source_uri):json(nullptr)}};
}
MarketObservation decode_observation(const json& j) {
    MarketObservation x;
    x.schema=j.at("schema").get<std::uint32_t>();x.id=j.at("id").get<std::string>();
    x.instrument_id=j.at("instrument_id").get<std::string>();x.provider_id=j.at("provider_id").get<std::string>();
    x.observed_at=parse_utc_timestamp(j.at("observed_at").get<std::string>());
    x.received_at=parse_utc_timestamp(j.at("received_at").get<std::string>());
    if(!j.at("provider_published_on").is_null())x.provider_published_on=parse_date(j.at("provider_published_on").get<std::string>());
    x.frequency=parse_enum<Frequency>(j.at("frequency").get<std::string>());
    x.period_status=parse_enum<PeriodStatus>(j.at("period_status").get<std::string>());
    x.price_kind=parse_enum<PriceKind>(j.at("price_kind").get<std::string>());x.value=j.at("value").get<double>();
    x.quality=parse_enum<QualityStatus>(j.at("quality").get<std::string>());
    x.quality_flags=j.at("quality_flags").get<std::vector<std::string>>();
    if(!j.at("provider_record_id").is_null())x.provider_record_id=j.at("provider_record_id").get<std::string>();
    if(!j.at("source_uri").is_null())x.source_uri=j.at("source_uri").get<std::string>();
    return x;
}
json encode(const SeriesMetadata& x) {
    return {{"instrument_id",x.instrument_id},{"provider_id",x.provider_id},{"frequency",enum_name(x.frequency)},
        {"first_observed_at",format_utc_timestamp(x.first_observed_at)},{"last_observed_at",format_utc_timestamp(x.last_observed_at)},
        {"expected_count",x.expected_count},{"observed_count",x.observed_count},{"completeness",enum_name(x.completeness)}};
}
SeriesMetadata decode_series(const json& j) {
    return SeriesMetadata{j.at("instrument_id").get<std::string>(),j.at("provider_id").get<std::string>(),
        parse_enum<Frequency>(j.at("frequency").get<std::string>()),parse_utc_timestamp(j.at("first_observed_at").get<std::string>()),
        parse_utc_timestamp(j.at("last_observed_at").get<std::string>()),j.at("expected_count").get<std::uint64_t>(),
        j.at("observed_count").get<std::uint64_t>(),parse_enum<SeriesCompleteness>(j.at("completeness").get<std::string>())};
}
json encode(const MarketObservationRevision& x) {
    return {{"previous",encode(x.previous)},{"superseded_at",format_utc_timestamp(x.superseded_at)},
        {"superseded_by_id",x.superseded_by_id}};
}
MarketObservationRevision decode_revision(const json& j) {
    return MarketObservationRevision{decode_observation(j.at("previous")),
        parse_utc_timestamp(j.at("superseded_at").get<std::string>()),j.at("superseded_by_id").get<std::string>()};
}

void verify_source_archives(const MarketDataSnapshot& snapshot,const std::filesystem::path& repository_file) {
    auto root=repository_file.parent_path();
    if(root.empty()) root=".";
    for(const auto& calendar:snapshot.venue_calendars) {
        const auto& source=calendar.source;
        if(!source.archive_relative_path) continue; // Legacy schema-v1 record.
        const std::filesystem::path relative(*source.archive_relative_path);
        if(relative.empty() || relative.is_absolute() || relative.has_root_name() || relative.has_root_directory())
            throw std::runtime_error("Niebezpieczna ścieżka archiwum źródła.");
        for(const auto& component:relative) {
            if(component==".." || component=="." || component.empty())
                throw std::runtime_error("Ścieżka archiwum źródła wychodzi poza katalog repozytorium.");
        }

        // Resolve every component relative to an already-open directory. This
        // prevents a symlink swap between a path check and opening the file.
        int current_fd=::open(root.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
        if(current_fd<0)
            throw std::runtime_error("Nie można bezpiecznie otworzyć katalogu repozytorium archiwum.");
        for(auto it=relative.begin();it!=relative.end();++it) {
            const bool final_component=std::next(it)==relative.end();
            const int flags=O_RDONLY|O_CLOEXEC|O_NOFOLLOW|
                (final_component ? 0 : O_DIRECTORY);
            const auto component_name=it->string();
            const int next_fd=::openat(current_fd,component_name.c_str(),flags);
            const int open_error=errno;
            ::close(current_fd);
            if(next_fd<0) {
                errno=open_error;
                throw std::runtime_error("Brak pliku archiwum lub niebezpieczny komponent ścieżki manifestu.");
            }
            current_fd=next_fd;
        }
        const int fd=current_fd;
        struct stat st{};
        if(::fstat(fd,&st)!=0 || !S_ISREG(st.st_mode) || st.st_size<0) {
            ::close(fd);
            throw std::runtime_error("Plik archiwum źródła nie jest prawidłowym plikiem regularnym.");
        }
        std::string bytes;
        std::array<char,65536> buffer{};
        while(true) {
            const auto n=::read(fd,buffer.data(),buffer.size());
            if(n==0) break;
            if(n<0) { if(errno==EINTR) continue; ::close(fd); throw std::runtime_error("Błąd odczytu archiwum źródła."); }
            bytes.append(buffer.data(),static_cast<std::size_t>(n));
        }
        ::close(fd);
        if(source.content_length_bytes && bytes.size()!=*source.content_length_bytes)
            throw std::runtime_error("Długość archiwum źródła nie zgadza się z manifestem.");
        if(!source.content_sha256 || sha256_hex(bytes)!=*source.content_sha256)
            throw std::runtime_error("SHA-256 archiwum źródła nie zgadza się z manifestem.");
    }
    for(const auto& object:snapshot.source_objects) {
        SourceCalendarDocument manifest;
        manifest.archive_relative_path=object.archive_relative_path;
        manifest.content_length_bytes=object.content_length_bytes;
        manifest.content_sha256=object.content_sha256;
        VenueSessionCalendar archive_record;
        archive_record.source=std::move(manifest);
        MarketDataSnapshot archive_snapshot;
        archive_snapshot.venue_calendars.push_back(std::move(archive_record));
        verify_source_archives(archive_snapshot,repository_file);
    }
}
std::string observation_key(const MarketObservation& x) {
    return x.instrument_id+"\x1f"+x.provider_id+"\x1f"+std::to_string(x.observed_at.unix_seconds)+"\x1f"+
        enum_name(x.price_kind)+"\x1f"+enum_name(x.frequency);
}

json encode(const MarketSessionCalendar& x) {
    json times=json::array();
    for(const auto timestamp:x.expected_observation_times) times.push_back(format_utc_timestamp(timestamp));
    return {{"calendar_id",x.calendar_id},{"version",x.version},{"instrument_id",x.instrument_id},
        {"provider_id",x.provider_id},{"source_frequency",enum_name(x.source_frequency)},
        {"timezone",x.timezone},{"expected_observation_times",std::move(times)}};
}
MarketSessionCalendar decode_session_calendar(const json& j) {
    MarketSessionCalendar x;
    x.calendar_id=j.at("calendar_id").get<std::string>();
    x.version=j.at("version").get<std::uint32_t>();
    x.instrument_id=j.at("instrument_id").get<std::string>();
    x.provider_id=j.at("provider_id").get<std::string>();
    x.source_frequency=parse_enum<Frequency>(j.at("source_frequency").get<std::string>());
    x.timezone=j.at("timezone").get<std::string>();
    for(const auto& value:j.at("expected_observation_times"))
        x.expected_observation_times.push_back(parse_utc_timestamp(value.get<std::string>()));
    return x;
}
std::string enum_name(VenueDayStatus value) {
    switch (value) {
    case VenueDayStatus::Closed: return "closed";
    case VenueDayStatus::Trading: return "trading";
    case VenueDayStatus::SettlementOnly: return "settlement_only";
    }
    throw std::invalid_argument("Nieznany status dnia kalendarza venue.");
}
VenueDayStatus parse_venue_day_status(const std::string& value) {
    if (value == "closed") return VenueDayStatus::Closed;
    if (value == "trading") return VenueDayStatus::Trading;
    if (value == "settlement_only") return VenueDayStatus::SettlementOnly;
    throw std::invalid_argument("Nieznany status dnia kalendarza venue w JSON.");
}
std::string enum_name(VenueCalendarLicenseStatus value) {
    switch (value) {
    case VenueCalendarLicenseStatus::Unknown: return "unknown";
    case VenueCalendarLicenseStatus::AttributionRequired: return "attribution_required";
    case VenueCalendarLicenseStatus::OpenData: return "open_data";
    case VenueCalendarLicenseStatus::Restricted: return "restricted";
    }
    throw std::invalid_argument("Nieznany status licencji kalendarza venue.");
}
VenueCalendarLicenseStatus parse_venue_license_status(const std::string& value) {
    if (value == "unknown") return VenueCalendarLicenseStatus::Unknown;
    if (value == "attribution_required") return VenueCalendarLicenseStatus::AttributionRequired;
    if (value == "open_data") return VenueCalendarLicenseStatus::OpenData;
    if (value == "restricted") return VenueCalendarLicenseStatus::Restricted;
    throw std::invalid_argument("Nieznany status licencji kalendarza venue w JSON.");
}
std::string enum_name(VenueSessionNoticeKind value) {
    switch(value) {
    case VenueSessionNoticeKind::Informational:return "informational";
    case VenueSessionNoticeKind::SessionOverride:return "session_override";
    case VenueSessionNoticeKind::Cancellation:return "cancellation";
    }
    throw std::invalid_argument("Nieznany rodzaj komunikatu sesyjnego.");
}
VenueSessionNoticeKind parse_venue_session_notice_kind(const std::string& value) {
    if(value=="informational") return VenueSessionNoticeKind::Informational;
    if(value=="session_override") return VenueSessionNoticeKind::SessionOverride;
    if(value=="cancellation") return VenueSessionNoticeKind::Cancellation;
    throw std::invalid_argument("Nieznany rodzaj komunikatu sesyjnego w JSON.");
}
json encode(const VenueSessionCalendar& x) {
    const auto& source = x.source;
    json source_json = {
        {"document_id",source.document_id},{"venue",source.venue},{"version",source.version},
        {"valid_from",format_date(source.valid_from)},
        {"valid_to_exclusive",format_date(source.valid_to_exclusive)},
        {"timezone",source.timezone},{"source_uri",source.source_uri},
        {"retrieved_at",source.retrieved_at ? json(format_utc_timestamp(*source.retrieved_at)) : json(nullptr)},
        {"content_sha256",source.content_sha256 ? json(*source.content_sha256) : json(nullptr)},
        {"source_object_id",source.source_object_id ? json(*source.source_object_id) : json(nullptr)},
        {"archive_relative_path",source.archive_relative_path ? json(*source.archive_relative_path) : json(nullptr)},
        {"content_length_bytes",source.content_length_bytes ? json(*source.content_length_bytes) : json(nullptr)},
        {"content_type",source.content_type ? json(*source.content_type) : json(nullptr)},
        {"attribution",source.attribution},{"license_status",enum_name(source.license_status)}
    };
    json days = json::array();
    for (const auto& day : x.days) {
        json sessions = json::array();
        for (const auto& session : day.product_sessions) {
            sessions.push_back({
                {"product_group",session.product_group},
                {"opens_at",{{"hour",session.opens_at.hour},{"minute",session.opens_at.minute},{"second",session.opens_at.second}}},
                {"closes_at",{{"hour",session.closes_at.hour},{"minute",session.closes_at.minute},{"second",session.closes_at.second}}}
            });
        }
        days.push_back({{"date",format_date(day.date)},{"status",enum_name(day.status)},
            {"product_sessions",std::move(sessions)},{"requires_official_notice",day.requires_official_notice}});
    }
    return {{"calendar_id",x.calendar_id},{"revision",x.revision},
            {"parser_version",x.parser_version},
            {"source",std::move(source_json)},{"days",std::move(days)}};
}
VenueSessionCalendar decode_venue_calendar(const json& j) {
    VenueSessionCalendar result;
    if(j.contains("calendar_id")) result.calendar_id=j.at("calendar_id").get<std::string>();
    if(j.contains("revision")) result.revision=j.at("revision").get<std::uint32_t>();
    if(j.contains("parser_version")) result.parser_version=j.at("parser_version").get<std::string>();
    const auto& s = j.at("source");
    result.source.document_id=s.at("document_id").get<std::string>();
    result.source.venue=s.at("venue").get<std::string>();
    result.source.version=s.at("version").get<std::uint32_t>();
    result.source.valid_from=parse_date(s.at("valid_from").get<std::string>());
    result.source.valid_to_exclusive=parse_date(s.at("valid_to_exclusive").get<std::string>());
    result.source.timezone=s.at("timezone").get<std::string>();
    result.source.source_uri=s.at("source_uri").get<std::string>();
    if (!s.at("retrieved_at").is_null())
        result.source.retrieved_at=parse_utc_timestamp(s.at("retrieved_at").get<std::string>());
    if (!s.at("content_sha256").is_null())
        result.source.content_sha256=s.at("content_sha256").get<std::string>();
    if (s.contains("source_object_id") && !s.at("source_object_id").is_null())
        result.source.source_object_id=s.at("source_object_id").get<std::string>();
    if (s.contains("archive_relative_path") && !s.at("archive_relative_path").is_null())
        result.source.archive_relative_path=s.at("archive_relative_path").get<std::string>();
    if (s.contains("content_length_bytes") && !s.at("content_length_bytes").is_null())
        result.source.content_length_bytes=s.at("content_length_bytes").get<std::uint64_t>();
    if (s.contains("content_type") && !s.at("content_type").is_null())
        result.source.content_type=s.at("content_type").get<std::string>();
    result.source.attribution=s.at("attribution").get<std::string>();
    result.source.license_status=parse_venue_license_status(s.at("license_status").get<std::string>());
    // Old schema-v1 records used source.document_id + source.version as the
    // calendar key. Materialize that legacy key into the new explicit fields.
    if(result.calendar_id.empty() && result.revision==0U) {
        result.calendar_id=result.source.document_id;
        result.revision=result.source.version;
    }
    for (const auto& value : j.at("days")) {
        VenueCalendarDay day;
        day.date=parse_date(value.at("date").get<std::string>());
        day.status=parse_venue_day_status(value.at("status").get<std::string>());
        day.requires_official_notice=value.at("requires_official_notice").get<bool>();
        for (const auto& item : value.at("product_sessions")) {
            const auto read_time=[](const json& t) {
                return LocalTimeOfDay{t.at("hour").get<unsigned>(),t.at("minute").get<unsigned>(),
                                      t.at("second").get<unsigned>()};
            };
            day.product_sessions.push_back({item.at("product_group").get<std::string>(),
                read_time(item.at("opens_at")),read_time(item.at("closes_at"))});
        }
        result.days.push_back(std::move(day));
    }
    return result;
}
json encode(const SourceArchiveObject& x) {
    return {{"source_object_id",x.source_object_id},{"content_sha256",x.content_sha256},
        {"archive_relative_path",x.archive_relative_path},{"content_length_bytes",x.content_length_bytes},
        {"content_type",x.content_type}};
}
SourceArchiveObject decode_source_object(const json& j) {
    SourceArchiveObject x;
    x.source_object_id=j.at("source_object_id").get<std::string>();
    x.content_sha256=j.at("content_sha256").get<std::string>();
    x.archive_relative_path=j.at("archive_relative_path").get<std::string>();
    x.content_length_bytes=j.at("content_length_bytes").get<std::uint64_t>();
    x.content_type=j.at("content_type").get<std::string>();
    return x;
}
void validate_source_object(const SourceArchiveObject& x) {
    const auto valid_hash=[](const std::string& hash) {
        return hash.size()==64U && std::all_of(hash.begin(),hash.end(),[](unsigned char ch) {
            return (ch>='0'&&ch<='9')||(ch>='a'&&ch<='f');
        });
    };
    const std::filesystem::path path(x.archive_relative_path);
    const auto filename=path.filename().string();
    if(!valid_hash(x.content_sha256) || x.source_object_id!="sha256:"+x.content_sha256 ||
       x.archive_relative_path.empty() || path.is_absolute() || path.has_root_name() ||
       path.has_root_directory() || filename.rfind(x.content_sha256+".",0)!=0U ||
       x.content_length_bytes==0U || x.content_type.empty())
        throw std::invalid_argument("Obiekt źródłowy wymaga identyfikatora SHA-256, ścieżki content-addressed i metadanych bajtów.");
    for(const auto& component:path)
        if(component.empty() || component=="." || component=="..")
            throw std::invalid_argument("Ścieżka obiektu źródłowego zawiera niedozwolony komponent.");
}
std::string effective_venue_calendar_id(const VenueSessionCalendar& x) {
    return x.calendar_id.empty() ? x.source.document_id : x.calendar_id;
}
std::uint32_t effective_venue_calendar_revision(const VenueSessionCalendar& x) {
    return x.revision==0U ? x.source.version : x.revision;
}
std::string venue_calendar_key(const VenueSessionCalendar& x) {
    return effective_venue_calendar_id(x) + "\x1f" +
           std::to_string(effective_venue_calendar_revision(x));
}
bool same_venue_calendar(const VenueSessionCalendar& a, const VenueSessionCalendar& b) {
    return encode(a) == encode(b);
}
void validate_venue_calendar(const VenueSessionCalendar& x) {
    validate_source_calendar_document(x.source);
    if(x.calendar_id.empty()!=(x.revision==0U))
        throw std::invalid_argument("Kalendarz wymaga jednocześnie identyfikatora i dodatniego numeru rewizji albo klucza legacy.");
    if(!x.calendar_id.empty() &&
       (x.calendar_id.size()>128U || std::any_of(x.calendar_id.begin(),x.calendar_id.end(),[](unsigned char ch) {
           return ch<0x20U || ch==0x7fU;
       })))
        throw std::invalid_argument("Identyfikator kalendarza zawiera niedozwolone znaki.");
    if(x.parser_version.size()>64U || std::any_of(x.parser_version.begin(),x.parser_version.end(),[](unsigned char ch) {
        return ch<0x20U || ch==0x7fU;
    }))
        throw std::invalid_argument("Wersja parsera kalendarza jest nieprawidłowa.");
    using namespace std::chrono;
    const sys_days begin{year{x.source.valid_from.year}/month{x.source.valid_from.month}/day{x.source.valid_from.day}};
    const sys_days end{year{x.source.valid_to_exclusive.year}/month{x.source.valid_to_exclusive.month}/day{x.source.valid_to_exclusive.day}};
    if (x.days.size() != static_cast<std::size_t>((end-begin).count()))
        throw std::invalid_argument("Kalendarz venue musi zawierać każdy dzień zakresu źródła.");
    std::set<std::string> keys;
    for (std::size_t i=0; i<x.days.size(); ++i) {
        const auto expected=begin+days{static_cast<days::rep>(i)};
        const year_month_day ymd{expected};
        const CivilDate expected_date{static_cast<int>(ymd.year()),static_cast<unsigned>(ymd.month()),static_cast<unsigned>(ymd.day())};
        const auto& day=x.days[i];
        if (!is_valid_date(day.date) || format_date(day.date)!=format_date(expected_date))
            throw std::invalid_argument("Dni kalendarza venue muszą być kompletne, uporządkowane i bez duplikatów.");
        (void)enum_name(day.status);
        if (day.status==VenueDayStatus::Trading && day.product_sessions.empty())
            throw std::invalid_argument("Dzień handlowy wymaga co najmniej jednej sesji produktowej.");
        if (day.status!=VenueDayStatus::Trading && !day.product_sessions.empty())
            throw std::invalid_argument("Dzień bez handlu nie może zawierać sesji produktowych.");
        std::set<std::string> products;
        for (const auto& session : day.product_sessions) {
            if (session.product_group.empty() || !products.insert(session.product_group).second)
                throw std::invalid_argument("Grupy produktów sesji muszą być niepuste i unikalne w danym dniu.");
            const auto valid_time=[](const LocalTimeOfDay& t) {
                return t.hour<24U && t.minute<60U && t.second<60U;
            };
            if (!valid_time(session.opens_at)||!valid_time(session.closes_at))
                throw std::invalid_argument("Godzina sesji venue jest poza zakresem.");
            const auto open=std::tuple{session.opens_at.hour,session.opens_at.minute,session.opens_at.second};
            const auto close=std::tuple{session.closes_at.hour,session.closes_at.minute,session.closes_at.second};
            if (!(open<close)) throw std::invalid_argument("Koniec sesji musi następować po jej otwarciu.");
        }
        if (!keys.insert(format_date(day.date)).second)
            throw std::invalid_argument("Powtórzona data w kalendarzu venue.");
    }
}

std::string calendar_key(const MarketSessionCalendar& x) {
    return x.calendar_id+"\x1f"+std::to_string(x.version);
}
bool same_calendar(const MarketSessionCalendar& a,const MarketSessionCalendar& b) {
    if(a.calendar_id!=b.calendar_id||a.version!=b.version||a.instrument_id!=b.instrument_id||
       a.provider_id!=b.provider_id||a.source_frequency!=b.source_frequency||a.timezone!=b.timezone||
       a.expected_observation_times.size()!=b.expected_observation_times.size()) return false;
    for(std::size_t i=0;i<a.expected_observation_times.size();++i)
        if(a.expected_observation_times[i].unix_seconds!=b.expected_observation_times[i].unix_seconds) return false;
    return true;
}
void validate_calendar(const MarketSessionCalendar& x) {
    if(x.calendar_id.empty()||x.version==0U||x.instrument_id.empty()||x.provider_id.empty()||
       x.timezone.empty()||x.expected_observation_times.empty())
        throw std::invalid_argument("Kalendarz wymaga identyfikatora, wersji, szeregu, strefy i terminów.");
    if(x.timezone!="UTC"&&x.timezone.find('/')==std::string::npos)
        throw std::invalid_argument("Strefa musi być identyfikatorem IANA albo UTC.");
    for(const unsigned char c:x.timezone)
        if(!(std::isalnum(c)||c=='/'||c=='_'||c=='-'||c=='+'))
            throw std::invalid_argument("Nieprawidłowy zapis identyfikatora strefy.");
    switch(x.source_frequency) {
    case Frequency::Minute: case Frequency::Hour: case Frequency::Daily: case Frequency::Weekly:
    case Frequency::Monthly: case Frequency::Quarterly: case Frequency::SemiAnnual: case Frequency::Annual: break;
    case Frequency::Tick: case Frequency::Irregular:
    default: throw std::invalid_argument("Kalendarz wymaga regularnej częstotliwości źródłowej.");
    }
    bool first=true; std::int64_t previous=0;
    for(const auto t:x.expected_observation_times) {
        if(!first&&t.unix_seconds<=previous)
            throw std::invalid_argument("Terminy kalendarza muszą być unikalne i ściśle rosnące.");
        previous=t.unix_seconds; first=false;
    }
}

std::string venue_session_notice_key(const VenueSessionNotice& notice);
std::string notice_stable_identity(const VenueSessionNotice& notice);
void validate_venue_session_notice(const VenueSessionNotice& notice);

void validate_snapshot(const MarketDataSnapshot& s) {
    if(s.schema_version!=json_document_schema_version)throw std::invalid_argument("Nieobsługiwana wersja dokumentu magazynu JSON.");
    std::map<std::string,const Instrument*> instruments;
    std::map<std::string,const Provider*> providers;
    for(const auto& x:s.instruments){validate_instrument(x);if(!instruments.emplace(x.id,&x).second)throw std::invalid_argument("Powielony identyfikator instrumentu: "+x.id);}
    for(const auto& x:s.providers){validate_provider(x);if(!providers.emplace(x.id,&x).second)throw std::invalid_argument("Powielony identyfikator dostawcy: "+x.id);}
    std::set<std::string> keys,ids;
    for(const auto& x:s.observations) {
        const auto i=instruments.find(x.instrument_id);
        const auto p=providers.find(x.provider_id);
        if(i==instruments.end()||p==providers.end())throw std::invalid_argument("Notowanie wskazuje nieznany instrument lub dostawcę.");
        validate_observation(*i->second,*p->second,x);
        if(!keys.insert(observation_key(x)).second)throw std::invalid_argument("Powielony klucz notowania w migawce.");
        if(!ids.insert(x.id).second)throw std::invalid_argument("Powielony identyfikator notowania.");
    }
    std::set<std::string> calendar_keys;
    std::map<std::string,std::uint32_t> latest_versions;
    std::map<std::string,std::pair<std::string,std::string>> calendar_owners;
    for(const auto& x:s.session_calendars) {
        validate_calendar(x);
        if(!instruments.contains(x.instrument_id)||!providers.contains(x.provider_id))
            throw std::invalid_argument("Kalendarz wskazuje nieznany instrument lub dostawcę.");
        if(!calendar_keys.insert(calendar_key(x)).second)
            throw std::invalid_argument("Powielona wersja kalendarza.");
        const auto owner=std::make_pair(x.instrument_id,x.provider_id);
        const auto [owner_it,inserted]=calendar_owners.emplace(x.calendar_id,owner);
        if(!inserted&&owner_it->second!=owner)
            throw std::invalid_argument("Wersje kalendarza muszą dotyczyć tego samego szeregu.");
        auto& latest=latest_versions[x.calendar_id];
        if(x.version<=latest) throw std::invalid_argument("Wersje kalendarza muszą być rosnące.");
        latest=x.version;
    }
    std::map<std::string,const SourceArchiveObject*> source_objects;
    for(const auto& object:s.source_objects) {
        validate_source_object(object);
        if(!source_objects.emplace(object.source_object_id,&object).second)
            throw std::invalid_argument("Powielony niezmienny obiekt źródłowy.");
    }
    std::set<std::string> venue_keys;
    std::map<std::string,std::uint32_t> latest_venue_versions;
    std::map<std::string,std::string> venue_by_calendar_id;
    std::map<std::string,std::string> source_version_hashes;
    for (const auto& calendar : s.venue_calendars) {
        validate_venue_calendar(calendar);
        if(calendar.source.source_object_id) {
            const auto object=source_objects.find(*calendar.source.source_object_id);
            if(object==source_objects.end() || !calendar.source.content_sha256 ||
               *calendar.source.content_sha256!=object->second->content_sha256)
                throw std::invalid_argument("Kalendarz venue wskazuje brakujący lub niezgodny obiekt źródłowy.");
        }
        const auto key=venue_calendar_key(calendar);
        if (!venue_keys.insert(key).second)
            throw std::invalid_argument("Powielona wersja kalendarza sesyjnego venue.");
        if(calendar.source.content_sha256) {
            const auto source_version_key=std::to_string(calendar.source.document_id.size())+":"+
                calendar.source.document_id+":"+std::to_string(calendar.source.version);
            const auto [source_it,source_inserted]=source_version_hashes.emplace(
                source_version_key,*calendar.source.content_sha256);
            if(!source_inserted && source_it->second!=*calendar.source.content_sha256)
                throw std::invalid_argument("Ta sama wersja dokumentu źródłowego nie może wskazywać różnych bajtów.");
        }
        const auto calendar_id=effective_venue_calendar_id(calendar);
        const auto revision=effective_venue_calendar_revision(calendar);
        const auto [venue_it,venue_inserted]=venue_by_calendar_id.emplace(calendar_id,calendar.source.venue);
        if(!venue_inserted && venue_it->second!=calendar.source.venue)
            throw std::invalid_argument("Identyfikator kalendarza nie może obejmować różnych rynków.");
        auto& latest=latest_venue_versions[calendar_id];
        if (revision<=latest)
            throw std::invalid_argument("Rewizje kalendarza venue muszą być rosnące.");
        latest=revision;
    }
    std::set<std::string> notice_keys;
    std::map<std::string,std::uint32_t> latest_notice_revisions;
    std::map<std::string,std::string> notice_identities;
    std::map<std::string,const VenueSessionNotice*> notice_versions;
    for(const auto& notice:s.venue_session_notices) {
        validate_venue_session_notice(notice);
        if(!source_objects.contains(notice.source_object_id))
            throw std::invalid_argument("Komunikat sesyjny wskazuje brakujący obiekt źródłowy.");
        const auto venue_it=venue_by_calendar_id.find(notice.calendar_id);
        if(venue_it==venue_by_calendar_id.end() || venue_it->second!=notice.venue)
            throw std::invalid_argument("Komunikat sesyjny wskazuje nieznany kalendarz lub inny rynek.");
        const auto calendar=std::find_if(s.venue_calendars.begin(),s.venue_calendars.end(),
            [&](const VenueSessionCalendar& item) {
                if(effective_venue_calendar_id(item)!=notice.calendar_id || item.source.venue!=notice.venue)
                    return false;
                using namespace std::chrono;
                const sys_days date{year{notice.effective_date.year}/month{notice.effective_date.month}/day{notice.effective_date.day}};
                const sys_days first{year{item.source.valid_from.year}/month{item.source.valid_from.month}/day{item.source.valid_from.day}};
                const sys_days last{year{item.source.valid_to_exclusive.year}/month{item.source.valid_to_exclusive.month}/day{item.source.valid_to_exclusive.day}};
                return date>=first && date<last;
            });
        if(calendar==s.venue_calendars.end())
            throw std::invalid_argument("Data komunikatu sesyjnego wypada poza zakresem kalendarza.");
        const auto key=venue_session_notice_key(notice);
        if(!notice_keys.insert(key).second)
            throw std::invalid_argument("Powielona rewizja komunikatu sesyjnego.");
        const auto identity=notice_stable_identity(notice);
        const auto [identity_it,identity_inserted]=notice_identities.emplace(notice.notice_id,identity);
        if(!identity_inserted && identity_it->second!=identity)
            throw std::invalid_argument("Rewizje komunikatu muszą zachować rynek, kalendarz i datę obowiązywania.");
        auto& latest=latest_notice_revisions[notice.notice_id];
        if(notice.revision<=latest)
            throw std::invalid_argument("Rewizje komunikatu sesyjnego muszą być rosnące.");
        latest=notice.revision;
        notice_versions.emplace(key,&notice);
    }
    for(const auto& notice:s.venue_session_notices) {
        if(notice.kind!=VenueSessionNoticeKind::Cancellation) continue;
        const auto target_key=notice.cancels_notice_id+"\x1f"+std::to_string(notice.cancels_revision);
        const auto target=notice_versions.find(target_key);
        if(target==notice_versions.end() || target->second->kind==VenueSessionNoticeKind::Cancellation ||
           target->second->calendar_id!=notice.calendar_id || target->second->venue!=notice.venue ||
           format_date(target->second->effective_date)!=format_date(notice.effective_date))
            throw std::invalid_argument("Anulowanie musi wskazywać istniejący komunikat dla tego samego rynku i dnia.");
    }
    std::set<std::string> series_keys;
    for(const auto& x:s.series_metadata) {
        if(!instruments.contains(x.instrument_id)||!providers.contains(x.provider_id))throw std::invalid_argument("Metadane szeregu wskazują nieznany instrument lub dostawcę.");
        validate_series_metadata(x);
        const auto key=x.instrument_id+"\x1f"+x.provider_id+"\x1f"+enum_name(x.frequency);
        if(!series_keys.insert(key).second)throw std::invalid_argument("Powielone metadane szeregu.");
    }
    for(const auto& x:s.observation_history) {
        const auto i=instruments.find(x.previous.instrument_id);
        const auto p=providers.find(x.previous.provider_id);
        if(i==instruments.end()||p==providers.end())throw std::invalid_argument("Historia notowania wskazuje nieznany instrument lub dostawcę.");
        validate_observation(*i->second,*p->second,x.previous);
        if(x.superseded_by_id.empty()||x.superseded_at.unix_seconds<x.previous.received_at.unix_seconds)
            throw std::invalid_argument("Nieprawidłowa historia korekty notowania.");
    }
}
json encode(const VenueSessionNotice& x) {
    json sessions=json::array();
    for(const auto& session:x.product_sessions) {
        sessions.push_back({{"product_group",session.product_group},
            {"opens_at",{{"hour",session.opens_at.hour},{"minute",session.opens_at.minute},{"second",session.opens_at.second}}},
            {"closes_at",{{"hour",session.closes_at.hour},{"minute",session.closes_at.minute},{"second",session.closes_at.second}}}});
    }
    return {{"notice_id",x.notice_id},{"revision",x.revision},{"calendar_id",x.calendar_id},
        {"venue",x.venue},{"effective_date",format_date(x.effective_date)},
        {"source_object_id",x.source_object_id},{"source_uri",x.source_uri},
        {"retrieved_at",x.retrieved_at ? json(format_utc_timestamp(*x.retrieved_at)) : json(nullptr)},
        {"attribution",x.attribution},{"license_status",enum_name(x.license_status)},
        {"kind",enum_name(x.kind)},
        {"day_status",x.day_status ? json(enum_name(*x.day_status)) : json(nullptr)},
        {"product_sessions",std::move(sessions)},
        {"cancels_notice_id",x.cancels_notice_id},{"cancels_revision",x.cancels_revision}};
}
VenueSessionNotice decode_venue_session_notice(const json& j) {
    VenueSessionNotice x;
    x.notice_id=j.at("notice_id").get<std::string>();
    x.revision=j.at("revision").get<std::uint32_t>();
    x.calendar_id=j.at("calendar_id").get<std::string>();
    x.venue=j.at("venue").get<std::string>();
    x.effective_date=parse_date(j.at("effective_date").get<std::string>());
    x.source_object_id=j.at("source_object_id").get<std::string>();
    x.source_uri=j.at("source_uri").get<std::string>();
    if(!j.at("retrieved_at").is_null()) x.retrieved_at=parse_utc_timestamp(j.at("retrieved_at").get<std::string>());
    x.attribution=j.at("attribution").get<std::string>();
    x.license_status=parse_venue_license_status(j.at("license_status").get<std::string>());
    x.kind=parse_venue_session_notice_kind(j.at("kind").get<std::string>());
    if(!j.at("day_status").is_null()) x.day_status=parse_venue_day_status(j.at("day_status").get<std::string>());
    for(const auto& item:j.at("product_sessions")) {
        const auto read_time=[](const json& t) {
            return LocalTimeOfDay{t.at("hour").get<unsigned>(),t.at("minute").get<unsigned>(),t.at("second").get<unsigned>()};
        };
        x.product_sessions.push_back({item.at("product_group").get<std::string>(),
            read_time(item.at("opens_at")),read_time(item.at("closes_at"))});
    }
    x.cancels_notice_id=j.at("cancels_notice_id").get<std::string>();
    x.cancels_revision=j.at("cancels_revision").get<std::uint32_t>();
    return x;
}
std::string venue_session_notice_key(const VenueSessionNotice& x) {
    return x.notice_id+"\x1f"+std::to_string(x.revision);
}
bool same_venue_session_notice(const VenueSessionNotice& a,const VenueSessionNotice& b) {
    return encode(a)==encode(b);
}
void validate_venue_session_notice(const VenueSessionNotice& x) {
    if(x.notice_id.empty() || x.revision==0U || x.calendar_id.empty() || x.venue.empty() ||
       !is_valid_date(x.effective_date) || x.source_object_id.rfind("sha256:",0)!=0 ||
       x.source_object_id.size()!=71U || x.source_uri.rfind("https://",0)!=0 ||
       x.attribution.empty())
        throw std::invalid_argument("Komunikat sesyjny wymaga identyfikatora, daty, archiwum i pochodzenia HTTPS.");
    (void)enum_name(x.license_status);
    (void)enum_name(x.kind);
    const auto valid_time=[](const LocalTimeOfDay& t) {
        return t.hour<=23U && t.minute<=59U && t.second<=59U;
    };
    std::set<std::string> product_groups;
    for(const auto& session:x.product_sessions) {
        if(session.product_group.empty() || !product_groups.insert(session.product_group).second ||
           !valid_time(session.opens_at) || !valid_time(session.closes_at) ||
           std::tie(session.opens_at.hour,session.opens_at.minute,session.opens_at.second)>=
           std::tie(session.closes_at.hour,session.closes_at.minute,session.closes_at.second))
            throw std::invalid_argument("Komunikat zawiera nieprawidłową lub powtórzoną sesję produktową.");
    }
    switch(x.kind) {
    case VenueSessionNoticeKind::Informational:
        if(x.day_status || !x.product_sessions.empty() || !x.cancels_notice_id.empty() || x.cancels_revision!=0U)
            throw std::invalid_argument("Komunikat informacyjny nie może zmieniać sesji ani anulować innego komunikatu.");
        break;
    case VenueSessionNoticeKind::SessionOverride:
        if(!x.day_status || !x.cancels_notice_id.empty() || x.cancels_revision!=0U ||
           (*x.day_status==VenueDayStatus::Trading && x.product_sessions.empty()) ||
           (*x.day_status!=VenueDayStatus::Trading && !x.product_sessions.empty()))
            throw std::invalid_argument("Nadpisanie sesji wymaga statusu dnia i zgodnych godzin produktów.");
        break;
    case VenueSessionNoticeKind::Cancellation:
        if(x.cancels_notice_id.empty() || x.cancels_revision==0U || x.day_status ||
           !x.product_sessions.empty() || x.cancels_notice_id==x.notice_id)
            throw std::invalid_argument("Anulowanie musi wskazywać wcześniejszy komunikat i nie może zawierać godzin.");
        break;
    }
}
std::string notice_stable_identity(const VenueSessionNotice& x) {
    return x.calendar_id+"\x1f"+x.venue+"\x1f"+format_date(x.effective_date);
}
json encode_snapshot(const MarketDataSnapshot& s) {
    json instruments=json::array(),providers=json::array(),observations=json::array(),series=json::array(),history=json::array(),calendars=json::array(),venue_calendars=json::array(),source_objects=json::array(),venue_session_notices=json::array();
    for(const auto& x:s.instruments)instruments.push_back(encode(x));
    for(const auto& x:s.providers)providers.push_back(encode(x));
    for(const auto& x:s.observations)observations.push_back(encode(x));
    for(const auto& x:s.series_metadata)series.push_back(encode(x));
    for(const auto& x:s.observation_history)history.push_back(encode(x));
    for(const auto& x:s.session_calendars)calendars.push_back(encode(x));
    for(const auto& x:s.venue_calendars)venue_calendars.push_back(encode(x));
    for(const auto& x:s.source_objects)source_objects.push_back(encode(x));
    for(const auto& x:s.venue_session_notices)venue_session_notices.push_back(encode(x));
    return {{"document_type","monitor.market_data"},{"schema_version",s.schema_version},{"revision",s.revision},
        {"updated_at",format_utc_timestamp(s.updated_at)},{"instruments",std::move(instruments)},
        {"providers",std::move(providers)},{"observations",std::move(observations)},
        {"series_metadata",std::move(series)},{"observation_history",std::move(history)},{"session_calendars",std::move(calendars)},{"venue_calendars",std::move(venue_calendars)},{"source_objects",std::move(source_objects)},{"venue_session_notices",std::move(venue_session_notices)}};
}
MarketDataSnapshot decode_snapshot(const json& j) {
    if(!j.is_object()||j.at("document_type").get<std::string>()!="monitor.market_data")
        throw std::invalid_argument("Nieznany typ dokumentu magazynu JSON.");
    MarketDataSnapshot s;
    s.schema_version=j.at("schema_version").get<std::uint32_t>();
    if(s.schema_version!=json_document_schema_version)throw std::invalid_argument("Brak migracji dla wersji dokumentu JSON "+std::to_string(s.schema_version)+".");
    s.revision=j.at("revision").get<std::uint64_t>();s.updated_at=parse_utc_timestamp(j.at("updated_at").get<std::string>());
    for(const auto& x:j.at("instruments"))s.instruments.push_back(decode_instrument(x));
    for(const auto& x:j.at("providers"))s.providers.push_back(decode_provider(x));
    for(const auto& x:j.at("observations"))s.observations.push_back(decode_observation(x));
    for(const auto& x:j.at("series_metadata"))s.series_metadata.push_back(decode_series(x));
    for(const auto& x:j.at("observation_history"))s.observation_history.push_back(decode_revision(x));
    if(j.contains("session_calendars")) for(const auto& x:j.at("session_calendars")) s.session_calendars.push_back(decode_session_calendar(x));
    if(j.contains("venue_calendars")) for(const auto& x:j.at("venue_calendars")) s.venue_calendars.push_back(decode_venue_calendar(x));
    if(j.contains("source_objects")) for(const auto& x:j.at("source_objects")) s.source_objects.push_back(decode_source_object(x));
    if(j.contains("venue_session_notices")) for(const auto& x:j.at("venue_session_notices")) s.venue_session_notices.push_back(decode_venue_session_notice(x));
    validate_snapshot(s);return s;
}
std::string read_file(const std::filesystem::path& path,std::size_t limit,bool missing_ok) {
    const int fd=::open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    if(fd<0) {
        if(missing_ok&&errno==ENOENT)return {};
        throw std::runtime_error("Nie można otworzyć magazynu JSON "+path.string()+": "+std::strerror(errno));
    }
    struct stat st{};
    if(::fstat(fd,&st)!=0){const int e=errno;::close(fd);throw std::runtime_error("Nie można odczytać metadanych pliku: "+std::string(std::strerror(e)));}
    if(!S_ISREG(st.st_mode)){::close(fd);throw std::runtime_error("Magazyn JSON nie jest zwykłym plikiem.");}
    if(st.st_size<0||static_cast<std::uintmax_t>(st.st_size)>limit){::close(fd);throw std::runtime_error("Plik magazynu JSON przekracza skonfigurowany limit.");}
    std::string data;data.reserve(static_cast<std::size_t>(st.st_size));
    std::array<char,65536> buffer{};
    while(true) {
        const auto n=::read(fd,buffer.data(),buffer.size());
        if(n==0)break;
        if(n<0){if(errno==EINTR)continue;const int e=errno;::close(fd);throw std::runtime_error("Błąd odczytu magazynu JSON: "+std::string(std::strerror(e)));}
        const auto count=static_cast<std::size_t>(n);
        if(data.size()>limit||count>limit-data.size()){::close(fd);throw std::runtime_error("Plik magazynu JSON przekracza skonfigurowany limit.");}
        data.append(buffer.data(),count);
    }
    ::close(fd);return data;
}
void write_all(int fd,std::string_view data) {
    std::size_t offset=0;
    while(offset<data.size()) {
        const auto n=::write(fd,data.data()+offset,data.size()-offset);
        if(n<0){if(errno==EINTR)continue;throw std::runtime_error("Błąd zapisu pliku: "+std::string(std::strerror(errno)));}
        if(n==0)throw std::runtime_error("Zapis pliku nie postępuje.");
        offset+=static_cast<std::size_t>(n);
    }
}
std::filesystem::path make_temp_path(const std::filesystem::path& target) {
    return target.string()+".tmp."+std::to_string(::getpid())+"."+std::to_string(temp_counter.fetch_add(1));
}
void durable_write(const std::filesystem::path& path,std::string_view data) {
    const auto temp=make_temp_path(path);
    const int fd=::open(temp.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
    if(fd<0)throw std::runtime_error("Nie można utworzyć pliku tymczasowego: "+std::string(std::strerror(errno)));
    int open_fd=fd;
    try {
        write_all(open_fd,data);
        if(::fsync(open_fd)!=0)throw std::runtime_error("Nie można zsynchronizować pliku: "+std::string(std::strerror(errno)));
        if(::close(open_fd)!=0){open_fd=-1;throw std::runtime_error("Nie można zamknąć pliku tymczasowego.");}
        open_fd=-1;
        if(::rename(temp.c_str(),path.c_str())!=0)throw std::runtime_error("Nie można atomowo podmienić pliku: "+std::string(std::strerror(errno)));
    } catch(...) {if(open_fd>=0)::close(open_fd);::unlink(temp.c_str());throw;}
}
void sync_directory(const std::filesystem::path& dir) {
    const int fd=::open(dir.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC);
    if(fd<0)throw std::runtime_error("Nie można otworzyć katalogu magazynu do synchronizacji.");
    const int result=::fsync(fd);const int saved=errno;::close(fd);
    if(result!=0)throw std::runtime_error("Nie można zsynchronizować katalogu magazynu: "+std::string(std::strerror(saved)));
}
class FileLock {
public:
    explicit FileLock(const std::filesystem::path& path) {
        fd_=::open(path.c_str(),O_CREAT|O_RDWR|O_CLOEXEC|O_NOFOLLOW,0600);
        if(fd_<0)throw std::runtime_error("Nie można otworzyć blokady magazynu: "+std::string(std::strerror(errno)));
        while(::flock(fd_,LOCK_EX)!=0){if(errno==EINTR)continue;const int e=errno;::close(fd_);fd_=-1;throw std::runtime_error("Nie można zablokować magazynu: "+std::string(std::strerror(e)));}
    }
    ~FileLock(){if(fd_>=0){::flock(fd_,LOCK_UN);::close(fd_);}}
    FileLock(const FileLock&)=delete;FileLock& operator=(const FileLock&)=delete;
private:int fd_{-1};
};
bool same_payload(const MarketObservation& a,const MarketObservation& b) {
    return a.schema==b.schema&&a.id==b.id&&a.instrument_id==b.instrument_id&&a.provider_id==b.provider_id&&
        a.observed_at.unix_seconds==b.observed_at.unix_seconds&&
        ((!a.provider_published_on&&!b.provider_published_on)||(a.provider_published_on&&b.provider_published_on&&
          a.provider_published_on->year==b.provider_published_on->year&&a.provider_published_on->month==b.provider_published_on->month&&
          a.provider_published_on->day==b.provider_published_on->day))&&
        a.frequency==b.frequency&&a.period_status==b.period_status&&a.price_kind==b.price_kind&&a.value==b.value&&
        a.quality==b.quality&&a.quality_flags==b.quality_flags&&a.provider_record_id==b.provider_record_id&&a.source_uri==b.source_uri;
}
void upsert_observation_in_snapshot(MarketDataSnapshot& s,const MarketObservation& incoming) {
    auto it=std::find_if(s.observations.begin(),s.observations.end(),[&](const auto& x){return observation_key(x)==observation_key(incoming);});
    if(it==s.observations.end()){s.observations.push_back(incoming);return;}
    if(incoming.received_at.unix_seconds<it->received_at.unix_seconds)return;
    if(same_payload(*it,incoming)){it->received_at=incoming.received_at;return;}
    s.observation_history.push_back(MarketObservationRevision{*it,incoming.received_at,incoming.id});
    *it=incoming;
}
}

JsonMarketDataRepository::JsonMarketDataRepository(std::filesystem::path file_path,JsonRepositoryConfig config)
    :file_path_(std::move(file_path)),config_(config) {
    if(file_path_.empty())throw std::invalid_argument("Ścieżka magazynu JSON jest pusta.");
    if(config_.max_document_bytes==0U)throw std::invalid_argument("Limit rozmiaru dokumentu musi być dodatni.");
    if(file_path_.filename().empty())throw std::invalid_argument("Ścieżka magazynu musi wskazywać plik.");
    auto parent=file_path_.parent_path();if(parent.empty())parent=".";
    if(config_.create_parent_directories)std::filesystem::create_directories(parent);
    if(!std::filesystem::exists(parent)||!std::filesystem::is_directory(parent))
        throw std::invalid_argument("Katalog magazynu JSON nie istnieje lub nie jest katalogiem.");
}
std::filesystem::path JsonMarketDataRepository::backup_path() const {return file_path_.string()+".bak";}
bool JsonMarketDataRepository::exists() const {return std::filesystem::exists(file_path_);}
MarketDataSnapshot JsonMarketDataRepository::load_unlocked() const {
    if(!std::filesystem::exists(file_path_))return MarketDataSnapshot{};
    const auto raw=read_file(file_path_,config_.max_document_bytes,false);
    if(raw.empty())throw std::runtime_error("Dokument magazynu JSON jest pusty.");
    try{auto snapshot=decode_snapshot(json::parse(raw));verify_source_archives(snapshot,file_path_);return snapshot;}
    catch(const std::exception& e){throw std::runtime_error("Magazyn JSON jest uszkodzony lub niezgodny ze schematem albo archiwum źródła narusza integralność: "+std::string(e.what()));}
}
MarketDataSnapshot JsonMarketDataRepository::load() const {
    auto lock_path=file_path_.string()+".lock";FileLock lock(lock_path);return load_unlocked();
}
void JsonMarketDataRepository::write_unlocked(const MarketDataSnapshot& input,bool rotate_backup) const {
    MarketDataSnapshot s=input;
    validate_snapshot(s);
    verify_source_archives(s,file_path_);
    const bool primary_exists=std::filesystem::exists(file_path_);
    const auto existing=read_file(file_path_,config_.max_document_bytes,true);
    if(primary_exists&&existing.empty())throw std::runtime_error("Dokument magazynu JSON jest pusty.");
    const auto previous=existing.empty()?MarketDataSnapshot{}:decode_snapshot(json::parse(existing));
    if(previous.revision==std::numeric_limits<std::uint64_t>::max())throw std::overflow_error("Przekroczono licznik rewizji magazynu.");
    for(const auto& old_calendar:previous.session_calendars) {
        const auto found=std::find_if(s.session_calendars.begin(),s.session_calendars.end(),
            [&](const auto& candidate){return candidate.calendar_id==old_calendar.calendar_id&&candidate.version==old_calendar.version;});
        if(found==s.session_calendars.end()||!same_calendar(old_calendar,*found))
            throw std::invalid_argument("Nie można usunąć ani zmienić opublikowanej wersji kalendarza.");
    }
    for(const auto& candidate:s.session_calendars) {
        std::uint32_t previous_latest=0U;
        for(const auto& old:previous.session_calendars)
            if(old.calendar_id==candidate.calendar_id) previous_latest=std::max(previous_latest,old.version);
        const bool existed=std::any_of(previous.session_calendars.begin(),previous.session_calendars.end(),
            [&](const auto& old){return old.calendar_id==candidate.calendar_id&&old.version==candidate.version;});
        if(!existed&&previous_latest>0U&&candidate.version<=previous_latest)
            throw std::invalid_argument("Nowa wersja kalendarza musi być większa od poprzednich.");
    }
    for(const auto& old_object:previous.source_objects) {
        const auto found=std::find_if(s.source_objects.begin(),s.source_objects.end(),
            [&](const auto& candidate){return candidate.source_object_id==old_object.source_object_id;});
        if(found==s.source_objects.end() || encode(*found)!=encode(old_object))
            throw std::invalid_argument("Nie można usunąć ani zmienić niezmiennego obiektu źródłowego.");
    }
    for(const auto& old_notice:previous.venue_session_notices) {
        const auto found=std::find_if(s.venue_session_notices.begin(),s.venue_session_notices.end(),
            [&](const auto& candidate) {
                return venue_session_notice_key(candidate)==venue_session_notice_key(old_notice);
            });
        if(found==s.venue_session_notices.end() || !same_venue_session_notice(old_notice,*found))
            throw std::invalid_argument("Nie można usunąć ani zmienić opublikowanej rewizji komunikatu sesyjnego.");
    }
    for(const auto& candidate:s.venue_session_notices) {
        std::uint32_t previous_latest=0U;
        for(const auto& old:previous.venue_session_notices)
            if(old.notice_id==candidate.notice_id) previous_latest=std::max(previous_latest,old.revision);
        const bool existed=std::any_of(previous.venue_session_notices.begin(),previous.venue_session_notices.end(),
            [&](const auto& old){return venue_session_notice_key(old)==venue_session_notice_key(candidate);});
        if(!existed && previous_latest>0U && candidate.revision<=previous_latest)
            throw std::invalid_argument("Nowa rewizja komunikatu sesyjnego musi mieć większy numer.");
    }
    for (const auto& old_calendar : previous.venue_calendars) {
        const auto found=std::find_if(s.venue_calendars.begin(),s.venue_calendars.end(),
            [&](const auto& candidate) {
                return venue_calendar_key(candidate)==venue_calendar_key(old_calendar);
            });
        if (found==s.venue_calendars.end() || !same_venue_calendar(old_calendar,*found))
            throw std::invalid_argument("Nie można usunąć ani zmienić opublikowanej wersji kalendarza venue.");
    }
    for (const auto& candidate : s.venue_calendars) {
        std::uint32_t previous_latest=0U;
        for (const auto& old : previous.venue_calendars)
            if (effective_venue_calendar_id(old)==effective_venue_calendar_id(candidate))
                previous_latest=std::max(previous_latest,effective_venue_calendar_revision(old));
        const bool existed=std::any_of(previous.venue_calendars.begin(),previous.venue_calendars.end(),
            [&](const auto& old) {
                return venue_calendar_key(old)==venue_calendar_key(candidate);
            });
        if (!existed && previous_latest>0U && effective_venue_calendar_revision(candidate)<=previous_latest)
            throw std::invalid_argument("Nowa rewizja kalendarza venue musi być większa od poprzednich.");
    }
    if(!existing.empty()&&rotate_backup&&config_.backup_on_write) {
        const auto backup=backup_path();
        durable_write(backup,existing);
        auto parent=file_path_.parent_path();if(parent.empty())parent=".";sync_directory(parent);
    }
    s.revision=existing.empty()?1U:(previous.revision+1U);
    if(s.revision==0U)throw std::overflow_error("Przekroczono licznik rewizji magazynu.");
    const auto now=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    s.updated_at=UtcTimestamp{now};
    const auto raw=encode_snapshot(s).dump(2)+'\n';
    if(raw.size()>config_.max_document_bytes)throw std::runtime_error("Zapisany dokument przekracza skonfigurowany limit.");
    durable_write(file_path_,raw);
    auto parent=file_path_.parent_path();if(parent.empty())parent=".";sync_directory(parent);
}
std::uint64_t JsonMarketDataRepository::replace(const MarketDataSnapshot& snapshot) {
    FileLock lock(file_path_.string()+".lock");
    write_unlocked(snapshot,true);
    return load_unlocked().revision;
}
std::uint64_t JsonMarketDataRepository::upsert_batch(const MarketDataBatch& batch) {
    FileLock lock(file_path_.string()+".lock");
    auto snapshot=load_unlocked();
    std::set<std::string> incoming_instruments,incoming_providers;
    for(const auto& item:batch.instruments) {
        validate_instrument(item);
        if(!incoming_instruments.insert(item.id).second)
            throw std::invalid_argument("Powielony instrument w jednej partii zapisu.");
        const auto it=std::find_if(snapshot.instruments.begin(),snapshot.instruments.end(),
            [&](const auto& value){return value.id==item.id;});
        if(it==snapshot.instruments.end())snapshot.instruments.push_back(item);else *it=item;
    }
    for(const auto& item:batch.providers) {
        validate_provider(item);
        if(!incoming_providers.insert(item.id).second)
            throw std::invalid_argument("Powielony dostawca w jednej partii zapisu.");
        const auto it=std::find_if(snapshot.providers.begin(),snapshot.providers.end(),
            [&](const auto& value){return value.id==item.id;});
        if(it==snapshot.providers.end())snapshot.providers.push_back(item);else *it=item;
    }
    for(const auto& item:batch.observations) {
        const auto instrument=std::find_if(snapshot.instruments.begin(),snapshot.instruments.end(),
            [&](const auto& value){return value.id==item.instrument_id;});
        const auto provider=std::find_if(snapshot.providers.begin(),snapshot.providers.end(),
            [&](const auto& value){return value.id==item.provider_id;});
        if(instrument==snapshot.instruments.end()||provider==snapshot.providers.end())
            throw std::invalid_argument("Partia notowań wskazuje instrument lub dostawcę spoza magazynu.");
        validate_observation(*instrument,*provider,item);
        upsert_observation_in_snapshot(snapshot,item);
    }
    for(const auto& item:batch.series_metadata) {
        if(std::none_of(snapshot.instruments.begin(),snapshot.instruments.end(),
                [&](const auto& value){return value.id==item.instrument_id;})||
           std::none_of(snapshot.providers.begin(),snapshot.providers.end(),
                [&](const auto& value){return value.id==item.provider_id;}))
            throw std::invalid_argument("Metadane szeregu wymagają istniejącego instrumentu i dostawcy.");
        validate_series_metadata(item);
        const auto it=std::find_if(snapshot.series_metadata.begin(),snapshot.series_metadata.end(),
            [&](const auto& value){return value.instrument_id==item.instrument_id&&
                value.provider_id==item.provider_id&&value.frequency==item.frequency;});
        if(it==snapshot.series_metadata.end())snapshot.series_metadata.push_back(item);else *it=item;
    }
    for(const auto& item:batch.session_calendars) {
        validate_calendar(item);
        if(std::none_of(snapshot.instruments.begin(),snapshot.instruments.end(),[&](const auto& x){return x.id==item.instrument_id;})||
           std::none_of(snapshot.providers.begin(),snapshot.providers.end(),[&](const auto& x){return x.id==item.provider_id;}))
            throw std::invalid_argument("Kalendarz wymaga istniejącego instrumentu i dostawcy.");
        const auto found=std::find_if(snapshot.session_calendars.begin(),snapshot.session_calendars.end(),
            [&](const auto& x){return x.calendar_id==item.calendar_id&&x.version==item.version;});
        if(found!=snapshot.session_calendars.end()) {
            if(!same_calendar(*found,item)) throw std::invalid_argument("Opublikowana wersja kalendarza jest niezmienna.");
            continue;
        }
        std::uint32_t latest=0U;
        for(const auto& x:snapshot.session_calendars) if(x.calendar_id==item.calendar_id) latest=std::max(latest,x.version);
        if(item.version<=latest) throw std::invalid_argument("Nowa wersja kalendarza musi mieć większy numer.");
        snapshot.session_calendars.push_back(item);
    }
    std::set<std::string> incoming_source_objects;
    for(const auto& item:batch.source_objects) {
        validate_source_object(item);
        if(!incoming_source_objects.insert(item.source_object_id).second)
            throw std::invalid_argument("Powielony obiekt źródłowy w jednej partii.");
        const auto found=std::find_if(snapshot.source_objects.begin(),snapshot.source_objects.end(),
            [&](const auto& value){return value.source_object_id==item.source_object_id;});
        if(found!=snapshot.source_objects.end()) {
            if(encode(*found)!=encode(item))
                throw std::invalid_argument("Obiekt źródłowy o tym samym SHA-256 ma sprzeczne metadane.");
            continue;
        }
        snapshot.source_objects.push_back(item);
    }
    std::set<std::string> incoming_venue_calendars;
    for (const auto& item : batch.venue_calendars) {
        validate_venue_calendar(item);
        const auto key=venue_calendar_key(item);
        if (!incoming_venue_calendars.insert(key).second)
            throw std::invalid_argument("Powielona wersja kalendarza venue w jednej partii.");
        const auto found=std::find_if(snapshot.venue_calendars.begin(),snapshot.venue_calendars.end(),
            [&](const auto& value) { return venue_calendar_key(value)==key; });
        if (found!=snapshot.venue_calendars.end()) {
            if (!same_venue_calendar(*found,item))
                throw std::invalid_argument("Opublikowana wersja kalendarza venue jest niezmienna.");
            continue;
        }
        std::uint32_t latest=0U;
        for (const auto& value : snapshot.venue_calendars)
            if (effective_venue_calendar_id(value)==effective_venue_calendar_id(item))
                latest=std::max(latest,effective_venue_calendar_revision(value));
        if (effective_venue_calendar_revision(item)<=latest)
            throw std::invalid_argument("Nowa rewizja kalendarza venue musi mieć większy numer.");
        snapshot.venue_calendars.push_back(item);
    }
    std::set<std::string> incoming_venue_notices;
    for(const auto& item:batch.venue_session_notices) {
        validate_venue_session_notice(item);
        const auto key=venue_session_notice_key(item);
        if(!incoming_venue_notices.insert(key).second)
            throw std::invalid_argument("Powielona rewizja komunikatu sesyjnego w jednej partii.");
        const auto found=std::find_if(snapshot.venue_session_notices.begin(),snapshot.venue_session_notices.end(),
            [&](const auto& value){return venue_session_notice_key(value)==key;});
        if(found!=snapshot.venue_session_notices.end()) {
            if(!same_venue_session_notice(*found,item))
                throw std::invalid_argument("Opublikowana rewizja komunikatu sesyjnego jest niezmienna.");
            continue;
        }
        std::uint32_t latest=0U;
        for(const auto& value:snapshot.venue_session_notices)
            if(value.notice_id==item.notice_id) latest=std::max(latest,value.revision);
        if(item.revision<=latest)
            throw std::invalid_argument("Nowa rewizja komunikatu sesyjnego musi mieć większy numer.");
        snapshot.venue_session_notices.push_back(item);
    }
    validate_snapshot(snapshot);
    write_unlocked(snapshot,true);
    return load_unlocked().revision;
}
std::uint64_t JsonMarketDataRepository::upsert_instrument(const Instrument& item) {
    MarketDataBatch batch;batch.instruments.push_back(item);return upsert_batch(batch);
}
std::uint64_t JsonMarketDataRepository::upsert_provider(const Provider& item) {
    MarketDataBatch batch;batch.providers.push_back(item);return upsert_batch(batch);
}
std::uint64_t JsonMarketDataRepository::upsert_observation(const MarketObservation& item) {
    MarketDataBatch batch;batch.observations.push_back(item);return upsert_batch(batch);
}
std::uint64_t JsonMarketDataRepository::upsert_series_metadata(const SeriesMetadata& item) {
    MarketDataBatch batch;batch.series_metadata.push_back(item);return upsert_batch(batch);
}
std::uint64_t JsonMarketDataRepository::upsert_session_calendar(const MarketSessionCalendar& item) {
    MarketDataBatch batch;batch.session_calendars.push_back(item);return upsert_batch(batch);
}
std::uint64_t JsonMarketDataRepository::upsert_venue_calendar(const VenueSessionCalendar& item) {
    MarketDataBatch batch;batch.venue_calendars.push_back(item);return upsert_batch(batch);
}
std::uint64_t JsonMarketDataRepository::upsert_source_object(const SourceArchiveObject& item) {
    MarketDataBatch batch;batch.source_objects.push_back(item);return upsert_batch(batch);
}
std::uint64_t JsonMarketDataRepository::upsert_venue_session_notice(const VenueSessionNotice& item) {
    MarketDataBatch batch;batch.venue_session_notices.push_back(item);return upsert_batch(batch);
}
std::vector<SourceArchiveObject> find_orphaned_source_objects(
    const MarketDataSnapshot& snapshot) {
    std::set<std::string> referenced;
    for(const auto& calendar:snapshot.venue_calendars)
        if(calendar.source.source_object_id)
            referenced.insert(*calendar.source.source_object_id);
    for(const auto& notice:snapshot.venue_session_notices)
        referenced.insert(notice.source_object_id);
    std::vector<SourceArchiveObject> orphaned;
    for(const auto& object:snapshot.source_objects)
        if(!referenced.contains(object.source_object_id))
            orphaned.push_back(object);
    return orphaned;
}
EffectiveVenueCalendarDay resolve_effective_venue_calendar_day(
    const MarketDataSnapshot& snapshot,std::string_view calendar_id,CivilDate date) {
    validate_snapshot(snapshot);
    if(calendar_id.empty() || !is_valid_date(date))
        throw std::invalid_argument("Efektywny dzień kalendarza wymaga identyfikatora i poprawnej daty.");
    const VenueSessionCalendar* base_calendar=nullptr;
    std::uint32_t latest_calendar_revision=0U;
    for(const auto& calendar:snapshot.venue_calendars) {
        if(effective_venue_calendar_id(calendar)!=calendar_id) continue;
        const auto revision=effective_venue_calendar_revision(calendar);
        if(base_calendar==nullptr || revision>latest_calendar_revision) {
            base_calendar=&calendar;
            latest_calendar_revision=revision;
        }
    }
    if(base_calendar==nullptr)
        throw std::invalid_argument("Nie znaleziono kalendarza venue o wskazanym identyfikatorze.");
    using namespace std::chrono;
    const sys_days effective_date{year{date.year}/month{date.month}/day{date.day}};
    const sys_days first{year{base_calendar->source.valid_from.year}/month{base_calendar->source.valid_from.month}/day{base_calendar->source.valid_from.day}};
    const sys_days last{year{base_calendar->source.valid_to_exclusive.year}/month{base_calendar->source.valid_to_exclusive.month}/day{base_calendar->source.valid_to_exclusive.day}};
    if(effective_date<first || effective_date>=last)
        throw std::invalid_argument("Data wypada poza zakresem najnowszej rewizji kalendarza venue.");
    const auto base_day=std::find_if(base_calendar->days.begin(),base_calendar->days.end(),
        [&](const VenueCalendarDay& item){return format_date(item.date)==format_date(date);});
    if(base_day==base_calendar->days.end())
        throw std::invalid_argument("Najnowsza rewizja kalendarza nie zawiera wskazanego dnia.");

    EffectiveVenueCalendarDay result;
    result.day=*base_day;
    result.requires_official_notice_unresolved=base_day->requires_official_notice;
    std::map<std::string,const VenueSessionNotice*> latest_notices;
    for(const auto& notice:snapshot.venue_session_notices) {
        if(notice.calendar_id!=calendar_id || format_date(notice.effective_date)!=format_date(date))
            continue;
        const auto found=latest_notices.find(notice.notice_id);
        if(found==latest_notices.end() || notice.revision>found->second->revision)
            latest_notices[notice.notice_id]=&notice;
    }
    std::set<std::string> cancelled_revisions;
    for(const auto& [id,notice]:latest_notices) {
        (void)id;
        if(notice->kind!=VenueSessionNoticeKind::Cancellation) continue;
        const auto target=latest_notices.find(notice->cancels_notice_id);
        if(target!=latest_notices.end() && target->second->revision==notice->cancels_revision) {
            const auto target_key=notice->cancels_notice_id+"\x1f"+std::to_string(notice->cancels_revision);
            if(!cancelled_revisions.insert(target_key).second)
                throw std::invalid_argument("Kilka aktywnych komunikatów anuluje tę samą rewizję.");
        }
        result.applied_notice_ids.push_back(notice->notice_id);
    }
    const VenueSessionNotice* active_override=nullptr;
    for(const auto& [id,notice]:latest_notices) {
        if(notice->kind==VenueSessionNoticeKind::Cancellation) continue;
        const auto key=id+"\x1f"+std::to_string(notice->revision);
        if(cancelled_revisions.contains(key)) continue;
        result.applied_notice_ids.push_back(id);
        if(notice->kind==VenueSessionNoticeKind::SessionOverride) {
            if(active_override!=nullptr)
                throw std::invalid_argument("Konflikt: kilka aktywnych nadpisań sesji dotyczy tego samego dnia.");
            active_override=notice;
        }
    }
    if(active_override!=nullptr) {
        if(!active_override->day_status)
            throw std::invalid_argument("Aktywne nadpisanie sesji nie ma statusu dnia.");
        result.day.status=*active_override->day_status;
        result.day.product_sessions=active_override->product_sessions;
        result.day.requires_official_notice=false;
        result.requires_official_notice_unresolved=false;
    }
    std::sort(result.applied_notice_ids.begin(),result.applied_notice_ids.end());
    return result;
}
std::vector<CivilDate> find_unresolved_venue_notice_dates(
    const MarketDataSnapshot& snapshot,std::string_view calendar_id) {
    validate_snapshot(snapshot);
    const VenueSessionCalendar* latest_calendar=nullptr;
    std::uint32_t latest_revision=0U;
    for(const auto& calendar:snapshot.venue_calendars) {
        if(effective_venue_calendar_id(calendar)!=calendar_id) continue;
        const auto revision=effective_venue_calendar_revision(calendar);
        if(latest_calendar==nullptr || revision>latest_revision) {
            latest_calendar=&calendar;
            latest_revision=revision;
        }
    }
    if(latest_calendar==nullptr)
        throw std::invalid_argument("Nie znaleziono kalendarza venue o wskazanym identyfikatorze.");
    std::vector<CivilDate> unresolved;
    for(const auto& day:latest_calendar->days) {
        if(!day.requires_official_notice) continue;
        const auto effective=resolve_effective_venue_calendar_day(snapshot,calendar_id,day.date);
        if(effective.requires_official_notice_unresolved) unresolved.push_back(day.date);
    }
    return unresolved;
}
MarketDataSnapshot JsonMarketDataRepository::restore_backup() {
    FileLock lock(file_path_.string()+".lock");
    const auto raw=read_file(backup_path(),config_.max_document_bytes,false);
    auto recovered=decode_snapshot(json::parse(raw));
    if(recovered.revision==std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("Przekroczono licznik rewizji podczas odtwarzania kopii.");
    ++recovered.revision;
    const auto now=std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    recovered.updated_at=UtcTimestamp{now};
    const auto restored=encode_snapshot(recovered).dump(2)+"\n";
    if(restored.size()>config_.max_document_bytes)
        throw std::runtime_error("Odtworzony dokument przekracza skonfigurowany limit.");
    durable_write(file_path_,restored);
    auto parent=file_path_.parent_path();if(parent.empty())parent=".";sync_directory(parent);
    return load_unlocked();
}
} // namespace monitor::market
