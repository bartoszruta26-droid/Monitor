#include "monitor/http_api.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace monitor::server {
namespace {
using json = nlohmann::json;
using namespace monitor::market;

std::string percent_decode(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    const auto hex=[](char c)->int {
        if(c>='0'&&c<='9') return c-'0';
        if(c>='a'&&c<='f') return c-'a'+10;
        if(c>='A'&&c<='F') return c-'A'+10;
        return -1;
    };
    for(std::size_t i=0;i<value.size();++i) {
        if(value[i]=='+') { out.push_back(' '); continue; }
        if(value[i]!='%') { out.push_back(value[i]); continue; }
        if(i+2>=value.size()) throw std::invalid_argument("Nieprawidłowe kodowanie parametru URL.");
        const int hi=hex(value[i+1]),lo=hex(value[i+2]);
        if(hi<0||lo<0) throw std::invalid_argument("Nieprawidłowe kodowanie parametru URL.");
        const char decoded=static_cast<char>((hi<<4)|lo);
        if(decoded=='\0') throw std::invalid_argument("Parametr URL zawiera znak NUL.");
        out.push_back(decoded); i+=2;
    }
    return out;
}

std::map<std::string,std::string> parse_query(std::string_view target) {
    std::map<std::string,std::string> result;
    const auto question=target.find('?');
    if(question==std::string_view::npos) return result;
    auto query=target.substr(question+1);
    while(!query.empty()) {
        const auto amp=query.find('&');
        const auto part=query.substr(0,amp);
        const auto equals=part.find('=');
        const auto key=percent_decode(part.substr(0,equals));
        const auto value=equals==std::string_view::npos?std::string{}:
            percent_decode(part.substr(equals+1));
        if(key.empty()||!result.emplace(key,value).second)
            throw std::invalid_argument("Pusty lub powtórzony parametr URL.");
        if(amp==std::string_view::npos) break;
        query.remove_prefix(amp+1);
    }
    if(result.size()>16) throw std::invalid_argument("Zbyt wiele parametrów URL.");
    return result;
}

std::optional<std::string> get_text(const std::map<std::string,std::string>& q,const char* key) {
    const auto it=q.find(key);
    if(it==q.end()) return std::nullopt;
    if(it->second.empty()) throw std::invalid_argument(std::string("Pusty parametr: ")+key);
    return it->second;
}

std::optional<std::int64_t> get_integer(const std::map<std::string,std::string>& q,const char* key) {
    const auto value=get_text(q,key);
    if(!value) return std::nullopt;
    std::int64_t parsed{};
    const auto result=std::from_chars(value->data(),value->data()+value->size(),parsed);
    if(result.ec!=std::errc{}||result.ptr!=value->data()+value->size()||parsed<0)
        throw std::invalid_argument(std::string("Nieprawidłowa liczba w parametrze: ")+key);
    return parsed;
}

bool token_equal(std::string_view a,std::string_view b) {
    const std::size_t count=std::max(a.size(),b.size());
    unsigned char diff=static_cast<unsigned char>(a.size()^b.size());
    for(std::size_t i=0;i<count;++i) {
        const auto av=i<a.size()?static_cast<unsigned char>(a[i]):0U;
        const auto bv=i<b.size()?static_cast<unsigned char>(b[i]):0U;
        diff=static_cast<unsigned char>(diff|(av^bv));
    }
    return diff==0;
}

json transaction_json(const PortfolioTransaction& t) {
    json j={{"id",t.id},{"asset_code",t.asset_code},
        {"asset_kind",t.asset_kind==PortfolioAssetKind::Gold?"gold":"currency"},
        {"side",t.side==TradeSide::Buy?"buy":"sell"},
        {"quantity",t.quantity},{"purity",t.purity},
        {"unit_price_reporting",t.unit_price_reporting},{"fee_reporting",t.fee_reporting},
        {"executed_at_unix",t.executed_at_unix},{"source_id",t.source_id},
        {"operation_id",t.operation_id}};
    if(t.fx_audit) {
        const auto& a=*t.fx_audit;
        j["fx_audit"]={{"base_currency",a.base_currency},{"quote_currency",a.quote_currency},
            {"bid",a.bid},{"ask",a.ask},{"mid",a.mid},
            {"spread_basis_points",a.spread_basis_points},
            {"gross_execution_rate",a.gross_execution_rate},
            {"effective_execution_rate",a.effective_execution_rate},
            {"fee_percent",a.fee_percent},{"fixed_fee_output",a.fixed_fee_output},
            {"fee_currency",a.fee_currency},{"provider_id",a.provider_id},
            {"observation_id",a.observation_id},{"source_uri",a.source_uri},
            {"observed_at_unix",a.observed_at_unix}};
    }
    return j;
}

json history_entry_json(const PortfolioHistoryEntry& e) {
    return {{"transaction",transaction_json(e.transaction)},
        {"fx_pair",e.fx_pair},
        {"price_evidence",e.price_evidence==PortfolioPriceEvidence::ExecutedWithQuoteAudit?
            "executed_with_quote_audit":"recorded_transaction"}};
}

http::response<http::string_body> response(http::status status,const json& body,
    unsigned version,bool keep_alive) {
    http::response<http::string_body> result{status,version};
    result.set(http::field::server,"Monitor-API/1");
    result.set(http::field::content_type,"application/json; charset=utf-8");
    result.set(http::field::cache_control,"no-store");
    result.keep_alive(keep_alive);
    result.body()=body.dump();
    result.prepare_payload();
    return result;
}

json error_json(const char* code,const char* message) {
    return {{"error",{{"code",code},{"message",message}}}};
}
} // namespace

ApiRouter::ApiRouter(JsonPortfolioRepository& repository,std::string bearer_token,
    std::size_t max_body_bytes)
    : repository_(repository),bearer_token_(std::move(bearer_token)),
      max_body_bytes_(max_body_bytes) {
    if(bearer_token_.empty()) throw std::invalid_argument("Token API nie może być pusty.");
    if(max_body_bytes_==0) throw std::invalid_argument("Limit body musi być dodatni.");
}

http::response<http::string_body> ApiRouter::handle(
    const http::request<http::string_body>& request) const {
    const auto version=request.version();
    const auto keep_alive=request.keep_alive();
    if(request.body().size()>max_body_bytes_)
        return response(http::status::payload_too_large,error_json("payload_too_large","Żądanie przekracza limit."),version,keep_alive);
    const std::string_view target{request.target().data(),request.target().size()};
    const auto question=target.find('?');
    const auto path=target.substr(0,question);
    if(path=="/healthz"&&request.method()==http::verb::get)
        return response(http::status::ok,{{"status","ok"},{"api_version",1}},version,keep_alive);
    if(request.method()!=http::verb::get)
        return response(http::status::method_not_allowed,error_json("method_not_allowed","Dozwolona jest wyłącznie metoda GET."),version,keep_alive);
    const auto auth=request[http::field::authorization];
    constexpr std::string_view prefix="Bearer ";
    if(auth.size()<=prefix.size()||std::string_view(auth.data(),prefix.size())!=prefix||
       !token_equal(std::string_view(auth.data()+prefix.size(),auth.size()-prefix.size()),bearer_token_))
        return response(http::status::unauthorized,error_json("unauthorized","Wymagany jest poprawny token Bearer."),version,keep_alive);
    try {
        if(path=="/api/v1/portfolio") {
            const auto snapshot=repository_.load();
            json transactions=json::array();
            for(const auto& item:snapshot.transactions) transactions.push_back(transaction_json(item));
            return response(http::status::ok,{{"schema_version",snapshot.schema_version},
                {"revision",snapshot.revision},{"reporting_currency",snapshot.reporting_currency},
                {"transactions",std::move(transactions)}},version,keep_alive);
        }
        if(path=="/api/v1/portfolio/history") {
            const auto q=parse_query(target);
            PortfolioHistoryPageRequest page;
            if(const auto v=get_integer(q,"api_version")) {
                if(*v>std::numeric_limits<std::uint32_t>::max())
                    throw std::invalid_argument("Nieobsługiwana wersja API.");
                page.api_version=static_cast<std::uint32_t>(*v);
            }
            if(const auto n=get_integer(q,"page_size")) {
                if(*n>std::numeric_limits<std::size_t>::max())
                    throw std::invalid_argument("Nieprawidłowy rozmiar strony.");
                page.page_size=static_cast<std::size_t>(*n);
            }
            page.cursor=get_text(q,"cursor");
            page.filter.asset_code=get_text(q,"asset_code");
            page.filter.fx_pair=get_text(q,"fx_pair");
            page.filter.source_id=get_text(q,"source_id");
            page.filter.operation_id=get_text(q,"operation_id");
            page.filter.executed_from_unix=get_integer(q,"executed_from");
            page.filter.executed_to_unix=get_integer(q,"executed_to");
            const auto snapshot=repository_.load();
            const auto result=get_portfolio_history_page(snapshot,page);
            json entries=json::array();
            for(const auto& entry:result.entries) entries.push_back(history_entry_json(entry));
            json body={{"api_version",result.api_version},{"snapshot_revision",result.snapshot_revision},
                {"total_matching",result.total_matching},{"entries",std::move(entries)}};
            if(result.next_cursor) body["next_cursor"]=*result.next_cursor;
            else body["next_cursor"]=nullptr;
            return response(http::status::ok,body,version,keep_alive);
        }
        return response(http::status::not_found,error_json("not_found","Nie znaleziono zasobu."),version,keep_alive);
    } catch(const std::invalid_argument& e) {
        return response(http::status::bad_request,error_json("invalid_argument",e.what()),version,keep_alive);
    } catch(const std::exception&) {
        return response(http::status::internal_server_error,error_json("internal_error","Wystąpił błąd wewnętrzny."),version,keep_alive);
    }
}

} // namespace monitor::server
