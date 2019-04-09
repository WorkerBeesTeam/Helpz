#include <iostream>

#include "dtls_client.h"
#include "dtls_client_node.h"
#include "dtls_client_controller.h"

namespace Helpz {
namespace DTLS {

Client_Controller::Client_Controller(Tools *dtls_tools, Client *client, Create_Client_Protocol_Func_T &&create_protocol_func) :
    Controller{ dtls_tools },
    node_{ new Client_Node{this, client} },
    create_protocol_func_(std::move(create_protocol_func))
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

void Client_Controller::process_data(std::shared_ptr<Node> &/*node*/, std::unique_ptr<uint8_t[]> &&data, std::size_t size)
{
    node_->reset_ping_flag();
    node_->process_received_data(std::move(data), size);
}

} // namespace DTLS
} // namespace Helpz
