#include "monitor/portfolio_repository.hpp"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace net=boost::asio;
namespace beast=boost::beast;
namespace http=beast::http;
using tcp=net::ip::tcp;

int main(int argc,char** argv) {
    if(argc!=2) return 2;
    const auto dir=std::filesystem::temp_directory_path()/("monitor-server-process-"+std::to_string(::getpid()));
    std::filesystem::remove_all(dir);
    pid_t child=-1;
    try {
        monitor::market::JsonPortfolioRepository repository{dir/"portfolio.json","PLN"};
        (void)repository.append(monitor::market::PortfolioTransaction{"process-eur","EUR",
            monitor::market::PortfolioAssetKind::Currency,monitor::market::TradeSide::Buy,
            100,1,4.25,0,100,"bank"},0);

        net::io_context context{1};
        tcp::acceptor probe{context,{net::ip::address_v4::loopback(),0}};
        const auto port=probe.local_endpoint().port();
        probe.close();

        child=::fork();
        if(child<0) throw std::runtime_error("fork server process failed");
        if(child==0) {
            const auto file=(dir/"portfolio.json").string();
            const auto port_text=std::to_string(port);
            ::setenv("MONITOR_API_TOKEN","process-test-token",1);
            ::setenv("MONITOR_PORTFOLIO_FILE",file.c_str(),1);
            ::setenv("MONITOR_PORT",port_text.c_str(),1);
            ::execl(argv[1],argv[1],static_cast<char*>(nullptr));
            ::_exit(127);
        }

        bool received=false;
        for(int attempt=0;attempt<120&&!received;++attempt) {
            tcp::socket socket{context};
            boost::system::error_code ec;
            socket.connect({net::ip::address_v4::loopback(),port},ec);
            if(!ec) {
                http::request<http::empty_body> request{http::verb::get,"/api/v1/portfolio",11};
                request.set(http::field::host,"127.0.0.1");
                request.set(http::field::authorization,"Bearer process-test-token");
                http::write(socket,request,ec);
                if(!ec) {
                    beast::flat_buffer buffer;
                    http::response<http::string_body> response;
                    http::read(socket,buffer,response,ec);
                    if(!ec&&response.result()==http::status::ok) {
                        const auto body=nlohmann::json::parse(response.body());
                        received=body["revision"]==1&&body["transactions"].size()==1;
                    }
                }
            }
            if(!received) std::this_thread::sleep_for(std::chrono::milliseconds{25});
        }
        if(!received) throw std::runtime_error("server process did not serve authenticated portfolio");

        // Verify that the transport enforces the documented HTTP header limit.
        {
            tcp::socket socket{context};
            socket.connect({net::ip::address_v4::loopback(),port});
            const std::string oversized =
                "GET /healthz HTTP/1.1\\r\\nHost: 127.0.0.1\\r\\nX-Oversized: "+
                std::string(17U*1024U,'a')+"\\r\\n\\r\\n";
            net::write(socket,net::buffer(oversized));
            beast::flat_buffer buffer;
            http::response<http::string_body> response;
            boost::system::error_code ec;
            http::read(socket,buffer,response,ec);
            if(ec||response.result()!=http::status::request_header_fields_too_large)
                throw std::runtime_error("oversized HTTP headers were not rejected with 431");
        }

        if(::kill(child,SIGTERM)!=0) throw std::runtime_error("could not send SIGTERM");
        int status{};
        if(::waitpid(child,&status,0)!=child) throw std::runtime_error("waitpid failed");
        child=-1;
        if(!WIFEXITED(status)||WEXITSTATUS(status)!=0)
            throw std::runtime_error("server did not exit cleanly after SIGTERM");
        std::filesystem::remove_all(dir);
        std::cout<<"HTTP server process integration test passed.\n";
        return 0;
    } catch(const std::exception& error) {
        if(child>0) {
            (void)::kill(child,SIGKILL);
            int ignored{};
            (void)::waitpid(child,&ignored,0);
        }
        std::filesystem::remove_all(dir);
        std::cerr<<"FAIL: "<<error.what()<<'\n';
        return 1;
    }
}
