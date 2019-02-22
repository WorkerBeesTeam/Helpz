#include <iostream>

#include <QBuffer>

#include "net_protocol.h"
#include "net_protocol_sender.h"

namespace Helpz {
namespace Network {

Protocol_Sender::Protocol_Sender(Protocol *p, quint16 command, quint16 flags) :
    protocol_(p)
{
    auto buffer = new QBuffer{&msg_.data_};
    buffer->open(QIODevice::ReadWrite);
    setDevice(buffer);
    setVersion(Protocol::DATASTREAM_VERSION);

    msg_.cmd_ = command;
    if (msg_.cmd_ & Protocol::ALL_FLAGS)
    {
        std::cerr << "ERROR: Try to send bad cmd with setted flags " << msg_.cmd_ << std::endl;
        msg_.cmd_ &= ~Protocol::ALL_FLAGS;
    }

    if (flags & ~Protocol::ALL_FLAGS)
    {
        std::cerr << "ERROR: Try to send bad flags with cmd bits " << flags << std::endl;
        flags &= Protocol::ALL_FLAGS;
    }

    msg_.cmd_ |= flags;
}

Protocol_Sender::Protocol_Sender(Protocol_Sender &&obj) noexcept :
    protocol_(std::move(obj.protocol_)), msg_(std::move(obj.msg_))
{
    QBuffer* buffer = static_cast<QBuffer*>(obj.device());
    obj.unsetDevice();
    buffer->close();
    delete buffer;

    std::cout << "Protocol_Sender MOVE" << std::endl;
    buffer = new QBuffer{&msg_.data_};
    buffer->open(QIODevice::ReadWrite);
    setDevice(buffer);

    device()->seek(obj.device()->pos());
    setVersion(obj.version());

    obj.protocol_ = nullptr;
}

Protocol_Sender::~Protocol_Sender()
{
    if (protocol_)
    {
        protocol_->send_message(std::move(msg_));
//        const QByteArray data = prepare_message();
//        protocol_->write(reinterpret_cast<const uint8_t*>(data.constData()), data.size());
    }

    QBuffer* buffer = static_cast<QBuffer*>(device());
    if (buffer)
    {
        unsetDevice();
        delete buffer;
    }
}

QByteArray Protocol_Sender::pop_packet()
{
    QByteArray data = protocol_->prepare_packet(msg_);
    protocol_ = nullptr;
    return data;
}

Protocol_Sender &Protocol_Sender::data_device(std::shared_ptr<QIODevice> data_dev)
{
    msg_.data_device_ = std::move(data_dev);
    return *this;
}

Protocol_Sender &Protocol_Sender::answer(std::function<void (QByteArray&&, QIODevice*)> answer_func)
{
    msg_.answer_func_ = std::move(answer_func);
    return *this;
}
Protocol_Sender &Protocol_Sender::timeout(std::function<void ()> timeout_func)
{
    msg_.timeout_func_ = std::move(timeout_func);
    return *this;
}

} // namespace Network
} // namespace Helpz
