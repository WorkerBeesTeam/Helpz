
#include "dtls_server_controller.h"
#include "dtls_server.h"

namespace Helpz {
namespace DTLS {

Server::Server(Tools *dtls_tools, boost::asio::io_context *io_context, uint16_t port,
               Create_Server_Protocol_Func_T &&create_protocol_func, std::chrono::seconds cleaning_timeout, int record_thread_count) :
    Socket{io_context, new udp::socket{*io_context, udp::endpoint(udp::v4(), port)}, new Server_Controller{ dtls_tools, this, std::move(create_protocol_func), record_thread_count }},
    cleaning_timeout_{cleaning_timeout},
    cleaning_timer_{*io_context, cleaning_timeout_}
{
    cleaning_timer_.async_wait(std::bind(&Server::cleaning, this, std::placeholders::_1));
}

uint16_t Server::get_local_port() const
{
    return socket_->local_endpoint().port();
}

std::shared_ptr<Server_Node> Server::find_client(std::function<bool (const Net::Protocol *)> check_protocol_func) const
{
    return controller()->find_client(check_protocol_func);
}

void Server::remove_copy(Net::Protocol *client)
{
    controller()->remove_copy(client);
}

void Server::cleaning(const boost::system::error_code &err)
{
    if (err)
    {
        std::cerr << "cleaning timer error " << err << std::endl;
        return;
    }

    controller()->remove_frozen_clients(cleaning_timeout_);

    cleaning_timer_.expires_at(cleaning_timer_.expiry() + cleaning_timeout_);
    cleaning_timer_.async_wait(std::bind(&Server::cleaning, this, std::placeholders::_1));
}

const Server_Controller *Server::controller() const
{
    return static_cast<const Server_Controller*>(controller_.get());
}

Server_Controller *Server::controller()
{
    return static_cast<Server_Controller*>(controller_.get());
}

} // namespace DTLS
} // namespace Helpz
