#include "net_message_item.h"

namespace Helpz {
namespace Network {

Message_Item::Message_Item() :
    cmd_(0), flags_(0), fragment_size_(HELPZ_MAX_MESSAGE_DATA_SIZE)
{
}

Message_Item::Message_Item(uint8_t command, std::optional<uint8_t> answer_id, std::unique_ptr<QIODevice> &&device_ptr,
                           std::chrono::milliseconds resend_timeout) :
    answer_id_{std::move(answer_id)}, resend_timeout_(resend_timeout),
    data_device_{std::move(device_ptr)}, cmd_(command), flags_(0), fragment_size_(HELPZ_MAX_MESSAGE_DATA_SIZE)
{
}

Message_Item::~Message_Item()
{
    if (finally_func_)
    {
        bool is_state_ok = !answer_func_;
        finally_func_(is_state_ok);
    }
}

uint8_t Message_Item::cmd() const { return cmd_; }

uint8_t Message_Item::flags() const { return flags_; }
void Message_Item::set_flags(uint8_t flags, const Only_Protocol op) { Q_UNUSED(op); flags_ = flags; }

uint32_t Message_Item::fragment_size() const { return fragment_size_; }
void Message_Item::set_fragment_size(uint32_t size)
{
    fragment_size_ = size;

    if (fragment_size_ < 32)
        fragment_size_ = 32;
    else
    if (fragment_size_ > HELPZ_MAX_PACKET_DATA_SIZE)
        fragment_size_ = HELPZ_MAX_PACKET_DATA_SIZE;
}

} // namespace Network
} // namespace Helpz
