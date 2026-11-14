#include "monitor/http_api.hpp"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>

namespace net=boost::asio;
namespace beast=boost::beast;
namespace http=beast::http;
using tcp=net::ip::tcp;

namespace {
void require(bool ok,const char* message) {
    if(!ok) throw std::runtime_error(message);
}
}

int main() {
    using namespace monitor::market;
    const auto dir=std::filesystem::temp_directory_path()/("monitor-http-tcp-"+std::to_string(::getpid()));
    std::filesystem::remove_all(dir);
    try {
        JsonPortfolioRepository repository(dir/"portfolio.json","PLN");
        (void)repository.append(PortfolioTransaction{"tcp-eur","EUR",PortfolioAssetKind::Currency,
            TradeSide::Buy,250,1,4.3,0,100,"bank"},0);
        monitor::server::ApiRouter router(repository,"tcp-test-token");

        net::io_context context{1};
        tcp::acceptor acceptor{context,{net::ip::address_v4::loopback(),0}};
        const auto endpoint=acceptor.local_endpoint();
        std::exception_ptr server_error;
        std::thread server([&] {
            try {
                tcp::socket socket{context};
                acceptor.accept(socket);
                beast::flat_buffer buffer;
                http::request<http::string_body> request;
                http::read(socket,buffer,request);
                auto response=router.handle(request);
                response.keep_alive(false);
                http::write(socket,response);
                boost::system::error_code ignored;
                socket.shutdown(tcp::socket::shutdown_both,ignored);
                socket.close(ignored);
            } catch(...) { server_error=std::current_exception(); }
        });

        tcp::socket client{context};
        client.connect(endpoint);
        http::request<http::empty_body> request{http::verb::get,"/api/v1/portfolio",11};
        request.set(http::field::host,"127.0.0.1");
        request.set(http::field::authorization,"Bearer tcp-test-token");
        http::write(client,request);
        beast::flat_buffer response_buffer;
        http::response<http::string_body> response;
        http::read(client,response_buffer,response);
        server.join();
        if(server_error) std::rethrow_exception(server_error);

        require(response.result()==http::status::ok,"TCP HTTP request returns 200");
        const auto body=nlohmann::json::parse(response.body());
        require(body["revision"]==1,"TCP response preserves portfolio revision");
        require(body["transactions"].size()==1,"TCP response serializes stored transaction");
        require(response[http::field::cache_control]=="no-store","portfolio response disables caching");
        std::filesystem::remove_all(dir);
        std::cout<<"HTTP TCP integration test passed.\n";
        return 0;
    } catch(const std::exception& error) {
        std::filesystem::remove_all(dir);
        std::cerr<<"FAIL: "<<error.what()<<'\n';
        return 1;
    }
}
