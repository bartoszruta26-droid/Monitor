#include "monitor/market_data_adapters.hpp"

#include <archive.h>
#include <archive_entry.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace monitor::market;
void require(bool ok,const char* message) {
    if(!ok) { std::cerr<<"FAIL: "<<message<<'\n';std::exit(EXIT_FAILURE); }
}
template<class F> void throws(F&& f,const char* message) {
    try { f(); } catch(const std::exception&) { return; }
    std::cerr<<"FAIL: expected exception: "<<message<<'\n';std::exit(EXIT_FAILURE);
}
class FakeTransport final:public HttpTransport {
public:
    std::map<std::string,std::vector<HttpResponse>> responses;
    std::vector<std::string> calls;
    HttpResponse get(const HttpRequest& request) override {
        calls.push_back(request.url);
        auto it=responses.find(request.url);
        if(it==responses.end()||it->second.empty()) throw std::runtime_error("Unexpected mock URL: "+request.url);
        auto response=it->second.front();
        if(it->second.size()>1) it->second.erase(it->second.begin());
        return response;
    }
};
HttpResponse response(long status,std::string body) { return HttpResponse{status,{},std::move(body)}; }
std::string make_xlsx() {
    std::vector<char> buffer(1024U*1024U);
    std::size_t used=0;
    archive* writer=archive_write_new();
    archive_write_set_format_zip(writer);
    if(archive_write_open_memory(writer,buffer.data(),buffer.size(),&used)!=ARCHIVE_OK)
        throw std::runtime_error("Cannot create XLSX test archive");
    const std::map<std::string,std::string> files{
      {"xl/workbook.xml",R"(<?xml version="1.0"?><workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"><sheets><sheet name="Monthly Prices" sheetId="1" r:id="rId1"/></sheets></workbook>)"},
      {"xl/_rels/workbook.xml.rels",R"(<?xml version="1.0"?><Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet1.xml"/></Relationships>)"},
      {"xl/sharedStrings.xml",R"(<?xml version="1.0"?><sst xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main"><si><t>Period</t></si><si><t>Gold</t></si><si><t>Silver</t></si><si><t>Unit</t></si><si><t>USD/troy oz</t></si><si><t>2026M08</t></si></sst>)"},
      {"xl/worksheets/sheet1.xml",R"(<?xml version="1.0"?><worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main"><sheetData><row r="1"><c r="A1" t="s"><v>0</v></c><c r="B1" t="s"><v>1</v></c><c r="C1" t="s"><v>2</v></c></row><row r="2"><c r="A2" t="s"><v>3</v></c><c r="B2" t="s"><v>4</v></c><c r="C2" t="s"><v>4</v></c></row><row r="3"><c r="A3" t="s"><v>5</v></c><c r="B3"><v>3400.5</v></c><c r="C3"><v>39.25</v></c></row></sheetData></worksheet>)"}
    };
    for(const auto& [name,content]:files) {
        archive_entry* entry=archive_entry_new();
        archive_entry_set_pathname(entry,name.c_str());
        archive_entry_set_filetype(entry,AE_IFREG);
        archive_entry_set_perm(entry,0644);
        archive_entry_set_size(entry,static_cast<la_int64_t>(content.size()));
        if(archive_write_header(writer,entry)!=ARCHIVE_OK) throw std::runtime_error("XLSX test entry header failed");
        if(archive_write_data(writer,content.data(),content.size())<0) throw std::runtime_error("XLSX test entry write failed");
        archive_entry_free(entry);
    }
    archive_write_close(writer);
    archive_write_free(writer);
    return std::string(buffer.data(),used);
}
}

int main() {
    using namespace monitor::market;
    auto transport=std::make_shared<FakeTransport>();
    HttpClientConfig config;
    config.minimum_request_interval=std::chrono::milliseconds{0};
    config.retry.max_attempts=3;
    config.retry.initial_backoff=std::chrono::milliseconds{0};
    config.retry.max_backoff=std::chrono::milliseconds{0};
    HttpClient http(transport,config);

    const std::string start="2026-01-05",end="2026-01-06";
    const auto gold_url="https://api.nbp.pl/api/cenyzlota/"+start+"/"+end+"/?format=json";
    const auto a_url="https://api.nbp.pl/api/exchangerates/tables/A/"+start+"/"+end+"/?format=json";
    const auto b_url="https://api.nbp.pl/api/exchangerates/tables/B/"+start+"/"+end+"/?format=json";
    const auto c_url="https://api.nbp.pl/api/exchangerates/tables/C/"+start+"/"+end+"/?format=json";
    transport->responses[gold_url]={response(200,R"([{"data":"2026-01-05","cena":300.25}])")};
    transport->responses[a_url]={response(200,R"([{"table":"A","no":"002/A/NBP/2026","effectiveDate":"2026-01-05","rates":[{"code":"EUR","mid":4.21}]}])")};
    transport->responses[b_url]={response(200,R"([{"table":"B","no":"001/B/NBP/2026","effectiveDate":"2026-01-05","rates":[{"code":"XDR","mid":5.12}]}])")};
    transport->responses[c_url]={response(200,R"([{"table":"C","no":"003/C/NBP/2026","tradingDate":"2026-01-02","effectiveDate":"2026-01-05","rates":[{"code":"USD","bid":3.95,"ask":4.02}]}])")};
    NbpAdapter nbp(http);
    auto nbp_data=nbp.fetch(parse_date(start),parse_date(end));
    require(nbp_data.providers.size()==4,"NBP provider identities include A/B/C and gold");
    require(nbp_data.observations.size()==5,"NBP emits A, B, C bid/ask and gold");
    auto c_bid=std::find_if(nbp_data.observations.begin(),nbp_data.observations.end(),
        [](const auto& o){return o.provider_id=="nbp-c"&&o.price_kind==PriceKind::Bid;});
    require(c_bid!=nbp_data.observations.end(),"NBP C bid exists");
    require(format_utc_timestamp(c_bid->observed_at)=="2026-01-02T00:00:00Z","NBP C uses tradingDate as observation date");
    require(c_bid->provider_published_on&&format_date(*c_bid->provider_published_on)=="2026-01-05","NBP C preserves effective publication date");
    auto gold=std::find_if(nbp_data.observations.begin(),nbp_data.observations.end(),
        [](const auto& o){return o.instrument_id=="METAL:XAU";});
    require(gold!=nbp_data.observations.end()&&gold->value==300.25&&gold->price_kind==PriceKind::Reference,"NBP gold is PLN per gram reference");

    const std::string frank_url="https://api.frankfurter.dev/v2/rates?from=2026-01-05&to=2026-01-06";
    transport->responses[frank_url]={response(503,"temporary"),response(200,
        R"([{"date":"2026-01-05","base":"EUR","quote":"PLN","rate":4.23},{"date":"2026-01-06","base":"EUR","quote":"USD","rate":1.17}])")};
    FrankfurterAdapter frank(http);
    auto frank_data=frank.fetch(parse_date(start),parse_date(end));
    require(frank_data.observations.size()==2,"Frankfurter parses daily historical rows");
    require(frank_data.observations.front().price_kind==PriceKind::Mid,"Frankfurter preserves mid-market semantics");
    require(transport->calls.size()>=6,"HTTP retry made an additional attempt");

    const std::string landing="https://www.worldbank.org/en/research/commodity-markets";
    const std::string workbook="https://thedocs.worldbank.org/en/doc/test/related/CMO-Historical-Data-Monthly.xlsx";
    transport->responses[landing]={response(200,"<a href=\""+workbook+"\">Monthly XLSX</a>")};
    transport->responses[workbook]={response(200,make_xlsx())};
    WorldBankPinkSheetAdapter worldbank(http);
    auto wb=worldbank.fetch(parse_date("2026-08-01"),parse_date("2026-08-31"));
    require(wb.observations.size()==2,"World Bank workbook emits gold and silver");
    require(wb.observations[0].frequency==Frequency::Monthly,"World Bank frequency stays monthly");
    require(wb.observations[0].price_kind==PriceKind::Average,"World Bank monthly prices are averages");
    const auto wb_gold=std::find_if(wb.observations.begin(),wb.observations.end(),[](const auto& o){return o.instrument_id=="METAL:XAU";});
    const auto wb_silver=std::find_if(wb.observations.begin(),wb.observations.end(),[](const auto& o){return o.instrument_id=="METAL:XAG";});
    require(wb_gold!=wb.observations.end()&&wb_gold->value==3400.5,"World Bank gold numeric value");
    require(wb_silver!=wb.observations.end()&&wb_silver->value==39.25,"World Bank silver numeric value");

    const std::string xaus_url="https://xaus.com/api/v1/spot?compact=1";
    transport->responses[xaus_url]={response(200,
        R"({"updated_at":"2026-01-05T10:00:00Z","data_state":{"status":"fresh","as_of":"2026-01-05T10:00:00Z"},"xau":{"price":4500.0,"currency":"USD","unit":"oz"},"xag":{"price":52.0,"currency":"USD","unit":"oz"}})")};
    XausSpotAdapter xaus(http);
    auto spot=xaus.fetch_latest();
    require(spot.observations.size()==2,"XAUS returns XAU and XAG");
    require(spot.observations[0].price_kind==PriceKind::Spot,"XAUS values are spot indicative");
    require(format_utc_timestamp(spot.observations[0].observed_at)=="2026-01-05T10:00:00Z","XAUS preserves intraday observation timestamp");

    auto duplicated=frank_data.observations;
    duplicated.push_back(duplicated.front());
    require(deduplicate_observations(std::move(duplicated)).size()==2,"observation deduplication");

    auto bad_transport=std::make_shared<FakeTransport>();
    HttpClient bad_http(bad_transport,config);
    throws([&]{(void)bad_http.get("http://api.nbp.pl/api/");},"HTTP scheme must be HTTPS");
    throws([&]{(void)bad_http.get("https://example.com/data");},"unknown hosts rejected");
    std::cout<<"All C++ provider adapter tests passed.\n";
    return EXIT_SUCCESS;
}
