
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

#include <iostream>

#include "dtlssocket.h"

namespace Helpz {

BIO * DTLSSocket::dtls_bio_err = nullptr;

void DTLSSocket::dtls_report_err (const char *fmt, ...)
{
    va_list args;

    va_start (args, fmt);

    vfprintf (stderr, fmt, args);
    ERR_print_errors (dtls_bio_err);
    va_end(args);
}

void DTLSSocket::dtls_info_callback (const SSL *ssl, int where, int ret)
{
    DTLSSocket::dtls_report_err ("SSL state [\"%s\"]: %s\n");
    const char *str
            = where & SSL_ST_CONNECT ? "connect" : where & SSL_ST_ACCEPT ? "accept" : "undefined";
    if (where & SSL_CB_LOOP)
        dtls_report_err ("SSL state [\"%s\"]: %s\n", str, SSL_state_string_long (ssl));
    else if (where & SSL_CB_ALERT)
        dtls_report_err ("SSL: alert [\"%s\"]: %s : %s\n", where & SSL_CB_READ ? "read" : "write",
            SSL_alert_type_string_long (ret), SSL_alert_desc_string_long (ret));
}

DTLSSocket::DTLSSocket(const SSL_METHOD *meth) :
    ctx(nullptr),
    ssl(nullptr),
    m_sock(-1)
{
    if (!dtls_bio_err)
    {
        SSL_library_init ();
        ERR_clear_error ();
        ERR_load_BIO_strings();
        SSL_load_error_strings ();
        OpenSSL_add_all_algorithms ();

        dtls_bio_err = BIO_new_fp (stderr, BIO_NOCLOSE);
    }

    ctx = SSL_CTX_new (meth);	// New SSL CTX object as Framework to establish new SSL connections

    // Enable this if u hve generated your certificates with password. If certs are generated with '-nodes' option, this is not required
//    SSL_CTX_set_default_passwd_cb(ctx, checkPasswordCallback);

    SSL_CTX_set_verify_depth (ctx, 2);
    SSL_CTX_set_options (ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2);
    SSL_CTX_set_read_ahead (ctx, 1); // Required for DTLS - please refer apps/s_client.c source file

#ifdef DEBUG
    SSL_CTX_set_info_callback (ctx, &dtls_info_callback);
#endif
}

DTLSSocket::~DTLSSocket()
{
    if (ssl)
    {
        SSL_shutdown (ssl);
        SSL_free(ssl);
    }
    if (ctx) SSL_CTX_free(ctx);
    close();
}

uint16_t DTLSSocket::port() const { return m_port; }
void DTLSSocket::setPort(const uint16_t &port) { m_port = port; }

std::string DTLSSocket::host() const { return m_host; }
void DTLSSocket::setHost(const std::string &server) { m_host = server; }

bool DTLSSocket::setCertificateChain(const char *file)
{
    bool res = SSL_CTX_use_certificate_chain_file (ctx, file);
    if (!res)
        ERR_print_errors (dtls_bio_err);
    return res;
}

bool DTLSSocket::setPrivateKey(const char *file)
{
    if (SSL_CTX_use_PrivateKey_file (ctx, file, SSL_FILETYPE_PEM) &&
            SSL_CTX_check_private_key (ctx))
        return true;
    return false;
}

bool DTLSSocket::setCACertificate(const char *file)
{
    return SSL_CTX_load_verify_locations (ctx, file, 0);
}

void DTLSSocket::close() { ::close( m_sock ); }

int DTLSSocket::sock() const { return m_sock; }

sockaddr_in DTLSSocket::addr()
{
    sockaddr_in saddr;
    memset((void *)&saddr,'\0',sizeof(struct sockaddr_in));

    hostent* he = nullptr;
    if (m_host.empty() || (he = gethostbyname( m_host.c_str() )))
    {
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons( port() );
        saddr.sin_addr.s_addr = m_host.empty() ? htonl(INADDR_ANY) : *(in_addr_t *)he->h_addr;
    }

    return saddr;
}

std::string DTLSSocket::certificatePassword() const { return m_certificatePassword; }
void DTLSSocket::setCertificatePassword(const std::string &certificatePassword) { m_certificatePassword = certificatePassword; }

bool DTLSSocket::open()
{
    ssl = SSL_new (ctx);
    if ((m_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        return false;
    return true;
}

int DTLSSocket::checkPasswordCallback(char */*buf*/, int /*num*/, int /*rwflag*/, void */*userdata*/)
{
//    if (buf == nullptr || num < (m_certificatePassword.length() + 1))
        return 0;

//    strcpy (buf, m_certificatePassword.c_str());
//    return m_certificatePassword.length();
}

} // namespace Helpz

