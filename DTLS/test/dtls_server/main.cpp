#include <iostream>

#include <QCoreApplication>
#include <QCryptographicHash>

#include <Helpz/net_protocol.h>
#include <Helpz/dtls_server_thread.h>

class Protocol_1_1 : public Helpz::Network::Protocol
{
public:
    bool operator ==(const Helpz::Network::Protocol& o) const override
    {
        // Need for remove same clients
        return false;
    }
private:
    enum Message_Type {
        MSG_UNKNOWN = Helpz::Network::Cmd::UserCommand,
        MSG_SIMPLE,
        MSG_ANSWERED,
        MSG_FILE,
    };

    void ready_write() override
    {
        std::cout << "CONNECTED" << std::endl;
    }
    void process_message(quint16 cmd, QByteArray&& data, QIODevice* data_dev) override
    {
        // TODO: after auth server->remove_copy(this);

        switch (cmd) {
        case MSG_SIMPLE:
            std::cout << "MSG_SIMPLE" << std::endl;
            break;

        case MSG_ANSWERED:
        {
            bool value1;
            quint32 value2;
            parse_out(data, value1, value2);

            send(cmd, ANSWER) << "OK";

            std::cout << "MSG_ANSWERED" << std::endl;
        }

        case MSG_FILE:
        {
            QString file_name;
            QByteArray file_hash;
            parse_out(data, file_name, file_hash);

            QCryptographicHash hash(QCryptographicHash::Sha1);
            if (!hash.addData(data_dev))
            {
                std::cerr << "Can't get file hash" << std::endl;
                return;
            }

            std::cout << "MSG_ANSWERED" << file_name.toStdString() << (file_hash == hash.result()) << data_dev->size() << std::endl;
            break;
        }

        default:
            std::cout << "process_message " << cmd << std::endl;
            break;
        }
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Helpz::DTLS::Create_Protocol_Func_T create_protocol = [](const std::vector<std::string> &client_protos, std::string* choose_out) -> Helpz::Network::Protocol*
    {
        std::cout << "create_protocol" << std::endl;
        int version;
        for (const std::string& proto: client_protos)
        {
            if (proto == "dai/1.1")
            {
                version = 11;
                *choose_out = proto;
                return new Protocol_1_1{};
            }
            else if (proto == "dai/1.0")
            {
                version = 10;
                *choose_out = proto;
                return new Protocol_1_1{};
            }
        }
        std::cerr << "Unsuported protocol" << std::endl;
        return nullptr;
    };

    std::string app_dir = qApp->applicationDirPath().toStdString();
    Helpz::DTLS::Server_Thread server_thread{{std::move(create_protocol), 25590, app_dir + "/tls_policy.conf", app_dir + "/dtls.pem", app_dir + "/dtls.key", std::chrono::seconds{30}, 1}};
    return a.exec();
}
