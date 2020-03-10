#ifndef HELPZ_DTLS_NODE_H
#define HELPZ_DTLS_NODE_H

#include <memory>

#include <botan-2/botan/tls_channel.h>
#include <botan-2/botan/tls_callbacks.h>
#include <botan-2/botan/tls_policy.h>

#include <Helpz/net_protocol.h>
#include <Helpz/dtls_socket.h>

namespace Helpz {
namespace DTLS {

class Node : public Botan::TLS::Callbacks, public Net::Protocol_Writer
{
public:
    Node(Controller* controller, Socket* socket);
    virtual ~Node();

    std::recursive_mutex mutex_;

    void close();

    std::shared_ptr<Net::Protocol> protocol() override final;
    void set_protocol(std::shared_ptr<Net::Protocol>&& protocol);

    const boost::asio::ip::udp::endpoint& receiver_endpoint() const;
    void set_receiver_endpoint(const boost::asio::ip::udp::endpoint& endpoint);

    std::string address() const;

    void write(const QByteArray& data) override;
    void write(Net::Message_Item message) override;

    void process_received_data(std::unique_ptr<uint8_t[]> &&data, std::size_t size);
protected:
    virtual std::shared_ptr<Node> get_shared() = 0;

    void add_timeout_at(std::chrono::system_clock::time_point time_point, void* data = nullptr) override;

    virtual std::shared_ptr<Net::Protocol> create_protocol();
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
    Controller* controller_;
private:

    Socket* socket_;
    std::shared_ptr<Net::Protocol> protocol_;
    boost::asio::ip::udp::endpoint receiver_endpoint_;
};

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_NODE_H
