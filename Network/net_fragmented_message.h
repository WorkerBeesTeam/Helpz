#ifndef HELPZ_NETWORK_FRAGMENTED_MESSAGE_H
#define HELPZ_NETWORK_FRAGMENTED_MESSAGE_H

#include <chrono>

#include <QIODevice>

namespace Helpz {
namespace Network {

struct Fragmented_Message
{
    Fragmented_Message(uint8_t id, uint16_t cmd, uint32_t max_fragment_size, uint32_t full_size);

    Fragmented_Message(Fragmented_Message&& o);
    Fragmented_Message& operator =(Fragmented_Message&& o);

    Fragmented_Message(const Fragmented_Message&) = delete;
    Fragmented_Message& operator =(const Fragmented_Message&) = delete;

    ~Fragmented_Message();

    bool operator ==(uint8_t id) const;

    void add_data(uint32_t pos, const char *data, uint32_t len);
    void remove_from_part_vect(uint32_t new_part_start, uint32_t new_part_end);
    bool is_parts_empty() const;
    QPair<uint32_t, uint32_t> get_next_part() const;

    uint8_t id_;
    uint16_t cmd_;
    uint32_t max_fragment_size_;
    QIODevice* data_device_;

    std::chrono::system_clock::time_point last_part_time_;

    std::vector<std::pair<uint32_t, uint32_t>> part_vect_;
};

} // namespace Network
} // namespace Helpz

#endif // HELPZ_NETWORK_FRAGMENTED_MESSAGE_H
