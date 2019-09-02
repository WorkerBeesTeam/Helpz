#include <thread>
#include <boost/algorithm/string/join.hpp>

#include <botan-2/botan/tls_server.h>

#include "dtls_tools.h"
#include "dtls_server_controller.h"
#include "dtls_server_node.h"

namespace Helpz {
namespace DTLS {

Server_Node::Server_Node(Server_Controller *controller, const boost::asio::ip::udp::endpoint &endpoint) :
    Node{ controller, controller->socket() }
{
    set_receiver_endpoint(endpoint);

    auto tools = controller->dtls_tools();

    std::lock_guard lock(mutex_);
    dtls_.reset(new Botan::TLS::Server{ *this, *tools->session_manager_, *tools->creds_,
                                        *tools->policy_, *tools->rng_, true });
}

void Server_Node::tls_record_received(Botan::u64bit, const uint8_t data[], size_t size)
{
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[size]);
    memcpy(buffer.get(), data, size);
    controller()->add_received_record(receiver_endpoint(), std::move(buffer), size);
}

void Server_Node::tls_alert(Botan::TLS::Alert alert)
{
    Node::tls_alert(alert);

    if (alert.type() == Botan::TLS::Alert::CLOSE_NOTIFY)
        controller()->socket()->get_io_context()->post(boost::bind(&Server_Controller::remove_client, controller(), receiver_endpoint()));
}

std::string Server_Node::tls_server_choose_app_protocol(const std::vector<std::string> &client_protos)
{
    std::string app_protocol;
    std::shared_ptr<Network::Protocol> protocol = controller()->create_protocol(client_protos, &app_protocol);
    if (protocol)
    {
        set_protocol(std::move(protocol));
    }
    else
    {
        controller()->socket()->get_io_context()->post(boost::bind(&Server_Controller::remove_client, controller(), receiver_endpoint()));
    }

    qCDebug(Log).noquote() << title() << "protocol is" << app_protocol.c_str() << '(' << boost::algorithm::join(client_protos, ", ").c_str() << ')';
    return app_protocol;
}

constexpr Server_Controller *Server_Node::controller()
{
    return static_cast<Server_Controller*>(controller_);
}

} // namespace DTLS
} // namespace Helpz
