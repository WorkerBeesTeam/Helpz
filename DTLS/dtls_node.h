#ifndef HELPZ_DTLS_NODE_H
#define HELPZ_DTLS_NODE_H

#include <memory>

#include <botan/tls_channel.h>
#include <botan/tls_callbacks.h>
#include <botan/tls_policy.h>

#include <Helpz/net_protocol.h>
#include <Helpz/dtls_socket.h>

namespace Helpz {
namespace DTLS {

class Node : public Botan::TLS::Callbacks, public Network::Protocol_Writer
{
public:
    Node(Socket* socket, Network::Protocol *protocol = nullptr);

    Network::Protocol *protocol();
    void set_protocol(Network::Protocol *protocol);

    const boost::asio::ip::udp::endpoint& receiver_endpoint() const;
    void set_receiver_endpoint(const boost::asio::ip::udp::endpoint& endpoint);

    std::string address() const;

    void write(const uint8_t* data, std::size_t size) override;

    void process_received_data(std::unique_ptr<uint8_t[]> &&data, std::size_t size);
protected:
    virtual void tls_record_received(Botan::u64bit, const uint8_t data[], size_t size) override;
    virtual void tls_alert(Botan::TLS::Alert alert) override;
    void tls_emit_data(const uint8_t data[], size_t size) override;
    void tls_verify_cert_chain(
       const std::vector<Botan::X509_Certificate>& cert_chain,
       const std::vector<std::shared_ptr<const Botan::OCSP::Response>>& ocsp,
       const std::vector<Botan::Certificate_Store*>& trusted_roots,
       Botan::Usage_Type usage,
       const std::string& hostname,
       const Botan::TLS::Policy& policy) override;
    bool tls_session_established(const Botan::TLS::Session &session) override;

    std::unique_ptr<Botan::TLS::Channel> dtls_;
private:
    Socket* socket_;
    Network::Protocol* protocol_;
    boost::asio::ip::udp::endpoint receiver_endpoint_;
};

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_NODE_H
