#ifndef ZIMNIKOV_DTLSSOCKET_H
#define ZIMNIKOV_DTLSSOCKET_H

#include <string>

typedef struct bio_st BIO;
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_method_st SSL_METHOD;

typedef struct x509_store_ctx_st X509_STORE_CTX;

struct sockaddr_in;

namespace Helpz {

class DTLSSocket
{
public:
    DTLSSocket(const SSL_METHOD *meth);
    ~DTLSSocket();

    uint16_t port() const;
    void setPort(const uint16_t &port);

    std::string host() const;
    void setHost(const std::string &server);

    bool setCertificateChain(const char* file);
    bool setPrivateKey(const char* file);
    bool setCACertificate(const char* file);

    void close();
    std::string certificatePassword() const;
    void setCertificatePassword(const std::string &certificatePassword);

protected:
    bool open();
    int sock() const;
    sockaddr_in addr();

    SSL_CTX *ctx;
    SSL *ssl;

    static BIO *dtls_bio_err;
    static void dtls_report_err (const char *fmt, ...);
    static void dtls_info_callback (const SSL *ssl, int where, int ret);
    friend int dtls_verify_callback (int ok, X509_STORE_CTX *ctx);
private:
    static int checkPasswordCallback(char *buf, int num, int rwflag, void *userdata);

    int m_sock;
    uint16_t m_port;
    std::string m_host;

    std::string m_certificatePassword;
};

} // namespace Helpz

#endif // ZIMNIKOV_DTLSSOCKET_H
