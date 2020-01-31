#include "server.h"
#include "protocol_impl.h"

namespace dtls {

protocol_base::protocol_base(const boost::asio::ip::udp::endpoint &remote_endpoint) :
    project::base(remote_endpoint)
{}

int protocol_base::version() const  { return version_; }

server_thread *protocol_base::thread() { return thread_; }

void protocol_base::set_thread(server_thread *th) { thread_ = th; }

void protocol_base::init()
{
    dtls_.reset(new Botan::TLS::Server{ *this, *dtls_tools->session_manager, *dtls_tools->creds,
                                        *dtls_tools->policy, *dtls_tools->rng, true });
}

void protocol_base::ready_write() {}

void protocol_base::closed() {}

void protocol_base::process_data(uint8_t *data, std::size_t size)
{
    bool first_active = !dtls_->is_active();

    try {
        //        std::cout << title() << " proc data " << size << ' ' << std::this_thread::get_id() << std::endl;
        //        data_incoming();
        dtls_->received_data(data, size);
    }
    catch(Botan::TLS::TLS_Exception& e)
    {
        std::string type_str = Botan::TLS::Alert(e.type()).type_string();
        std::cerr << title() << ' ' << type_str << e.what() << std::endl;
    }
    catch(std::exception& e) { std::cerr << title() << e.what() << std::endl; }
    catch(...) { std::cerr << title() << " Error in receided data Server::procData" << std::endl; }

    if (first_active)
    {
        if (dtls_->timeout_check())
            std::cerr << title() << " Handshake timeout detected" << std::endl;

        if (dtls_->is_active())
            ready_write();
    }
}

void protocol_base::write(const quint8 *data, std::size_t size)
{
    //    std::cout << title() << " WRITE CMD: " << ((data[2] << 8 | data[3]) & ~(1 << 15))
    //            << " SIZE: " << size << ' ' << std::this_thread::get_id() << std::endl;
    if (dtls_ && dtls_->is_active())
        dtls_->send(data, size);
}

void protocol_base::process_message(quint16 cmd, QByteArray &&data)
{
    thread_->add_message(cmd, std::move(data), this);
}
void protocol_base::process_answer_message(quint16 cmd, QByteArray &&data)
{
    thread_->add_message(cmd, std::move(data), this, true);
}
void protocol_base::tls_emit_data(const uint8_t data[], size_t size)
{
    dtls_server->send(remote_endpoint(), data, size);
}
void protocol_base::tls_record_received(Botan::u64bit, const uint8_t data[], size_t size)
{
    process_bytes(data, size);
}
void protocol_base::tls_alert(Botan::TLS::Alert alert)
{
    std::cout << title() << " Alert: " << alert.type_string() << std::endl;
    if (alert.type() == Botan::TLS::Alert::CLOSE_NOTIFY)
    {
        closed();
        dtls_server->close_slot(thread_, *this);
    }
}

void protocol_base::tls_verify_cert_chain(const std::vector<Botan::X509_Certificate> &cert_chain, const std::vector<std::shared_ptr<const Botan::OCSP::Response> > &ocsp, const std::vector<Botan::Certificate_Store *> &trusted_roots, Botan::Usage_Type usage, const std::string &hostname, const Botan::TLS::Policy &policy)
{
    if(cert_chain.empty())
        throw std::invalid_argument("Certificate chain was empty");

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
    if(result.successful_validation())
    {
        std::cout << status_string << std::endl;

        auto status = result.all_statuses();
        if(status.size() > 0 && status[0].count(Botan::Certificate_Status_Code::OCSP_RESPONSE_GOOD))
            std::cout << title() << " Valid OCSP response for this server" << std::endl;
    }
    else
        std::cerr << status_string << std::endl;
}

bool protocol_base::tls_session_established(const Botan::TLS::Session &session)
{
    std::cout << title() << " Handshake complete, " << session.version().to_string()
              << " using " << session.ciphersuite().to_string() << std::endl;

    if(!session.session_id().empty())
        std::cout << title() << " Session ID " << Botan::hex_encode(session.session_id()) << std::endl;

    if(!session.session_ticket().empty())
        std::cout << title() << " Session ticket " << Botan::hex_encode(session.session_ticket()) << std::endl;

    return true;
}

std::string protocol_base::tls_server_choose_app_protocol(const std::vector<std::string> &client_protos)
{
    std::string proto_ver;
    std::cout << title() << " Client offered protocol";
    for (const std::string& proto: client_protos) {
        std::cout << ' ' << proto;
        if (proto_ver.empty()) {
            if (proto == "helpz_test/1.1") {
                version_ = 11;
                proto_ver = proto;
            } else if (proto == "helpz_test/1.0") {
                version_ = 10;
                proto_ver = proto;
            }
        }
    }

    std::cout << std::endl;

    if (proto_ver.empty()) {
        std::cerr << "Unsupported portocols\n";
        // TODO: Close connection if bad protocol
    }
    return proto_ver;
}

void protocol_impl::process_message_stream(uint16_t cmd, QDataStream &msg)
{

}

} // namespace dtls
