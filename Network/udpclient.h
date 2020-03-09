#ifndef HELPZ_UDPCLIENT_H
#define HELPZ_UDPCLIENT_H

#include <QHostAddress>

namespace Helpz {
namespace Net {

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

} // namespace Net
} // namespace Helpz

#endif // HELPZ_UDPCLIENT_H
