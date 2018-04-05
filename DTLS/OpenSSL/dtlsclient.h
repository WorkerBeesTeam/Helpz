#ifndef DTLSCLIENT_H
#define DTLSCLIENT_H

#include <string>

#include "dtlssocket.h"

namespace Helpz {

class DTLSClient : public DTLSSocket
{
public:
    DTLSClient();

    bool connect();
protected:
private:

    BIO *sbio;
    int dtls_connect();
    int handle_data();
};

} // namespace Helpz

// Example:
//Helpz::DTLSClient client;
//client.setHost("localhost");
//client.setPort(41234);
//qDebug() << client.connect();

#endif // DTLSCLIENT_H
