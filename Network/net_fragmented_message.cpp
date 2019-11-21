#include <QDir>
#include <QDateTime>
#include <QBuffer>

#include "net_fragmented_message.h"

namespace Helpz {
namespace Network {

Fragmented_Message::Fragmented_Message(uint8_t id, uint16_t cmd, uint32_t max_fragment_size, bool in_memory) :
    id_(id), cmd_(cmd), max_fragment_size_(max_fragment_size)
{
    if (in_memory)
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
}

Fragmented_Message::Fragmented_Message(Fragmented_Message&& o) :
    id_(std::move(o.id_)), cmd_(std::move(o.cmd_)), max_fragment_size_(std::move(o.max_fragment_size_)),
    data_device_(std::move(o.data_device_))
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

bool Fragmented_Message::operator ==(uint8_t id) const
{
    return id_ == id;
}

} // namespace Network
} // namespace Helpz
