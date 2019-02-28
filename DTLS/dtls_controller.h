#ifndef HELPZ_DTLS_CONTROLLER_H
#define HELPZ_DTLS_CONTROLLER_H

#include <memory>

#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>

//#include <botan/tls_channel.h>
//#include <botan/tls_callbacks.h>

namespace Helpz {
namespace DTLS {

class Tools;
class Controller
{
public:
    using udp = boost::asio::ip::udp;

    Controller(Tools* dtls_tools);

    Tools* dtls_tools();

    virtual void process_data(const udp::endpoint& remote_endpoint, std::unique_ptr<uint8_t[]>&& data, std::size_t size) = 0;
protected:
    Tools* dtls_tools_;
//    std::unique_ptr<Botan::TLS::Channel> dtls_;
};

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_CONTROLLER_H
