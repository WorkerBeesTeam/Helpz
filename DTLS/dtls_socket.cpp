#include <iostream>

#include "dtls_socket.h"

#define MAX_UDP_PACKET_SIZE 65507

namespace Helpz {
namespace DTLS {

Socket::Socket(boost::asio::io_context &io_context, udp::socket *socket, Controller *controller) :
    io_context_(&io_context), socket_(socket), controller_(controller)
{
}

void Socket::start_receive(udp::endpoint& remote_endpoint)
{
    std::unique_ptr<uint8_t[]> recv_buffer(new uint8_t[ MAX_UDP_PACKET_SIZE ]);
    auto buffer = boost::asio::buffer(recv_buffer.get(), MAX_UDP_PACKET_SIZE);

    socket_->async_receive_from(
                std::move(buffer), remote_endpoint,
                std::bind(&Socket::handle_receive, this,
                          std::ref(remote_endpoint), std::move(recv_buffer),
                          std::placeholders::_1,   // boost::asio::placeholders::error,
                          std::placeholders::_2)); // boost::asio::placeholders::bytes_transferred
}

void Socket::send(const udp::endpoint &remote_endpoint, const uint8_t *data, std::size_t size)
{
    std::unique_ptr<uint8_t[]> send_buffer(new uint8_t[ size ]);
    memcpy(send_buffer.get(), data, size);
    auto buffer = boost::asio::buffer(send_buffer.get(), size);

    socket_->async_send_to(std::move(buffer), remote_endpoint,
                           std::bind(&Socket::handle_send, this, remote_endpoint,
                                     std::move(send_buffer), size,
                                     std::placeholders::_1,   // boost::asio::placeholders::error,
                                     std::placeholders::_2)); // boost::asio::placeholders::bytes_transferred
}

void Socket::handle_receive(udp::endpoint& remote_endpoint, std::unique_ptr<uint8_t[]> &data, const boost::system::error_code &err, std::size_t size)
{
    if (err)
    {
        error_message(std::string("RECV ERROR ") + err.category().name() + ": " + err.message());
        return;
    }

    controller_->process_data(remote_endpoint, std::move(data), size);
    remote_endpoint = udp::endpoint();
    start_receive(remote_endpoint);
}

void Socket::handle_send(const boost::asio::ip::udp::endpoint &remote_endpoint, std::unique_ptr<uint8_t[]> &data, std::size_t size, const boost::system::error_code &error, const std::size_t &bytes_transferred)
{
    if (error.value() != 0)
    {
        std::stringstream strm;
        strm << remote_endpoint << " SEND ERROR " << error.category().name() << ": "
             << error.message() << " size: " << size;
        error_message(strm.str());
    }
    else if (size != bytes_transferred)
    {
        std::stringstream strm;
        strm << remote_endpoint << " SEND ERROR wrong size " << size
             << " transfered: " << bytes_transferred;
        error_message(strm.str());
    }
    else if (!data)
    {
        std::stringstream strm;
        strm << remote_endpoint << " SEND ERROR empty data pointer";
        error_message(strm.str());
    }
}

void Socket::error_message(const std::string &msg)
{
    std::cerr << msg << std::endl;
}

} // namespace DTLS
} // namespace Helpz
