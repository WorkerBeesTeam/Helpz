#ifndef DTLS_CLIENT_H
#define DTLS_CLIENT_H

#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <Helpz/dtls_tools.h>
#include <Helpz/dtls_client_controller.h>
#include <Helpz/dtls_socket.h>

namespace Helpz {
namespace DTLS {

class Client final : public Socket
{
public:
    Client(Tools *dtls_tools, boost::asio::io_context* io_context, Network::Protocol* protocol);

    ~Client()
    {
    }

    void start_connection(const std::string& host, const std::string& port, const std::vector<std::string> &next_protocols = {});
private:
    constexpr Client_Controller* controller();
    void check_deadline();

    void start_receive(udp::endpoint& remote_endpoint) override;
    void error_message(const std::string& msg) override;

    boost::asio::deadline_timer deadline_;
    udp::endpoint remote_endpoint_;
};

} // namespace DTLS
} // namespace Helpz

#endif // DTLS_CLIENT_H
