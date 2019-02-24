#ifndef HELPZ_NETWORK_PROTOCOL_SENDER_H
#define HELPZ_NETWORK_PROTOCOL_SENDER_H

#include <memory>
#include <chrono>
#include <functional>

#include <QDataStream>

namespace Helpz {
namespace Network {

class Protocol;

struct Message_Item
{
    uint16_t cmd_;
    std::chrono::time_point<std::chrono::system_clock> begin_time_, end_time_;
    QByteArray data_;
    std::shared_ptr<QIODevice> data_device_;
    std::function<void(QByteArray&&, QIODevice*)> answer_func_;
    std::function<void()> timeout_func_;
};

class Protocol_Sender : public QDataStream
{
public:
    Protocol_Sender(Protocol *p, quint16 command, quint16 flags = 0);
    Protocol_Sender(Protocol_Sender&& obj) noexcept;
    ~Protocol_Sender();

    QByteArray pop_packet();

    Protocol_Sender &data_device(std::shared_ptr<QIODevice> data_dev);
    Protocol_Sender &answer(std::function<void(QByteArray &&, QIODevice *)> answer_func);
    Protocol_Sender &timeout(std::function<void()> timeout_func, std::chrono::milliseconds timeout_duration);

    template<typename T>
    QDataStream& operator <<(const T& item) { return static_cast<QDataStream&>(*this) << item; }
private:
    Protocol* protocol_;

    Message_Item msg_;
};

} // namespace Network
} // namespace Helpz

#endif // HELPZ_NETWORK_PROTOCOL_SENDER_H
