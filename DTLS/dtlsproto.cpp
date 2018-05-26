#include <botan/hex.h>
#include <botan/x509path.h>
#include <botan/tls_exceptn.h>

#include "dtlsproto.h"

namespace Helpz {
namespace DTLS {

Q_LOGGING_CATEGORY(Log, "net.DTLS")

void ProtoHelper::setSock(QUdpSocket *sock, bool proc_data)
{
    m_sock = sock;
    if (proc_data)
        QObject::connect(sock, &QUdpSocket::readyRead, std::bind(&ProtoHelper::processData, this));
}
QUdpSocket *ProtoHelper::sock() const { return m_sock; }

void ProtoHelper::processData()
{
    QHostAddress senderAdr;
    quint16 senderPort;

    qint64 size, r_size, lastSize = 0;
    std::unique_ptr<uint8_t[]> buff(nullptr);
    Proto* client;

    while (m_sock && m_sock->state() == QUdpSocket::BoundState && m_sock->hasPendingDatagrams())
    {
        size = sock()->pendingDatagramSize();
        if (size <= 0)
            continue;

        if (size > lastSize)
        {
            lastSize = size;
            buff.reset(new uint8_t[size]);
        }

        r_size = sock()->readDatagram((char*)buff.get(), size, &senderAdr, &senderPort);
        if (r_size != size)
            qCCritical(Log).noquote() << "Something wrong. Not all read. Read:" << r_size << "pending:" << size << sock()->errorString();

        client = getClient(senderAdr, senderPort);
        if (!client)
        {
            qCCritical(Log).noquote() << "No client" << senderAdr << senderPort;
            continue;
        }

        Botan::TLS::Channel* channel = client->channel();
        if (!channel)
        {
            qCCritical(Log).noquote() << "No channel" << client->clientName();
            continue;
        }

        bool first_active = !channel->is_active();

        try {
            client->updateLastMessageTime();
            channel->received_data(buff.get(), size);
        }
        catch(Botan::TLS::TLS_Exception& e)
        {
            QString type_str = QString::fromStdString(Botan::TLS::Alert(e.type()).type_string());
            qCCritical(Log).noquote() << client->clientName() << type_str << e.what();
        }
        catch(std::exception& e) { qCCritical(Log).noquote() << client->clientName() << e.what(); }
        catch(...) { qCCritical(Log).noquote() << client->clientName() << "Error in receided data Server::procData"; }

        if (first_active)
        {
            if (channel->timeout_check())
                qCWarning(Log).noquote() << client->clientName() << "Handshake timeout detected";

            if (channel->is_active())
                readyWrite();
        }
    }
}

// ------------------------------------

Proto::Proto()
{
    dev.open(QBuffer::ReadWrite);
}

QString Proto::clientName() const
{
    return hostString() + ':' + QString::number(port());
}

Proto *Proto::getClient(const QHostAddress &clientAddress, quint16 clientPort) { return equal(clientAddress, clientPort) ? this : nullptr; }

void Proto::tls_emit_data(const uint8_t data[], size_t size)
{
    qint64 sent = sock()->writeDatagram((char*)data, size, host(), port());

    if(sent == -1)
    {
        qCCritical(Log).noquote() << clientName() << tr("Error writing to socket.") + sock()->errorString();
        sock()->close();
    }
    else if(sent != static_cast<qint64>(size))
        qCWarning(Log).noquote() << clientName() << tr("Packet of length %1 truncated to %2").arg(clientName()).arg(size).arg(sent);
}

void Proto::tls_record_received(Botan::u64bit /*seq_no*/, const uint8_t data[], size_t size)
{
    dev.write((const char*)data, size);
    dev.seek(0);
    proccess_bytes( &dev );

    int nextStartPos = 0;
    if (dev.bytesAvailable())
    {
        nextStartPos = dev.bytesAvailable();
        dev.buffer() = dev.buffer().right(nextStartPos);
    }
    else
        dev.buffer().clear();
    dev.seek(nextStartPos);
}

void Proto::tls_alert(Botan::TLS::Alert alert)
{
    if (alert.type() == Botan::TLS::Alert::CLOSE_NOTIFY)
        emit DTLS_closed();
    qCDebug(Log).noquote() << clientName() << tr("Alert: %1").arg(alert.type_string().c_str());
}

void Proto::tls_verify_cert_chain(const std::vector<Botan::X509_Certificate> &cert_chain,
                                                const std::vector<std::shared_ptr<const Botan::OCSP::Response> > &ocsp,
                                                const std::vector<Botan::Certificate_Store *> &trusted_roots,
                                                Botan::Usage_Type usage,
                                                const std::string &hostname,
                                                const Botan::TLS::Policy &policy)
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

    auto status_string = clientName() + ' ' + tr("Certificate validation status: %1").arg(result.result_string().c_str());
    if(result.successful_validation())
    {
        qCDebug(Log).noquote() << status_string;

        auto status = result.all_statuses();
        if(status.size() > 0 && status[0].count(Botan::Certificate_Status_Code::OCSP_RESPONSE_GOOD))
            qCDebug(Log).noquote() << clientName() << tr("Valid OCSP response for this server");
    }
    else
        qCCritical(Log).noquote() << status_string;
}

bool Proto::tls_session_established(const Botan::TLS::Session &session)
{
    qCDebug(Log).noquote() << clientName() << tr("Handshake complete, %1 using %2")
                           .arg(session.version().to_string().c_str())
                           .arg(session.ciphersuite().to_string().c_str());

    if(!session.session_id().empty())
        qCDebug(Log).noquote() << clientName() << tr("Session ID %1").arg(Botan::hex_encode(session.session_id()).c_str());

    if(!session.session_ticket().empty())
        qCDebug(Log).noquote() << clientName() << tr("Session ticket %1").arg(Botan::hex_encode(session.session_ticket()).c_str());

    return true;
}

} // namespace DTLS
} // namespace Helpz
