#include <thread>
#include <boost/algorithm/string/join.hpp>

#include <botan/tls_server.h>

#include "dtls_tools.h"
#include "dtls_server_controller.h"
#include "dtls_server_node.h"

namespace Helpz {
namespace DTLS {

Server_Node::Server_Node(Server_Controller *controller, const boost::asio::ip::udp::endpoint &endpoint) :
    Node{ controller->socket() },
    controller_(controller)
{
    set_receiver_endpoint(endpoint);

    auto tools = controller->dtls_tools();
    dtls_.reset(new Botan::TLS::Server{ *this, *tools->session_manager_, *tools->creds_,
                                        *tools->policy_, *tools->rng_, true });
}

void Server_Node::add_timeout_at(std::chrono::time_point<std::chrono::system_clock> time_point)
{
    controller_->add_timeout_at(receiver_endpoint(), time_point);
}

void Server_Node::tls_record_received(Botan::u64bit, const uint8_t data[], size_t size)
{
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[size]);
    memcpy(buffer.get(), data, size);
    controller_->add_received_record(receiver_endpoint(), std::move(buffer), size);
}

void Server_Node::tls_alert(Botan::TLS::Alert alert)
{
    Node::tls_alert(alert);

    if (alert.type() == Botan::TLS::Alert::CLOSE_NOTIFY)
    {
        std::thread(&Server_Controller::remove_client, controller_, receiver_endpoint()).detach();
    }
}

std::string Server_Node::tls_server_choose_app_protocol(const std::vector<std::string> &client_protos)
{
    std::string app_protocol;
    std::shared_ptr<Network::Protocol> protocol = controller_->create_protocol(client_protos, &app_protocol);
    if (protocol)
    {
        set_protocol(std::move(protocol));
    }
    else
    {
        std::thread(&Server_Controller::remove_client, controller_, receiver_endpoint()).detach();
    }

    std::cout << title() << " protocol is " << app_protocol << " (" << boost::algorithm::join(client_protos, ", ") << ')' << std::endl;

    return app_protocol;
}

} // namespace DTLS
} // namespace Helpz
