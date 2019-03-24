#include <iostream>
#include <vector>

#include "dtls_tools.h"
#include "dtls_server.h"
#include "dtls_server_thread.h"

namespace Helpz {
namespace DTLS {

Server_Thread_Config::Server_Thread_Config(uint16_t port, const std::string &tls_police_file_name, const std::string &certificate_file_name,
                                           const std::string &certificate_key_file_name, uint32_t cleaning_timeout_sec, uint16_t receive_thread_count,
                                           uint16_t record_thread_count) :
    port_(port), receive_thread_count_(receive_thread_count), record_thread_count_(record_thread_count), cleaning_timeout_(std::chrono::seconds{cleaning_timeout_sec}),
    tls_police_file_name_(tls_police_file_name), certificate_file_name_(certificate_file_name), certificate_key_file_name_(certificate_key_file_name)
{
}

Create_Server_Protocol_Func_T &&Server_Thread_Config::create_protocol_func()
{
    return std::move(create_protocol_func_);
}

void Server_Thread_Config::set_create_protocol_func(Create_Server_Protocol_Func_T &&create_protocol_func)
{
    create_protocol_func_ = std::move(create_protocol_func);
}

std::chrono::seconds Server_Thread_Config::cleaning_timeout() const
{
    return cleaning_timeout_;
}

void Server_Thread_Config::set_cleaning_timeout(const std::chrono::seconds &cleaning_timeout)
{
    cleaning_timeout_ = cleaning_timeout;
}

uint16_t Server_Thread_Config::port() const
{
    return port_;
}

void Server_Thread_Config::set_port(const uint16_t &port)
{
    port_ = port;
}

std::string Server_Thread_Config::tls_police_file_name() const
{
    return tls_police_file_name_;
}

void Server_Thread_Config::set_tls_police_file_name(const std::string &tls_police_file_name)
{
    tls_police_file_name_ = tls_police_file_name;
}

std::string Server_Thread_Config::certificate_file_name() const
{
    return certificate_file_name_;
}

void Server_Thread_Config::set_certificate_file_name(const std::string &certificate_file_name)
{
    certificate_file_name_ = certificate_file_name;
}

std::string Server_Thread_Config::certificate_key_file_name() const
{
    return certificate_key_file_name_;
}

void Server_Thread_Config::set_certificate_key_file_name(const std::string &certificate_key_file_name)
{
    certificate_key_file_name_ = certificate_key_file_name;
}

uint16_t Server_Thread_Config::receive_thread_count() const
{
    return receive_thread_count_;
}

void Server_Thread_Config::set_receive_thread_count(const uint16_t &receive_thread_count)
{
    receive_thread_count_ = receive_thread_count;
}

uint16_t Server_Thread_Config::record_thread_count() const
{
    return record_thread_count_;
}

void Server_Thread_Config::set_record_thread_count(const uint16_t &record_thread_count)
{
    record_thread_count_ = record_thread_count;
}

// -------------------------------------------------------------------------------------------------------------------

Server_Thread::Server_Thread(Server_Thread_Config&& conf) :
    std::thread(&Server_Thread::run, this, std::move(conf))
{
}

Server_Thread::~Server_Thread()
{
    if (joinable())
    {
        stop();
        join();
    }
}

void Server_Thread::stop()
{
    if (io_context_)
    {
        io_context_->stop();
    }
}

Server *Server_Thread::server()
{
    return server_.load();
}

void Server_Thread::run(Server_Thread_Config&& conf)
{
    io_context_ = nullptr;
    std::vector<std::thread> additional_threads;

    try
    {
        io_context_ = new boost::asio::io_context{};
        Tools dtls_tools{ conf.tls_police_file_name(), conf.certificate_file_name(), conf.certificate_key_file_name() };

        Server server(&dtls_tools, io_context_, conf.port(), std::move(conf.create_protocol_func()), conf.cleaning_timeout(), conf.record_thread_count());
        server_.store(&server);

        boost::asio::ip::udp::endpoint remote_endpoint;
        server.start_receive(remote_endpoint);

        for (uint16_t i = 1; i < conf.receive_thread_count(); ++i)
        {
            additional_threads.emplace_back(std::thread{&Server_Thread::run_context, this, i});
        }
        run_context(0);
    }
    catch (std::exception& e)
    {
        std::cerr << "DTLS Server_Thread: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "DTLS Server_Thread exception" << std::endl;
    }

    if (!io_context_->stopped())
    {
        io_context_->stop();
    }

    for (std::thread& thread: additional_threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    delete io_context_;
}

void Server_Thread::run_context(uint16_t thread_number)
{
    try
    {
        io_context_->run();
    }
    catch (std::exception& e)
    {
        std::cerr << "DTLS Server_Thread: " << e.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "DTLS Server_Thread exception" << std::endl;
    }
    std::cerr << ("DTLS Server_Thread #" + std::to_string(thread_number) + " finished.\n");
}

} // namespace DTLS
} // namespace Helpz
