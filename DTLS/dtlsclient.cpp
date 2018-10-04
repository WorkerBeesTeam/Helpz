#include <QDebug>
#include <QHostInfo>
#include <QUdpSocket>
#include <QDateTime>

#include <botan/tls_exceptn.h>

#include "dtlsclient.h"

namespace Helpz {
namespace DTLS {

Q_LOGGING_CATEGORY(ClientLog, "net.DTLS.client")

Client::Client(const std::vector<std::string> &next_protocols, const Database::ConnectionInfo &db_info, const QString& tls_policy_file, const QString &hostname, quint16 port, int checkServerInterval) :
    ProtoTemplate<Botan::TLS::Client,
            const Botan::TLS::Server_Information&,
            const Botan::TLS::Protocol_Version&,
            const std::vector<std::string>&>(),
    BotanHelpers(db_info, tls_policy_file),
    m_hostname(hostname), next_protocols(next_protocols)
{
    if (port)
        setPort(port);

    setSock(new QUdpSocket(this), true);
    connect(this, &Proto::DTLS_closed, this, &Client::close_connection);

    connect(&checkTimer, &QTimer::timeout, this, &Client::checkServer);
    checkTimer.setInterval(checkServerInterval);
    checkTimer.start();
}

void Client::setCheckServerInterval(int msec) {
    checkTimer.setInterval(msec);
}

void Client::setHostname(const QString &hostname) { m_hostname = hostname; }

void Client::init_client()
{
    if (!canConnect())
    {
        qCWarning(ClientLog) << "Can't initialize connection";
        return;
    }

    close_connection();

    QHostInfo hi = QHostInfo::fromName(m_hostname);
    QList<QHostAddress> addrs = hi.addresses();
    if (addrs.size())
    {
        setHost( addrs.first() );
        qCInfo(ClientLog).noquote().nospace() << "Try connect to " << m_hostname
                                              << (m_hostname != addrs.first().toString() ? " " + addrs.first().toString() : "")
                                              << ':' << port();

        init(this,
             Botan::TLS::Server_Information(m_hostname.toStdString(), port()),
             Botan::TLS::Protocol_Version::latest_dtls_version(),
             next_protocols);
    } else
        qCWarning(ClientLog).noquote() << "Can't get host info" << m_hostname << hi.errorString();
}

void Client::close_connection()
{
    if (dtls)
        dtls->close();
    sock()->close();
    resetCheckReturned();
}

void Client::checkServer()
{
    if (!canConnect())
        return;

    qint64 current_time = QDateTime::currentMSecsSinceEpoch();
    auto isBadTime = [&](int quality) -> bool {
//        if ((current_time - lastMsgTime()) >= ((checkTimer.interval() * quality) + 500))
//            std::cerr << "[" << lastMsgTime() << ' ' << (current_time - lastMsgTime()) << "] Quality " << quality << std::endl;
        return (current_time - lastMsgTime()) > ((checkTimer.interval() * quality) + 500);
    };

    if (!dtls || !dtls->is_active() ||
            (isBadTime(4) && !checkReturned()) )
    {
        if (ClientLog().isDebugEnabled()) {
            QString debug = "Reinit becose ";
            if (!dtls) debug += "DTLS is not init.";
            else if (!dtls->is_active()) debug += "DTLS is not active.";
            else if (isBadTime(4) && !checkReturned())
                debug += QString("freeze. %1 > %2").arg(current_time - lastMsgTime()).arg((checkTimer.interval() * 4) + 500);
            QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC,
                           ClientLog().categoryName()).debug().noquote() << debug;
        }

        init_client();
    }
    else if (isBadTime(3)) dtls->renegotiate(true);
    else if (isBadTime(2)) dtls->renegotiate();
    else if (isBadTime(1)) sendCmd(Network::Cmd::Ping); // Отправка проверочного сообщения
}

} // namespace DTLS
} // namespace Helpz
