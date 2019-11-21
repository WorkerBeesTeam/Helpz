#include <iostream>

#include "dtls_client.h"
#include "dtls_client_node.h"
#include "dtls_client_controller.h"

namespace Helpz {
namespace DTLS {

Client_Controller::Client_Controller(Tools *dtls_tools, Client *client, const Create_Client_Protocol_Func_T& create_protocol_func) :
    Controller{ dtls_tools },
    node_{ new Client_Node{this, client} },
    create_protocol_func_(create_protocol_func)
{
}

std::shared_ptr<Network::Protocol> Client_Controller::create_protocol(const std::string &app_proto)
{
    if (create_protocol_func_)
    {
        return create_protocol_func_(app_proto);
    }
    return {};
}

std::shared_ptr<Node> Client_Controller::get_node(const boost::asio::ip::udp::endpoint &/*remote_endpoint*/)
{
    return node_;
}

void Client_Controller::process_data(std::shared_ptr<Node> &node, std::unique_ptr<uint8_t[]> &&data, std::size_t size)
{
    node_->reset_ping_flag();
    node_->process_received_data(node, std::move(data), size);
}

void Client_Controller::on_protocol_timeout(boost::asio::ip::udp::endpoint /*remote_endpoint*/)
{
    std::shared_ptr<Network::Protocol> proto = node_->protocol();
    if (proto)
    {
        std::lock_guard node_lock(node_->mutex_);
        proto->process_wait_list();
    }
}

} // namespace DTLS
} // namespace Helpz
