#ifndef ZIMNIKOV_UDPCLIENT_H
#define ZIMNIKOV_UDPCLIENT_H

#include <QHostAddress>

namespace Helpz {
namespace Network {

class UDPClient
{
public:
    void setHost(const QHostAddress &host);
    QHostAddress host() const;
    QString hostString() const;

    void setPort(quint16 port);
    quint16 port() const;

    bool equal(const QHostAddress& host, quint16 port) const;
protected:
    QHostAddress m_host;
    quint16 m_port;
};

} // namespace Network
} // namespace Helpz

#endif // ZIMNIKOV_UDPCLIENT_H
