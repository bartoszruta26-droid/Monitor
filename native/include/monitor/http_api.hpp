#pragma once

#include "monitor/portfolio_repository.hpp"
#include "monitor/portfolio_service.hpp"

#include <boost/beast/http.hpp>

#include <cstddef>
#include <string>

namespace monitor::server {

namespace http = boost::beast::http;

class ApiRouter {
public:
    explicit ApiRouter(monitor::market::JsonPortfolioRepository& repository,
        std::string bearer_token, std::size_t max_body_bytes = 65536);

    [[nodiscard]] http::response<http::string_body> handle(
        const http::request<http::string_body>& request) const;

private:
    monitor::market::JsonPortfolioRepository& repository_;
    std::string bearer_token_;
    std::size_t max_body_bytes_;
};

} // namespace monitor::server
