#include <iostream>

#include "dtls_client.h"
#include "dtls_client_thread.h"

namespace Helpz {
namespace DTLS {

Client_Thread_Config::Client_Thread_Config(const std::string &tls_police_file_name, const std::string &host,
                                           const std::string &port, const std::vector<std::string> &next_protocols, uint32_t reconnect_interval_sec) :
    reconnect_interval_(reconnect_interval_sec),
    tls_police_file_name_(tls_police_file_name), host_(host), port_(port), next_protocols_(next_protocols)
{
}

Create_Client_Protocol_Func_T &&Client_Thread_Config::create_protocol_func()
{
    return std::move(create_protocol_func_);
}

void Client_Thread_Config::set_create_protocol_func(Create_Client_Protocol_Func_T &&create_protocol_func)
{
    create_protocol_func_ = std::move(create_protocol_func);
}

std::chrono::seconds Client_Thread_Config::reconnect_interval() const
{
    return reconnect_interval_;
}

void Client_Thread_Config::set_reconnect_interval(const std::chrono::seconds &reconnect_interval)
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

Client_Thread::Client_Thread(Client_Thread_Config &&conf)
{
    thread_ = new std::thread(&Client_Thread::run, this, std::move(conf));
}

Client_Thread::~Client_Thread()
{
    if (thread_->joinable())
    {
        stop();
        thread_->join();
    }

    delete thread_;
}

void Client_Thread::stop()
{
    stop_flag_ = true;
    std::lock_guard lock(mutex_);
    if (io_context_)
    {
        io_context_->stop();
    }
}

std::shared_ptr<Client> Client_Thread::client()
{
    std::lock_guard lock(mutex_);
    return client_;
}

void Client_Thread::run(Client_Thread_Config conf)
{
    io_context_ = nullptr;
    try
    {
        stop_flag_ = false;
        Tools dtls_tools{ conf.tls_police_file_name() };

        while(!stop_flag_.load())
        {
            try
            {
                {
                    std::lock_guard lock(mutex_);
                    client_.reset();
                    if (io_context_)
                        delete io_context_;
                    io_context_ = new boost::asio::io_context{};
                    client_ = std::make_shared<Client>(&dtls_tools, io_context_, conf.create_protocol_func());
                }

                std::cout << "try connect to " << conf.host() << ':' << conf.port() << std::endl;
                client_->start_connection(conf.host(), conf.port(), conf.next_protocols());
                io_context_->run();
            }
            catch (std::exception& e)
            {
                std::cerr << "DTLS Client: " << e.what() << " reconnect in: " << conf.reconnect_interval().count() << "s" << std::endl;
                std::this_thread::sleep_for(conf.reconnect_interval());
            }
            catch(...)
            {
                std::cerr << "DTLS Client exception. reconnect in: " << conf.reconnect_interval().count() << "s" << std::endl;
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

    client_.reset();
    delete io_context_;
}

} // namespace DTLS
} // namespace Helpz
