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
    Message_Item(uint8_t command, std::optional<uint8_t> answer_id, std::unique_ptr<QIODevice>&& device_ptr,
                 std::chrono::milliseconds resend_timeout = std::chrono::milliseconds{3000},
                 uint32_t fragment_size = HELPZ_MAX_MESSAGE_DATA_SIZE);
    Message_Item(const Message_Item&) = delete;
    Message_Item(Message_Item&&) = default;
    Message_Item() = default;

    std::optional<uint8_t> id_, answer_id_;
    uint8_t cmd_;
    uint32_t fragment_size_;
    std::chrono::milliseconds resend_timeout_;
    std::chrono::time_point<std::chrono::system_clock> begin_time_, end_time_;
    std::unique_ptr<QIODevice> data_device_;
    std::function<void(QIODevice&)> answer_func_;
    std::function<void()> timeout_func_;
private:
    uint8_t flags_;

    friend class Protocol;
};

} // namespace Network
} // namespace Helpz

#endif // HELPZ_NETWORK_MESSAGE_ITEM_H
