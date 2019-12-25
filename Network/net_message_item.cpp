#include "net_message_item.h"

namespace Helpz {
namespace Network {

Message_Item::Message_Item(quint16 command, std::optional<uint8_t> &&answer_id, std::shared_ptr<QIODevice> &&device_ptr,
                           std::chrono::milliseconds resend_timeout, uint32_t fragment_size) :
    answer_id_{std::move(answer_id)}, cmd_(command), fragment_size_(fragment_size), resend_timeout_(resend_timeout), data_device_{std::move(device_ptr)}
{
}

} // namespace Network
} // namespace Helpz
