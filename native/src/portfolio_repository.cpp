#include "monitor/portfolio_repository.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <fcntl.h>
#include <fstream>
#include <map>
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace monitor::market {
namespace {
using json = nlohmann::json;

class FileLock {
public:
    FileLock(const std::filesystem::path& path, int mode) {
        fd_ = ::open(path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
        if (fd_ < 0 || ::flock(fd_, mode) != 0) {
            if (fd_ >= 0) ::close(fd_);
            throw std::runtime_error("Nie można zablokować repozytorium portfela.");
        }
    }
    ~FileLock() { if (fd_ >= 0) { ::flock(fd_, LOCK_UN); ::close(fd_); } }
    FileLock(const FileLock&) = delete;
    FileLock& operator=(const FileLock&) = delete;
private:
    int fd_{-1};
};

std::string side_name(TradeSide side) {
    if (side == TradeSide::Buy) return "buy";
    if (side == TradeSide::Sell) return "sell";
    throw std::invalid_argument("Nieznana strona transakcji.");
}
TradeSide parse_side(const std::string& value) {
    if (value == "buy") return TradeSide::Buy;
    if (value == "sell") return TradeSide::Sell;
    throw std::invalid_argument("Nieznana strona transakcji w JSON.");
}
std::string kind_name(PortfolioAssetKind kind) {
    if (kind == PortfolioAssetKind::Currency) return "currency";
    if (kind == PortfolioAssetKind::Gold) return "gold";
    throw std::invalid_argument("Nieznany rodzaj aktywa.");
}
PortfolioAssetKind parse_kind(const std::string& value) {
    if (value == "currency") return PortfolioAssetKind::Currency;
    if (value == "gold") return PortfolioAssetKind::Gold;
    throw std::invalid_argument("Nieznany rodzaj aktywa w JSON.");
}
json encode_audit(const FxExecutionAudit& a) {
    return {{"base_currency",a.base_currency},{"quote_currency",a.quote_currency},
        {"bid",a.bid},{"ask",a.ask},{"mid",a.mid},
        {"spread_basis_points",a.spread_basis_points},
        {"gross_execution_rate",a.gross_execution_rate},
        {"effective_execution_rate",a.effective_execution_rate},
        {"fee_percent",a.fee_percent},{"fixed_fee_output",a.fixed_fee_output},
        {"fee_currency",a.fee_currency},{"provider_id",a.provider_id},
        {"observation_id",a.observation_id},{"source_uri",a.source_uri},
        {"observed_at_unix",a.observed_at_unix}};
}
FxExecutionAudit decode_audit(const json& j) {
    return {j.at("base_currency").get<std::string>(),
        j.at("quote_currency").get<std::string>(),j.at("bid").get<double>(),
        j.at("ask").get<double>(),j.at("mid").get<double>(),
        j.at("spread_basis_points").get<double>(),
        j.at("gross_execution_rate").get<double>(),
        j.at("effective_execution_rate").get<double>(),
        j.at("fee_percent").get<double>(),j.at("fixed_fee_output").get<double>(),
        j.at("fee_currency").get<std::string>(),j.at("provider_id").get<std::string>(),
        j.at("observation_id").get<std::string>(),j.at("source_uri").get<std::string>(),
        j.at("observed_at_unix").get<std::int64_t>()};
}
json encode(const PortfolioTransaction& tx) {
    json result={{"id",tx.id},{"asset_code",tx.asset_code},{"asset_kind",kind_name(tx.asset_kind)},
        {"side",side_name(tx.side)},{"quantity",tx.quantity},{"purity",tx.purity},
        {"unit_price_reporting",tx.unit_price_reporting},{"fee_reporting",tx.fee_reporting},
        {"executed_at_unix",tx.executed_at_unix},{"source_id",tx.source_id},
        {"operation_id",tx.operation_id}};
    if(tx.fx_audit) result["fx_audit"]=encode_audit(*tx.fx_audit);
    return result;
}
PortfolioTransaction decode(const json& j) {
    PortfolioTransaction tx{j.at("id").get<std::string>(),j.at("asset_code").get<std::string>(),
        parse_kind(j.at("asset_kind").get<std::string>()),parse_side(j.at("side").get<std::string>()),
        j.at("quantity").get<double>(),j.at("purity").get<double>(),
        j.at("unit_price_reporting").get<double>(),j.at("fee_reporting").get<double>(),
        j.at("executed_at_unix").get<std::int64_t>(),j.at("source_id").get<std::string>(),
        j.value("operation_id",std::string{})};
    if(j.contains("fx_audit")&&!j.at("fx_audit").is_null())
        tx.fx_audit=decode_audit(j.at("fx_audit"));
    return tx;
}
bool valid_currency(const std::string& code) {
    if (code.size()!=3U || code=="XAU") return false;
    for (const char c:code) if(c<'A'||c>'Z') return false;
    return true;
}
void validate_snapshot(const PortfolioSnapshot& snapshot) {
    if (snapshot.schema_version != portfolio_json_schema_version ||
        !valid_currency(snapshot.reporting_currency))
        throw std::invalid_argument("Nieprawidłowa wersja lub waluta dokumentu portfela.");
    std::map<std::string,double> latest_prices;
    for (const auto& tx : snapshot.transactions) latest_prices[tx.asset_code]=tx.unit_price_reporting;
    std::vector<std::pair<std::string,double>> prices(latest_prices.begin(),latest_prices.end());
    (void)calculate_portfolio(snapshot.reporting_currency,snapshot.transactions,prices);
}
json encode_snapshot(const PortfolioSnapshot& snapshot) {
    json transactions=json::array();
    for(const auto& tx:snapshot.transactions) transactions.push_back(encode(tx));
    return {{"document_type","monitor.portfolio"},
        {"schema_version",snapshot.schema_version},{"revision",snapshot.revision},
        {"reporting_currency",snapshot.reporting_currency},{"transactions",std::move(transactions)}};
}
PortfolioSnapshot decode_snapshot(const json& document) {
    if(document.at("document_type").get<std::string>()!="monitor.portfolio")
        throw std::runtime_error("Nieprawidłowy typ dokumentu portfela.");
    PortfolioSnapshot snapshot;
    snapshot.schema_version=document.at("schema_version").get<std::uint32_t>();
    snapshot.revision=document.at("revision").get<std::uint64_t>();
    snapshot.reporting_currency=document.at("reporting_currency").get<std::string>();
    for(const auto& item:document.at("transactions")) snapshot.transactions.push_back(decode(item));
    validate_snapshot(snapshot);
    return snapshot;
}
}

JsonPortfolioRepository::JsonPortfolioRepository(std::filesystem::path file_path,
    std::string reporting_currency, PortfolioRepositoryConfig config)
    : file_path_(std::move(file_path)),
      initial_reporting_currency_(std::move(reporting_currency)), config_(config) {
    if(file_path_.empty() || !valid_currency(initial_reporting_currency_) ||
       config_.max_document_bytes==0U)
        throw std::invalid_argument("Nieprawidłowa konfiguracja repozytorium portfela.");
}
std::filesystem::path JsonPortfolioRepository::backup_path() const {
    return std::filesystem::path(file_path_.string()+".bak");
}
bool JsonPortfolioRepository::exists() const { return std::filesystem::exists(file_path_); }

PortfolioSnapshot JsonPortfolioRepository::load_unlocked() const {
    if(!std::filesystem::exists(file_path_)) {
        PortfolioSnapshot empty;
        empty.reporting_currency=initial_reporting_currency_;
        return empty;
    }
    const auto size=std::filesystem::file_size(file_path_);
    if(size==0U || size>config_.max_document_bytes)
        throw std::runtime_error("Dokument portfela jest pusty lub przekracza limit rozmiaru.");
    std::ifstream input(file_path_,std::ios::binary);
    if(!input) throw std::runtime_error("Nie można otworzyć dokumentu portfela.");
    json document;
    try { input>>document; }
    catch(const std::exception& e) {
        throw std::runtime_error(std::string("Nie można odczytać JSON portfela: ")+e.what());
    }
    auto snapshot=decode_snapshot(document);
    if(snapshot.reporting_currency!=initial_reporting_currency_)
        throw std::runtime_error("Waluta raportowania dokumentu różni się od konfiguracji repozytorium.");
    return snapshot;
}
PortfolioSnapshot JsonPortfolioRepository::load() const {
    auto parent=file_path_.parent_path();
    if(parent.empty()) parent=".";
    if(config_.create_parent_directories) std::filesystem::create_directories(parent);
    FileLock lock(file_path_.string()+".lock",LOCK_SH);
    return load_unlocked();
}

void JsonPortfolioRepository::write_unlocked(const PortfolioSnapshot& snapshot,bool rotate_backup) const {
    validate_snapshot(snapshot);
    auto parent=file_path_.parent_path();
    if(parent.empty()) parent=".";
    if(config_.create_parent_directories) std::filesystem::create_directories(parent);
    const auto bytes=encode_snapshot(snapshot).dump(2)+"\n";
    if(bytes.size()>config_.max_document_bytes)
        throw std::runtime_error("Zapis portfela przekracza limit rozmiaru.");
    const auto temp=std::filesystem::path(file_path_.string()+".tmp."+std::to_string(::getpid()));
    const int fd=::open(temp.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);
    if(fd<0) throw std::runtime_error("Nie można utworzyć pliku tymczasowego portfela.");
    std::size_t offset{};
    while(offset<bytes.size()) {
        const auto count=::write(fd,bytes.data()+offset,bytes.size()-offset);
        if(count<0) { if(errno==EINTR) continue; ::close(fd); ::unlink(temp.c_str()); throw std::runtime_error("Błąd zapisu portfela."); }
        if(count==0) { ::close(fd); ::unlink(temp.c_str()); throw std::runtime_error("Niepełny zapis dokumentu portfela."); }
        offset+=static_cast<std::size_t>(count);
    }
    const int sync_result=::fsync(fd);
    const int close_result=::close(fd);
    if(sync_result!=0 || close_result!=0) {
        ::unlink(temp.c_str());
        throw std::runtime_error("Nie można utrwalić pliku portfela.");
    }
    if(rotate_backup && config_.backup_on_write && std::filesystem::exists(file_path_))
        std::filesystem::copy_file(file_path_,backup_path(),std::filesystem::copy_options::overwrite_existing);
    if(::rename(temp.c_str(),file_path_.c_str())!=0) {
        ::unlink(temp.c_str());
        throw std::runtime_error("Nie można atomowo podmienić dokumentu portfela.");
    }
    const int dirfd=::open(parent.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC);
    if(dirfd>=0) { (void)::fsync(dirfd); ::close(dirfd); }
}

std::uint64_t JsonPortfolioRepository::replace(
    const PortfolioSnapshot& snapshot,std::uint64_t expected_revision) {
    auto parent=file_path_.parent_path();
    if(parent.empty()) parent=".";
    if(config_.create_parent_directories) std::filesystem::create_directories(parent);
    FileLock lock(file_path_.string()+".lock",LOCK_EX);
    auto current=load_unlocked();
    if(current.revision!=expected_revision)
        throw std::runtime_error("Konflikt rewizji repozytorium portfela.");
    auto next=snapshot;
    next.schema_version=portfolio_json_schema_version;
    next.reporting_currency=current.reporting_currency;
    next.revision=current.revision+1U;
    validate_snapshot(next);
    write_unlocked(next);
    return next.revision;
}

std::uint64_t JsonPortfolioRepository::append(
    const PortfolioTransaction& transaction,std::uint64_t expected_revision) {
    return append_batch({transaction},expected_revision);
}

std::uint64_t JsonPortfolioRepository::append_batch(
    const std::vector<PortfolioTransaction>& transactions,std::uint64_t expected_revision) {
    if(transactions.empty())
        throw std::invalid_argument("Partia transakcji portfela nie może być pusta.");
    auto parent=file_path_.parent_path();
    if(parent.empty()) parent=".";
    if(config_.create_parent_directories) std::filesystem::create_directories(parent);
    FileLock lock(file_path_.string()+".lock",LOCK_EX);
    auto next=load_unlocked();
    if(next.revision!=expected_revision)
        throw std::runtime_error("Konflikt rewizji repozytorium portfela.");
    next.transactions.insert(next.transactions.end(),transactions.begin(),transactions.end());
    next.revision+=1U;
    validate_snapshot(next);
    write_unlocked(next);
    return next.revision;
}

PortfolioSnapshot JsonPortfolioRepository::restore_backup() {
    auto parent=file_path_.parent_path();
    if(parent.empty()) parent=".";
    if(config_.create_parent_directories) std::filesystem::create_directories(parent);
    FileLock lock(file_path_.string()+".lock",LOCK_EX);
    const auto backup=backup_path();
    if(!std::filesystem::exists(backup))
        throw std::runtime_error("Nie istnieje kopia zapasowa portfela.");
    const auto size=std::filesystem::file_size(backup);
    if(size==0U||size>config_.max_document_bytes)
        throw std::runtime_error("Kopia zapasowa portfela jest pusta lub przekracza limit.");
    std::ifstream input(backup,std::ios::binary);
    if(!input) throw std::runtime_error("Nie można otworzyć kopii zapasowej portfela.");
    json document;
    try { input>>document; }
    catch(const std::exception& e) {
        throw std::runtime_error(std::string("Nie można odczytać kopii portfela: ")+e.what());
    }
    auto restored=decode_snapshot(document);
    if(restored.reporting_currency!=initial_reporting_currency_)
        throw std::runtime_error("Waluta raportowania kopii różni się od konfiguracji repozytorium.");
    std::uint64_t current_revision=restored.revision;
    try { current_revision=std::max(current_revision,load_unlocked().revision); }
    catch(const std::exception&) { /* Corrupt primary: backup revision is the recovery base. */ }
    if(current_revision==std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("Wyczerpano zakres rewizji portfela.");
    restored.revision=current_revision+1U;
    write_unlocked(restored,false);
    return restored;
}

} // namespace monitor::market
