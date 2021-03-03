#include <iostream>

#include <boost/date_time/posix_time/posix_time_types.hpp>
//#include <boost/bind/bind.hpp>

#include "dtls_client_node.h"
#include "dtls_client.h"

namespace Helpz {
namespace DTLS {

using boost::asio::ip::udp;

Client::Client(const std::shared_ptr<boost::asio::io_context>& io_context,
               const std::shared_ptr<Tools> &tools, const Create_Client_Protocol_Func_T& create_protocol_func) :
    Socket{io_context.get(), new udp::socket{*io_context}, new Client_Controller{tools.get(), this, create_protocol_func}},
    io_context_(io_context), deadline_{*io_context}, tools_(tools)
{
    deadline_.expires_at(boost::posix_time::pos_infin);
}

Client::~Client()
{
    close();
}

std::shared_ptr<Helpz::Net::Protocol> Client::protocol()
{
    return controller()->get_node()->protocol();
}

void Client::run()
{
    io_context_->run();
}

void Client::start_connection(const std::string &host, const std::string &port, const std::vector<std::string> &next_protocols)
{
    udp::resolver resolver(*get_io_context());
    udp::resolver::query query(udp::v4(), host, port);
    udp::endpoint receiver_endpoint = *resolver.resolve(query);

    socket_->open(udp::v4());

    node()->start(host, receiver_endpoint, next_protocols);

    start_receive(remote_endpoint_);
    check_deadline();
}

void Client::close()
{
//    io_context_->post([this]()
//    {
        node()->close();

        if (socket_ && socket_->is_open())
        {
            socket_->cancel();
            socket_->close();
        }
        io_context_->stop();
//    });
}

Client_Node *Client::node()
{
    return std::static_pointer_cast<Client_Node>(controller()->get_node()).get();
}

Client_Controller *Client::controller()
{
    return static_cast<Client_Controller*>(controller_.get());
}

void Client::check_deadline(const boost::system::error_code &err)
{
    // Check whether the deadline has passed. We compare the deadline against
    // the current time since a new asynchronous operation may have moved the
    // deadline before this actor had a chance to run.
    if (err != boost::asio::error::operation_aborted && // Changing the expiry time
        deadline_.expires_at() <= boost::asio::deadline_timer::traits_type::now())
    {
        if (node()->is_reconnect_needed())
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
    deadline_.async_wait(std::bind(&Client::check_deadline, this, std::placeholders::_1));
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
