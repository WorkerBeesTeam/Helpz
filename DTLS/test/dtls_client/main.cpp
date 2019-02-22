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
        MSG_FILE,
    };

    void test_simple_message()
    {
        send(MSG_SIMPLE) << "Hello simple";
    }

    void test_message_with_answer()
    {
        send(MSG_ANSWERED).answer([this](QByteArray&& data, QIODevice* data_dev)
        {
            QString answer_text;
            parse_out(data, answer_text);

            std::cout << "Answer text: " << answer_text.toStdString() << std::endl;
        }).timeout([]() {
            std::cout << "MSG_ANSWERED timeout" << std::endl;
        }) << true << quint32(777);
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

        send(MSG_FILE).data_device(std::move(device)) << file_name << hash.result();
    }
private:

    void ready_write() override
    {
        std::cout << "CONNECTED" << std::endl;

        test_message_with_answer();
    }
    void process_message(quint16 cmd, QByteArray&& data, QIODevice* data_dev) override
    {
        std::cout << "process_message " << cmd << std::endl;
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    {
        Protocol p;
        p.test_send_file();
    }
    return 1;

    Helpz::DTLS::Client_Thread client_thread{{std::make_shared<Protocol>(), (qApp->applicationDirPath() + "/tls_policy.conf").toStdString(), "localhost", "25590", {"dai/1.1"}, std::chrono::seconds(5)}};

    return a.exec();
}
