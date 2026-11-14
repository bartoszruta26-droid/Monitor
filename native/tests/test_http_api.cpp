#include "monitor/http_api.hpp"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <unistd.h>

namespace {
void require(bool ok,const char* message) {
    if(!ok) { std::cerr<<"FAIL: "<<message<<'\n'; std::exit(EXIT_FAILURE); }
}
using Request=boost::beast::http::request<boost::beast::http::string_body>;
Request get(std::string target,std::string token="test-secret") {
    Request request{boost::beast::http::verb::get,std::move(target),11};
    request.set(boost::beast::http::field::host,"127.0.0.1");
    if(!token.empty()) request.set(boost::beast::http::field::authorization,"Bearer "+token);
    return request;
}
}
int main() {
    using namespace monitor::market;
    namespace http=boost::beast::http;
    const auto dir=std::filesystem::temp_directory_path()/("monitor-http-api-"+std::to_string(::getpid()));
    std::filesystem::remove_all(dir);
    JsonPortfolioRepository repository(dir/"portfolio.json","PLN");
    repository.append(PortfolioTransaction{"eur-1","EUR",PortfolioAssetKind::Currency,
        TradeSide::Buy,100,1,4.2,1,100,"bank"},0);
    repository.append(PortfolioTransaction{"gold-1","XAU",PortfolioAssetKind::Gold,
        TradeSide::Buy,2,0.585,300,0,200,"dealer"},1);
    monitor::server::ApiRouter router(repository,"test-secret",128);

    const auto health=router.handle(get("/healthz",""));
    require(health.result()==http::status::ok,"health is public");
    require(nlohmann::json::parse(health.body())["status"]=="ok","health body");

    auto unauthorized=router.handle(get("/api/v1/portfolio",""));
    require(unauthorized.result()==http::status::unauthorized,"portfolio requires token");
    auto bad_token=router.handle(get("/api/v1/portfolio","wrong"));
    require(bad_token.result()==http::status::unauthorized,"invalid token rejected");

    const auto portfolio=router.handle(get("/api/v1/portfolio"));
    require(portfolio.result()==http::status::ok,"authorized portfolio request");
    const auto portfolio_json=nlohmann::json::parse(portfolio.body());
    require(portfolio_json["revision"]==2&&portfolio_json["transactions"].size()==2,
        "portfolio response includes snapshot revision and ledger");

    const auto history=router.handle(get("/api/v1/portfolio/history?page_size=1"));
    require(history.result()==http::status::ok,"history route");
    const auto history_json=nlohmann::json::parse(history.body());
    require(history_json["total_matching"]==2&&history_json["entries"].size()==1&&
        !history_json["next_cursor"].is_null(),"history pagination delegates to API v1");

    const auto filtered=router.handle(get("/api/v1/portfolio/history?asset_code=XAU"));
    require(nlohmann::json::parse(filtered.body())["total_matching"]==1,"history filters");

    const auto invalid=router.handle(get("/api/v1/portfolio/history?page_size=501"));
    require(invalid.result()==http::status::bad_request,"invalid page size mapped to 400");
    const auto duplicate=router.handle(get("/api/v1/portfolio/history?page_size=1&page_size=2"));
    require(duplicate.result()==http::status::bad_request,"duplicate query parameters rejected");
    const auto unknown=router.handle(get("/api/v1/unknown"));
    require(unknown.result()==http::status::not_found,"unknown path mapped to 404");

    Request post{http::verb::post,"/api/v1/portfolio",11};
    post.set(http::field::authorization,"Bearer test-secret");
    require(router.handle(post).result()==http::status::method_not_allowed,"write methods rejected");
    Request oversized=get("/api/v1/portfolio");
    oversized.body()=std::string(129,'x');
    require(router.handle(oversized).result()==http::status::payload_too_large,"oversized body rejected");
    std::filesystem::remove_all(dir);
    std::cout<<"All HTTP API router tests passed.\n";
}
