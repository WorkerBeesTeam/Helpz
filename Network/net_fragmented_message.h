#ifndef HELPZ_NETWORK_FRAGMENTED_MESSAGE_H
#define HELPZ_NETWORK_FRAGMENTED_MESSAGE_H

#include <QIODevice>

namespace Helpz {
namespace Network {

struct Fragmented_Message
{
    Fragmented_Message(uint8_t id, uint16_t cmd, uint32_t max_fragment_size, bool in_memory);

    Fragmented_Message(Fragmented_Message&& o);
    Fragmented_Message& operator =(Fragmented_Message&& o);

    Fragmented_Message(const Fragmented_Message&) = delete;
    Fragmented_Message& operator =(const Fragmented_Message&) = delete;

    ~Fragmented_Message();

    bool operator ==(uint8_t id) const;

    uint8_t id_;
    uint16_t cmd_;
    uint32_t max_fragment_size_;
    QIODevice* data_device_;
};

} // namespace Network
} // namespace Helpz

#endif // HELPZ_NETWORK_FRAGMENTED_MESSAGE_H
