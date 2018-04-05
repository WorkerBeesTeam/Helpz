#include "udpclient.h"

namespace Helpz {
namespace Network {

void UDPClient::setHost(const QHostAddress &host) { m_host = host; }
QHostAddress UDPClient::host() const { return m_host; }

QString UDPClient::hostString() const
{
    QString h = host().toString();
    if (h.startsWith("::ffff:"))
        h = h.mid(7);
    return h;
}

void UDPClient::setPort(quint16 port) { m_port = port; }
quint16 UDPClient::port() const { return m_port; }

bool UDPClient::equal(const QHostAddress& host, quint16 port) const
{
    return host.toIPv4Address() == m_host.toIPv4Address() && port == m_port;
}

} // namespace Network
} // namespace Helpz
