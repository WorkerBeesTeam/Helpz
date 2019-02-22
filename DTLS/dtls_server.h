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
    Server(Tools* dtls_tools, boost::asio::io_context& io_context, uint16_t port, const Create_Protocol_Func_T& create_protocol_func, std::chrono::seconds cleaning_timeout);
private:
    void cleaning(const boost::system::error_code &err);
    constexpr Server_Controller* controller();

    std::chrono::seconds cleaning_timeout_;
    boost::asio::steady_timer cleaning_timer_;
};

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_SERVER_H
