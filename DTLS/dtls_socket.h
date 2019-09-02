#ifndef HELPZ_DTLS_SOCKET_H
#define HELPZ_DTLS_SOCKET_H

#include <memory>

#include <boost/asio/ip/udp.hpp>

#include <Helpz/dtls_controller.h>

namespace Helpz {
namespace DTLS {

class Socket
{
public:
    using udp = boost::asio::ip::udp;

    Socket(boost::asio::io_context *io_context, udp::socket* socket, Controller* controller);
    virtual ~Socket() = default;

    virtual void start_receive(udp::endpoint& remote_endpoint);

    void send(const udp::endpoint& remote_endpoint, const uint8_t* data, std::size_t size);

    boost::asio::io_context* get_io_context();
private:
    void handle_receive(udp::endpoint &remote_endpoint, std::unique_ptr<uint8_t[]> &data, const boost::system::error_code& err,
                        std::size_t size);

    void handle_send(const udp::endpoint& remote_endpoint,
                     std::unique_ptr<uint8_t[]> &data, std::size_t size,
                     const boost::system::error_code& error,
                     const std::size_t & bytes_transferred);

protected:
    virtual void error_message(const std::string& msg);

    std::unique_ptr<udp::socket> socket_;

    std::unique_ptr<Controller> controller_;

private:
    boost::asio::io_context* io_context_;
};

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_SOCKET_H
