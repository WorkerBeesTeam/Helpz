#include <iostream>

#include <QBuffer>

#include "net_protocol.h"
#include "net_protocol_sender.h"

namespace Helpz {
namespace Network {

Q_DECLARE_LOGGING_CATEGORY(Log)

Message_Item::Message_Item(quint16 command, std::optional<uint8_t> &&answer_id, std::shared_ptr<QIODevice> &&device_ptr,
                           std::chrono::milliseconds resend_timeout, uint32_t fragment_size) :
    answer_id_{std::move(answer_id)}, cmd_(command), fragment_size_(fragment_size), resend_timeout_(resend_timeout), data_device_{std::move(device_ptr)}
{
}

// -------------------------------------------------------------------------------------------------------------------------------------

Protocol_Sender::Protocol_Sender(Protocol *p, uint16_t command, std::optional<uint8_t> answer_id, std::shared_ptr<QIODevice> device_ptr, std::chrono::milliseconds resend_timeout) :
    QDataStream(device_ptr.get()),
    protocol_(p), msg_{command, std::move(answer_id), std::move(device_ptr), std::move(resend_timeout)}
{
    if (!msg_.data_device_)
    {
        msg_.data_device_.reset(new QBuffer{});
        setDevice(msg_.data_device_.get());
    }

    if (!device()->isOpen())
    {
        device()->open(QIODevice::ReadWrite);
    }
    setVersion(Protocol::DATASTREAM_VERSION);

    if (msg_.cmd_ & Protocol::ALL_FLAGS)
    {
        qCCritical(Log) << "ERROR: Try to send bad cmd with setted flags. cmd:" << msg_.cmd_;
        msg_.cmd_ &= ~Protocol::ALL_FLAGS;
    }
}

Protocol_Sender::Protocol_Sender(Protocol_Sender &&obj) noexcept :
    QDataStream(obj.device()),
    protocol_(std::move(obj.protocol_)), msg_(std::move(obj.msg_))
{
    setVersion(obj.version());
    obj.setDevice(nullptr);
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

    setDevice(nullptr);
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

void Protocol_Sender::set_fragment_size(uint32_t fragment_size)
{
    msg_.fragment_size_ = fragment_size;
}

void Protocol_Sender::set_data_device(std::shared_ptr<QIODevice> data_dev, uint32_t fragment_size)
{
    if (!data_dev)
    {
        return;
    }

    setDevice(nullptr);
    msg_.fragment_size_ = fragment_size;
    msg_.data_device_ = std::move(data_dev);
    setDevice(msg_.data_device_.get());
}

Protocol_Sender& Protocol_Sender::answer(std::function<void(QIODevice&)> answer_func)
{
    msg_.answer_func_ = std::move(answer_func);
    auto now = std::chrono::system_clock::now();
    if (msg_.end_time_ < now)
    {
        msg_.end_time_ = now + std::chrono::seconds(10);
    }
    return *this;
}
Protocol_Sender& Protocol_Sender::timeout(std::function<void()> timeout_func, std::chrono::milliseconds timeout_duration, std::chrono::milliseconds resend_timeout)
{
    msg_.timeout_func_ = std::move(timeout_func);
    msg_.end_time_ = std::chrono::system_clock::now() + timeout_duration;
    msg_.resend_timeout_ = resend_timeout;
    return *this;
}

} // namespace Network
} // namespace Helpz
