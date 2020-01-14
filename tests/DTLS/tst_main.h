#ifndef DAI_TST_MAIN_H
#define DAI_TST_MAIN_H

#include <Helpz/dtls_server_thread.h>
#include <Helpz/dtls_client_thread.h>

namespace Helpz
{

class Client_Protocol;
class Server_Protocol;
class DTLS_Test : public QObject
{
    Q_OBJECT

public:
    DTLS_Test();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void check_client_connected();
    void check_server_client_connected();
    void check_simple_message();
    void check_answered_message();
    void check_file_message();
    void check_parallel_message();

private:
    std::shared_ptr<Client_Protocol> get_client_protocol();
    std::shared_ptr<Server_Protocol> get_server_protocol();
    QStringList config_files_;

    std::unique_ptr<Helpz::DTLS::Server_Thread> server_thread_;
    std::unique_ptr<Helpz::DTLS::Client_Thread> client_thread_;
};

} // namespace Helpz

#endif // DAI_TST_MAIN_H
