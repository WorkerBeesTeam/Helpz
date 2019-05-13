#include <Helpz/net_protocol.h>

#include "dtls_node.h"
#include "dtls_controller.h"

namespace Helpz {
namespace DTLS {

Q_LOGGING_CATEGORY(Log, "DTLS")

Controller::Controller(Tools* dtls_tools) :
    dtls_tools_(dtls_tools),
    protocol_timer_(this)
{
}

Tools *Controller::dtls_tools()
{
    return dtls_tools_;
}

void Controller::add_timeout_at(const boost::asio::ip::udp::endpoint &remote_endpoint,
                                std::chrono::time_point<std::chrono::system_clock> time_point)
{
    protocol_timer_.add(time_point, remote_endpoint);
}

void Controller::on_protocol_timeout(boost::asio::ip::udp::endpoint remote_endpoint)
{
    auto node = get_node(remote_endpoint);
    if (node)
    {
        std::shared_ptr<Network::Protocol> proto = node->protocol();
        if (proto)
        {
            proto->process_wait_list();
        }
    }
}

} // namespace DTLS
} // namespace Helpz
