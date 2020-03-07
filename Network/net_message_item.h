#ifndef HELPZ_NETWORK_MESSAGE_ITEM_H
#define HELPZ_NETWORK_MESSAGE_ITEM_H

#include <memory>
#include <chrono>
#include <functional>
#include <optional>

#include <QIODevice>

#include <Helpz/net_defs.h>

namespace Helpz {
namespace Network {

struct Message_Item
{
    Message_Item();
    Message_Item(uint8_t command, std::optional<uint8_t> answer_id, std::unique_ptr<QIODevice>&& device_ptr,
                 std::chrono::milliseconds resend_timeout = std::chrono::milliseconds{3000});
    virtual ~Message_Item();
    Message_Item(Message_Item&&) = default;
    Message_Item& operator =(Message_Item&&) = default;
    Message_Item(const Message_Item&) = delete;
    Message_Item& operator =(const Message_Item&) = delete;

    std::optional<uint8_t> id_, answer_id_;
    std::chrono::milliseconds resend_timeout_;
    std::chrono::time_point<std::chrono::system_clock> begin_time_, end_time_;
    std::unique_ptr<QIODevice> data_device_;
    std::function<void(QIODevice&)> answer_func_;
    std::function<void()> timeout_func_;
    std::function<void(bool)> finally_func_;

    uint8_t cmd() const;

    class Only_Protocol { explicit Only_Protocol() = default; friend class Protocol; };
    uint8_t flags() const;
    void set_flags(uint8_t flags, const Only_Protocol op);

    uint32_t fragment_size() const;
    void set_fragment_size(uint32_t size);

private:
    uint8_t cmd_, flags_;
    uint32_t fragment_size_;
};

} // namespace Network
} // namespace Helpz

#endif // HELPZ_NETWORK_MESSAGE_ITEM_H
