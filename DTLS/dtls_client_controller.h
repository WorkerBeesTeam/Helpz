#ifndef HTLPZ_DTLS_CLIENT_CONTROLLER_H
#define HTLPZ_DTLS_CLIENT_CONTROLLER_H

#include <Helpz/dtls_tools.h>

#include <Helpz/dtls_controller.h>
#include <Helpz/dtls_node.h>

namespace Helpz {
namespace DTLS {

typedef std::function<std::shared_ptr<Network::Protocol>(const std::string &)> Create_Client_Protocol_Func_T;

class Client;
class Client_Node;
class Client_Controller final : public Controller
{
public:
    Client_Controller(Tools *dtls_tools, Client* client, Create_Client_Protocol_Func_T&& create_protocol_func);

    std::shared_ptr<Network::Protocol> create_protocol(const std::string& app_proto);

    std::shared_ptr<Node> get_node(const udp::endpoint& remote_endpoint = udp::endpoint()) override;
    void process_data(std::shared_ptr<Node> &, std::unique_ptr<uint8_t[]> &&data, std::size_t size) override;
private:
//    Client* client_;
    std::shared_ptr<Client_Node> node_;
    Create_Client_Protocol_Func_T create_protocol_func_;
};

} // namespace DTLS
} // namespace Helpz

#endif // HTLPZ_DTLS_CLIENT_CONTROLLER_H
