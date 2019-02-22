#ifndef HELPZ_DTLS_CLIENT_THREAD_H
#define HELPZ_DTLS_CLIENT_THREAD_H

#include <thread>
#include <vector>

#include <boost/asio/io_context.hpp>

namespace Helpz {
namespace Network {
class Protocol;
}
namespace DTLS {

class Client_Thread_Config
{
public:
    Client_Thread_Config(std::shared_ptr<Network::Protocol> protocol, const std::string& tls_police_file_name, const std::string &host, const std::string &port, const std::vector<std::string> &next_protocols, std::chrono::milliseconds reconnect_interval = std::chrono::milliseconds(30000));
    Client_Thread_Config(const Client_Thread_Config&) = default;
    Client_Thread_Config(Client_Thread_Config&&) = default;

    Network::Protocol *protocol() const;
    void set_protocol(std::shared_ptr<Network::Protocol> protocol);

    std::chrono::milliseconds reconnect_interval() const;
    void set_reconnect_interval(const std::chrono::milliseconds &reconnect_interval);

    std::string tls_police_file_name() const;
    void set_tls_police_file_name(const std::string &tls_police_file_name);

    std::string host() const;
    void set_host(const std::string &host);

    std::string port() const;
    void set_port(const std::string &port);

    std::vector<std::string> next_protocols() const;
    void set_next_protocols(const std::vector<std::string> &next_protocols);

private:
    std::shared_ptr<Network::Protocol> protocol_;
    std::chrono::milliseconds reconnect_interval_;
    std::string tls_police_file_name_, host_, port_;
    std::vector<std::string> next_protocols_;
};

class Client_Thread : public std::thread
{
public:
    Client_Thread(const Client_Thread_Config &conf);
    ~Client_Thread();

    void stop();

private:
    void run(const Client_Thread_Config& conf);

    boost::asio::io_context* io_context_;
    std::atomic<bool> stop_flag_;
};

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_CLIENT_THREAD_H
