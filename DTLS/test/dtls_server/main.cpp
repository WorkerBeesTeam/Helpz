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
        MSG_FILE_META,
        MSG_FILE,
    };

    struct {
        uint32_t size_;
        QString name_;
        QByteArray hash_;
    } file_;

    void ready_write() override
    {
        std::cout << title() << " CONNECTED" << std::endl;
    }
    void process_message(quint16 cmd, QIODevice* data_dev) override
    {
        // TODO: after auth server->remove_copy(this);

        switch (cmd) {
        case MSG_SIMPLE:
        {
            data_dev->open(QIODevice::ReadOnly);
            QDataStream msg(data_dev);
            std::cout << "MSG_SIMPLE " << Helpz::parse<QString>(msg).toStdString() << std::endl;
            break;
        }

        case MSG_ANSWERED:
        {
            data_dev->open(QIODevice::ReadOnly);
            QDataStream msg(data_dev);
            bool value1;
            quint32 value2;
            Helpz::parse_out(msg, value1, value2);
            std::cout << "MSG_ANSWERED " << value1 << " v " << value2 << std::endl;

            send(cmd, ANSWER) << QString("OK");
            break;
        }

        case MSG_FILE_META:
        {
            data_dev->open(QIODevice::ReadOnly);
            QDataStream msg(data_dev);
            Helpz::parse_out(msg, file_.name_, file_.size_, file_.hash_);

            std::cout << "MSG_FILE_META " << file_.name_.toStdString() << " size " << file_.size_ << " hash " << file_.hash_.toHex().toUpper().constData() << std::endl;
            break;
        }
        case MSG_FILE:
        {
            if (!data_dev->isOpen())
            {
                data_dev->open(QIODevice::ReadOnly);
            }
            data_dev->seek(0);
            QCryptographicHash hash(QCryptographicHash::Sha1);
            if (!hash.addData(data_dev))
            {
                std::cerr << "Can't get file hash" << std::endl;
                return;
            }
            std::cout << "MSG_FILE is valid hash: " << (file_.hash_ == hash.result() ? "true" : "false") << " size: " << data_dev->size() << "(" << file_.size_ << ')' << std::endl;
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
        for (const std::string& proto: client_protos)
        {
            if (proto == "dai/1.1")
            {
                *choose_out = proto;
                return new Protocol_1_1{};
            }
            else if (proto == "dai/1.0")
            {
                *choose_out = proto;
                return new Protocol_1_1{};
            }
        }
        std::cerr << "Unsuported protocol" << std::endl;
        return nullptr;
    };

    std::string app_dir = qApp->applicationDirPath().toStdString();
    Helpz::DTLS::Server_Thread_Config conf{25590, app_dir + "/tls_policy.conf", app_dir + "/dtls.pem", app_dir + "/dtls.key", 30, 5};
    conf.set_create_protocol_func(std::move(create_protocol));

    Helpz::DTLS::Server_Thread server_thread{std::move(conf)};
    return a.exec();
}
