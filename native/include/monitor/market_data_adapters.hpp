#pragma once

#include "monitor/market_data.hpp"

#include <chrono>
#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace monitor::market {

struct HttpRequest {
    std::string url;
    std::map<std::string, std::string> headers;
    std::chrono::milliseconds timeout{30000};
    std::size_t max_response_bytes{32U * 1024U * 1024U};
};

struct HttpResponse {
    long status_code{};
    std::map<std::string, std::string> headers;
    std::string body;
};

class HttpTransport {
public:
    virtual ~HttpTransport() = default;
    [[nodiscard]] virtual HttpResponse get(const HttpRequest& request) = 0;
};

class CurlHttpTransport final : public HttpTransport {
public:
    [[nodiscard]] HttpResponse get(const HttpRequest& request) override;
};

struct RetryPolicy {
    unsigned max_attempts{4};
    std::chrono::milliseconds initial_backoff{250};
    std::chrono::milliseconds max_backoff{4000};
};

struct HttpClientConfig {
    RetryPolicy retry;
    std::chrono::milliseconds minimum_request_interval{100};
    std::chrono::milliseconds default_timeout{30000};
    std::size_t max_json_bytes{8U * 1024U * 1024U};
    std::size_t max_workbook_bytes{32U * 1024U * 1024U};
};

class HttpClient {
public:
    explicit HttpClient(std::shared_ptr<HttpTransport> transport,
                        HttpClientConfig config = {});
    [[nodiscard]] HttpResponse get(const std::string& url,
                                   std::size_t max_bytes = 0);
private:
    void throttle();
    std::shared_ptr<HttpTransport> transport_;
    HttpClientConfig config_;
    std::mutex throttle_mutex_;
    std::chrono::steady_clock::time_point next_request_{};
};

struct AdapterResult {
    std::vector<Instrument> instruments;
    std::vector<Provider> providers;
    std::vector<MarketObservation> observations;
};

class ProviderAdapter {
public:
    virtual ~ProviderAdapter() = default;
    [[nodiscard]] virtual std::string id() const = 0;
    [[nodiscard]] virtual AdapterResult fetch(const CivilDate& start,
                                               const CivilDate& end) = 0;
};

class NbpAdapter final : public ProviderAdapter {
public:
    explicit NbpAdapter(HttpClient& http);
    [[nodiscard]] std::string id() const override { return "nbp"; }
    [[nodiscard]] AdapterResult fetch(const CivilDate& start,
                                      const CivilDate& end) override;
private:
    HttpClient& http_;
};

class FrankfurterAdapter final : public ProviderAdapter {
public:
    explicit FrankfurterAdapter(HttpClient& http,
                                std::string provider_filter = {});
    [[nodiscard]] std::string id() const override { return "frankfurter"; }
    [[nodiscard]] AdapterResult fetch(const CivilDate& start,
                                      const CivilDate& end) override;
private:
    HttpClient& http_;
    std::string provider_filter_;
};

class WorldBankPinkSheetAdapter final : public ProviderAdapter {
public:
    explicit WorldBankPinkSheetAdapter(HttpClient& http);
    [[nodiscard]] std::string id() const override { return "world-bank-pink-sheet"; }
    [[nodiscard]] AdapterResult fetch(const CivilDate& start,
                                      const CivilDate& end) override;
private:
    HttpClient& http_;
};

class XausSpotAdapter final : public ProviderAdapter {
public:
    explicit XausSpotAdapter(HttpClient& http);
    [[nodiscard]] std::string id() const override { return "xaus"; }
    [[nodiscard]] AdapterResult fetch(const CivilDate& start,
                                      const CivilDate& end) override;
    [[nodiscard]] AdapterResult fetch_latest();
private:
    HttpClient& http_;
};

[[nodiscard]] std::vector<MarketObservation>
deduplicate_observations(std::vector<MarketObservation> observations);

} // namespace monitor::market
