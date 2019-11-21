#ifndef HELPZ_DTLS_CLIENT_NODE_H
#define HELPZ_DTLS_CLIENT_NODE_H

#include <Helpz/dtls_node.h>

namespace Helpz {
namespace DTLS {

class Client_Controller;
class Client_Node final : public Node
{
public:
    Client_Node(Client_Controller* controller, Socket* socket);

    void start(const std::string &host, const boost::asio::ip::udp::endpoint& receiver_endpoint, const std::vector<std::string> &next_protocols);
    void reset_ping_flag();

    std::string application_protocol() const;
    bool is_reconnect_needed();

private:
    std::shared_ptr<Network::Protocol> create_protocol() override;

    bool ping_flag_;

    constexpr Client_Controller* controller();
};

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_CLIENT_NODE_H
