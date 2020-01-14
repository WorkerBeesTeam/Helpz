#ifndef HELPZ_SERVER_PROTOCOL_H
#define HELPZ_SERVER_PROTOCOL_H

#include <future>

#include <Helpz/net_protocol.h>

namespace Helpz {

struct FileMetaInfo
{
    uint32_t size_;
    QString name_;
    QByteArray hash_;
};

QDataStream& operator >>(QDataStream& ds, FileMetaInfo& info);

class Server_Protocol : public Helpz::Network::Protocol
{
public:
    static std::string name();

    std::future<QString> get_simple_future();
    std::future<uint32_t> get_answer_future(const QString& answer_text);
    std::future<QByteArray> get_file_hash_future();
    std::future<QByteArray> get_file_future();

    void reset_promises();

    bool operator ==(const Helpz::Network::Protocol& o) const override;

    static std::shared_ptr<Helpz::Network::Protocol> create(const std::vector<std::string> &client_protos, std::string* choose_out);
private:
    enum Message_Type {
        MSG_UNKNOWN = Helpz::Network::Cmd::USER_COMMAND,
        MSG_SIMPLE,
        MSG_ANSWERED,
        MSG_FILE_META,
        MSG_FILE,
    };
    FileMetaInfo file_info_;

    void ready_write() override;
    void process_message(uint8_t msg_id, uint16_t cmd, QIODevice& data_dev) override;
    void process_answer_message(uint8_t msg_id, uint16_t cmd, QIODevice& data_dev) override;

    void process_simple(QString value1);
    void process_answered(bool value1, uint32_t value2, uint16_t cmd, uint8_t msg_id);
    void process_file_meta(FileMetaInfo info);
    void process_file(QIODevice& data_dev);

    QString answer_text_;
    std::promise<QString> promise_simple_;
    std::promise<uint32_t> promise_answer_;
    std::promise<QByteArray> promise_file_, promise_file_hash_;
};

} // namespace Helpz

#endif // HELPZ_SERVER_PROTOCOL_H
