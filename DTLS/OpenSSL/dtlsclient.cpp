#include <iostream>

#include "dtlsclient.h"

#if 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>

#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

//extern "C" {
//extern BIO *dtls_bio_err;

//int dtls_exit_err (char *string);
//int dtls_generate_rsa_key (SSL_CTX *ctx);
//void dtls_destroy_ctx (SSL_CTX *);
//void dtls_report_berr (char *, ...);
//int dtls_password_cb (char *, int, int, void *);
//void dtls_load_dh_params (SSL_CTX *ctx, char *dh_file);
//void dtls_info_callback (const SSL *ssl, int where, int ret);
//int dtls_verify_callback (int ok, X509_STORE_CTX *ctx);
//}

#define SIZ_SERIAL_NO   10
//#define PASSWORD "dtls-example"
//#define DTLSC_CERT "dtlsc.pem"
//#define DTLSC_KEY_CERT "dtlsc.key"
//#define DTLSC_ROOT_CACERT "dtlsCA.pem"

/* global variables */

//extern int errno;
//static SSL_CTX *dtls_setup_sslclient (void);
//extern void dtls_info_callback (const SSL *ssl, int where, int ret);

#endif

namespace Helpz {

int DTLSClient::dtls_connect ()
{
    struct timeval timeout;
    fd_set wfd;
    int width = 0;
    struct sockaddr peer;
    int peerlen = sizeof (struct sockaddr);

//    ssl = SSL_new (ctx);
//	if (DTLS1_VERSION == SSL_version (ssl))
//		fprintf (stderr, "%s: %s(): Yes: DTLS1_VERSION CLIENT\n", __FILE__, __func__);
    sbio = BIO_new_dgram (sock(), BIO_NOCLOSE);
    if (getsockname (sock(), &peer, (socklen_t *)&peerlen) < 0)
    {
        dtls_report_err("asd");
        dtls_report_err ("%s: %s(): getsockname FAILED: %s\n", __FILE__, __func__, strerror (errno));
        shutdown (sock(), SHUT_RDWR);
        return -1;
    }
    (void)BIO_ctrl_set_connected (sbio, 1, &peer);
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    BIO_ctrl (sbio, BIO_CTRL_DGRAM_SET_SEND_TIMEOUT, 0, &timeout);
    BIO_ctrl(sbio, BIO_CTRL_DGRAM_MTU_DISCOVER, 0, NULL);
    SSL_set_bio (ssl, sbio, sbio);
    SSL_set_connect_state (ssl);
//	fprintf (stderr, "%s: %s(): Am now here @ %d\n", __FILE__, __func__, __LINE__);
    width = SSL_get_fd (ssl) + 1;
    FD_ZERO (&wfd);
    FD_SET (SSL_get_fd (ssl), &wfd);
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    if (select (width, NULL, &wfd, NULL, &timeout) < 0)
    {
        return 1;
    }
    return 0;
}

int DTLSClient::handle_data()
{
    int retval;
    char sendbuf[128/*SSL3_RT_MAX_PLAIN_LENGTH*/] = {0};

//    char* sendbuf = malloc(1024 * 1024);
//    fprintf (stderr, "Buff size %d\n", sizeof (sendbuf));

//	fprintf (stderr, "%s(): received a call...\n", __func__);
    strncpy (sendbuf, "MSG from CLIENT: Hello Server! And biiig many big text la-la-la-la-la-la-la-la-la-la-la-la-la-la-la-la\n", sizeof (sendbuf));
//    memset (sendbuf, '=', sizeof (sendbuf) - 2);

    std::string val;
    while (1)
    {
        std::cin >> val;
        retval = SSL_write (ssl, val.c_str(), val.length());

//        retval = SSL_write (ssl, sendbuf, sizeof (sendbuf));
//		fprintf (stderr, "%s: %s(): count: %d\n", __FILE__, __func__, retval);
        switch (SSL_get_error (ssl, retval))
        {
            case SSL_ERROR_NONE:
                if (retval == sizeof (sendbuf))
                {
//                    fprintf (stderr, "%s(): Am done with my write\n", __func__);
//                    goto WRITEDONE;
                }
                break;
            case SSL_ERROR_WANT_READ:
                fprintf (stderr, "%s: %s(): Want read - am now @ %d\n", __FILE__, __func__, __LINE__);
                break;
            case SSL_ERROR_WANT_X509_LOOKUP:
            case SSL_ERROR_WANT_WRITE:
                fprintf (stderr, "%s: %s(): Want write - am now @ %d\n", __FILE__, __func__, __LINE__);
                break;
            case SSL_ERROR_ZERO_RETURN:
                goto WRITEDONE;
            case SSL_ERROR_SSL:
            case SSL_ERROR_SYSCALL:
                dtls_report_err ("%s: %s(): Data send failed.\n", __FILE__, __func__);
                return -1;
        }
    }

WRITEDONE:
    memset (sendbuf, 0, sizeof (sendbuf));
    for (;;)
    {
        retval = SSL_read (ssl, sendbuf, sizeof (sendbuf));
        switch (SSL_get_error (ssl, retval))
        {
            case SSL_ERROR_NONE:
                write (fileno (stderr), sendbuf, (unsigned int )retval);
                if (SSL_pending (ssl))
                {
                    fprintf (stderr, "%s(): Some more stuff yet to come... letz wait for "\
                        "that..\n", __func__);
                    break;
                }
                else
                {
                    fprintf (stderr, "%s(): mmm ... no more to come... letz finish it "\
                        "off...\n", __func__);
                    return 0;
                }
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_X509_LOOKUP:
                fprintf (stderr, "%s: %s(): Read BLOCK - am now @ %d\n", __FILE__, __func__, __LINE__);
                break;
            case SSL_ERROR_SYSCALL:
            case SSL_ERROR_SSL:
                dtls_report_err ("%s: %s(): Data READ failed - am now @ %d\n", __FILE__, __func__, \
                    __LINE__);
                return -1;
            case SSL_ERROR_ZERO_RETURN:
                fprintf (stderr, "%s: %s(): Am DONE\n", __FILE__, __func__);
                return 0;
        }
    }

    return 0;
}

DTLSClient::DTLSClient() :
    DTLSSocket( DTLSv1_client_method() )
{

}

bool DTLSClient::connect()
{
    if (!open())
        return false;

    auto adr = addr();

    volatile int ret = ::connect(sock(), (sockaddr *) &adr , sizeof(sockaddr_in));
    if(ret == -1)
    {
        std::cerr << "DTLSClient: Failed connect" << strerror(errno) << std::endl;
        close();
        return false;
    }

    while ((ret = dtls_connect ()) == 1);

    if (ret == -1)
        return false;

    ret = handle_data();
    if (ret == -1)
    {
        std::cerr << "DTLSClient: Unable to send beat :" << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

} // namespace Helpz
