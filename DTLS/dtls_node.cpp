#include <iostream>

#include <botan-2/botan/x509path.h>
#include <botan-2/botan/hex.h>
#include <botan-2/botan/tls_exceptn.h>

#include "dtls_node.h"

namespace Helpz {
namespace DTLS {

Node::Node(Controller *controller, Helpz::DTLS::Socket *socket) :
    controller_(controller), socket_(socket)
{
}

Node::~Node()
{
    close();
}

void Node::close()
{
    std::lock_guard lock(mutex_);
    if (protocol_)
    {
        protocol_->set_writer(nullptr);
        protocol_.reset();
    }
    if (dtls_)
    {
        dtls_->close();
        dtls_.reset();
    }
}

std::shared_ptr<Net::Protocol> Node::protocol()
{
    return protocol_;
}

void Node::set_protocol(std::shared_ptr<Net::Protocol>&& protocol)
{
    protocol_ = std::move(protocol);
    if (protocol_)
    {
        protocol_->set_writer(std::static_pointer_cast<Net::Protocol_Writer>(get_shared()));
    }
}

const boost::asio::ip::udp::endpoint &Node::receiver_endpoint() const { return receiver_endpoint_; }

void Node::set_receiver_endpoint(const boost::asio::ip::udp::endpoint &endpoint)
{
    receiver_endpoint_ = endpoint;
    if (title().isEmpty())
    {
        set_title(QString::fromStdString(address()));
    }
}

std::string Node::address() const
{
    std::stringstream title_s; title_s << receiver_endpoint_;
    return title_s.str();
}

void Node::process_received_data(std::unique_ptr<uint8_t[]> &&data, std::size_t size)
{
    try
    {
        std::lock_guard lock(mutex_);
        if (!dtls_)
            return;

        bool first_active = !dtls_->is_active();

        dtls_->received_data(data.get(), size);

        if (first_active && dtls_)
        {
            if (dtls_->timeout_check())
            {
                qCWarning(Log).noquote() << title() << "Handshake timeout detected";
            }

            if (dtls_->is_active())
            {
                if (!protocol_)
                {
                    set_protocol(create_protocol());
                }

                if (protocol_)
                {
                    protocol_->ready_write();
                }
            }
        }
    }
    catch(Botan::TLS::TLS_Exception& e)
    {
        std::string type_str = Botan::TLS::Alert(e.type()).type_string();
        qCWarning(Log).noquote() << title() << ' ' << type_str.c_str() << e.what();
    }
    catch(std::exception& e)
    {
        qCWarning(Log).noquote() << title() << e.what();
    }
    catch(...)
    {
        qCWarning(Log).noquote() << title() << "Error in receided data process_received_data";
    }
}

void Node::write(const QByteArray& data)
{
    auto* ctrl = controller_;
    boost::asio::ip::udp::endpoint endpoint = receiver_endpoint_;

    socket_->get_io_context()->post([ctrl, endpoint, data]()
    {
        auto node = ctrl->get_node(endpoint);
        if (node)
            node->write_impl(data);
    });
}

void Node::write(std::shared_ptr<Net::Message_Item> message)
{
    auto* ctrl = controller_;
    boost::asio::ip::udp::endpoint endpoint = receiver_endpoint_;

    socket_->get_io_context()->post([ctrl, endpoint, message]()
    {
        auto node = ctrl->get_node(endpoint);
        if (node)
            node->write_impl(message);
    });
}

void Node::write_impl(const QByteArray &data)
{
    std::lock_guard lock(mutex_);
    if (dtls_ && dtls_->is_active())
        dtls_->send(reinterpret_cast<const uint8_t*>(data.constData()), data.size());
}

void Node::write_impl(std::shared_ptr<Net::Message_Item> message)
{
    std::lock_guard lock(mutex_);
    if (dtls_ && dtls_->is_active() && protocol_)
    {
        const QByteArray data = protocol_->prepare_packet_to_send(std::move(message));
        dtls_->send(reinterpret_cast<const uint8_t*>(data.constData()), data.size());
    }
}

void Node::add_timeout_at(std::chrono::system_clock::time_point time_point, void *data)
{
    controller_->add_timeout_at(receiver_endpoint(), time_point, data);
}

std::shared_ptr<Net::Protocol> Node::create_protocol() { return {}; }

void Node::tls_record_received(Botan::u64bit, const uint8_t data[], size_t size)
{
    if (protocol_)
        protocol_->process_bytes(data, size);
}

void Node::tls_alert(Botan::TLS::Alert alert)
{
    qCWarning(Log).noquote() << title() << "tls_alert" << alert.type_string().c_str();

    if (protocol_ && alert.type() == Botan::TLS::Alert::CLOSE_NOTIFY)
    {
        protocol_->closed();
        close();
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
        throw std::invalid_argument(title().toStdString() + " Certificate chain was empty");
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

    QString status_string = title() + " Certificate validation status: " + result.result_string().c_str();
    if (result.successful_validation())
    {
        qCDebug(Log).noquote() << status_string;

        auto status = result.all_statuses();
        if (status.size() > 0 && status[0].count(Botan::Certificate_Status_Code::OCSP_RESPONSE_GOOD))
        {
            qCDebug(Log).noquote() << title() << "Valid OCSP response for this server";
        }
    }
    else
    {
        qCWarning(Log).noquote() << status_string;
    }
}

bool Node::tls_session_established(const Botan::TLS::Session &session)
{
    qCDebug(Log).noquote() << title() << "Handshake complete," << session.version().to_string().c_str()
              << "using" << session.ciphersuite().to_string().c_str();

    if (!session.session_id().empty())
    {
        qCDebug(Log).noquote() << title() << "Session ID" << Botan::hex_encode(session.session_id()).c_str();
    }

    if (!session.session_ticket().empty())
    {
        qCDebug(Log).noquote() << title() << "Session ticket" << Botan::hex_encode(session.session_ticket()).c_str();
    }

    return true;
}

} // namespace DTLS
} // namespace Helpz
