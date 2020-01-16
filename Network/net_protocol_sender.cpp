#include <iostream>
#include <assert.h>

#include <QBuffer>

#include "net_protocol.h"
#include "net_protocol_sender.h"

namespace Helpz {
namespace Network {

Q_DECLARE_LOGGING_CATEGORY(Log)

Protocol_Sender::Protocol_Sender(std::shared_ptr<Protocol> p, uint8_t command, std::optional<uint8_t> answer_id,
                                 std::unique_ptr<QIODevice> device_ptr, std::chrono::milliseconds resend_timeout) :
    QDataStream(device_ptr.get()),
    protocol_(std::move(p)), msg_{command, std::move(answer_id), std::move(device_ptr), std::move(resend_timeout)}
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
}

Protocol_Sender::Protocol_Sender(Protocol_Sender &&obj) noexcept :
    QDataStream(obj.device()),
    protocol_(std::move(obj.protocol_)), msg_(std::move(obj.msg_))
{
    setVersion(obj.version());
    obj.setDevice(nullptr);
    obj.protocol_.reset();
}

Protocol_Sender::~Protocol_Sender()
{
    if (protocol_ && msg_.data_device_)
    {
        msg_.data_device_->seek(msg_.data_device_->size());
        protocol_->send_message(std::move(msg_));
    }

    setDevice(nullptr);
}

void Protocol_Sender::release()
{
    protocol_.reset();
}

void Protocol_Sender::set_fragment_size(uint32_t fragment_size)
{
    msg_.fragment_size_ = fragment_size;
}

void Protocol_Sender::set_data_device(std::unique_ptr<QIODevice> data_dev, uint32_t fragment_size)
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
    assert(!msg_.answer_id_ && "Attempt to wait answer to answer");
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
