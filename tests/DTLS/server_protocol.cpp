#include <QCryptographicHash>
#include <QFile>

#include "server_protocol.h"

namespace Helpz {

QDataStream &operator >>(QDataStream &ds, FileMetaInfo &info)
{
    return ds >> info.name_ >> info.size_ >> info.hash_;
}

/*static*/ std::string Server_Protocol::name()
{
    return "das_test/1.0";
}

std::future<QString> Server_Protocol::get_simple_future()
{
    return promise_simple_.get_future();
}

std::future<uint32_t> Server_Protocol::get_answer_future(const QString &answer_text)
{
    answer_text_ = answer_text;
    return promise_answer_.get_future();
}

std::future<QByteArray> Server_Protocol::get_file_hash_future()
{
    return promise_file_hash_.get_future();
}

std::future<QByteArray> Server_Protocol::get_file_future()
{
    return promise_file_.get_future();
}

void Server_Protocol::reset_promises()
{
    promise_simple_ = std::promise<QString>();
    promise_answer_ = std::promise<uint32_t>();
    promise_file_hash_ = std::promise<QByteArray>();
    promise_file_ = std::promise<QByteArray>();
}

bool Server_Protocol::operator ==(const Helpz::Network::Protocol &o) const
{
    (void)o;
    // Need for remove copy clients
    return false;
}

std::shared_ptr<Helpz::Network::Protocol> Server_Protocol::create(const std::vector<std::string> &client_protos, std::string *choose_out)
{
    for (const std::string& proto: client_protos)
    {
        if (proto == name())
        {
            *choose_out = proto;
            return std::shared_ptr<Helpz::Network::Protocol>(new Server_Protocol{});
        }
    }
    qCritical() << "Unsuported protocol";
    return std::shared_ptr<Helpz::Network::Protocol>{};
}

void Server_Protocol::ready_write()
{
    qDebug().noquote() << title() << "CONNECTED";
}

void Server_Protocol::process_message(uint8_t msg_id, uint8_t cmd, QIODevice &data_dev)
{
    // Maybe you want call server->remove_copy(this); after authentication process done.

    switch (cmd) {
    case MSG_SIMPLE:    apply_parse(data_dev, &Server_Protocol::process_simple);                   break;
    case MSG_ANSWERED:  apply_parse(data_dev, &Server_Protocol::process_answered, cmd, msg_id);    break;
    case MSG_FILE_META: apply_parse(data_dev, &Server_Protocol::process_file_meta);                break;
    case MSG_FILE:      process_file(data_dev);                                                 break;

    default:
        qDebug().noquote() << title() << "process_message " << int(cmd);
        break;
    }
}

void Server_Protocol::process_answer_message(uint8_t msg_id, uint8_t cmd, QIODevice &data_dev)
{
    qDebug().noquote() << title() << "process_answer_message #" << int(msg_id) << int(cmd) << "size" << data_dev.size();
}

void Server_Protocol::process_simple(QString value1)
{
    promise_simple_.set_value(value1);
    qDebug().noquote() << title() << "MSG_SIMPLE" << value1;
}

void Server_Protocol::process_answered(bool value1, uint32_t value2, uint8_t cmd, uint8_t msg_id)
{
    promise_answer_.set_value(value2);
    qDebug().noquote() << title() << "MSG_ANSWERED" << value1 << "v" << value2;
    if (value1)
    {
        send_answer(cmd, msg_id) << QString("ANSWER: %1").arg(value2);
    }
}

void Server_Protocol::process_file_meta(FileMetaInfo info)
{
    promise_file_hash_.set_value(info.hash_);
    qDebug().noquote() << title() << "MSG_FILE_META" << info.name_ << "size" << info.size_
                       << "hash" << info.hash_.toHex();
    file_info_ = std::move(info);
}

void Server_Protocol::process_file(QIODevice &data_dev)
{
    if (!data_dev.isOpen())
        data_dev.open(QIODevice::ReadOnly);

    data_dev.seek(0);
    QCryptographicHash hash(QCryptographicHash::Sha1);
    if (!hash.addData(&data_dev))
    {
        promise_file_.set_exception(std::make_exception_ptr(std::logic_error("Can't get file hash")));
        return;
    }
    QByteArray hash_result = hash.result();
    promise_file_.set_value(hash_result);
    qDebug().noquote() << title() << "MSG_FILE is valid hash:" << (file_info_.hash_ == hash_result ? "true" : "false")
                       << "size:" << data_dev.size() << "(" << file_info_.size_ << ')';
    QFile::remove(file_info_.name_);
}

} // namespace Helpz
