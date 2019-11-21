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

#include <Helpz/net_defs.h>

namespace Helpz {
namespace Network {

class Protocol;

struct Message_Item
{
    Message_Item(uint16_t command, std::optional<uint8_t>&& answer_id, std::shared_ptr<QIODevice>&& device_ptr,
                 std::chrono::milliseconds resend_timeout = std::chrono::milliseconds{3000}, uint32_t fragment_size = MAX_MESSAGE_DATA_SIZE);
    Message_Item(Message_Item&&) = default;
    Message_Item() = default;

    std::optional<uint8_t> id_, answer_id_;
    uint16_t cmd_;
    uint32_t fragment_size_;
    std::chrono::milliseconds resend_timeout_;
    std::chrono::time_point<std::chrono::system_clock> begin_time_, end_time_;
    std::shared_ptr<QIODevice> data_device_;
    std::function<void(QIODevice&)> answer_func_;
    std::function<void()> timeout_func_;
};

class Protocol_Sender : public QDataStream
{
public:
    Protocol_Sender(Protocol *p, uint16_t command, std::optional<uint8_t> answer_id = {}, std::shared_ptr<QIODevice> device_ptr = nullptr,
                    std::chrono::milliseconds resend_timeout = std::chrono::milliseconds{3000});
    Protocol_Sender(const Protocol_Sender& obj) = delete;
    Protocol_Sender(Protocol_Sender&& obj) noexcept;
    ~Protocol_Sender();

    void release();

    QByteArray pop_packet();

    void set_fragment_size(uint32_t fragment_size);

    void set_data_device(std::shared_ptr<QIODevice> data_dev, uint32_t fragment_size = MAX_MESSAGE_DATA_SIZE);
    Protocol_Sender &answer(std::function<void(QIODevice &)> answer_func);
    Protocol_Sender &timeout(std::function<void()> timeout_func, std::chrono::milliseconds timeout_duration,
                             std::chrono::milliseconds resend_timeout = std::chrono::milliseconds{3000});

    template<typename T>
    QDataStream& operator <<(const T& item) { return static_cast<QDataStream&>(*this) << item; }
private:
    Protocol* protocol_;
    Message_Item msg_;

    friend class Protocol;
};

} // namespace Network
} // namespace Helpz

#endif // HELPZ_NETWORK_PROTOCOL_SENDER_H
