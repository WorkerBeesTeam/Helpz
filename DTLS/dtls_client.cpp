#include <iostream>

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/bind.hpp>

#include "dtls_client.h"

namespace Helpz {
namespace DTLS {

using boost::asio::ip::udp;

Client::Client(Tools *dtls_tools, boost::asio::io_context *io_context, Network::Protocol *protocol) :
    Socket{io_context, new udp::socket{*io_context}, new Client_Controller{dtls_tools, this, protocol}},
    deadline_{*io_context}
{
    deadline_.expires_at(boost::posix_time::pos_infin);
    check_deadline();
}

void Client::start_connection(const std::string &host, const std::string &port, const std::vector<std::string> &next_protocols)
{
    if (socket_->is_open())
    {
        socket_->close();
    }

    udp::resolver resolver(*io_context_);
    udp::resolver::query query(udp::v4(), host, port);
    udp::endpoint receiver_endpoint = *resolver.resolve(query);

    socket_->open(udp::v4());

    controller()->start(host, receiver_endpoint, next_protocols);

    start_receive(remote_endpoint_);
}

constexpr Client_Controller *Client::controller()
{
    return static_cast<Client_Controller*>(controller_);
}

void Client::check_deadline()
{
    // Check whether the deadline has passed. We compare the deadline against
    // the current time since a new asynchronous operation may have moved the
    // deadline before this actor had a chance to run.
    if (deadline_.expires_at() <= boost::asio::deadline_timer::traits_type::now())
    {
        if (controller()->is_reconnect_needed())
        {
            // The deadline has passed. The outstanding asynchronous operation needs
            // to be cancelled so that the blocked receive() function will return.
            //
            // Please note that cancel() has portability issues on some versions of
            // Microsoft Windows, and it may be necessary to use close() instead.
            // Consult the documentation for cancel() for further information.
            socket_->cancel();

            // There is no longer an active deadline. The expiry is set to positive
            // infinity so that the actor takes no action until a new deadline is set.
            deadline_.expires_at(boost::posix_time::pos_infin);
        }
        else
        {
            deadline_.expires_from_now(boost::posix_time::seconds(10));
        }
    }

    // Put the actor back to sleep.
    deadline_.async_wait(boost::bind(&Client::check_deadline, this));
}

void Client::start_receive(udp::endpoint &remote_endpoint)
{
    Socket::start_receive(remote_endpoint);
    deadline_.expires_from_now(boost::posix_time::seconds(10));
}

void Client::error_message(const std::string &msg)
{
    throw std::runtime_error(msg);
}


} // namespace DTLS
} // namespace Helpz
