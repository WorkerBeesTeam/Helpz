#include <QDir>
#include <QDateTime>
#include <QBuffer>

#include "net_protocol.h"
#include "net_fragmented_message.h"

namespace Helpz {
namespace Net {

Fragmented_Message::Fragmented_Message(uint8_t id, uint8_t cmd, uint32_t max_fragment_size, uint32_t full_size) :
    id_(id), cmd_(cmd), max_fragment_size_(max_fragment_size)
{
    if (full_size < 1000000)
    {
        data_device_ = new QBuffer;
    }
    else
    {
        QString file_name = QDir::tempPath() + "/helpz_net_proto_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + '_' + QString::number(id) + ".dat";
        auto file = new QFile(file_name);
        file->setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        data_device_ = file;
    }

    part_vect_.push_back(std::pair<uint32_t,uint32_t>(0, full_size));
}

Fragmented_Message::Fragmented_Message(Fragmented_Message&& o) :
    id_(std::move(o.id_)), cmd_(std::move(o.cmd_)), max_fragment_size_(std::move(o.max_fragment_size_)),
    data_device_(std::move(o.data_device_)), last_part_time_(std::move(o.last_part_time_)), part_vect_(std::move(o.part_vect_))
{
    o.data_device_ = nullptr;
}

Fragmented_Message& Fragmented_Message::operator =(Fragmented_Message&& o)
{
    id_ = std::move(o.id_);
    cmd_ = std::move(o.cmd_);
    max_fragment_size_ = std::move(o.max_fragment_size_);
    data_device_ = std::move(o.data_device_);
    o.data_device_ = nullptr;
    last_part_time_ = std::move(o.last_part_time_);
    part_vect_ = std::move(o.part_vect_);
    return *this;
}

Fragmented_Message::~Fragmented_Message()
{
    if (data_device_)
    {
        data_device_->close();

        QFile* file = dynamic_cast<QFile*>(data_device_);
        if (file)
            file->remove();

        delete data_device_;
    }
}

bool Fragmented_Message::operator <(const Fragmented_Message &o) const { return id_ < o.id_; }
bool Fragmented_Message::operator <(uint8_t id) const { return id_ < id; }
bool Fragmented_Message::operator ==(uint8_t id) const { return id_ == id; }

void Fragmented_Message::add_data(uint32_t pos, const char *data, uint32_t len)
{
    remove_from_part_vect(pos, pos + len);

    data_device_->seek(pos);
    data_device_->write(data, len);
}

void Fragmented_Message::remove_from_part_vect(uint32_t new_part_start, uint32_t new_part_end)
{
    for (auto it = part_vect_.begin(); it != part_vect_.end(); )
    {
        // Если вся часть до текущей в списке
        if (new_part_end <= it->first)
            break;

        // Если часть после текущей в списке
        if (new_part_start > it->second)
        {
            // То переходим дальше по списку
            ++it;
            continue;
        }

        // Если начало новой части до текущей в списке
        if (new_part_start < it->first)
        {
            // Если новая часть больше текущей в списке
            if (new_part_end > it->second)
            {
                // То у новой части отрезаем спереди до конца текущей в списке
                new_part_start = std::move(it->second);

                // И удаляем пустую часть из списка
                it = part_vect_.erase(it);
            }
            else
            {
                // Если новая часть меньше текущей в списке
                // То отрезаем у текущей в списке до конца новой части
                it->first = std::move(new_part_end);

                // Если часть из списка пустая то удаляем её
                if (it->first == it->second)
                    it = part_vect_.erase(it);
                break;
            }
        }
        // Если начало новой части за текущей в списке
        else if (new_part_start > it->first)
        {
            uint32_t current_end = std::move(it->second);

            // То текущая часть теперь заканчивается началом новой
            it->second = std::move(new_part_start);

            // Если новая часть выходит за текущую в списке
            if (new_part_end > current_end)
            {
                // То у новой части отрезаем спереди до конца текущей в списке
                new_part_start = std::move(current_end);

                // И переходим дальше по списку
                ++it;
            }
            else
            {
                // Если новая часть меньше текущей в списке
                if (new_part_end < current_end)
                {
                    // То добавляем часть в список которая
                    // Начинается с конца новой и заканчивается концом текущей

                    std::pair<uint32_t, uint32_t> new_part(std::move(new_part_end), std::move(current_end));
                    part_vect_.insert(++it, std::move(new_part));
                }
                break;
            }
        }
        // Если начало новой части совпадает с началом текущей в списке
        else // ==
        {
            // Если новая часть выходит за текущую в списке
            if (new_part_end > it->second)
            {
                // То у новой части отрезаем спереди до конца текущей в списке
                new_part_start = std::move(it->second);

                // Удаляем пустую и переходим дальше по списку
                it = part_vect_.erase(it);
            }
            // Если новая часть меньше текущей в списке
            else if (new_part_end < it->second)
            {
                // То текущая часть теперь начинается с конца новой
                it->first = std::move(new_part_end);
                break;
            }
            // Если новая часть совпадает с текущей в списке
            else
            {
                // То удаляем её
                part_vect_.erase(it);
                break;
            }
        }
    }
}

bool Fragmented_Message::is_parts_empty() const
{
    return part_vect_.empty();
}

QPair<uint32_t, uint32_t> Fragmented_Message::get_next_part() const
{
    if (part_vect_.empty())
        return {0, 0};

    const std::pair<uint32_t, uint32_t>& part = part_vect_.front();
    if (part.second - part.first > max_fragment_size_)
        return {part.first, max_fragment_size_};
//        part.second = max_fragment_size_;
    else
        return {part.first, part.second - part.first};
//        part.second = part.second - part.first;
//    return part;
}

} // namespace Net
} // namespace Helpz
