#include <QDebug>
#include <QHostInfo>
#include <QUdpSocket>
#include <QDateTime>

#include <botan/tls_exceptn.h>

#include "dtlsclient.h"

namespace Helpz {
namespace DTLS {

Q_LOGGING_CATEGORY(ClientLog, "net.DTLS.client")

Client::Client(const std::vector<std::string> &next_protocols, Helpz::Database::Base* db, const QString& tls_policy_file, const QString &hostname, quint16 port, int checkServerInterval) :
    ProtoTemplate<Botan::TLS::Client,
            const Botan::TLS::Server_Information&,
            const Botan::TLS::Protocol_Version&,
            const std::vector<std::string>&>(),
    BotanHelpers(db, tls_policy_file),
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

void Client::setCheckServerInterval(int msec)
{
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
    qCInfo(ClientLog) << "Try connect to" << m_hostname << port();

    QHostInfo hi = QHostInfo::fromName(m_hostname);
    auto addrs = hi.addresses();
    if (addrs.size())
    {
        setHost( addrs.first() );

        init(this,
             Botan::TLS::Server_Information(m_hostname.toStdString(), port()),
             Botan::TLS::Protocol_Version::latest_dtls_version(),
             next_protocols);
    }
}

void Client::close_connection()
{
    if (dtls)
        dtls->close();
    sock()->close();
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
        qCDebug(ClientLog) << "REINIT"
                 << "OBJ" << (!!dtls)
                 << "IS_ACTIVE" << (dtls ? dtls->is_active() : false)
                 << "LAST_MESSAGE" << '[' << current_time << lastMsgTime() << (current_time - lastMsgTime()) << ']'
                 << "CheckReturned" << checkReturned();

        init_client();
    }
    else if (isBadTime(3)) dtls->renegotiate(true);
    else if (isBadTime(2)) dtls->renegotiate();
    else if (isBadTime(1)) sendCmd(Network::Cmd::Ping); // Отправка проверочного сообщения
}

} // namespace DTLS
} // namespace Helpz
