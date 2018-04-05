
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>

#include "dtlsserver.h"

namespace Helpz {

int verify_depth = 0;
int dtls_verify_callback(int ok, X509_STORE_CTX *ctx)
{
    int depth = X509_STORE_CTX_get_error_depth (ctx);
    if (!ok && verify_depth >= depth)
            ok = 1;

    char buf[256] = {0};
    X509 *err_cert = X509_STORE_CTX_get_current_cert (ctx);
    X509_NAME_oneline(X509_get_subject_name(err_cert), buf, sizeof buf);

    std::cerr << "dtls_verify_callback" << buf << std::endl;

    switch (ctx->error)
    {
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
        X509_NAME_oneline(X509_get_issuer_name (ctx->current_cert), buf, sizeof buf);
        std::cerr << "issuer=" << buf << std::endl;
        break;
    case X509_V_ERR_CERT_NOT_YET_VALID:
    case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
        std::cerr << "notBefore=";
        ASN1_TIME_print(DTLSSocket::dtls_bio_err, X509_get_notBefore (ctx->current_cert));
        std::cerr << std::endl;
        break;
    case X509_V_ERR_CERT_HAS_EXPIRED:
    case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
        std::cerr << "notAfter=";
        ASN1_TIME_print(DTLSSocket::dtls_bio_err, X509_get_notAfter(ctx->current_cert));
        std::cerr << std::endl;
        break;
    }

    return (ok);
}


BIO *bio_err = NULL;
BIO *bio_s_out = NULL;

DTLSServer* DTLSServerObj = nullptr;

DTLSServer::DTLSServer() :
    DTLSSocket( DTLSv1_server_method() )
{
    DTLSServerObj = this;

    bio_err = BIO_new_fp (stderr, BIO_NOCLOSE);
    bio_s_out = BIO_new_fp (stderr, BIO_NOCLOSE);

    SSL_CTX_set_quiet_shutdown (ctx, 1);

    SSL_CTX_set_cookie_generate_cb(ctx, &cookie.GenerateCookie);
    SSL_CTX_set_cookie_verify_cb(ctx, &cookie.VerifyCookie);

    genRsaKey();

    int s_server_verify = SSL_VERIFY_CLIENT_ONCE; //SSL_VERIFY_NONE;
    SSL_CTX_set_verify(ctx, s_server_verify, dtls_verify_callback);

    SSL_CTX_set_options (ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2);
    SSL_CTX_set_session_id_context (ctx, (unsigned char*)"dtls-example", strlen ("dtls-example"));
}

bool DTLSServer::setDHParams(const char *file)
{
    // DH shud of max size 512 (dnt use 1024)

    bool res = false;
    DH *ret = 0;

    BIO *bio = BIO_new_file (file, "r");

    if (bio)
    {
        ret = PEM_read_bio_DHparams (bio, NULL, NULL, NULL);
        BIO_free (bio);

        if (SSL_CTX_set_tmp_dh (ctx, ret) >= 0)
            res = true;
        DH_free (ret);
    }
    return res;
}

bool DTLSServer::bind()
{
    if (!open())
        return false;

    auto adr = addr();
    if (::bind(sock(), (sockaddr *)&adr, sizeof (sockaddr)) == -1)
        return false;

    int flags = fcntl(sock(), F_GETFL, 0);
    /* Setting O_NODELAY option for socket */
    if (flags != -1)
    {
        flags |= O_NDELAY;
        fcntl (sock(), F_SETFL, flags);
    }

    for (;;)
        if (dtls_get_data() < 0)
            break;

    close();
    return true;
}

static int init_ssl_connection(SSL *con)
{
    int i = SSL_accept(con);
    if (i <= 0)
    {
        int err = SSL_get_error (con, i);
        if ((SSL_ERROR_WANT_READ == err) || (SSL_ERROR_WANT_WRITE == err))
            std::cerr << (SSL_ERROR_WANT_READ == err ? "SSL_ERROR_WANT_READ" : "SSL_ERROR_WANT_WRITE") << std::endl;

        if (BIO_sock_should_retry (i))
            return 1;

        BIO_printf(bio_err, "ERROR\n");

        long verify_error = SSL_get_verify_result (con);
        if (verify_error != X509_V_OK)
            BIO_printf(bio_err,"verify error:%s\n",
                X509_verify_cert_error_string(verify_error));
        else
        {
            std::cerr << __func__ << "X509_V_OK but error" << std::endl;
            ERR_print_errors (bio_err);
        }
        return 0;
    }

    if (con->hit) BIO_printf (bio_s_out, "Reused session-id\n");
    if (SSL_ctrl (con, SSL_CTRL_GET_FLAGS, 0, NULL) & TLS1_FLAGS_TLS_PADDING_BUG)
        BIO_printf (bio_s_out, "Peer has incorrect TLSv1 block padding\n");

    return 1;
}

int DTLSServer::dtls_get_data()
{
    int i = 0;
    int ret = 1, width = 0;
    BIO *sbio = NULL;

//    SSL *con = SSL_new(ctx);
    std::string ext = "exit";
    SSL_clear (ssl);

    int bufsize = SSL3_RT_MAX_PLAIN_LENGTH;
    char *buf = (char*)OPENSSL_malloc (bufsize);
    if (!buf)
        goto ERR;

    if (SSL_version (ssl) == DTLS1_VERSION)
    {
        sbio = BIO_new_dgram(sock(), BIO_NOCLOSE);

        timeval t_recv = {30,0}, t_send = t_recv;
        BIO_ctrl (sbio, BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &t_recv);
        BIO_ctrl (sbio, BIO_CTRL_DGRAM_SET_SEND_TIMEOUT, 0, &t_send);
        BIO_ctrl (sbio, BIO_CTRL_DGRAM_MTU_DISCOVER, 0, NULL);
        SSL_set_options (ssl, SSL_OP_COOKIE_EXCHANGE);
    }

    SSL_set_bio(ssl, sbio, sbio);
    SSL_set_accept_state (ssl);

    width = sock() + 1;
    int read_from_sslcon;
    fd_set readfds;

    for (;;)
    {
        read_from_sslcon = SSL_pending(ssl);
        if (!read_from_sslcon)
        {
            FD_ZERO(&readfds);
            FD_SET(sock(), &readfds);

            timeval tv = {1,0};
            i = select(width, &readfds, NULL, NULL, &tv);

            if ((SSL_version(ssl) == DTLS1_VERSION) && DTLSv1_handle_timeout(ssl) > 0)
                BIO_printf(bio_err,"TIMEOUT occured\n");

            if (i < 0)
                continue;

            if (FD_ISSET (sock(), &readfds))
                read_from_sslcon = 1;
            else
            {
                ret = 2;
                goto shut;
            }
        }

        if (read_from_sslcon)
        {
            if (!SSL_is_init_finished(ssl))
            {
                if (!init_ssl_connection(ssl))
                {
                    ret = 1;
                    goto ERR;
                }
            }
            else
            {
                bool again;
                do {
                    again = false;

                    i = SSL_read(ssl, (char *) buf, bufsize);
                    switch (SSL_get_error(ssl, i))
                    {
                        case SSL_ERROR_NONE:
                            write( fileno(stdout), buf, (uint32_t)i );
                            if (SSL_pending(ssl))
                                again = true;
                            else
                                ret = 0;

                            again = ext != buf;
                            break;
                        case SSL_ERROR_WANT_READ:
                        again = true;
                        case SSL_ERROR_WANT_WRITE:
                        case SSL_ERROR_WANT_X509_LOOKUP:
//                            BIO_printf(bio_s_out,"Read BLOCK\n");
                            break;
                        case SSL_ERROR_SYSCALL:
                        case SSL_ERROR_SSL:
                            BIO_printf(bio_s_out,"ERROR\n");
                            ERR_print_errors(bio_err);
                            ret = 1;
                            goto ERR;
                        case SSL_ERROR_ZERO_RETURN:
                            BIO_printf(bio_s_out,"\nDONE\n");
                            ret = 0;
                            goto ERR;
                    }
                }
                while (again);

                break;
            }
        }
    }
ERR:
    if (0 == ret)
    {
        char temp [] = "ACK from SERVER: READ SUCCESSFULLY DONE\n";
        for (;;)
        {
            i = SSL_write (ssl, temp, strlen (temp));

            switch (SSL_get_error (ssl, i))
            {
                case SSL_ERROR_NONE:
                    if (SSL_pending (ssl))
                        break;
                    else
                        goto WRITEDONE;
                case SSL_ERROR_WANT_WRITE:
                case SSL_ERROR_WANT_READ:
                case SSL_ERROR_WANT_X509_LOOKUP:
                    BIO_printf (bio_s_out, "Write BLOCK\n");
                    break;
                case SSL_ERROR_SYSCALL:
                case SSL_ERROR_SSL:
                    BIO_printf (bio_s_out, "ERROR\n");
                    ERR_print_errors (bio_err);
                    ret = 1;
                    goto WRITEDONE;
                case SSL_ERROR_ZERO_RETURN:
                    BIO_printf (bio_s_out, "\nDONE\n");
                    ret = 1;
                    goto WRITEDONE;
            }
        }
    }
WRITEDONE:

#if 1
    SSL_set_shutdown (ssl, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
#else
    SSL_shutdown(con);
#endif

shut:
//    if (con != NULL) SSL_free(con);
    if (2 != ret)
        BIO_printf(bio_s_out,"CONNECTION CLOSED\n");

    if (buf != NULL)
    {
        OPENSSL_cleanse (buf, bufsize);
        OPENSSL_free (buf);
    }

    if ((ret >= 0) && (2 != ret))
        BIO_printf (bio_s_out, "ACCEPT\n");
    return(ret);
}

bool DTLSServer::genRsaKey()
{
    RSA *rsa = RSA_generate_key (1024, RSA_F4, NULL, NULL);
    if (!SSL_CTX_set_tmp_rsa (ctx, rsa))
        return true;

    RSA_free (rsa);
    return false;
}

} // namespace Helpz

