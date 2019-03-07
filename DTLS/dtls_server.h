#ifndef HELPZ_DTLS_SERVER_H
#define HELPZ_DTLS_SERVER_H

#include <boost/asio/steady_timer.hpp>

#include <Helpz/dtls_socket.h>
#include <Helpz/dtls_server_controller.h>

namespace Helpz {
namespace DTLS {

class Server final : public Socket
{
public:
    Server(Tools* dtls_tools, boost::asio::io_context *io_context, uint16_t port, Create_Server_Protocol_Func_T&& create_protocol_func,
           std::chrono::seconds cleaning_timeout, int record_thread_count = 5);

    std::shared_ptr<Server_Node> find_client(std::function<bool(const Network::Protocol *)> check_protocol_func) const;
    void remove_copy(Network::Protocol *client);
private:
    void cleaning(const boost::system::error_code &err);
    const Server_Controller* controller() const;
    Server_Controller* controller();

    std::chrono::seconds cleaning_timeout_;
    boost::asio::steady_timer cleaning_timer_;
};

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_SERVER_H
