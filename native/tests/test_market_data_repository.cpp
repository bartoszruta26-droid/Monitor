#include "monitor/market_data_repository.hpp"

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
    x.observed_at=parse_utc_timestamp("2026-11-12T12:00:00Z");x.received_at=UtcTimestamp{received};
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
    require(loaded.observations.front().received_at.unix_seconds==2100,"receipt timestamp refreshed");

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
    auto batch_second=observation(4.21,3001);
    batch_second.id="batch-2";
    batch_second.observed_at.unix_seconds+=86400;
    batch.observations={batch_first,batch_second};
    batch.series_metadata.push_back(SeriesMetadata{fx.id,nbp.id,Frequency::Daily,
        batch_first.observed_at,batch_second.observed_at,2,2,SeriesCompleteness::Complete});
    require(batch_repo.upsert_batch(batch)==1,"provider batch is committed as one revision");
    auto batch_loaded=batch_repo.load();
    require(batch_loaded.instruments.size()==1&&batch_loaded.providers.size()==1&&
            batch_loaded.observations.size()==2&&batch_loaded.series_metadata.size()==1,
            "atomic batch persists all adapter result collections");
    require(batch_repo.upsert_batch(batch)==2,"replaying a batch is safe and revisioned");
    batch_loaded=batch_repo.load();
    require(batch_loaded.observations.size()==2&&batch_loaded.observation_history.empty(),
            "replayed identical batch does not duplicate observations");
    MarketDataBatch invalid_batch;
    auto orphan=observation(4.5,4000);
    orphan.provider_id="missing-provider";
    invalid_batch.observations.push_back(orphan);
    throws([&]{(void)batch_repo.upsert_batch(invalid_batch);},"invalid batch rejected as a whole");
    require(batch_repo.load().revision==2,"failed batch leaves committed revision unchanged");

    const auto raw_before=std::filesystem::file_size(file);
    throws([&]{auto bad=loaded;bad.instruments.front().quote_currency="PL";(void)repo.replace(bad);},"invalid snapshot rejected");
    require(std::filesystem::file_size(file)==raw_before,"failed validation leaves primary file unchanged");

    // Explicit backup recovery restores the previous valid revision.
    {std::ofstream out(file,std::ios::binary|std::ios::trunc);out<<"{broken";}
    throws([&]{(void)repo.load();},"corrupt primary is not silently hidden");
    auto recovered=repo.restore_backup();
    require(recovered.revision==4,"backup restores previous committed revision");
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

    std::filesystem::remove_all(dir);
    std::cout<<"All JSON repository tests passed.\n";
}
