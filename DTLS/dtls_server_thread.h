#ifndef HELPZ_DTLS_SERVER_THREAD_H
#define HELPZ_DTLS_SERVER_THREAD_H

#include <thread>
#include <atomic>

#include <boost/asio/io_context.hpp>

#include <Helpz/dtls_server_controller.h>

namespace Helpz {
namespace DTLS {

class Server_Thread_Config
{
public:
    Server_Thread_Config(uint16_t port, const std::string& tls_police_file_name, const std::string& certificate_file_name, const std::string& certificate_key_file_name,
                         uint32_t cleaning_timeout_sec = 3 * 60, uint16_t receive_thread_count = 5, uint16_t record_thread_count = 5);
    Server_Thread_Config(Server_Thread_Config&&) = default;
    Server_Thread_Config(const Server_Thread_Config&) = delete;

    Create_Server_Protocol_Func_T&& create_protocol_func();
    void set_create_protocol_func(Create_Server_Protocol_Func_T &&create_protocol_func);

    std::chrono::seconds cleaning_timeout() const;
    void set_cleaning_timeout(const std::chrono::seconds &cleaning_timeout);

    uint16_t port() const;
    void set_port(const uint16_t &port);

    std::string tls_police_file_name() const;
    void set_tls_police_file_name(const std::string &tls_police_file_name);

    std::string certificate_file_name() const;
    void set_certificate_file_name(const std::string &certificate_file_name);

    std::string certificate_key_file_name() const;
    void set_certificate_key_file_name(const std::string &certificate_key_file_name);

    uint16_t receive_thread_count() const;
    void set_receive_thread_count(const uint16_t &receive_thread_count);

    uint16_t record_thread_count() const;
    void set_record_thread_count(const uint16_t &record_thread_count);

private:
    uint16_t port_, receive_thread_count_, record_thread_count_;
    std::chrono::seconds cleaning_timeout_;
    std::string tls_police_file_name_, certificate_file_name_, certificate_key_file_name_;
    Create_Server_Protocol_Func_T create_protocol_func_;
};

class Server;
class Server_Thread
{
public:
    Server_Thread(Server_Thread_Config&& conf);

    ~Server_Thread();

    void stop();

    Server* server();
private:
    void run(Server_Thread_Config conf);

    void run_context(uint16_t thread_number);

    boost::asio::io_context* io_context_;
    std::atomic<Server*> server_;
    std::thread* thread_;
};

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_SERVER_THREAD_H
