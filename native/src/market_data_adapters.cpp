#include "monitor/market_data_adapters.hpp"

#include <archive.h>
#include <archive_entry.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <limits>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <utility>

namespace monitor::market {
namespace {
using json = nlohmann::json;
using namespace std::chrono;

UtcTimestamp now_utc() {
    return UtcTimestamp{duration_cast<seconds>(system_clock::now().time_since_epoch()).count()};
}
UtcTimestamp at_midnight(const CivilDate& d) {
    const auto day = sys_days{year{d.year}/month{d.month}/std::chrono::day{d.day}};
    return UtcTimestamp{duration_cast<seconds>(day.time_since_epoch()).count()};
}
CivilDate date_from_timestamp(UtcTimestamp t) {
    const sys_days day = floor<days>(sys_seconds{seconds{t.unix_seconds}});
    const year_month_day ymd{day};
    return CivilDate{static_cast<int>(ymd.year()), static_cast<unsigned>(ymd.month()),
                     static_cast<unsigned>(ymd.day())};
}
std::string date_string(const CivilDate& d) { return format_date(d); }
std::string timestamp_string(const CivilDate& d) { return date_string(d) + "T00:00:00Z"; }
void validate_range(const CivilDate& start, const CivilDate& end) {
    if (!is_valid_date(start) || !is_valid_date(end) || at_midnight(start).unix_seconds > at_midnight(end).unix_seconds)
        throw std::invalid_argument("Nieprawidłowy zakres dat adaptera.");
}
std::vector<std::pair<CivilDate,CivilDate>> chunks_93(const CivilDate& start,const CivilDate& end) {
    validate_range(start,end);
    std::vector<std::pair<CivilDate,CivilDate>> out;
    auto cursor = sys_days{year{start.year}/month{start.month}/std::chrono::day{start.day}};
    const auto last = sys_days{year{end.year}/month{end.month}/std::chrono::day{end.day}};
    while(cursor<=last) {
        const auto stop=std::min(cursor+days{92},last);
        const year_month_day a{cursor}, b{stop};
        out.emplace_back(CivilDate{int(a.year()),unsigned(a.month()),unsigned(a.day())},
                         CivilDate{int(b.year()),unsigned(b.month()),unsigned(b.day())});
        cursor=stop+days{1};
    }
    return out;
}
std::string encode_query(std::string_view input) {
    std::ostringstream out;
    for(unsigned char c:input) {
        if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||
           c=='-'||c=='_'||c=='.'||c=='~') out<<static_cast<char>(c);
        else out<<'%'<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(c)<<std::nouppercase<<std::dec;
    }
    return out.str();
}
std::string upper(std::string s) {
    for(char& c:s) if(c>='a'&&c<='z') c=static_cast<char>(c-'a'+'A');
    return s;
}
bool valid_currency_code(const std::string& s) {
    if(s.size()!=3) return false;
    return std::all_of(s.begin(),s.end(),[](char c){return c>='A'&&c<='Z';});
}
double number(const json& value,const char* label) {
    if(!value.is_number()) throw std::runtime_error(std::string("Pole ")+label+" nie jest liczbą.");
    const double n=value.get<double>();
    if(!std::isfinite(n)) throw std::runtime_error(std::string("Pole ")+label+" nie jest skończone.");
    return n;
}
std::string required_string(const json& value,const char* key) {
    if(!value.contains(key)||!value[key].is_string()||value[key].get<std::string>().empty())
        throw std::runtime_error(std::string("Brak poprawnego pola JSON: ")+key);
    return value[key].get<std::string>();
}
std::string safe_host(const std::string& url) {
    constexpr std::string_view prefix="https://";
    if(url.rfind(prefix,0)!=0) throw std::invalid_argument("Adapter dopuszcza wyłącznie HTTPS.");
    const auto end=url.find_first_of("/?#",prefix.size());
    const auto host=url.substr(prefix.size(),end==std::string::npos?std::string::npos:end-prefix.size());
    if(host.empty()||host.find('@')!=std::string::npos||host.find(':')!=std::string::npos)
        throw std::invalid_argument("Nieprawidłowy host URL.");
    return host;
}
void require_host(const std::string& url,const std::string& allowed) {
    if(safe_host(url)!=allowed) throw std::invalid_argument("Host URL spoza listy dozwolonych dostawców.");
}
std::string make_observation_id(const std::string& provider,const std::string& instrument,
                                UtcTimestamp observed,PriceKind kind) {
    return provider+":"+instrument+":"+std::to_string(observed.unix_seconds)+":"+
           std::to_string(static_cast<int>(kind));
}
Instrument fx_instrument(const std::string& base,const std::string& quote) {
    if(!valid_currency_code(base)||!valid_currency_code(quote)||base==quote)
        throw std::runtime_error("Dostawca zwrócił niepoprawną parę walutową.");
    const auto symbol=base+"/"+quote;
    return Instrument{"FX:"+symbol,symbol,symbol,AssetClass::Currency,base,quote,Unit::CurrencyUnit,false};
}
Instrument metal_instrument(const std::string& symbol,const std::string& name,
                            const std::string& quote,Unit unit) {
    return Instrument{"METAL:"+symbol,symbol,name,AssetClass::PreciousMetal,std::nullopt,quote,unit,false};
}
Provider provider(std::string id,std::string name,std::string home,std::string license) {
    return Provider{std::move(id),std::move(name),std::move(home),std::move(license)};
}
MarketObservation observation(const Provider& p,const Instrument& i,const CivilDate& date,
                              UtcTimestamp received,double value,PriceKind kind,Frequency frequency,
                              std::optional<CivilDate> published,std::string source,
                              std::optional<std::string> record={},QualityStatus quality=QualityStatus::Validated,
                              std::vector<std::string> flags={}) {
    MarketObservation o;
    o.id=make_observation_id(p.id,i.id,at_midnight(date),kind);
    o.instrument_id=i.id;o.provider_id=p.id;o.observed_at=at_midnight(date);o.received_at=received;
    o.provider_published_on=published;o.frequency=frequency;o.price_kind=kind;o.value=value;
    o.quality=quality;o.quality_flags=std::move(flags);o.source_uri=std::move(source);
    o.provider_record_id=std::move(record);
    validate_observation(i,p,o);
    return o;
}
std::string join_url(std::string_view base,std::string_view path) {
    if(path.rfind("https://",0)==0) return std::string(path);
    if(path.empty()) return std::string(base);
    if(path.front()=='/') {
        const auto scheme=base.find("://");
        const auto host_end=base.find('/',scheme+3);
        return std::string(base.substr(0,host_end==std::string_view::npos?base.size():host_end))+std::string(path);
    }
    const auto slash=base.rfind('/');
    return std::string(base.substr(0,slash==std::string_view::npos?base.size():slash+1))+std::string(path);
}
std::string xml_text(xmlNode* node) {
    xmlChar* raw=xmlNodeGetContent(node);
    if(!raw) return {};
    std::string result(reinterpret_cast<const char*>(raw));
    xmlFree(raw);
    return result;
}
std::string attr(xmlNode* node,const char* name) {
    xmlChar* raw=xmlGetProp(node,BAD_CAST name);
    if(!raw && std::string_view{name}=="id")
        raw=xmlGetNsProp(node,BAD_CAST "id",BAD_CAST "http://schemas.openxmlformats.org/officeDocument/2006/relationships");
    if(!raw) return {};
    std::string result(reinterpret_cast<const char*>(raw));
    xmlFree(raw);
    return result;
}
std::string cell_column(const std::string& ref) {
    std::string out;
    for(char c:ref) { if(c>='A'&&c<='Z') out.push_back(c); else if(c>='a'&&c<='z') out.push_back(static_cast<char>(c-'a'+'A')); else break; }
    return out;
}
std::size_t column_index(const std::string& letters) {
    std::size_t n=0;
    for(char c:letters) n=n*26U+static_cast<std::size_t>(c-'A'+1);
    return n==0?0:n-1;
}
std::string month_date(const std::string& period) {
    static const std::regex pattern(R"(^\s*(\d{4})M(\d{2})\s*$)",std::regex::icase);
    std::smatch m;
    if(!std::regex_match(period,m,pattern)) return {};
    const int year_value=std::stoi(m[1].str()), month_value=std::stoi(m[2].str());
    if(month_value<1||month_value>12) return {};
    return format_date(CivilDate{year_value,static_cast<unsigned>(month_value),1U});
}
struct ZipArchive {
    archive* handle{archive_read_new()};
    explicit ZipArchive(const std::string& bytes) {
        archive_read_support_format_zip(handle);
        archive_read_support_filter_all(handle);
        if(archive_read_open_memory(handle,bytes.data(),bytes.size())!=ARCHIVE_OK)
            throw std::runtime_error("Nie można otworzyć skoroszytu XLSX jako ZIP.");
    }
    ~ZipArchive(){if(handle) archive_read_free(handle);}
    std::string entry(const std::string& wanted) {
        archive_entry* item=nullptr;
        while(archive_read_next_header(handle,&item)==ARCHIVE_OK) {
            const char* name=archive_entry_pathname(item);
            if(name&&wanted==name) {
                std::string data;
                std::array<char,16384> buffer{};
                la_ssize_t count{};
                while((count=archive_read_data(handle,buffer.data(),buffer.size()))>0) data.append(buffer.data(),static_cast<std::size_t>(count));
                if(count<0) throw std::runtime_error("Błąd odczytu pliku XLSX.");
                return data;
            }
            archive_read_data_skip(handle);
        }
        throw std::runtime_error("Brak elementu skoroszytu XLSX: "+wanted);
    }
};
xmlDoc* parse_xml(const std::string& data) {
    auto* doc=xmlReadMemory(data.data(),static_cast<int>(data.size()),"workbook.xml",nullptr,
                            XML_PARSE_NONET|XML_PARSE_NOBLANKS|XML_PARSE_NOERROR|XML_PARSE_NOWARNING);
    if(!doc) throw std::runtime_error("Niepoprawny XML w skoroszycie World Bank.");
    return doc;
}
void collect_text_nodes(xmlNode* node,std::string& out) {
    for(auto* cur=node;cur;cur=cur->next) {
        if(cur->type==XML_ELEMENT_NODE&&xmlStrEqual(cur->name,BAD_CAST "t")) out+=xml_text(cur);
        collect_text_nodes(cur->children,out);
    }
}
std::vector<std::string> parse_shared_strings(const std::string& xml) {
    xmlDoc* doc=parse_xml(xml);std::vector<std::string> out;
    auto* root=xmlDocGetRootElement(doc);
    for(auto* si=root?root->children:nullptr;si;si=si->next) if(si->type==XML_ELEMENT_NODE&&xmlStrEqual(si->name,BAD_CAST "si")) {
        std::string value;collect_text_nodes(si->children,value);out.push_back(std::move(value));
    }
    xmlFreeDoc(doc);return out;
}
std::string resolve_monthly_sheet(ZipArchive& zip) {
    xmlDoc* wb=parse_xml(zip.entry("xl/workbook.xml"));
    std::string rel_id;
    auto* root=xmlDocGetRootElement(wb);
    for(auto* n=root?root->children:nullptr;n;n=n->next) if(n->type==XML_ELEMENT_NODE&&xmlStrEqual(n->name,BAD_CAST "sheets"))
        for(auto* sh=n->children;sh;sh=sh->next) if(sh->type==XML_ELEMENT_NODE&&attr(sh,"name")=="Monthly Prices") rel_id=attr(sh,"id");
    xmlFreeDoc(wb);
    if(rel_id.empty()) throw std::runtime_error("World Bank XLSX nie zawiera arkusza Monthly Prices.");
    xmlDoc* rel=parse_xml(zip.entry("xl/_rels/workbook.xml.rels"));
    std::string target;
    auto* rr=xmlDocGetRootElement(rel);
    for(auto* n=rr?rr->children:nullptr;n;n=n->next) if(n->type==XML_ELEMENT_NODE&&attr(n,"Id")==rel_id) target=attr(n,"Target");
    xmlFreeDoc(rel);
    if(target.empty()) throw std::runtime_error("Brak relacji arkusza Monthly Prices.");
    if(target.rfind("/",0)==0) return target.substr(1);
    if(target.rfind("xl/",0)==0) return target;
    return "xl/"+target;
}
using SheetRow=std::map<std::size_t,std::string>;
std::vector<SheetRow> parse_sheet_rows(const std::string& xml,const std::vector<std::string>& shared) {
    xmlDoc* doc=parse_xml(xml);std::vector<SheetRow> rows;
    auto* root=xmlDocGetRootElement(doc);
    for(auto* sheet=root?root->children:nullptr;sheet;sheet=sheet->next) if(sheet->type==XML_ELEMENT_NODE&&xmlStrEqual(sheet->name,BAD_CAST "sheetData"))
        for(auto* row=sheet->children;row;row=row->next) if(row->type==XML_ELEMENT_NODE&&xmlStrEqual(row->name,BAD_CAST "row")) {
            SheetRow values;
            for(auto* c=row->children;c;c=c->next) if(c->type==XML_ELEMENT_NODE&&xmlStrEqual(c->name,BAD_CAST "c")) {
                const auto index=column_index(cell_column(attr(c,"r")));
                const auto type=attr(c,"t");std::string value;
                for(auto* child=c->children;child;child=child->next) if(child->type==XML_ELEMENT_NODE) {
                    if(xmlStrEqual(child->name,BAD_CAST "v")) value=xml_text(child);
                    else if(xmlStrEqual(child->name,BAD_CAST "is")) collect_text_nodes(child->children,value);
                }
                if(type=="s"&&!value.empty()) {
                    std::size_t pos{};const auto idx=std::stoul(value,&pos);
                    if(pos!=value.size()||idx>=shared.size()) throw std::runtime_error("Niepoprawny indeks shared string XLSX.");
                    value=shared[idx];
                }
                values[index]=value;
            }
            rows.push_back(std::move(values));
        }
    xmlFreeDoc(doc);return rows;
}
std::vector<std::pair<CivilDate,std::pair<double,double>>> parse_world_bank_xlsx(const std::string& bytes) {
    ZipArchive zip(bytes);
    const auto shared=parse_shared_strings(zip.entry("xl/sharedStrings.xml"));
    const auto sheet_path=resolve_monthly_sheet(zip);
    const auto rows=parse_sheet_rows(zip.entry(sheet_path),shared);
    std::optional<std::size_t> period_col,gold_col,silver_col;std::size_t header_row=0;
    for(std::size_t r=0;r<rows.size();++r) {
        for(const auto& [col,value]:rows[r]) {
            std::string lower=value;
            std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
            if(lower=="period") period_col=col;
            if(lower=="gold") gold_col=col;
            if(lower=="silver") silver_col=col;
        }
        if(period_col&&gold_col&&silver_col){header_row=r;break;}
        period_col.reset();gold_col.reset();silver_col.reset();
    }
    if(!period_col||!gold_col||!silver_col) throw std::runtime_error("Nie znaleziono kolumn Period, Gold i Silver w World Bank Pink Sheet.");
    std::vector<std::pair<CivilDate,std::pair<double,double>>> result;
    for(std::size_t r=header_row+1;r<rows.size();++r) {
        const auto& row=rows[r];
        const auto p=row.find(*period_col),g=row.find(*gold_col),s=row.find(*silver_col);
        if(p==row.end()||g==row.end()||s==row.end()) continue;
        const auto month=month_date(p->second);
        if(month.empty()||g->second.empty()||s->second.empty()||g->second==".."||s->second=="..") continue;
        std::size_t pg{},ps{};
        const double gold=std::stod(g->second,&pg),silver=std::stod(s->second,&ps);
        if(pg!=g->second.size()||ps!=s->second.size()||!std::isfinite(gold)||!std::isfinite(silver)||gold<0||silver<0)
            throw std::runtime_error("Niepoprawna wartość ceny w World Bank Pink Sheet.");
        result.emplace_back(parse_date(month),std::pair<double,double>{gold,silver});
    }
    if(result.empty()) throw std::runtime_error("World Bank Pink Sheet nie zawiera rozpoznanych miesięcznych cen.");
    return result;
}
} // namespace

HttpClient::HttpClient(std::shared_ptr<HttpTransport> transport,HttpClientConfig config)
    :transport_(std::move(transport)),config_(config) {
    if(!transport_) throw std::invalid_argument("HttpClient wymaga transportu HTTP.");
    if(config_.retry.max_attempts==0) throw std::invalid_argument("Liczba prób HTTP musi być większa od zera.");
}
void HttpClient::throttle() {
    std::unique_lock lock(throttle_mutex_);
    const auto now=steady_clock::now();
    if(next_request_>now) std::this_thread::sleep_until(next_request_);
    next_request_=steady_clock::now()+config_.minimum_request_interval;
}
HttpResponse HttpClient::get(const std::string& url,std::size_t max_bytes) {
    const auto host=safe_host(url);
    static const std::set<std::string> allowed{"api.nbp.pl","api.frankfurter.dev","thedocs.worldbank.org","www.worldbank.org","xaus.com"};
    if(!allowed.contains(host)) throw std::invalid_argument("Host HTTP nie znajduje się na liście dostawców.");
    HttpRequest request;
    request.url=url;request.timeout=config_.default_timeout;
    request.max_response_bytes=max_bytes?max_bytes:config_.max_json_bytes;
    request.headers={{"Accept","application/json"},{"User-Agent","Monitor-Rynkow/0.2 C++20"}};
    std::chrono::milliseconds backoff=config_.retry.initial_backoff;
    for(unsigned attempt=1;attempt<=config_.retry.max_attempts;++attempt) {
        throttle();
        HttpResponse response;
        try {
            response=transport_->get(request);
        } catch(const std::exception&) {
            if(attempt==config_.retry.max_attempts) throw;
            std::this_thread::sleep_for(backoff);
            backoff=std::min(backoff*2,config_.retry.max_backoff);
            continue;
        }
        if(response.body.size()>request.max_response_bytes)
            throw std::runtime_error("Odpowiedź HTTP przekracza skonfigurowany limit rozmiaru.");
        const long status=response.status_code;
        if(status==404) return response;
        if(status==429||status==500||status==502||status==503||status==504) {
            if(attempt==config_.retry.max_attempts)
                throw std::runtime_error("Dostawca zwrócił HTTP "+std::to_string(status)+" po wyczerpaniu prób.");
            auto wait=backoff;
            const auto it=response.headers.find("retry-after");
            if(it!=response.headers.end()) {
                long seconds_value{};
                const auto parsed=std::from_chars(it->second.data(),it->second.data()+it->second.size(),seconds_value);
                if(parsed.ec==std::errc{}&&seconds_value>=0)
                    wait=std::min(milliseconds{seconds_value*1000},config_.retry.max_backoff);
            }
            std::this_thread::sleep_for(wait);
            backoff=std::min(backoff*2,config_.retry.max_backoff);
            continue;
        }
        if(status<200||status>=300)
            throw std::runtime_error("Żądanie HTTP zakończone kodem "+std::to_string(status)+".");
        return response;
    }
    throw std::runtime_error("Nieoczekiwany koniec ponawiania żądania HTTP.");
}

HttpResponse CurlHttpTransport::get(const HttpRequest& request) {
    static const int initialized=[](){return curl_global_init(CURL_GLOBAL_DEFAULT);}();
    if(initialized!=CURLE_OK) throw std::runtime_error("Nie udało się zainicjalizować libcurl.");
    CURL* curl=curl_easy_init();
    if(!curl) throw std::runtime_error("Nie udało się utworzyć uchwytu libcurl.");
    struct Context { HttpResponse* response;std::size_t limit;bool overflow{}; } context;
    HttpResponse response;context.response=&response;context.limit=request.max_response_bytes;
    auto write_cb=[](char* data,size_t size,size_t count,void* opaque)->size_t {
        auto* ctx=static_cast<Context*>(opaque);const std::size_t bytes=size*count;
        if(bytes>ctx->limit-std::min(ctx->limit,ctx->response->body.size())) {ctx->overflow=true;return 0;}
        ctx->response->body.append(data,bytes);return bytes;
    };
    auto header_cb=[](char* data,size_t size,size_t count,void* opaque)->size_t {
        const std::size_t bytes=size*count;auto* ctx=static_cast<Context*>(opaque);
        std::string line(data,bytes);const auto colon=line.find(':');
        if(colon!=std::string::npos) {
            auto key=line.substr(0,colon);std::transform(key.begin(),key.end(),key.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
            auto value=line.substr(colon+1);const auto first=value.find_first_not_of(" \t\r\n");
            if(first!=std::string::npos) value=value.substr(first);
            while(!value.empty()&&(value.back()=='\r'||value.back()=='\n'||value.back()==' ')) value.pop_back();
            ctx->response->headers[key]=value;
        }
        return bytes;
    };
    curl_slist* headers=nullptr;
    for(const auto& [key,value]:request.headers) headers=curl_slist_append(headers,(key+": "+value).c_str());
    curl_easy_setopt(curl,CURLOPT_URL,request.url.c_str());
    curl_easy_setopt(curl,CURLOPT_HTTPGET,1L);
    curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,0L);
    curl_easy_setopt(curl,CURLOPT_PROTOCOLS,CURLPROTO_HTTPS);
    curl_easy_setopt(curl,CURLOPT_REDIR_PROTOCOLS,CURLPROTO_HTTPS);
    curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT_MS,static_cast<long>(std::min<std::int64_t>(request.timeout.count(),10000)));
    curl_easy_setopt(curl,CURLOPT_TIMEOUT_MS,static_cast<long>(request.timeout.count()));
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L);
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2L);
    curl_easy_setopt(curl,CURLOPT_NOSIGNAL,1L);
    curl_easy_setopt(curl,CURLOPT_HTTPHEADER,headers);
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,+write_cb);
    curl_easy_setopt(curl,CURLOPT_WRITEDATA,&context);
    curl_easy_setopt(curl,CURLOPT_HEADERFUNCTION,+header_cb);
    curl_easy_setopt(curl,CURLOPT_HEADERDATA,&context);
    const CURLcode code=curl_easy_perform(curl);
    if(code==CURLE_OK) curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&response.status_code);
    curl_slist_free_all(headers);curl_easy_cleanup(curl);
    if(code!=CURLE_OK) {
        if(context.overflow) throw std::runtime_error("Odpowiedź HTTP przekracza limit rozmiaru.");
        throw std::runtime_error(std::string("Błąd transportu libcurl: ")+curl_easy_strerror(code));
    }
    return response;
}

std::vector<MarketObservation> deduplicate_observations(std::vector<MarketObservation> rows) {
    using Key=std::tuple<std::string,std::string,std::int64_t,int>;
    std::map<Key,MarketObservation> unique;
    for(auto& row:rows) {
        const Key key{row.provider_id,row.instrument_id,row.observed_at.unix_seconds,static_cast<int>(row.price_kind)};
        auto it=unique.find(key);
        if(it==unique.end()||row.received_at.unix_seconds>=it->second.received_at.unix_seconds)
            unique.insert_or_assign(key,std::move(row));
    }
    std::vector<MarketObservation> result;result.reserve(unique.size());
    for(auto& [key,row]:unique) { (void)key;result.push_back(std::move(row)); }
    return result;
}

NbpAdapter::NbpAdapter(HttpClient& http):http_(http){}
AdapterResult NbpAdapter::fetch(const CivilDate& start,const CivilDate& end) {
    validate_range(start,end);
    AdapterResult result;const auto received=now_utc();
    const Provider gold_provider=provider("nbp-gold","Narodowy Bank Polski — cena złota","https://api.nbp.pl/","Dane publiczne NBP; warunki API NBP");
    result.providers.push_back(gold_provider);
    const auto gold=metal_instrument("XAU","Złoto","PLN",Unit::Gram);result.instruments.push_back(gold);
    for(const auto& [a,b]:chunks_93(start,end)) {
        const auto url="https://api.nbp.pl/api/cenyzlota/"+date_string(a)+"/"+date_string(b)+"/?format=json";
        const auto response=http_.get(url);
        if(response.status_code==404) continue;
        const auto payload=json::parse(response.body);
        if(!payload.is_array()) throw std::runtime_error("Nieoczekiwany format odpowiedzi cen złota NBP.");
        for(const auto& row:payload) {
            const auto date=parse_date(required_string(row,"data"));
            const double value=number(row.at("cena"),"cena");
            result.observations.push_back(observation(gold_provider,gold,date,received,value,PriceKind::Reference,
                Frequency::Daily,date,url,required_string(row,"data")));
        }
    }
    for(const char table:std::string("ABC")) {
        const std::string table_code(1,table);
        const Provider p=provider("nbp-"+std::string(1,static_cast<char>(std::tolower(table))),
            "Narodowy Bank Polski — tabela "+table_code,"https://api.nbp.pl/","Dane publiczne NBP; warunki API NBP");
        result.providers.push_back(p);
        for(const auto& [a,b]:chunks_93(start,end)) {
            const auto url="https://api.nbp.pl/api/exchangerates/tables/"+table_code+"/"+date_string(a)+"/"+date_string(b)+"/?format=json";
            const auto response=http_.get(url);
            if(response.status_code==404) continue;
            const auto payload=json::parse(response.body);
            if(!payload.is_array()) throw std::runtime_error("Nieoczekiwany format tabeli NBP.");
            for(const auto& daily:payload) {
                const auto effective=parse_date(required_string(daily,"effectiveDate"));
                const auto observed=table=='C'&&daily.contains("tradingDate")?
                    parse_date(required_string(daily,"tradingDate")):effective;
                const auto& rates=daily.at("rates");
                if(!rates.is_array()) throw std::runtime_error("Pole rates NBP nie jest tablicą.");
                for(const auto& rate:rates) {
                    const auto code=upper(required_string(rate,"code"));
                    const auto instrument=fx_instrument(code,"PLN");
                    result.instruments.push_back(instrument);
                    const auto record=required_string(daily,"no")+":"+code;
                    if(table=='A'||table=='B') {
                        const double value=number(rate.at("mid"),"mid");
                        result.observations.push_back(observation(p,instrument,observed,received,value,PriceKind::Average,
                            Frequency::Daily,effective,url,record,QualityStatus::Validated,
                            {"nbp_official_table_"+table_code,"reference_not_transaction"}));
                    } else {
                        result.observations.push_back(observation(p,instrument,observed,received,
                            number(rate.at("bid"),"bid"),PriceKind::Bid,Frequency::Daily,effective,url,record));
                        result.observations.push_back(observation(p,instrument,observed,received,
                            number(rate.at("ask"),"ask"),PriceKind::Ask,Frequency::Daily,effective,url,record));
                    }
                }
            }
        }
    }
    std::map<std::string,Instrument> instruments;
    for(auto& i:result.instruments) instruments.insert_or_assign(i.id,std::move(i));
    result.instruments.clear();for(auto& [id,i]:instruments){(void)id;result.instruments.push_back(std::move(i));}
    result.observations=deduplicate_observations(std::move(result.observations));
    return result;
}

FrankfurterAdapter::FrankfurterAdapter(HttpClient& http,std::string filter)
    :http_(http),provider_filter_(std::move(filter)) {
    if(!provider_filter_.empty()&&!std::regex_match(provider_filter_,std::regex("[A-Za-z0-9._-]+")))
        throw std::invalid_argument("Niepoprawny filtr dostawcy Frankfurter.");
}
AdapterResult FrankfurterAdapter::fetch(const CivilDate& start,const CivilDate& end) {
    validate_range(start,end);
    AdapterResult result;const auto received=now_utc();
    const auto provider_id=provider_filter_.empty()?"frankfurter-blend":"frankfurter-"+provider_filter_;
    const Provider p=provider(provider_id,"Frankfurter v2 — "+(provider_filter_.empty()?std::string("blend"):provider_filter_),
                              "https://api.frankfurter.dev/","Dane referencyjne; warunki Frankfurter i źródeł składowych");
    result.providers.push_back(p);
    for(const auto& [a,b]:chunks_93(start,end)) {
        std::string url="https://api.frankfurter.dev/v2/rates?from="+date_string(a)+"&to="+date_string(b);
        if(!provider_filter_.empty()) url+="&providers="+encode_query(provider_filter_);
        const auto response=http_.get(url);
        if(response.status_code==404) continue;
        const auto payload=json::parse(response.body);
        if(!payload.is_array()) throw std::runtime_error("Frankfurter v2 zwrócił nieoczekiwany format.");
        for(const auto& row:payload) {
            const auto date=parse_date(required_string(row,"date"));
            const auto base=upper(required_string(row,"base")),quote=upper(required_string(row,"quote"));
            const auto instrument=fx_instrument(base,quote);
            result.instruments.push_back(instrument);
            const double rate=number(row.at("rate"),"rate");
            const auto record=date_string(date)+":"+base+":"+quote;
            result.observations.push_back(observation(p,instrument,date,received,rate,PriceKind::Mid,
                Frequency::Daily,date,url,record,QualityStatus::Validated,
                {"frankfurter_mid_market",provider_filter_.empty()?"provider_blend":"provider_pinned"}));
        }
    }
    std::map<std::string,Instrument> instruments;
    for(auto& i:result.instruments) instruments.insert_or_assign(i.id,std::move(i));
    result.instruments.clear();for(auto& [id,i]:instruments){(void)id;result.instruments.push_back(std::move(i));}
    result.observations=deduplicate_observations(std::move(result.observations));
    return result;
}

WorldBankPinkSheetAdapter::WorldBankPinkSheetAdapter(HttpClient& http):http_(http){}
AdapterResult WorldBankPinkSheetAdapter::fetch(const CivilDate& start,const CivilDate& end) {
    validate_range(start,end);
    constexpr const char* landing="https://www.worldbank.org/en/research/commodity-markets";
    constexpr const char* fallback="https://thedocs.worldbank.org/en/doc/74e8be41ceb20fa0da750cda2f6b9e4e-0050012026/related/CMO-Historical-Data-Monthly.xlsx";
    auto page=http_.get(landing,5U*1024U*1024U);
    if(page.status_code!=200) throw std::runtime_error("Nie można pobrać strony World Bank Commodity Markets.");
    const std::regex link_pattern(R"re(href\s*=\s*["']([^"']*CMO-Historical-Data-Monthly\.xlsx[^"']*)["'])re",std::regex::icase);
    std::smatch match;std::string url=fallback;
    if(std::regex_search(page.body,match,link_pattern)) url=join_url(landing,match[1].str());
    require_host(url,"thedocs.worldbank.org");
    auto workbook=http_.get(url,32U*1024U*1024U);
    if(workbook.status_code!=200) throw std::runtime_error("Nie można pobrać miesięcznego skoroszytu World Bank.");
    const auto prices=parse_world_bank_xlsx(workbook.body);
    AdapterResult result;const auto received=now_utc();
    const Provider p=provider("world-bank-pink-sheet","World Bank — Commodity Markets Pink Sheet",
        "https://www.worldbank.org/en/research/commodity-markets","World Bank dataset terms; retain attribution and review terms for redistribution");
    result.providers.push_back(p);
    const auto gold=metal_instrument("XAU","Złoto","USD",Unit::TroyOunce);
    const auto silver=metal_instrument("XAG","Srebro","USD",Unit::TroyOunce);
    result.instruments={gold,silver};
    for(const auto& [date,values]:prices) {
        if(at_midnight(date).unix_seconds<at_midnight(start).unix_seconds||
           at_midnight(date).unix_seconds>at_midnight(end).unix_seconds) continue;
        result.observations.push_back(observation(p,gold,date,received,values.first,PriceKind::Average,
            Frequency::Monthly,std::nullopt,url,date_string(date),QualityStatus::Validated,{"pink_sheet_monthly_average","usd_per_troy_ounce"}));
        result.observations.push_back(observation(p,silver,date,received,values.second,PriceKind::Average,
            Frequency::Monthly,std::nullopt,url,date_string(date),QualityStatus::Validated,{"pink_sheet_monthly_average","usd_per_troy_ounce"}));
    }
    result.observations=deduplicate_observations(std::move(result.observations));
    return result;
}

XausSpotAdapter::XausSpotAdapter(HttpClient& http):http_(http){}
AdapterResult XausSpotAdapter::fetch(const CivilDate& start,const CivilDate& end) {
    validate_range(start,end);
    auto result=fetch_latest();
    result.observations.erase(std::remove_if(result.observations.begin(),result.observations.end(),[&](const auto& o){
        const auto d=date_from_timestamp(o.observed_at);
        return at_midnight(d).unix_seconds<at_midnight(start).unix_seconds||
               at_midnight(d).unix_seconds>at_midnight(end).unix_seconds;
    }),result.observations.end());
    return result;
}
AdapterResult XausSpotAdapter::fetch_latest() {
    constexpr const char* url="https://xaus.com/api/v1/spot?compact=1";
    const auto response=http_.get(url);
    if(response.status_code!=200) throw std::runtime_error("XAUS spot endpoint jest niedostępny.");
    const auto payload=json::parse(response.body);
    if(!payload.is_object()) throw std::runtime_error("XAUS zwrócił niepoprawny dokument JSON.");
    const auto state=payload.value("data_state",json::object());
    const auto status=state.value("status",std::string("unknown"));
    if(status=="unavailable") throw std::runtime_error("XAUS nie udostępnia aktualnie rzeczywistej ceny spot.");
    const std::string as_of=state.value("as_of",payload.value("updated_at",std::string{}));
    if(as_of.empty()) throw std::runtime_error("XAUS nie zwrócił czasu obserwacji.");
    const auto observed=parse_utc_timestamp(as_of);
    const auto date=date_from_timestamp(observed);
    const auto received=now_utc();
    const bool stale=status=="stale";
    AdapterResult result;
    const Provider p=provider("xaus","XAUS Gold Data API","https://xaus.com/","Public API data; attribution requested by provider");
    result.providers.push_back(p);
    auto add_metal=[&](const std::string& key,const std::string& symbol,const std::string& name) {
        if(!payload.contains(key)||!payload[key].is_object()||!payload[key].contains("price")) return;
        const auto& item=payload[key];
        const auto currency=upper(item.value("currency",std::string("USD")));
        const auto unit_text=item.value("unit",std::string("oz"));
        Unit unit=Unit::TroyOunce;
        if(unit_text=="gram"||unit_text=="g") unit=Unit::Gram;
        else if(unit_text=="kg") unit=Unit::Kilogram;
        else if(unit_text!="oz"&&unit_text!="toz"&&unit_text!="troy_ounce")
            throw std::runtime_error("XAUS zwrócił nieobsługiwaną jednostkę ceny spot.");
        const auto instrument=metal_instrument(symbol,name,currency,unit);
        result.instruments.push_back(instrument);
        auto quote=observation(p,instrument,date,received,number(item.at("price"),"price"),
            PriceKind::Spot,Frequency::Minute,date,url,as_of,stale?QualityStatus::Suspect:QualityStatus::Validated,
            stale?std::vector<std::string>{"provider_stale_snapshot","indicative_mid_market"}:
                  std::vector<std::string>{"indicative_mid_market","provider_fresh"});
        quote.observed_at=observed;
        quote.id=make_observation_id(p.id,instrument.id,observed,PriceKind::Spot);
        validate_observation(instrument,p,quote);
        result.observations.push_back(std::move(quote));
    };
    add_metal("xau","XAU","Złoto");
    add_metal("xag","XAG","Srebro");
    if(result.observations.empty()) {
        if(payload.contains("spot_usd_oz")) {
            const auto instrument=metal_instrument("XAU","Złoto","USD",Unit::TroyOunce);
            result.instruments.push_back(instrument);
            auto quote=observation(p,instrument,date,received,number(payload.at("spot_usd_oz"),"spot_usd_oz"),
                PriceKind::Spot,Frequency::Minute,date,url,as_of,stale?QualityStatus::Suspect:QualityStatus::Validated,
                stale?std::vector<std::string>{"provider_stale_snapshot","indicative_mid_market"}:
                      std::vector<std::string>{"indicative_mid_market","provider_fresh"});
            quote.observed_at=observed;
            quote.id=make_observation_id(p.id,instrument.id,observed,PriceKind::Spot);
            validate_observation(instrument,p,quote);
            result.observations.push_back(std::move(quote));
        } else throw std::runtime_error("XAUS nie zwrócił rozpoznanych notowań XAU/XAG.");
    }
    return result;
}

} // namespace monitor::market
