#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>

#include <Helpz/dtls_server.h>
#include <Helpz/dtls_client.h>

#include "server_protocol.h"
#include "client_protocol.h"
#include "tst_main.h"

namespace Helpz
{

DTLS_Test::DTLS_Test() :
    config_files_({"/tls_policy.conf", "/dtls.pem", "/dtls.key"})
{
}

void DTLS_Test::initTestCase()
{
    // Init config files
    for (QString& file_name: config_files_)
    {
        const QString file_path = qApp->applicationDirPath() + file_name;
        if (QFile::exists(file_path))
            QFile::remove(file_path);
        QVERIFY2(QFile::copy(':' + file_name, file_path), qPrintable("Failed copy file to: " + file_path));
        file_name = file_path;
    }

    // Init server
    Helpz::DTLS::Server_Thread_Config server_conf;
    server_conf.set_port(0);
    server_conf.set_tls_police_file_name(config_files_.at(0).toStdString());
    server_conf.set_certificate_file_name(config_files_.at(1).toStdString());
    server_conf.set_certificate_key_file_name(config_files_.at(2).toStdString());
    server_conf.set_cleaning_timeout(std::chrono::seconds{30});
    server_conf.set_receive_thread_count(3);
    server_conf.set_record_thread_count(3);
    server_conf.set_create_protocol_func(Helpz::DTLS::Create_Server_Protocol_Func_T(Server_Protocol::create));

    server_thread_.reset(new Helpz::DTLS::Server_Thread{std::move(server_conf)});

    // Init client
    Helpz::DTLS::Client_Thread_Config client_conf;
    client_conf.set_tls_police_file_name(config_files_.at(0).toStdString());
    client_conf.set_host("localhost");
    client_conf.set_port(std::to_string(server_thread_->server()->get_local_port()));
    client_conf.set_next_protocols({Server_Protocol::name()});
    client_conf.set_reconnect_interval(std::chrono::seconds(5));
    client_conf.set_create_protocol_func(Client_Protocol::create);

    client_thread_.reset(new Helpz::DTLS::Client_Thread{std::move(client_conf)});

    QVERIFY(server_thread_ && client_thread_);
}

void DTLS_Test::cleanupTestCase()
{
    client_thread_.reset();
    server_thread_.reset();

    for (const QString& file_path: config_files_)
        QFile::remove(file_path);
    qDebug() << "clean";
}

void DTLS_Test::check_client_connected()
{
    int attempt = 0;
    while((!client_thread_->client() || !client_thread_->client()->protocol()) && ++attempt <= 3)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{1500});
    }

    QVERIFY(attempt <= 3);

    auto client = client_thread_->client();
    QVERIFY(client);
    auto client_protocol_raw = client->protocol();
    QVERIFY(client_protocol_raw);
    std::shared_ptr<Client_Protocol> client_protocol = std::dynamic_pointer_cast<Client_Protocol>(client_protocol_raw);
    QVERIFY(client_protocol);
}

void DTLS_Test::check_server_client_connected()
{
    auto node = server_thread_->server()->find_client([](const Helpz::Network::Protocol* ) { return true; });
    QVERIFY(node);
    std::shared_ptr<Server_Protocol> server_protocol = std::dynamic_pointer_cast<Server_Protocol>(node->protocol());
    QVERIFY(server_protocol);
}

void DTLS_Test::check_simple_message()
{
    std::shared_ptr<Client_Protocol> client_protocol = get_client_protocol();
    std::shared_ptr<Server_Protocol> server_protocol = get_server_protocol();
    QVERIFY(client_protocol && server_protocol);

    // Test simple message
    QString test_simple_text = "Test";
    std::future<QString> simple_future = server_protocol->get_simple_future();
    std::future<void> timeout_future = client_protocol->test_simple_message(test_simple_text);

    QCOMPARE(simple_future.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    QCOMPARE(simple_future.get(), test_simple_text);
    QCOMPARE(timeout_future.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    timeout_future.get();
}

void DTLS_Test::check_answered_message()
{
    std::shared_ptr<Client_Protocol> client_protocol = get_client_protocol();
    std::shared_ptr<Server_Protocol> server_protocol = get_server_protocol();
    QVERIFY(client_protocol && server_protocol);

    // Test simple message with answer
    uint32_t test_value = 745;
    QString test_text = "ANSWER: " + QString::number(test_value);
    std::future<uint32_t> answer_future = server_protocol->get_answer_future(test_text);
    std::future<QString> client_answer_future = client_protocol->test_message_with_answer(test_value);

    QCOMPARE(answer_future.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    QCOMPARE(answer_future.get(), test_value);
    QCOMPARE(client_answer_future.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    QCOMPARE(client_answer_future.get(), test_text);
}

void DTLS_Test::check_file_message()
{
    std::shared_ptr<Client_Protocol> client_protocol = get_client_protocol();
    std::shared_ptr<Server_Protocol> server_protocol = get_server_protocol();
    QVERIFY(client_protocol && server_protocol);

    // Test send file 100mb size
    QByteArray hash = client_protocol->test_send_file();

    std::future<QByteArray> hash_future = server_protocol->get_file_hash_future();

    QCOMPARE(hash_future.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    QCOMPARE(hash_future.get(), hash);

    std::future<QByteArray> file_future = server_protocol->get_file_future();

    QCOMPARE(file_future.wait_for(std::chrono::seconds(30)), std::future_status::ready);
    QCOMPARE(file_future.get(), hash);
}

void DTLS_Test::check_parallel_message()
{
    std::shared_ptr<Client_Protocol> client_protocol = get_client_protocol();
    std::shared_ptr<Server_Protocol> server_protocol = get_server_protocol();
    QVERIFY(client_protocol && server_protocol);

    server_protocol->reset_promises();

    // Test parallel message send
    QString test_simple_text = "Test again";
    uint32_t test_value = 6823527;
    QString test_text = "ANSWER: " + QString::number(test_value);
    std::future<QString> simple_future = server_protocol->get_simple_future();
    std::future<uint32_t> answer_future = server_protocol->get_answer_future(test_text);
    std::future<QByteArray> hash_future = server_protocol->get_file_hash_future();
    std::future<QByteArray> file_future = server_protocol->get_file_future();

    // Send simple messages after send file can lead to parallel sending
    QByteArray hash = client_protocol->test_send_file();
    std::future<void> timeout_future = client_protocol->test_simple_message(test_simple_text);
    std::future<QString> client_answer_future = client_protocol->test_message_with_answer(test_value);


    QCOMPARE(simple_future.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    QCOMPARE(simple_future.get(), test_simple_text);
    QCOMPARE(timeout_future.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    timeout_future.get();
    QCOMPARE(answer_future.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    QCOMPARE(answer_future.get(), test_value);
    QCOMPARE(client_answer_future.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    QCOMPARE(client_answer_future.get(), test_text);
    QCOMPARE(hash_future.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    QCOMPARE(hash_future.get(), hash);
    QCOMPARE(file_future.wait_for(std::chrono::seconds(30)), std::future_status::ready);
    QCOMPARE(file_future.get(), hash);
}

std::shared_ptr<Client_Protocol> DTLS_Test::get_client_protocol()
{
    auto client = client_thread_->client();
    if (client)
    {
        auto client_protocol_raw = client->protocol();
        if (client_protocol_raw)
            return std::dynamic_pointer_cast<Client_Protocol>(client_protocol_raw);
    }

    return {};
}

std::shared_ptr<Server_Protocol> DTLS_Test::get_server_protocol()
{
    auto node = server_thread_->server()->find_client([](const Helpz::Network::Protocol* ) { return true; });
    if (node)
        return std::dynamic_pointer_cast<Server_Protocol>(node->protocol());
    return {};
}

} // namespace Helpz

QTEST_MAIN(Helpz::DTLS_Test)
