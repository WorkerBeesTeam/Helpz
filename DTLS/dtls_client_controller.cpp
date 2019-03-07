#include <iostream>

#include "dtls_client.h"
#include "dtls_client_controller.h"

namespace Helpz {
namespace DTLS {

Client_Controller::Client_Controller(Tools *dtls_tools, Client *client, Network::Protocol *protocol) :
    Controller{ dtls_tools },
    Node{ client, protocol },
    client_(client)
{
}

void Client_Controller::start(const std::string &host, const udp::endpoint& receiver_endpoint, const std::vector<std::string> &next_protocols)
{
    ping_flag_ = false;
    set_receiver_endpoint(receiver_endpoint);
    dtls_.reset(new Botan::TLS::Client{ *this, *dtls_tools_->session_manager_, *dtls_tools_->creds_, *dtls_tools_->policy_, *dtls_tools_->rng_,
                                        Botan::TLS::Server_Information{host, receiver_endpoint.port()}, Botan::TLS::Protocol_Version::latest_dtls_version(), next_protocols });
}

bool Client_Controller::is_reconnect_needed()
{
    if (dtls_->is_active())
    {
        auto last_recv_delta = std::chrono::system_clock::now() - last_msg_recv_time();
        if (last_recv_delta < std::chrono::seconds(30))
        {
            return false;
        }

        if (!ping_flag_)
        {
            ping_flag_ = true;
            protocol()->send(Network::Cmd::PING);
            return false;
        }
    }
    return true;
}

std::string Client_Controller::application_protocol() const
{
    return static_cast<Botan::TLS::Client*>(dtls_.get())->application_protocol();
}

void Client_Controller::process_data(const udp::endpoint &/*remote_endpoint*/, std::unique_ptr<uint8_t[]> &&data, std::size_t size)
{
    if (ping_flag_)
    {
        ping_flag_ = false;
    }

    process_received_data(std::move(data), size);
}

void Client_Controller::add_timeout_at(std::chrono::time_point<std::chrono::system_clock> time_point)
{
    protocol_timer_.add(time_point, receiver_endpoint());
}

void Client_Controller::on_protocol_timeout(boost::asio::ip::udp::endpoint /*endpoint*/)
{
    protocol()->process_wait_list();
}

} // namespace DTLS
} // namespace Helpz
