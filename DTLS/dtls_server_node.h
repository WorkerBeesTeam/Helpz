#ifndef HELPZ_DTLS_SERVER_NODE_H
#define HELPZ_DTLS_SERVER_NODE_H

#include <mutex>
#include <functional>

#include <boost/asio/ip/udp.hpp>

#include <Helpz/dtls_node.h>

namespace Helpz {
namespace DTLS {

class Server_Controller;

class Server_Node final : public Node, public std::enable_shared_from_this<Server_Node>
{
public:
    std::mutex record_mutex_;

    Server_Node(Server_Controller* controller, const boost::asio::ip::udp::endpoint& endpoint);
private:
    std::shared_ptr<Node> get_shared() override;

    void tls_record_received(Botan::u64bit, const uint8_t data[], size_t size) override final;
    void tls_alert(Botan::TLS::Alert alert) override final;
    std::string tls_server_choose_app_protocol(const std::vector<std::string> &client_protos) override final;

    constexpr Server_Controller* controller();
};

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_SERVER_NODE_H
