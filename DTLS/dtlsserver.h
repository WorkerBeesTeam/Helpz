#ifndef HELPZ_DTLS_SERVER_H
#define HELPZ_DTLS_SERVER_H

#include <functional>
#include <Helpz/dtlsproto.h>

#include <QUdpSocket>
#include <QTimer>
#include <QBuffer>

namespace Helpz {
namespace DTLS {

class ServerNode;

class Server : public QUdpSocket, public BotanHelpers, public ProtoHelper
{
    Q_OBJECT
public:
    Server(const Database::ConnectionInfo &db_info, const QString& tls_policy_file, const QString &crt_file, const QString &key_file);
    ~Server();

    void clear_clients();

    ServerNode* client(QHostAddress host, quint16 port) const;
    void remove_client(ServerNode* node);
//    void remove_client_if(std::function<bool(ServerNode*)> cond_func);
    const std::vector<ServerNode *>& clients() const;

    virtual ServerNode* createClient() = 0;
public slots:
    bool bind(quint16 port);
private:
    void sock_return_zero() override;
    Proto* getClient(const QHostAddress& clientAddress, quint16 clientPort) override;
    std::vector<ServerNode*> m_clients;
};

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_SERVER_H
