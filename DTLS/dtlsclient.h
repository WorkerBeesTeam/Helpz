#ifndef ZIMNIKOV_DTLS_CLIENT_H
#define ZIMNIKOV_DTLS_CLIENT_H

#include <QBuffer>
#include <QTimer>

#include <Helpz/dtlsbasic.h>
#include <Helpz/dtlsproto.h>
#include <botan/tls_client.h>

namespace Helpz {
namespace DTLS {

Q_DECLARE_LOGGING_CATEGORY(ClientLog)

class Client :
        public ProtoTemplate<Botan::TLS::Client,
        const Botan::TLS::Server_Information&,
        const Botan::TLS::Protocol_Version&,
        const std::vector<std::string>&>,
        public BotanHelpers
{
    Q_OBJECT
public:
    Client(const std::vector<std::string>& next_protocols, Helpz::Database::Base *db, const QString &tls_policy_file,
           const QString &hostname = QString(), quint16 port = 0, int checkServerInterval = 15000);

    void setCheckServerInterval(int msec);

    virtual bool canConnect() const { return true; }
public slots:
    void setHostname(const QString &hostname);
    void init_client();
    void close_connection();
private slots:
    void checkServer();
private:
    QString m_hostname;
    std::vector<std::string> next_protocols;

    QTimer checkTimer;
};

} // namespace DTLS
} // namespace Helpz

#endif // ZIMNIKOV_DTLS_CLIENT_H
