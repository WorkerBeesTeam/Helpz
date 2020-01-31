#include <iostream>

#include <Helpz/dtls_client.h>
#include <Helpz/dtls_client_thread.h>
#include <Helpz/dtls_client_controller.h>
#include <Helpz/net_protocol.h>

#include <QCoreApplication>
#include <QTimer>
#include <QCryptographicHash>
#include <QFile>

class Protocol : public Helpz::Network::Protocol
{
public:
    enum Message_Type {
        MSG_UNKNOWN = Helpz::Network::Cmd::USER_COMMAND,
        MSG_SIMPLE,
        MSG_ANSWERED,
        MSG_FILE_META,
        MSG_FILE,
    };

    void test_simple_message()
    {
        send(MSG_SIMPLE).timeout([]() {
            std::cout << "MSG_SIMPLE timeout test" << std::endl;
        }, std::chrono::seconds(1))  << QString("Hello simple");
    }

    void test_message_with_answer(quint32 value2 = 777)
    {
        send(MSG_ANSWERED).answer([this](QIODevice& data_dev)
        {
            QString answer_text;
            parse_out(data_dev, answer_text);

            std::cout << "Answer text: " << answer_text.toStdString() << std::endl;
        }).timeout([]() {
            std::cout << "MSG_ANSWERED timeout" << std::endl;
        }, std::chrono::seconds(15), std::chrono::milliseconds{3000}) << bool(true) << value2;
    }

    void test_send_file()
    {
        QString file_name = qApp->applicationDirPath() + "/test_file.dat"; //"/usr/lib/libcc1.so.0.0.0";
        QFile::remove(file_name);
        std::shared_ptr<QFile> device(new QFile(file_name));
        if (!device->open(QIODevice::ReadWrite))
        {
            std::cerr << "Can't open file: " << device->errorString().toStdString() << std::endl;
            return;
        }

        while (device->size() < 100000000)
        {
            std::unique_ptr<char[]> data(new char[4096]);
            device->write(data.get(), 4096);
        }
        device->seek(0);

        QCryptographicHash hash(QCryptographicHash::Sha1);
        if (!hash.addData(device.get()))
        {
            std::cerr << "Can't get file hash" << std::endl;
            return;
        }
        device->seek(0);

        send(MSG_FILE_META) << file_name << static_cast<uint32_t>(device->size()) << hash.result();
        send(MSG_FILE).set_data_device(std::move(device));
    }
private:

    void ready_write() override
    {
        std::cout << "Connected" << std::endl;

        test_send_file();
        test_simple_message();
        test_message_with_answer();
    }
    void process_message(uint8_t msg_id, uint16_t cmd, QIODevice& data_dev) override
    {
        std::cout << "process_message #" << int(msg_id) << ' ' << cmd << " size " << data_dev.size() << std::endl;
    }
    void process_answer_message(uint8_t msg_id, uint16_t cmd, QIODevice& data_dev) override
    {
        std::cout << "process_answer_message #" << int(msg_id) << ' ' << cmd << " size " << data_dev.size() << std::endl;
    }
};

void periodic_send_thread_func(Helpz::DTLS::Client_Thread* client_thread)
{
    std::shared_ptr<Helpz::Network::Protocol> protocol_ptr;
    for(int i = 0; i < 1000; ++i)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        protocol_ptr = client_thread->client()->protocol();
        if (protocol_ptr)
        {
            std::static_pointer_cast<Protocol>(protocol_ptr)->test_simple_message();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::static_pointer_cast<Protocol>(protocol_ptr)->test_message_with_answer(i);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::static_pointer_cast<Protocol>(protocol_ptr)->test_send_file();
        }
        else
        {
            i = 0;
        }
    }
    qApp->quit();
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Helpz::DTLS::Create_Client_Protocol_Func_T func = [](const std::string& app_protocol) -> std::shared_ptr<Helpz::Network::Protocol>
    {
        return std::shared_ptr<Helpz::Network::Protocol>(new Protocol{});
    };

    std::string port = "25590";
    if (argc >= 2)
    {
        port = argv[1];
    }
    Helpz::DTLS::Client_Thread_Config conf{(qApp->applicationDirPath() + "/tls_policy.conf").toStdString(), "localhost", port, {"helpz_test/1.1"}, 5};
    conf.set_create_protocol_func(std::move(func));
    Helpz::DTLS::Client_Thread client_thread{std::move(conf)};

    // std::thread(periodic_send_thread_func, &client_thread).detach();
//    std::thread([]() { std::this_thread::sleep_for(std::chrono::seconds(2)); qApp->quit(); }).detach();
    return a.exec();
}
