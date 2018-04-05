
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <netinet/in.h>

#include "dtlscookie.h"

namespace Helpz {

DTLSCookie* DTLSCookieObj = nullptr;
DTLSCookie::DTLSCookie()
{
    if (!DTLSCookieObj)
    {
        DTLSCookieObj = this;
        init();
    }
    else throw;
}

int DTLSCookie::GenerateCookie(SSL *ssl, uint8_t *cookie, uint32_t *cookie_len) {
    return DTLSCookieObj->genCookie(ssl, cookie, cookie_len);
}

int DTLSCookie::VerifyCookie(SSL *ssl, uint8_t *cookie, uint32_t cookie_len) {
    return DTLSCookieObj->verifyCookie(ssl, cookie, cookie_len);
}

bool DTLSCookie::init()
{
    if (!cookie_initialized)
        cookie_initialized = RAND_bytes(secret, COOKIE_SECRET_LENGTH);
    return cookie_initialized;
}

bool DTLSCookie::cookieProc(SSL *ssl, uint8_t* result, uint32_t *resultlength)
{
    if (!init())
        return false;
    union {
        struct sockaddr sa;
        struct sockaddr_in s4;
    } peer;

    /* Read peer information */
    (void)BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);

    if (peer.sa.sa_family == AF_INET)
    {
        uint32_t length = sizeof(peer.s4.sin_port)
                + sizeof(struct in_addr);

        uint8_t* buffer = (uint8_t*)OPENSSL_malloc(length);
        if (buffer == NULL)
            return false;

        memcpy(buffer,
               &peer.s4.sin_port,
               sizeof(peer.s4.sin_port));
        memcpy(buffer + sizeof(peer.s4.sin_port),
               &peer.s4.sin_addr,
               sizeof(struct in_addr));

        /* Calculate HMAC of buffer using the secret */
        HMAC(EVP_sha1(), secret, COOKIE_SECRET_LENGTH,
             buffer, length, result, resultlength);
        OPENSSL_free(buffer);

        return true;
    }
    return false;
}

int DTLSCookie::genCookie(SSL *ssl, uint8_t *cookie, uint32_t *cookie_len)
{
    uint8_t result[EVP_MAX_MD_SIZE];
    uint32_t resultlength;

    if (cookieProc(ssl, result, &resultlength))
    {
        memcpy(cookie, result, resultlength);
        *cookie_len = resultlength;
        return 1;
    }
    return 0;
}

int DTLSCookie::verifyCookie(SSL *ssl, uint8_t *cookie, uint32_t cookie_len)
{
    uint8_t result[EVP_MAX_MD_SIZE];
    uint32_t resultlength;
    if (cookieProc(ssl, result, &resultlength))
        if (cookie_len == resultlength && memcmp(result, cookie, resultlength) == 0)
            return 1;
    return 0;
}
} // namespace Helpz

