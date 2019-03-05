#include <iostream>

#include <Helpz/dtls_client_thread.h>
#include <Helpz/net_protocol.h>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>

class Protocol : public Helpz::Network::Protocol
{
public:
    enum Message_Type {
        MSG_UNKNOWN = Helpz::Network::Cmd::UserCommand,
        MSG_SIMPLE,
        MSG_ANSWERED,
        MSG_FILE_META,
        MSG_FILE,
    };

    void test_simple_message()
    {
        send(MSG_SIMPLE) << QString("Hello simple");
    }

    void test_message_with_answer()
    {
        send(MSG_ANSWERED).answer([this](QIODevice* data_dev)
        {
            data_dev->open(QIODevice::ReadOnly);
            QDataStream msg(data_dev);
            QString answer_text;
            Helpz::parse_out(msg, answer_text);

            std::cout << "Answer text: " << answer_text.toStdString() << std::endl;
        }).timeout([]() {
            std::cout << "MSG_ANSWERED timeout" << std::endl;
        }, std::chrono::seconds(5)) << bool(true) << quint32(777);
    }

    void test_send_file()
    {
        QString file_name = "/usr/lib/libcc1.so.0.0.0";
        std::shared_ptr<QFile> device(new QFile(file_name));
        if (!device->open(QIODevice::ReadOnly))
        {
            std::cerr << "Can't open file: " << device->errorString().toStdString() << std::endl;
            return;
        }

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
        std::cout << "CONNECTED" << std::endl;

//        test_simple_message();
        test_message_with_answer();
//        test_send_file();
    }
    void process_message(quint16 cmd, QIODevice* data_dev) override
    {
        std::cout << "process_message " << cmd << std::endl;
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Helpz::DTLS::Client_Thread_Config conf{std::make_shared<Protocol>(), (qApp->applicationDirPath() + "/tls_policy.conf").toStdString(), "localhost", "25590", {"dai/1.1"}, std::chrono::seconds(5)};
    Helpz::DTLS::Client_Thread client_thread{std::move(conf)};

    return a.exec();
}
