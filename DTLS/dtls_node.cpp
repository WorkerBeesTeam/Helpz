#include <iostream>

#include <botan/x509path.h>
#include <botan/hex.h>
#include <botan/tls_exceptn.h>

#include "dtls_node.h"

namespace Helpz {
namespace DTLS {

Node::Node(Helpz::DTLS::Socket *socket, Network::Protocol *protocol) :
    socket_(socket), protocol_(protocol)
{
    if (protocol_)
    {
        protocol_->set_protocol_writer(this);
    }
}

Network::Protocol *Node::protocol()
{
    return protocol_;
}

void Node::set_protocol(Network::Protocol *protocol)
{
    protocol_ = protocol;
    protocol_->set_protocol_writer(this);
}

const boost::asio::ip::udp::endpoint &Node::receiver_endpoint() const { return receiver_endpoint_; }

void Node::set_receiver_endpoint(const boost::asio::ip::udp::endpoint &endpoint)
{
    receiver_endpoint_ = endpoint;
    if (title().empty())
    {
        set_title(address());
    }
}

std::string Node::address() const
{
    std::stringstream title_s; title_s << receiver_endpoint_;
    return title_s.str();
}

void Node::write(const uint8_t *data, std::size_t size)
{
    if (dtls_->is_active())
    {
        dtls_->send(data, size);
    }
}

void Node::process_received_data(std::unique_ptr<uint8_t[]> &&data, std::size_t size)
{
    try
    {
        bool first_active = !dtls_->is_active();

        dtls_->received_data(data.get(), size);

        if (first_active)
        {
            if (dtls_->timeout_check())
            {
                std::cerr << title() << " Handshake timeout detected" << std::endl;
            }

            if (protocol_ && dtls_->is_active())
            {
                protocol_->ready_write();
            }
        }
    }
    catch(Botan::TLS::TLS_Exception& e)
    {
        std::string type_str = Botan::TLS::Alert(e.type()).type_string();
        std::cerr << title() << ' ' << type_str << e.what() << std::endl;
    }
    catch(std::exception& e)
    {
        std::cerr << title() << e.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << title() << " Error in receided data Server::procData" << std::endl;
    }
}

void Node::tls_record_received(Botan::u64bit, const uint8_t data[], size_t size)
{
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[size]);
    memcpy(buffer.get(), data, size);

    if (protocol_)
    {
        protocol_->process_bytes(data, size);
    }
}

void Node::tls_alert(Botan::TLS::Alert alert)
{
    std::cout << title() << " tls_alert " << alert.type_string() << std::endl;

    if (protocol_ && alert.type() == Botan::TLS::Alert::CLOSE_NOTIFY)
    {
        protocol_->closed();
    }
}

void Node::tls_emit_data(const uint8_t data[], size_t size)
{
    socket_->send(receiver_endpoint_, data, size);
}

void Node::tls_verify_cert_chain(const std::vector<Botan::X509_Certificate> &cert_chain, const std::vector<std::shared_ptr<const Botan::OCSP::Response> > &ocsp, const std::vector<Botan::Certificate_Store *> &trusted_roots, Botan::Usage_Type usage, const std::string &hostname, const Botan::TLS::Policy &policy)
{
    if (cert_chain.empty())
    {
        throw std::invalid_argument(title() + " Certificate chain was empty");
    }

    Botan::Path_Validation_Restrictions restrictions(policy.require_cert_revocation_info(),
                                                     policy.minimum_signature_strength());

    auto ocsp_timeout = std::chrono::milliseconds(1000);

    Botan::Path_Validation_Result result =
            Botan::x509_path_validate(cert_chain,
                                      restrictions,
                                      trusted_roots,
                                      hostname,
                                      usage,
                                      std::chrono::system_clock::now(),
                                      ocsp_timeout,
                                      ocsp);

    std::string status_string = title() + " Certificate validation status: " + result.result_string();
    if (result.successful_validation())
    {
        std::cout << status_string << std::endl;

        auto status = result.all_statuses();
        if (status.size() > 0 && status[0].count(Botan::Certificate_Status_Code::OCSP_RESPONSE_GOOD))
        {
            std::cout << title() + " Valid OCSP response for this server" << std::endl;
        }
    }
    else
    {
        std::cerr << status_string << std::endl;
    }
}

bool Node::tls_session_established(const Botan::TLS::Session &session)
{
    std::cout << title() << " Handshake complete, " << session.version().to_string()
              << " using " << session.ciphersuite().to_string() << std::endl;

    if (!session.session_id().empty())
    {
        std::cout << title() << " Session ID " << Botan::hex_encode(session.session_id()) << std::endl;
    }

    if (!session.session_ticket().empty())
    {
        std::cout << title() << " Session ticket " << Botan::hex_encode(session.session_ticket()) << std::endl;
    }

    return true;
}

} // namespace DTLS
} // namespace Helpz
