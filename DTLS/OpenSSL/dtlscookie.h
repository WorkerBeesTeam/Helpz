#ifndef DTLSCOOKIE_H
#define DTLSCOOKIE_H

#include <stdint.h>

#define	COOKIE_SECRET_LENGTH	16

typedef struct ssl_st SSL;

namespace Helpz {

class DTLSCookie
{
public:
    DTLSCookie();

    static int GenerateCookie(SSL *ssl, uint8_t *cookie, uint32_t *cookie_len);
    static int VerifyCookie(SSL *ssl, uint8_t *cookie, uint32_t cookie_len);
private:
    bool cookieProc(SSL *ssl, uint8_t* result, uint32_t *resultlength);
    int genCookie(SSL *ssl, uint8_t *cookie, uint32_t *cookie_len);
    int verifyCookie(SSL *ssl, uint8_t *cookie, uint32_t cookie_len);

    bool init();

    bool cookie_initialized = false;
    uint8_t secret[COOKIE_SECRET_LENGTH];

};

} // namespace Helpz

#endif // DTLSCOOKIE_H
