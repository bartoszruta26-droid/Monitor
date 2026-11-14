#include "monitor/http_api.hpp"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <utility>

namespace net=boost::asio;
namespace beast=boost::beast;
namespace http=beast::http;
using tcp=net::ip::tcp;

namespace {
volatile std::sig_atomic_t keep_running=1;
void request_stop(int) { keep_running=0; }

unsigned short configured_port() {
    const char* value=std::getenv("MONITOR_PORT");
    if(value==nullptr||*value=='\0') return 8080;
    unsigned int parsed{};
    const std::string text{value};
    const auto result=std::from_chars(text.data(),text.data()+text.size(),parsed);
    if(result.ec!=std::errc{}||result.ptr!=text.data()+text.size()||parsed==0||parsed>65535)
        throw std::runtime_error("MONITOR_PORT musi być liczbą z zakresu 1–65535.");
    return static_cast<unsigned short>(parsed);
}

void send_error(tcp::socket& socket,http::status status,const char* code,const char* message) {
    http::response<http::string_body> response{status,11};
    response.set(http::field::server,"Monitor-API/1");
    response.set(http::field::content_type,"application/json; charset=utf-8");
    response.set(http::field::cache_control,"no-store");
    response.keep_alive(false);
    response.body()="{\"error\":{\"code\":\""+std::string(code)+"\",\"message\":\""+
        std::string(message)+"\"}}";
    response.prepare_payload();
    boost::system::error_code ignored;
    http::write(socket,response,ignored);
}
}

int main() {
    try {
        const char* token=std::getenv("MONITOR_API_TOKEN");
        const char* portfolio_path=std::getenv("MONITOR_PORTFOLIO_FILE");
        if(token==nullptr||*token=='\0') {
            std::cerr<<"Brak MONITOR_API_TOKEN — serwer odmawia uruchomienia.\n";
            return 2;
        }
        if(portfolio_path==nullptr||*portfolio_path=='\0') {
            std::cerr<<"Brak MONITOR_PORTFOLIO_FILE — serwer odmawia uruchomienia.\n";
            return 2;
        }

        monitor::market::JsonPortfolioRepository repository{
            std::filesystem::path{portfolio_path}};
        monitor::server::ApiRouter router{repository,token,65536};

        net::io_context context{1};
        tcp::acceptor acceptor{context};
        boost::system::error_code ec;
        acceptor.open(tcp::v4(),ec);
        if(ec) throw boost::system::system_error(ec);
        acceptor.set_option(net::socket_base::reuse_address(true),ec);
        if(ec) throw boost::system::system_error(ec);
        // Deliberately bind only to IPv4 loopback. Remote access is unavailable
        // until a separately reviewed TLS deployment mode is implemented.
        acceptor.bind(tcp::endpoint{net::ip::address_v4::loopback(),configured_port()},ec);
        if(ec) throw boost::system::system_error(ec);
        acceptor.listen(net::socket_base::max_listen_connections,ec);
        if(ec) throw boost::system::system_error(ec);
        acceptor.non_blocking(true,ec);
        if(ec) throw boost::system::system_error(ec);

        std::signal(SIGINT,request_stop);
        std::signal(SIGTERM,request_stop);
        std::cout<<"Monitor API nasłuchuje wyłącznie na 127.0.0.1:"
                 <<configured_port()<<"\n";

        while(keep_running!=0) {
            tcp::socket socket{context};
            acceptor.accept(socket,ec);
            if(ec==net::error::would_block||ec==net::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds{40});
                continue;
            }
            if(ec) {
                if(keep_running!=0) std::cerr<<"Błąd accept: "<<ec.message()<<"\n";
                ec.clear();
                continue;
            }

            beast::tcp_stream stream{std::move(socket)};
            stream.expires_after(std::chrono::seconds{15});
            beast::flat_buffer buffer;
            http::request_parser<http::string_body> parser;
            parser.header_limit(16U*1024U);
            parser.body_limit(65536U);
            http::read(stream,buffer,parser,ec);
            if(ec==http::error::body_limit) {
                send_error(stream.socket(),http::status::payload_too_large,
                    "payload_too_large","Żądanie przekracza limit.");
            } else if(ec) {
                send_error(stream.socket(),http::status::bad_request,
                    "bad_request","Nieprawidłowe żądanie HTTP.");
            } else {
                auto request=parser.release();
                auto response=router.handle(request);
                response.keep_alive(false);
                stream.expires_after(std::chrono::seconds{15});
                http::write(stream,response,ec);
            }
            stream.socket().shutdown(tcp::socket::shutdown_both,ec);
            stream.socket().close(ec);
        }
        acceptor.close(ec);
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"Monitor API: "<<error.what()<<"\n";
        return 1;
    }
}
