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
                                std::chrono::system_clock::time_point time_point, void *data)
{
    protocol_timer_.add(time_point, remote_endpoint, data);
}

} // namespace DTLS
} // namespace Helpz
