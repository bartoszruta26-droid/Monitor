#include "monitor/market_data_repository.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>
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
    if(s=="currency")return AssetClass::Currency;if(s=="precious_metal")return AssetClass::PreciousMetal;if(s=="commodity")return AssetClass::Commodity;
    throw std::invalid_argument("Nieznana klasa aktywa w JSON.");
}
template<> Unit parse_enum(const std::string& s) {
    if(s=="currency_unit")return Unit::CurrencyUnit;if(s=="gram")return Unit::Gram;if(s=="kilogram")return Unit::Kilogram;if(s=="troy_ounce")return Unit::TroyOunce;if(s=="metric_ton")return Unit::MetricTon;if(s=="barrel")return Unit::Barrel;if(s=="megawatt_hour")return Unit::MegawattHour;
    throw std::invalid_argument("Nieznana jednostka w JSON.");
}
template<> PriceKind parse_enum(const std::string& s) {
    if(s=="reference")return PriceKind::Reference;if(s=="average")return PriceKind::Average;if(s=="bid")return PriceKind::Bid;if(s=="ask")return PriceKind::Ask;if(s=="mid")return PriceKind::Mid;if(s=="spot")return PriceKind::Spot;if(s=="transaction")return PriceKind::Transaction;
    throw std::invalid_argument("Nieznany rodzaj ceny w JSON.");
}
template<> Frequency parse_enum(const std::string& s) {
    if(s=="tick")return Frequency::Tick;if(s=="minute")return Frequency::Minute;if(s=="hour")return Frequency::Hour;if(s=="daily")return Frequency::Daily;if(s=="weekly")return Frequency::Weekly;if(s=="monthly")return Frequency::Monthly;if(s=="quarterly")return Frequency::Quarterly;if(s=="semi_annual")return Frequency::SemiAnnual;if(s=="annual")return Frequency::Annual;if(s=="irregular")return Frequency::Irregular;
    throw std::invalid_argument("Nieznana częstotliwość w JSON.");
}
template<> PeriodStatus parse_enum(const std::string& s) {
    if(s=="complete")return PeriodStatus::Complete;if(s=="partial")return PeriodStatus::Partial;if(s=="provisional")return PeriodStatus::Provisional;
    throw std::invalid_argument("Nieznany status okresu w JSON.");
}
template<> QualityStatus parse_enum(const std::string& s) {
    if(s=="unreviewed")return QualityStatus::Unreviewed;if(s=="validated")return QualityStatus::Validated;if(s=="suspect")return QualityStatus::Suspect;
    throw std::invalid_argument("Nieznany status jakości w JSON.");
}
template<> SeriesCompleteness parse_enum(const std::string& s) {
    if(s=="complete")return SeriesCompleteness::Complete;if(s=="partial")return SeriesCompleteness::Partial;if(s=="sparse")return SeriesCompleteness::Sparse;if(s=="unknown")return SeriesCompleteness::Unknown;
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
std::string observation_key(const MarketObservation& x) {
    return x.instrument_id+"\x1f"+x.provider_id+"\x1f"+std::to_string(x.observed_at.unix_seconds)+"\x1f"+
        enum_name(x.price_kind)+"\x1f"+enum_name(x.frequency);
}
void validate_snapshot(const MarketDataSnapshot& s) {
    if(s.schema_version!=json_document_schema_version)throw std::invalid_argument("Nieobsługiwana wersja dokumentu magazynu JSON.");
    std::map<std::string,const Instrument*> instruments;
    std::map<std::string,const Provider*> providers;
    for(const auto& x:s.instruments){validate_instrument(x);if(!instruments.emplace(x.id,&x).second)throw std::invalid_argument("Powielony identyfikator instrumentu: "+x.id);}
    for(const auto& x:s.providers){validate_provider(x);if(!providers.emplace(x.id,&x).second)throw std::invalid_argument("Powielony identyfikator dostawcy: "+x.id);}
    std::set<std::string> keys,ids;
    for(const auto& x:s.observations) {
        const auto i=instruments.find(x.instrument_id),p=providers.find(x.provider_id);
        if(i==instruments.end()||p==providers.end())throw std::invalid_argument("Notowanie wskazuje nieznany instrument lub dostawcę.");
        validate_observation(*i->second,*p->second,x);
        if(!keys.insert(observation_key(x)).second)throw std::invalid_argument("Powielony klucz notowania w migawce.");
        if(!ids.insert(x.id).second)throw std::invalid_argument("Powielony identyfikator notowania.");
    }
    std::set<std::string> series_keys;
    for(const auto& x:s.series_metadata) {
        if(!instruments.contains(x.instrument_id)||!providers.contains(x.provider_id))throw std::invalid_argument("Metadane szeregu wskazują nieznany instrument lub dostawcę.");
        validate_series_metadata(x);
        const auto key=x.instrument_id+"\x1f"+x.provider_id+"\x1f"+enum_name(x.frequency);
        if(!series_keys.insert(key).second)throw std::invalid_argument("Powielone metadane szeregu.");
    }
    for(const auto& x:s.observation_history) {
        const auto i=instruments.find(x.previous.instrument_id),p=providers.find(x.previous.provider_id);
        if(i==instruments.end()||p==providers.end())throw std::invalid_argument("Historia notowania wskazuje nieznany instrument lub dostawcę.");
        validate_observation(*i->second,*p->second,x.previous);
        if(x.superseded_by_id.empty()||x.superseded_at.unix_seconds<x.previous.received_at.unix_seconds)
            throw std::invalid_argument("Nieprawidłowa historia korekty notowania.");
    }
}
json encode_snapshot(const MarketDataSnapshot& s) {
    json instruments=json::array(),providers=json::array(),observations=json::array(),series=json::array(),history=json::array();
    for(const auto& x:s.instruments)instruments.push_back(encode(x));
    for(const auto& x:s.providers)providers.push_back(encode(x));
    for(const auto& x:s.observations)observations.push_back(encode(x));
    for(const auto& x:s.series_metadata)series.push_back(encode(x));
    for(const auto& x:s.observation_history)history.push_back(encode(x));
    return {{"document_type","monitor.market_data"},{"schema_version",s.schema_version},{"revision",s.revision},
        {"updated_at",format_utc_timestamp(s.updated_at)},{"instruments",std::move(instruments)},
        {"providers",std::move(providers)},{"observations",std::move(observations)},
        {"series_metadata",std::move(series)},{"observation_history",std::move(history)}};
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
        if(count>limit-data.size()){::close(fd);throw std::runtime_error("Plik magazynu JSON przekracza skonfigurowany limit.");}
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
    try {
        write_all(fd,data);
        if(::fsync(fd)!=0)throw std::runtime_error("Nie można zsynchronizować pliku: "+std::string(std::strerror(errno)));
        if(::close(fd)!=0)throw std::runtime_error("Nie można zamknąć pliku tymczasowego.");
        if(::rename(temp.c_str(),path.c_str())!=0)throw std::runtime_error("Nie można atomowo podmienić pliku: "+std::string(std::strerror(errno)));
    } catch(...) {::close(fd);::unlink(temp.c_str());throw;}
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
    try{return decode_snapshot(json::parse(raw));}
    catch(const std::exception& e){throw std::runtime_error("Magazyn JSON jest uszkodzony lub niezgodny ze schematem: "+std::string(e.what()));}
}
MarketDataSnapshot JsonMarketDataRepository::load() const {
    auto lock_path=file_path_.string()+".lock";FileLock lock(lock_path);return load_unlocked();
}
void JsonMarketDataRepository::write_unlocked(const MarketDataSnapshot& input,bool rotate_backup) const {
    MarketDataSnapshot s=input;
    validate_snapshot(s);
    const auto existing=read_file(file_path_,config_.max_document_bytes,true);
    const auto previous=existing.empty()?MarketDataSnapshot{}:decode_snapshot(json::parse(existing));
    if(previous.revision==std::numeric_limits<std::uint64_t>::max())throw std::overflow_error("Przekroczono licznik rewizji magazynu.");
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
    FileLock lock(file_path_.string()+".lock");write_unlocked(snapshot,true);return load_unlocked().revision;
}
std::uint64_t JsonMarketDataRepository::upsert_instrument(const Instrument& x) {
    validate_instrument(x);FileLock lock(file_path_.string()+".lock");auto s=load_unlocked();
    auto it=std::find_if(s.instruments.begin(),s.instruments.end(),[&](const auto& v){return v.id==x.id;});
    if(it==s.instruments.end())s.instruments.push_back(x);else *it=x;
    write_unlocked(s,true);return load_unlocked().revision;
}
std::uint64_t JsonMarketDataRepository::upsert_provider(const Provider& x) {
    validate_provider(x);FileLock lock(file_path_.string()+".lock");auto s=load_unlocked();
    auto it=std::find_if(s.providers.begin(),s.providers.end(),[&](const auto& v){return v.id==x.id;});
    if(it==s.providers.end())s.providers.push_back(x);else *it=x;
    write_unlocked(s,true);return load_unlocked().revision;
}
std::uint64_t JsonMarketDataRepository::upsert_observation(const MarketObservation& x) {
    FileLock lock(file_path_.string()+".lock");auto s=load_unlocked();
    const auto i=std::find_if(s.instruments.begin(),s.instruments.end(),[&](const auto& v){return v.id==x.instrument_id;});
    const auto p=std::find_if(s.providers.begin(),s.providers.end(),[&](const auto& v){return v.id==x.provider_id;});
    if(i==s.instruments.end()||p==s.providers.end())throw std::invalid_argument("Przed zapisem notowania należy zapisać instrument i dostawcę.");
    validate_observation(*i,*p,x);upsert_observation_in_snapshot(s,x);
    write_unlocked(s,true);return load_unlocked().revision;
}
std::uint64_t JsonMarketDataRepository::upsert_series_metadata(const SeriesMetadata& x) {
    FileLock lock(file_path_.string()+".lock");auto s=load_unlocked();
    if(std::none_of(s.instruments.begin(),s.instruments.end(),[&](const auto& v){return v.id==x.instrument_id;})||
       std::none_of(s.providers.begin(),s.providers.end(),[&](const auto& v){return v.id==x.provider_id;}))
        throw std::invalid_argument("Metadane szeregu wymagają istniejącego instrumentu i dostawcy.");
    validate_series_metadata(x);
    const auto it=std::find_if(s.series_metadata.begin(),s.series_metadata.end(),[&](const auto& v){
        return v.instrument_id==x.instrument_id&&v.provider_id==x.provider_id&&v.frequency==x.frequency;});
    if(it==s.series_metadata.end())s.series_metadata.push_back(x);else *it=x;
    write_unlocked(s,true);return load_unlocked().revision;
}
MarketDataSnapshot JsonMarketDataRepository::restore_backup() {
    FileLock lock(file_path_.string()+".lock");
    const auto raw=read_file(backup_path(),config_.max_document_bytes,false);
    const auto recovered=decode_snapshot(json::parse(raw));
    (void)recovered;
    durable_write(file_path_,raw);
    auto parent=file_path_.parent_path();if(parent.empty())parent=".";sync_directory(parent);
    return load_unlocked();
}
} // namespace monitor::market
