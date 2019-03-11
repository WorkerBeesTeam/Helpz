#include <iostream>

#include <QBuffer>

#include "net_protocol.h"
#include "net_protocol_sender.h"

namespace Helpz {
namespace Network {

Message_Item::Message_Item(quint16 command, std::optional<uint8_t> &&answer_id, std::shared_ptr<QIODevice> &&device_ptr, uint32_t fragment_size) :
    answer_id_{std::move(answer_id)}, cmd_(command), fragment_size_(fragment_size), data_device_{std::move(device_ptr)}
{
}

// -------------------------------------------------------------------------------------------------------------------------------------

Protocol_Sender::Protocol_Sender(Protocol *p, uint16_t command, std::optional<uint8_t> answer_id, std::shared_ptr<QIODevice> device_ptr) :
    protocol_(p), msg_{command, std::move(answer_id), std::move(device_ptr)}
{
    if (!msg_.data_device_)
    {
        msg_.data_device_.reset(new QBuffer{});
    }

    msg_.data_device_->open(QIODevice::ReadWrite);
    setDevice(msg_.data_device_.get());
    setVersion(Protocol::DATASTREAM_VERSION);

    if (msg_.cmd_ & Protocol::ALL_FLAGS)
    {
        std::cerr << "ERROR: Try to send bad cmd with setted flags " << msg_.cmd_ << std::endl;
        msg_.cmd_ &= ~Protocol::ALL_FLAGS;
    }
}

Protocol_Sender::Protocol_Sender(Protocol_Sender &&obj) noexcept :
    protocol_(std::move(obj.protocol_)), msg_(std::move(obj.msg_))
{
    obj.unsetDevice();

    std::cout << "Protocol_Sender MOVE" << std::endl;
    setDevice(msg_.data_device_.get());
    setVersion(obj.version());

    obj.protocol_ = nullptr;
}

Protocol_Sender::~Protocol_Sender()
{
    if (protocol_ && msg_.data_device_)
    {
        uint32_t start_pos = 0;

        if (msg_.data_device_->size() > msg_.fragment_size_)
        {
            start_pos = std::numeric_limits<uint32_t>::max();
            msg_.end_time_ = std::chrono::system_clock::now() + std::chrono::minutes(3);
        }

        protocol_->send_message(std::move(msg_), start_pos);
    }

    unsetDevice();
}

void Protocol_Sender::release()
{
    protocol_ = nullptr;
}

QByteArray Protocol_Sender::pop_packet()
{
    QByteArray data = protocol_->prepare_packet(msg_);
    protocol_ = nullptr;
    return data;
}

void Protocol_Sender::set_data_device(std::shared_ptr<QIODevice> data_dev, uint32_t fragment_size)
{
    if (!data_dev)
    {
        return;
    }

    unsetDevice();
    msg_.fragment_size_ = fragment_size;
    msg_.data_device_ = std::move(data_dev);
    setDevice(msg_.data_device_.get());
}

Protocol_Sender &Protocol_Sender::answer(std::function<void (QIODevice&)> answer_func)
{
    msg_.answer_func_ = std::move(answer_func);
    auto now = std::chrono::system_clock::now();
    if (msg_.end_time_ < now)
    {
        msg_.end_time_ = now + std::chrono::seconds(10);
    }
    return *this;
}
Protocol_Sender &Protocol_Sender::timeout(std::function<void ()> timeout_func, std::chrono::milliseconds timeout_duration)
{
    msg_.timeout_func_ = std::move(timeout_func);
    msg_.end_time_ = std::chrono::system_clock::now() + timeout_duration;
    return *this;
}

} // namespace Network
} // namespace Helpz
