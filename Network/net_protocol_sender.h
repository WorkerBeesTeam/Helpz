#ifndef HELPZ_NETWORK_PROTOCOL_SENDER_H
#define HELPZ_NETWORK_PROTOCOL_SENDER_H

#include <memory>
#include <chrono>
#include <functional>
#include <optional>

/*#if (__cplusplus > 201402L) && !defined(__ANDROID__)
#include <optional>
using std::optional;
#elif __cplusplus > 201103L
#include <experimental/optional>
using std::experimental::optional;
#else
#endif

//typedef optional<bool> BoolTriState;
*/

#include <QDataStream>

namespace Helpz {
namespace Network {

#define MAX_MESSAGE_DATA_SIZE (65507-17)

class Protocol;

struct Message_Item
{
    std::optional<uint8_t> id_;
    uint16_t cmd_;
    std::chrono::time_point<std::chrono::system_clock> begin_time_, end_time_;
    std::shared_ptr<QIODevice> data_device_;
    std::function<void(QIODevice*)> answer_func_;
    std::function<void()> timeout_func_;
};

class Protocol_Sender : public QDataStream
{
public:
    Protocol_Sender(Protocol *p, quint16 command, quint16 flags = 0, std::shared_ptr<QIODevice> device_ptr = nullptr);
    Protocol_Sender(Protocol_Sender&& obj) noexcept;
    ~Protocol_Sender();

    QByteArray pop_packet();

    void set_data_device(std::shared_ptr<QIODevice> data_dev, uint32_t fragment_size = MAX_MESSAGE_DATA_SIZE);
    Protocol_Sender &answer(std::function<void(QIODevice *)> answer_func);
    Protocol_Sender &timeout(std::function<void()> timeout_func, std::chrono::milliseconds timeout_duration);

    template<typename T>
    QDataStream& operator <<(const T& item) { return static_cast<QDataStream&>(*this) << item; }
private:
    Protocol* protocol_;

    uint32_t fragment_size_;
    Message_Item msg_;
};

} // namespace Network
} // namespace Helpz

#endif // HELPZ_NETWORK_PROTOCOL_SENDER_H
