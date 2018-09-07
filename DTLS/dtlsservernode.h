#ifndef HELPZ_DTLS_SERVERNODE_H
#define HELPZ_DTLS_SERVERNODE_H

#include <botan/tls_server.h>
#include <Helpz/dtlsproto.h>

namespace Helpz {
namespace DTLS {

class ServerNode : public ProtoTemplate<Botan::TLS::Server, bool/*is_datagram*/>
{
    Q_OBJECT
public:
    explicit ServerNode();
    ~ServerNode();
};

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_SERVERNODE_H
