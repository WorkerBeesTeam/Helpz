#include <botan-2/botan/tls_client.h>

#include "dtls_client_controller.h"
#include "dtls_client_node.h"

namespace Helpz {
namespace DTLS {

Client_Node::Client_Node(Client_Controller *controller, Socket *socket) :
    Node(controller, socket)
{
}

void Client_Node::start(const std::string &host, const boost::asio::ip::udp::endpoint& receiver_endpoint, const std::vector<std::string> &next_protocols)
{
    reset_ping_flag();
    set_receiver_endpoint(receiver_endpoint);
    auto tools = controller_->dtls_tools();
    dtls_.reset(new Botan::TLS::Client{ *this, *tools->session_manager_, *tools->creds_, *tools->policy_, *tools->rng_,
                                        Botan::TLS::Server_Information{host, receiver_endpoint.port()}, Botan::TLS::Protocol_Version::latest_dtls_version(), next_protocols });
}

void Client_Node::reset_ping_flag()
{
    if (ping_flag_)
    {
        ping_flag_ = false;
    }
}

std::string Client_Node::application_protocol() const
{
    return dtls_ ? static_cast<Botan::TLS::Client*>(dtls_.get())->application_protocol() : std::string{};
}

bool Client_Node::is_reconnect_needed()
{
    if (dtls_ && dtls_->is_active())
    {
        auto last_recv_delta = std::chrono::system_clock::now() - last_msg_recv_time();
        if (last_recv_delta < std::chrono::seconds(90))
        {
            return false;
        }

        auto proto = protocol();
        if (proto)
        {
            if (!ping_flag_)
            {
                ping_flag_ = true;
                proto->send(Network::Cmd::PING).timeout(nullptr, std::chrono::seconds(30), std::chrono::seconds(3));
                return false;
            }
        }
    }
    return true;
}

std::shared_ptr<Network::Protocol> Client_Node::create_protocol()
{
    return controller()->create_protocol(application_protocol());
}

constexpr Client_Controller *Client_Node::controller()
{
    return static_cast<Client_Controller*>(controller_);
}

} // namespace DTLS
} // namespace Helpz
