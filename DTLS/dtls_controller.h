#ifndef HELPZ_DTLS_CONTROLLER_H
#define HELPZ_DTLS_CONTROLLER_H

#include <memory>

#include <boost/asio/ip/udp.hpp>

#include <QLoggingCategory>

#include <Helpz/net_protocol_timer.h>

namespace Helpz {
namespace DTLS {

Q_DECLARE_LOGGING_CATEGORY(Log)

class Tools;
class Node;
class Controller : public Network::Protocol_Timer_Emiter
{
public:
    using udp = boost::asio::ip::udp;

    Controller(Tools* dtls_tools);

    Tools* dtls_tools();

    void add_timeout_at(const udp::endpoint& remote_endpoint, std::chrono::time_point<std::chrono::system_clock> time_point);
    void on_protocol_timeout(boost::asio::ip::udp::endpoint remote_endpoint) override;

    virtual std::shared_ptr<Node> get_node(const udp::endpoint& remote_endpoint) = 0;
    virtual void process_data(std::shared_ptr<Node>& node, std::unique_ptr<uint8_t[]>&& data, std::size_t size) = 0;
protected:
    Tools* dtls_tools_;

    Network::Protocol_Timer protocol_timer_;
};

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_CONTROLLER_H
