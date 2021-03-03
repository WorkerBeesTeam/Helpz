#ifndef HELPZ_DTLS_CLIENT_THREAD_H
#define HELPZ_DTLS_CLIENT_THREAD_H

#include <vector>
#include <thread>
#include <atomic>

#include <boost/asio/io_context.hpp>

#include <Helpz/dtls_client_controller.h>

namespace Helpz {
namespace Net {
class Protocol;
}
namespace DTLS {

class Client_Thread_Config
{
public:
    Client_Thread_Config(const std::string& tls_police_file_name = {}, const std::string &host = {}, const std::string &port = "25588",
                         const std::vector<std::string> &next_protocols = {},
                         std::chrono::seconds reconnect_interval_sec = std::chrono::seconds{30},
                         std::chrono::milliseconds ocsp_timeout_msec = std::chrono::milliseconds{50},
                         const std::vector<std::string>& cert_paths = { "/usr/share/ca-certificates", "/etc/ssl/certs" });
    Client_Thread_Config(const Client_Thread_Config&) = delete;
    Client_Thread_Config(Client_Thread_Config&&) = default;

    Create_Client_Protocol_Func_T create_protocol_func() const;
    void set_create_protocol_func(Create_Client_Protocol_Func_T create_protocol_func);

    std::chrono::milliseconds ocsp_timeout() const;
    void set_ocsp_timeout(const std::chrono::milliseconds &ocsp_timeout);

    std::chrono::seconds reconnect_interval() const;
    void set_reconnect_interval(const std::chrono::seconds &reconnect_interval);

    std::string tls_police_file_name() const;
    void set_tls_police_file_name(const std::string &tls_police_file_name);

    std::string host() const;
    void set_host(const std::string &host);

    std::string port() const;
    void set_port(const std::string &port);

    std::vector<std::string> next_protocols() const;
    void set_next_protocols(const std::vector<std::string> &next_protocols);

    std::vector<std::string> cert_paths() const;
    void set_cert_paths(const std::vector<std::string> &cert_paths);

private:
    std::chrono::milliseconds _ocsp_timeout;
    std::chrono::seconds reconnect_interval_;
    std::string tls_police_file_name_, host_, port_;
    std::vector<std::string> next_protocols_;
    std::vector<std::string> _cert_paths;
    Create_Client_Protocol_Func_T create_protocol_func_;
};

class Tools;
class Client;
class Client_Thread
{
public:
    Client_Thread(Client_Thread_Config&& conf);
    ~Client_Thread();

    void stop();

    std::shared_ptr<Client> client();

private:
    void run(Client_Thread_Config conf);
    bool start(const std::shared_ptr<Tools> &tools, const Client_Thread_Config& conf);

    bool stop_flag_;
    std::shared_ptr<Client> client_;
    std::thread thread_;

    std::mutex mutex_;
};

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_CLIENT_THREAD_H
