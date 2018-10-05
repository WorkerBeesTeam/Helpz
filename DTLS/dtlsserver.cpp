#include <QTimer>
#include <QCoreApplication>

#include "dtlsservernode.h"
#include "dtlsserver.h"

QDebug &operator<< (QDebug &dbg, const std::string &str) { return dbg << str.c_str(); }

namespace Helpz {
namespace DTLS {

Server::Server(const Database::ConnectionInfo &db_info, const QString &tls_policy_file, const QString &crt_file, const QString &key_file) :
    QUdpSocket(), BotanHelpers(db_info, tls_policy_file, crt_file, key_file)
{
    setSock(this, true);
}

Server::~Server()
{
    clear_clients();
    close();
}

void Server::clear_clients()
{
    for(ServerNode* cl: m_clients)
        cl->deleteLater();
    m_clients.clear();
}

ServerNode *Server::client(QHostAddress host, quint16 port) const
{
    for (ServerNode* item: m_clients)
        if (item->equal(host, port))
            return item;
    return nullptr;
}

void Server::remove_client(Helpz::DTLS::ServerNode *node)
{
    qCDebug(Log).noquote() << node->clientName() << "Remove";
    m_clients.erase(std::remove(m_clients.begin(), m_clients.end(), node), m_clients.end());
    node->deleteLater();
}

//void Server::remove_client_if(std::function<bool (ServerNode *)> cond_func) {
//    m_clients.erase(std::remove_if(m_clients.begin(), m_clients.end(), cond_func), m_clients.end());
//}

const std::vector<Helpz::DTLS::ServerNode *> &Server::clients() const { return m_clients; }

bool Server::bind(quint16 port)
{
    if (state() != ClosingState)
        close();

    bool binded = QUdpSocket::bind(port);
    if (binded)
        qCDebug(Log).noquote() << tr("Listening for new connections on udp port") << localPort();
    else
        qCCritical(Log).noquote() << tr("Fail bind to udp port") << localPort() << errorString();
    return binded;
}
void Server::sock_return_zero() {
    qDebug(Log) << "Reinit sock";
    clear_clients();
    Server::bind(localPort());
}

Proto *Server::getClient(const QHostAddress &clientAddress, quint16 clientPort)
{
    ServerNode* node = client(clientAddress, clientPort);
    if (!node)
    {
        node = createClient();
        node->setSock(this);
        node->setHost(clientAddress);
        node->setPort(clientPort);

        qCDebug(Log).noquote() << "Create client" << node->clientName() << (qintptr)node << "Count" << clients().size();

        node->init(this, true/*is_datagram*/);

        m_clients.push_back(node);
    }

    return node;
}

} // namespace DTLS
} // namespace Helpz
