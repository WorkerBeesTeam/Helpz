#ifndef ZIMNIKOV_DTLSSERVER_H
#define ZIMNIKOV_DTLSSERVER_H

#include "dtlssocket.h"
#include "dtlscookie.h"

namespace Helpz {

class DTLSServer : public DTLSSocket
{
public:
    DTLSServer();

    bool setDHParams(const char* file);

    bool bind();
private:
    int dtls_get_data();
    bool genRsaKey();

    DTLSCookie cookie;
};

} // namespace Helpz

// Example:
//Helpz::DTLSServer server;
//server.setPort(41234);
//server.setCertificateChain("dtls.pem");
//server.setCertificatePassword("dtls-example");
//server.setPrivateKey("dtls.key");
//server.setCACertificate("dtlsCA.pem");
//server.setDHParams("dtlsDH.pem");
//server.bind();

#endif // ZIMNIKOV_DTLSSERVER_H
