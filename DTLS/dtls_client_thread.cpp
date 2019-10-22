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

Create_Client_Protocol_Func_T Client_Thread_Config::create_protocol_func() const
{
    return create_protocol_func_;
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

Client_Thread::Client_Thread(Client_Thread_Config &&conf) :
    stop_flag_(false)
{
    std::thread thread(&Client_Thread::run, this, std::move(conf));
    thread_.swap(thread);
}

Client_Thread::~Client_Thread()
{
    if (thread_.joinable())
    {
        stop();
        thread_.join();
    }
}

void Client_Thread::stop()
{
    std::lock_guard lock(mutex_);
    stop_flag_ = true;
    if (client_)
    {
        client_->close();
    }
}

std::shared_ptr<Client> Client_Thread::client()
{
    std::lock_guard lock(mutex_);
    return client_;
}

void Client_Thread::run(Client_Thread_Config conf)
{
    try
    {
        bool is_first_connect = true;
        std::shared_ptr<Tools> tools{new Tools{conf.tls_police_file_name()}};

        while(!stop_flag_)
        {
            try
            {
                if (is_first_connect)
                {
                    is_first_connect = false;
                }
                else
                {
                    std::this_thread::sleep_for(conf.reconnect_interval());
                }

                if (start(tools, conf))
                {
                    client_->run();
                }
            }
            catch (std::exception& e)
            {
                std::cerr << "DTLS Client: " << e.what() << " reconnect in: " << conf.reconnect_interval().count() << "s" << std::endl;
            }
            catch(...)
            {
                std::cerr << "DTLS Client exception. reconnect in: " << conf.reconnect_interval().count() << "s" << std::endl;
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

    stop();
    if (client_)
    {
        client_.reset();
    }
}

bool Client_Thread::start(const std::shared_ptr<Tools>& tools, const Client_Thread_Config& conf)
{
    std::lock_guard lock(mutex_);
    if (stop_flag_)
    {
        return false;
    }

    if (client_)
    {
        client_->close();
    }

    client_ = std::make_shared<Client>(tools, conf.create_protocol_func());

    std::cout << "try connect to " << conf.host() << ':' << conf.port() << std::endl;
    client_->start_connection(conf.host(), conf.port(), conf.next_protocols());
    return !stop_flag_;
}

} // namespace DTLS
} // namespace Helpz
