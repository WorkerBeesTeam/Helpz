#ifndef HELPZ_NETWORK_PROTOCOL_SENDER_H
#define HELPZ_NETWORK_PROTOCOL_SENDER_H

#include <memory>
#include <chrono>
#include <functional>

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

#include <Helpz/net_message_item.h>

namespace Helpz {
namespace Network {

class Protocol;

class Protocol_Sender : public QDataStream
{
public:
    Protocol_Sender(std::shared_ptr<Protocol> p, uint8_t command, std::optional<uint8_t> answer_id = {}, std::unique_ptr<QIODevice> device_ptr = nullptr,
                    std::chrono::milliseconds resend_timeout = std::chrono::milliseconds{3000});
    Protocol_Sender(const Protocol_Sender& obj) = delete;
    Protocol_Sender(Protocol_Sender&& obj) noexcept;
    ~Protocol_Sender();

    void release();

    void set_fragment_size(uint32_t fragment_size);
    void set_min_compress_size(uint32_t min_compress_size);

    void set_data_device(std::unique_ptr<QIODevice> data_dev, uint32_t fragment_size = HELPZ_MAX_MESSAGE_DATA_SIZE);
    Protocol_Sender &answer(std::function<void(QIODevice &)> answer_func);
    Protocol_Sender &timeout(std::function<void()> timeout_func, std::chrono::milliseconds timeout_duration,
                             std::chrono::milliseconds resend_timeout = std::chrono::milliseconds{3000});
    Protocol_Sender &finally(std::function<void(bool)> func);

    template<typename T>
    QDataStream& operator <<(const T& item) { return static_cast<QDataStream&>(*this) << item; }
private:
    std::shared_ptr<Protocol> protocol_;
    Message_Item msg_;

    friend class Protocol;
};

} // namespace Network
} // namespace Helpz

#endif // HELPZ_NETWORK_PROTOCOL_SENDER_H
