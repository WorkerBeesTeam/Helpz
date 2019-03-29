#include <iostream>

#include <QCoreApplication>
#include <QCryptographicHash>

#include <Helpz/net_protocol.h>
#include <Helpz/dtls_server_thread.h>

struct FileMetaInfo {
    uint32_t size_;
    QString name_;
    QByteArray hash_;
};

QDataStream& operator >>(QDataStream& ds, FileMetaInfo& info)
{
    return ds >> info.name_ >> info.size_ >> info.hash_;
}

class Protocol_1_1 : public Helpz::Network::Protocol
{
public:
    bool operator ==(const Helpz::Network::Protocol& o) const override
    {
        (void)o;
        // Need for remove copy clients
        return false;
    }
private:
    enum Message_Type {
        MSG_UNKNOWN = Helpz::Network::Cmd::USER_COMMAND,
        MSG_SIMPLE,
        MSG_ANSWERED,
        MSG_FILE_META,
        MSG_FILE,
    };
    FileMetaInfo file_info_;

    void ready_write() override
    {
        std::cout << title() << " CONNECTED" << std::endl;
    }
    void process_message(uint8_t msg_id, uint16_t cmd, QIODevice& data_dev) override
    {
        // Maybe you want call server->remove_copy(this); after authentication process done.

        switch (cmd) {
        case MSG_SIMPLE:    apply_parse(data_dev, &Protocol_1_1::process_simple);                   break;
        case MSG_ANSWERED:  apply_parse(data_dev, &Protocol_1_1::process_answered, cmd, msg_id);    break;
        case MSG_FILE_META: apply_parse(data_dev, &Protocol_1_1::process_file_meta);                break;
        case MSG_FILE:      process_file(data_dev);                                                 break;

        default:
            std::cout << title() << " process_message " << cmd << std::endl;
            break;
        }
    }
    void process_answer_message(uint8_t msg_id, uint16_t cmd, QIODevice& data_dev) override
    {
        std::cout << title() << " process_answer_message #" << int(msg_id) << ' ' << cmd << " size " << data_dev.size() << std::endl;
    }

    void process_simple(QString value1)
    {
        std::cout << title() << " MSG_SIMPLE " << value1.toStdString() << std::endl;
    }
    void process_answered(bool value1, quint32 value2, uint16_t cmd, uint8_t msg_id)
    {
        std::cout << title() << " MSG_ANSWERED " << value1 << " v " << value2 << std::endl;
        send_answer(cmd, msg_id) << QString("OK");
    }
    void process_file_meta(FileMetaInfo info)
    {
        std::cout << title() << " MSG_FILE_META " << info.name_.toStdString() << " size " << info.size_
                  << " hash " << info.hash_.toHex().constData() << std::endl;
        file_info_ = std::move(info);
    }
    void process_file(QIODevice& data_dev)
    {
        if (!data_dev.isOpen())
        {
            data_dev.open(QIODevice::ReadOnly);
        }
        data_dev.seek(0);
        QCryptographicHash hash(QCryptographicHash::Sha1);
        if (!hash.addData(&data_dev))
        {
            std::cerr << title() << " Can't get file hash" << std::endl;
            return;
        }
        std::cout << title() << " MSG_FILE is valid hash: " << (file_info_.hash_ == hash.result() ? "true" : "false")
                  << " size: " << data_dev.size() << "(" << file_info_.size_ << ')' << std::endl;
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Helpz::DTLS::Create_Server_Protocol_Func_T create_protocol = [](const std::vector<std::string> &client_protos, std::string* choose_out) -> std::shared_ptr<Helpz::Network::Protocol>
    {
        std::cout << "create_protocol" << std::endl;
        for (const std::string& proto: client_protos)
        {
            if (proto == "dai/1.1")
            {
                *choose_out = proto;
                return std::shared_ptr<Helpz::Network::Protocol>(new Protocol_1_1{});
            }
            else if (proto == "dai/1.0")
            {
                *choose_out = proto;
                return std::shared_ptr<Helpz::Network::Protocol>(new Protocol_1_1{});
            }
        }
        std::cerr << "Unsuported protocol" << std::endl;
        return std::shared_ptr<Helpz::Network::Protocol>{};
    };

    std::string app_dir = qApp->applicationDirPath().toStdString();
    Helpz::DTLS::Server_Thread_Config conf{25590, app_dir + "/tls_policy.conf", app_dir + "/dtls.pem", app_dir + "/dtls.key", 30, 5, 5};
    conf.set_create_protocol_func(std::move(create_protocol));

    Helpz::DTLS::Server_Thread server_thread{std::move(conf)};
    return a.exec();
}
