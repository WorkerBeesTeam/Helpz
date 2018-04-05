#ifndef ZIMNIKOV_DTLS_PROTOCLIENT_H
#define ZIMNIKOV_DTLS_PROTOCLIENT_H

#include <QUdpSocket>

#include <Helpz/udpclient.h>
#include <Helpz/prototemplate.h>

#include <Helpz/dtlsbasic.h>

#include <botan/tls_callbacks.h>
#include <botan/tls_channel.h>

namespace Helpz {
namespace DTLS {

Q_DECLARE_LOGGING_CATEGORY(Log)

class Proto;
class ProtoHelper
{
public:
    void setSock(QUdpSocket *sock, bool proc_data = false);
    QUdpSocket *sock() const;

    virtual void readyWrite() {}
    virtual Proto* getClient(const QHostAddress& clientAddress, quint16 clientPort) = 0;
private:
    void processData();
    QUdpSocket* m_sock;
};

class Proto : public QObject, public ProtoHelper, public Network::UDPClient, public Network::ProtoTemplate, public Botan::TLS::Callbacks
{
    Q_OBJECT
public:
    Proto();

    virtual QString clientName() const;
    virtual Proto* getClient(const QHostAddress& clientAddress, quint16 clientPort);
    virtual Botan::TLS::Channel* channel() const = 0;
signals:
    void DTLS_closed();
private:
    QBuffer dev;

    void tls_emit_data(const uint8_t data[], size_t size) override;
    void tls_record_received(Botan::u64bit, const uint8_t data[], size_t size) override;
    void tls_alert(Botan::TLS::Alert alert) override;
    void tls_verify_cert_chain(
       const std::vector<Botan::X509_Certificate>& cert_chain,
       const std::vector<std::shared_ptr<const Botan::OCSP::Response>>& ocsp,
       const std::vector<Botan::Certificate_Store*>& trusted_roots,
       Botan::Usage_Type usage,
       const std::string& hostname,
       const Botan::TLS::Policy& policy) override;
    bool tls_session_established(const Botan::TLS::Session &session) override;
};

// T == Botan::TLS::Server or Botan::TLS::Client
template<class T, typename... DTLS_Args>
class ProtoTemplate : public Proto
{
public:
    ~ProtoTemplate()
    {
        QString nameStr = hostString() + ':' + QString::number(port());
        qCDebug(Log).noquote() << "Delete" << (clientName() == nameStr ? nameStr : clientName() + ' ' + nameStr) << (qintptr)this;

        if (dtls && dtls->is_active())
            dtls->close();
    }

    /**
     * @brief init Создаёт this->dtls
     * @param extra_args Если T = Server, то bool - is_datagrame,
     * если Client, то Server_Information server_info,
     *      const Protocol_Version& offer_version,
     *      const std::vector<std::string>& next_protocols
     */
    void init(BotanHelpers* helper, DTLS_Args... extra_args)
    {
        try {
             dtls = std::make_shared< T >(
                         *this,
                         *helper->session_manager,
                         *helper->creds,
                         *helper->policy,
                         *helper->rng,
                         extra_args...);
        }
        catch(std::exception& e) { qCCritical(Log).noquote() << (clientName() + ' ' + tr("Fail init DTLS.") + ' ' + e.what()); }
        catch(...) { qCCritical(Log).noquote() << (clientName() + ' ' + tr("Fail create DTLS")); }
    }

    virtual void write(const QByteArray &buff) override
    {
        if (dtls && dtls->is_active())
            dtls->send((uchar*)buff.data(), buff.size());
    }

    Botan::TLS::Channel* channel() const override { return dtls.get(); }
//protected:
    std::shared_ptr< T > dtls;
};

//typedef DTLSProtoTemplate<Botan::TLS::Server, bool> DTLSProtoServer;
//typedef Helpz::DTLSProtoTemplate<Botan::TLS::Client,
//    const Botan::TLS::Server_Information&,
//    const Botan::TLS::Protocol_Version&,
//    const std::vector<std::string>&> DTLSProtoClient;

} // namespace DTLS
} // namespace Helpz

#endif // ZIMNIKOV_DTLS_PROTOCLIENT_H
