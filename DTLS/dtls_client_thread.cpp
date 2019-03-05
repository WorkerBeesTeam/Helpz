#include <iostream>

#include "dtls_client.h"
#include "dtls_client_thread.h"

namespace Helpz {
namespace DTLS {

Client_Thread_Config::Client_Thread_Config(std::shared_ptr<Network::Protocol> protocol, const std::string &tls_police_file_name, const std::string &host,
                                           const std::string &port, const std::vector<std::string> &next_protocols, std::chrono::milliseconds reconnect_interval) :
    protocol_(protocol), reconnect_interval_(reconnect_interval),
    tls_police_file_name_(tls_police_file_name), host_(host), port_(port), next_protocols_(next_protocols)
{
}

Network::Protocol *Client_Thread_Config::protocol() const
{
    return protocol_.get();
}

void Client_Thread_Config::set_protocol(std::shared_ptr<Network::Protocol> protocol)
{
    protocol_ = protocol;
}

std::chrono::milliseconds Client_Thread_Config::reconnect_interval() const
{
    return reconnect_interval_;
}

void Client_Thread_Config::set_reconnect_interval(const std::chrono::milliseconds &reconnect_interval)
{
    reconnect_interval_ = reconnect_interval;
}

std::string Client_Thread_Config::tls_police_file_name() const
{
    return tls_police_file_name_;
}

void Client_Thread_Config::set_tls_police_file_name(const std::string &tls_police_file_name)
{
    tls_police_file_name_ = tls_police_file_name;
}

std::string Client_Thread_Config::host() const
{
    return host_;
}

void Client_Thread_Config::set_host(const std::string &host)
{
    host_ = host;
}

std::string Client_Thread_Config::port() const
{
    return port_;
}

void Client_Thread_Config::set_port(const std::string &port)
{
    port_ = port;
}

std::vector<std::string> Client_Thread_Config::next_protocols() const
{
    return next_protocols_;
}

void Client_Thread_Config::set_next_protocols(const std::vector<std::string> &next_protocols)
{
    next_protocols_ = next_protocols;
}

// ---------------------------------------------------------------------------------------------

Client_Thread::Client_Thread(const Client_Thread_Config& conf) :
    std::thread(&Client_Thread::run, this, conf)
{
}

Client_Thread::~Client_Thread()
{
    if (joinable())
    {
        stop();
        join();
    }
}

void Client_Thread::stop()
{
    stop_flag_ = true;
    if (io_context_)
    {
        io_context_->stop();
    }
}

void Client_Thread::run(const Client_Thread_Config &conf)
{
    io_context_ = nullptr;
    try
    {
        stop_flag_ = false;
        io_context_ = new boost::asio::io_context{};
        Tools dtls_tools{ conf.tls_police_file_name() };

        Client client(&dtls_tools, io_context_, conf.protocol());

        while(!stop_flag_.load())
        {
            try
            {
                client.start_connection(conf.host(), conf.port(), conf.next_protocols());
                io_context_->run();
            }
            catch (std::exception& e)
            {
                std::cerr << "DTLS Client: " << e.what() << std::endl;
                std::this_thread::sleep_for(conf.reconnect_interval());
            }
            catch(...)
            {
                std::cerr << "DTLS Client exception" << std::endl;
                std::this_thread::sleep_for(conf.reconnect_interval());
            }
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "DTLS Client_Thread: " << e.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "DTLS Client_Thread exception" << std::endl;
    }

    delete io_context_;
}

} // namespace DTLS
} // namespace Helpz
