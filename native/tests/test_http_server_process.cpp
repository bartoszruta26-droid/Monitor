#include "monitor/portfolio_repository.hpp"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <sys/socket.h>
#include <sys/time.h>
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

        // Startup must fail closed when required configuration is missing or invalid.
        const auto expect_start_failure=[&](bool provide_token,bool provide_file,
            const char* configured_port,const char* scenario,int expected_exit) {
            const pid_t probe_child=::fork();
            if(probe_child<0) throw std::runtime_error("fork startup validation process failed");
            if(probe_child==0) {
                if(provide_token) ::setenv("MONITOR_API_TOKEN","startup-test-token",1);
                else ::unsetenv("MONITOR_API_TOKEN");
                if(provide_file) {
                    const auto file=(dir/"portfolio.json").string();
                    ::setenv("MONITOR_PORTFOLIO_FILE",file.c_str(),1);
                } else ::unsetenv("MONITOR_PORTFOLIO_FILE");
                if(configured_port!=nullptr) ::setenv("MONITOR_PORT",configured_port,1);
                else ::unsetenv("MONITOR_PORT");
                ::execl(argv[1],argv[1],static_cast<char*>(nullptr));
                ::_exit(127);
            }
            int probe_status{};
            if(::waitpid(probe_child,&probe_status,0)!=probe_child)
                throw std::runtime_error(std::string("waitpid failed for ")+scenario);
            if(!WIFEXITED(probe_status)||WEXITSTATUS(probe_status)!=expected_exit)
                throw std::runtime_error(std::string("unexpected startup result: ")+scenario);
        };
        expect_start_failure(false,true,nullptr,"missing API token",2);
        expect_start_failure(true,false,nullptr,"missing portfolio file",2);
        expect_start_failure(true,true,"0","invalid port",1);
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

        // The real process must reject requests without a Bearer credential.
        {
            tcp::socket socket{context};
            socket.connect({net::ip::address_v4::loopback(),port});
            http::request<http::empty_body> request{http::verb::get,"/api/v1/portfolio",11};
            request.set(http::field::host,"127.0.0.1");
            http::write(socket,request);
            beast::flat_buffer buffer;
            http::response<http::string_body> response;
            http::read(socket,buffer,response);
            if(response.result()!=http::status::unauthorized)
                throw std::runtime_error("missing Bearer token was not rejected with 401");
        }

        // A second process cannot take over the port already owned by this server.
        {
            const pid_t competing=::fork();
            if(competing<0) throw std::runtime_error("fork competing server failed");
            if(competing==0) {
                const auto file=(dir/"portfolio.json").string();
                const auto port_text=std::to_string(port);
                ::setenv("MONITOR_API_TOKEN","competing-test-token",1);
                ::setenv("MONITOR_PORTFOLIO_FILE",file.c_str(),1);
                ::setenv("MONITOR_PORT",port_text.c_str(),1);
                ::execl(argv[1],argv[1],static_cast<char*>(nullptr));
                ::_exit(127);
            }
            int competing_status{};
            if(::waitpid(competing,&competing_status,0)!=competing||
               !WIFEXITED(competing_status)||WEXITSTATUS(competing_status)!=1)
                throw std::runtime_error("server unexpectedly reused an occupied loopback port");
        }

        // Content-Length above the configured 64 KiB limit must fail immediately.
        {
            tcp::socket socket{context};
            socket.connect({net::ip::address_v4::loopback(),port});
            const std::string oversized_body=
                "POST /healthz HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Length: 65537\r\n\r\n";
            net::write(socket,net::buffer(oversized_body));
            beast::flat_buffer buffer;
            http::response<http::string_body> response;
            boost::system::error_code ec;
            http::read(socket,buffer,response,ec);
            if(ec||response.result()!=http::status::payload_too_large)
                throw std::runtime_error("oversized HTTP body was not rejected with 413");
        }

        // Malformed request lines must produce a bounded 400 response.
        {
            tcp::socket socket{context};
            socket.connect({net::ip::address_v4::loopback(),port});
            const std::string malformed=
                "G@T /healthz HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
            net::write(socket,net::buffer(malformed));
            beast::flat_buffer buffer;
            http::response<http::string_body> response;
            boost::system::error_code ec;
            http::read(socket,buffer,response,ec);
            if(ec||response.result()!=http::status::bad_request)
                throw std::runtime_error("malformed HTTP request was not rejected with 400");
        }

        // A client that leaves the request header incomplete is timed out by the
        // kernel receive timeout; the process returns HTTP 408 and remains usable.
        {
            tcp::socket socket{context};
            socket.connect({net::ip::address_v4::loopback(),port});
            const timeval client_timeout{20,0};
            if(::setsockopt(socket.native_handle(),SOL_SOCKET,SO_RCVTIMEO,
                    &client_timeout,sizeof(client_timeout))!=0)
                throw std::runtime_error("could not set timeout for HTTP timeout test");
            const std::string incomplete=
                "GET /healthz HTTP/1.1\r\nHost: 127.0.0.1\r\n";
            net::write(socket,net::buffer(incomplete));
            beast::flat_buffer buffer;
            http::response<http::string_body> response;
            boost::system::error_code ec;
            http::read(socket,buffer,response,ec);
            if(ec||response.result()!=http::status::request_timeout)
                throw std::runtime_error("incomplete HTTP request was not rejected with 408");
        }

        // Verify that the transport enforces the documented HTTP header limit.
        {
            tcp::socket socket{context};
            socket.connect({net::ip::address_v4::loopback(),port});
            const std::string oversized =
                "GET /healthz HTTP/1.1\r\nHost: 127.0.0.1\r\nX-Oversized: "+
                std::string(17U*1024U,'a')+"\r\n\r\n";
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
