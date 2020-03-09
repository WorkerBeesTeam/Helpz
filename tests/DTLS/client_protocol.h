#ifndef HELPZ_CLIENT_PROTOCOL_H
#define HELPZ_CLIENT_PROTOCOL_H

#include <future>

#include <Helpz/net_protocol.h>

namespace Helpz {

class Client_Protocol : public Helpz::Net::Protocol
{
public:
    enum Message_Type {
        MSG_UNKNOWN = Helpz::Net::Cmd::USER_COMMAND,
        MSG_SIMPLE,
        MSG_ANSWERED,
        MSG_FILE_META,
        MSG_FILE,
    };

    static std::shared_ptr<Helpz::Net::Protocol> create(const std::string& app_protocol);

    std::future<void> test_simple_message(const QString& text);
    std::future<QString> test_message_with_answer(uint32_t value2);
    QByteArray test_send_file();
private:

    void ready_write() override;
    void process_message(uint8_t msg_id, uint8_t cmd, QIODevice& data_dev) override;
    void process_answer_message(uint8_t msg_id, uint8_t cmd, QIODevice& data_dev) override;
};

} // namespace Helpz

#endif // HELPZ_CLIENT_PROTOCOL_H
