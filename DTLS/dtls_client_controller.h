#ifndef HTLPZ_DTLS_CLIENT_CONTROLLER_H
#define HTLPZ_DTLS_CLIENT_CONTROLLER_H

#include <botan/tls_client.h>

#include <Helpz/dtls_tools.h>

#include <Helpz/dtls_controller.h>
#include <Helpz/dtls_node.h>

namespace Helpz {
namespace DTLS {

class Client;
class Client_Controller final : public Controller, public Node
{
public:
    Client_Controller(Tools *dtls_tools, Client* client, Network::Protocol* protocol);

    void start(const std::string& host, const udp::endpoint& receiver_endpoint, const std::vector<std::string> &next_protocols = {});

    bool is_reconnect_needed();

    std::string application_protocol() const;

    void process_data(const udp::endpoint& remote_endpoint, std::unique_ptr<uint8_t[]> &&data, std::size_t size) override;
private:
    void add_timeout_at(std::chrono::time_point<std::chrono::system_clock> time_point) override;
    void on_protocol_timeout(boost::asio::ip::udp::endpoint endpoint) override;

    bool ping_flag_;
    Client* client_;
};

} // namespace DTLS
} // namespace Helpz

#endif // HTLPZ_DTLS_CLIENT_CONTROLLER_H
