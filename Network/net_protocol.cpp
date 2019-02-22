#include <QDebug>

#include "net_protocol.h"

namespace Helpz {
namespace Network {

Q_LOGGING_CATEGORY(Log, "net")
Q_LOGGING_CATEGORY(DetailLog, "net.detail")

const std::string &Protocol_Writer::title() const { return title_; }
void Protocol_Writer::set_title(const std::string &title) { title_ = title; }

std::chrono::time_point<std::chrono::system_clock> Protocol_Writer::last_msg_recv_time() const { return last_msg_recv_time_; }
void Protocol_Writer::set_last_msg_recv_time(std::chrono::time_point<std::chrono::system_clock> value) { last_msg_recv_time_ = value; }

// ----------------------------------------------------------------------------------

Protocol::Protocol() :
    protocol_writer_(nullptr)
{
    device_.open(QBuffer::ReadWrite);
    msg_stream_.setDevice(&device_);
    msg_stream_.setVersion(DATASTREAM_VERSION);
}

void Protocol::set_protocol_writer(Protocol_Writer *protocol_writer)
{
    protocol_writer_ = protocol_writer;
}

std::chrono::time_point<std::chrono::system_clock> Protocol::last_msg_send_time() const
{
    return last_msg_send_time_;
}

Protocol_Sender Protocol::send(quint16 cmd, quint16 flags) { return Protocol_Sender(this, cmd, flags); }

void Protocol::send_cmd(quint16 cmd, quint16 flags)
{
    send(cmd, flags);
}
void Protocol::send_byte(quint16 cmd, char byte) { send(cmd) << byte; }
void Protocol::send_array(quint16 cmd, const QByteArray &buff) { send(cmd) << buff; }

void Protocol::send_message(Message_Item message)
{
    if (!protocol_writer_)
    {
        return;
    }

    last_msg_send_time_ = std::chrono::system_clock::now();

    const QByteArray data = prepare_packet(message);
    protocol_writer_->write(reinterpret_cast<const uint8_t*>(data.constData()), data.size());
}

QByteArray Protocol::prepare_packet(Message_Item &msg)
{
    if (msg.data_.size() > 512)
    {
        msg.cmd_ |= COMPRESSED;
    }

    QByteArray packet;
    QDataStream ds(&packet, QIODevice::ReadWrite);
    ds << uint16_t(0) << msg.cmd_;

    if (msg.cmd_ & COMPRESSED)
    {
        ds << qCompress(msg.data_);
    }
    else
    {
        ds << msg.data_;
    }

    ds.device()->seek(0);
    ds << qChecksum(packet.constData() + 2, 6);

//    qCDebug(DetailLog) << "CMD OUT" << (cmd & ~Protocol::ALL_FLAGS) << "SIZE" << data.size() << "WRITE" << buffer.size();
    return packet;
}

void Protocol::process_bytes(const quint8* data, size_t size)
{
    protocol_writer_->set_last_msg_recv_time(std::chrono::system_clock::now());

    device_.write((const char*)data, size);
    device_.seek(0);
    process_stream();

    int next_start_pos = 0;
    if (device_.bytesAvailable())
    {
        next_start_pos = device_.bytesAvailable();
        device_.buffer() = device_.buffer().right(next_start_pos);
    }
    else
        device_.buffer().clear();
    device_.seek(next_start_pos);
}

void Protocol::process_stream()
{
    bool checksum_ok;
    quint16 checksum, cmd, flags;
    quint32 buffer_size;
    quint64 pos;

    while (device_.bytesAvailable() >= 8)
    {
        pos = device_.pos();

        // Using QDataStream for auto swap bytes for big or little endian
        msg_stream_ >> checksum >> cmd >> buffer_size;
        checksum_ok = checksum == qChecksum(device_.buffer().constData() + pos + 2, 6);

        if (buffer_size == 0xffffffff)
            buffer_size = 0;

        if (!checksum_ok || buffer_size > MAX_MESSAGE_SIZE) // Drop message if checksum bad or too high size
        {
            if (!device_.atEnd())
            {
                device_.seek(device_.size());
            }
            return;
        }
        else if (device_.bytesAvailable() < buffer_size) // else if all ok, but not enough bytes
        {
            device_.seek(pos);
            return; // Wait more bytes
        }

        flags = cmd & ALL_FLAGS;
        cmd &= ~ALL_FLAGS;

        try {
            internal_process_message(cmd, flags, device_.buffer().constData() + pos + 8, buffer_size);
        }
        catch(const std::exception& e)
        {
            qCCritical(Log) << "EXCEPTION: process_stream" << cmd << e.what();
        }
        catch(...)
        {
            qCCritical(Log) << "EXCEPTION Unknown: process_stream" << cmd;
        }

        device_.seek(pos + 8 + buffer_size);
    }
}

void Protocol::internal_process_message(quint16 cmd, quint16 flags, const char *data_ptr, quint32 data_size)
{
    // If COMPRESSED or FRAGMENT flag is setted, then data_size can not be zero.
    if (!data_size && (flags & (COMPRESSED | FRAGMENT)))
    {
        return;
    }

    QByteArray data = flags & COMPRESSED ?
                qUncompress(reinterpret_cast<const uchar*>(data_ptr), data_size) :
                QByteArray(data_ptr, data_size);

    if (flags & ANSWER)
    {
        quint8 msg_id, confirmed_flag;
        parse_out(data, msg_id, confirmed_flag);

        if (confirmed_flag)
        {
            // remove from confirmation map, stop timer
            return;
        }

        confirmed_flag = true;
        send(cmd, flags) << msg_id << confirmed_flag;
        if (last_confirmation_id_ == msg_id)
        {
            return;
        }

        last_confirmation_id_ = msg_id;
        data.remove(0, 2);
    }

    if (flags & FRAGMENT)
    {
        quint8 msg_id, fragment_flag;
        quint32 full_size, pos;
        parse_out(data, msg_id, fragment_flag, full_size, pos);

        if (full_size >= MAX_MESSAGE_SIZE)
        {
            // remove fragment_id from wait list
            return;
        }

        data.remove(0, 6);
        // TODO: write data to device
    }
    else
    {
        if (flags & ANSWER)
        {
//            process_answer_message(cmd, std::move(data));
        }
        else
        {
            if (cmd == Cmd::Ping)
            {
                send_cmd(Cmd::Ping, ANSWER);
            }
            process_message(cmd, std::move(data), nullptr);
        }
    }
}

} // namespace Network
} // namespace Helpz
