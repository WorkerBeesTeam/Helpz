#include <QTimer>

#include "dtlsservernode.h"
#include "dtlsserver.h"

QDebug &operator<< (QDebug &dbg, const std::string &str) { return dbg << str.c_str(); }

namespace Helpz {
namespace DTLS {

Server::Server(Helpz::Database::Base* db, const QString &tls_policy_file, const QString &crt_file, const QString &key_file) :
    QUdpSocket(), BotanHelpers(db, tls_policy_file, crt_file, key_file)
{
    setSock(this, true);
}

Server::~Server()
{
    close();
    for(auto cl: m_clients)
        cl->deleteLater();
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
    m_clients.erase(std::find(m_clients.begin(), m_clients.end(), node));
}

void Server::remove_client_if(std::function<bool (ServerNode *)> cond_func)
{
    m_clients.erase(std::remove_if(m_clients.begin(), m_clients.end(), cond_func), m_clients.end());
}

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
