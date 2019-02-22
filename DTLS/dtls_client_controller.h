#ifndef HTLPZ_DTLS_CLIENT_CONTROLLER_H
#define HTLPZ_DTLS_CLIENT_CONTROLLER_H

#include <botan/tls_client.h>

#include <Helpz/dtls_tools.h>

#include <Helpz/dtls_controller.h>
#include <Helpz/dtls_node.h>

namespace Helpz {
namespace DTLS {

class Client;
class Client_Controller : public Controller, public Node
{
public:
    Client_Controller(Tools *dtls_tools, Client* client, Network::Protocol* protocol);

    void start(const std::string& host, const udp::endpoint& receiver_endpoint, const std::vector<std::string> &next_protocols = {});

    bool is_reconnect_needed();

    void process_data(const udp::endpoint& remote_endpoint, std::unique_ptr<uint8_t[]> &&data, std::size_t size) override final;
private:
    bool ping_flag_;
    Client* client_;
};

} // namespace DTLS
} // namespace Helpz

#endif // HTLPZ_DTLS_CLIENT_CONTROLLER_H
