#include <QDebug>
#include <QTemporaryFile>

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

Fragmented_Message::Fragmented_Message(uint8_t id, uint16_t cmd, uint32_t max_fragment_size) :
    id_(id), cmd_(cmd), max_fragment_size_(max_fragment_size)
{
    auto file = new QTemporaryFile{};
    file->setAutoRemove(true);
    file->open();
    data_device_.reset(file);
}

bool Fragmented_Message::operator ==(uint8_t id) const
{
    return id_ == id;
}

// ----------------------------------------------------------------------------------

Protocol::Protocol() :
    protocol_writer_(nullptr),
    next_rx_msg_id_(0), next_tx_msg_id_(0),
    last_msg_send_time_(Time_Point{})
{
    device_.open(QBuffer::ReadWrite);
    msg_stream_.setDevice(&device_);
    msg_stream_.setVersion(DATASTREAM_VERSION);
}

std::string Protocol::title() const
{
    return protocol_writer_ ? protocol_writer_->title() : std::string{};
}

void Protocol::reset_msg_id()
{
    next_rx_msg_id_ = 0;
    next_tx_msg_id_ = 0;
}

Protocol_Writer *Protocol::writer()
{
    return protocol_writer_;
}

void Protocol::set_writer(Protocol_Writer *protocol_writer)
{
    protocol_writer_ = protocol_writer;
}

Protocol::Time_Point Protocol::last_msg_send_time() const
{
    return last_msg_send_time_.load();
}

Protocol_Sender Protocol::send(quint16 cmd, quint16 flags) { return Protocol_Sender(this, cmd, flags); }
void Protocol::send_cmd(quint16 cmd, quint16 flags) { send(cmd, flags); }
void Protocol::send_byte(quint16 cmd, char byte) { send(cmd) << byte; }
void Protocol::send_array(quint16 cmd, const QByteArray &buff) { send(cmd) << buff; }

void Protocol::send_message(Message_Item message, uint32_t pos, uint32_t max_data_size, std::chrono::milliseconds resend_timeout)
{
    if (!protocol_writer_)
    {
        return;
    }

    last_msg_send_time_ = std::chrono::system_clock::now();

    if (!message.id_)
    {
        message.id_ = next_tx_msg_id_++;
    }

    const QByteArray packet = prepare_packet(message, pos, max_data_size);
    if (!packet.size())
    {
        return;
    }

    Time_Point now = std::chrono::system_clock::now();
    if (message.end_time_ > now)
    {
        Time_Point time_point;
        if ((message.end_time_ - now) > resend_timeout)
        {
            time_point = now + resend_timeout;
        }
        else
        {
            time_point = message.end_time_;
        }
        add_to_waiting(time_point, std::move(message));
        protocol_writer_->add_timeout_at(time_point);
    }

    protocol_writer_->write(reinterpret_cast<const uint8_t*>(packet.constData()), packet.size());
}

QByteArray Protocol::prepare_packet(const Message_Item &msg, uint32_t pos, uint32_t max_data_size)
{
    if (!msg.data_device_ || (pos > msg.data_device_->size() && pos != std::numeric_limits<uint32_t>::max()))
    {
        return {};
    }

    QByteArray packet, data;
    uint16_t cmd = msg.cmd_;

    if (msg.data_device_->size() > max_data_size || pos != 0)
    {
        cmd |= FRAGMENT;

        QDataStream ds(&data, QIODevice::WriteOnly);
        ds.setVersion(DATASTREAM_VERSION);
        ds << static_cast<uint32_t>(msg.data_device_->size());

        if (pos != std::numeric_limits<uint32_t>::max())
        {
            ds << pos;
            uint32_t raw_size = std::min<uint32_t>(max_data_size, msg.data_device_->size() - pos);
            data.resize(8 + raw_size);
            msg.data_device_->seek(pos);
            msg.data_device_->read(data.data() + 8, raw_size);
        }
        else
        {
            ds << max_data_size;
        }
    }
    else
    {
        msg.data_device_->seek(pos);
        data = msg.data_device_->read(max_data_size);
    }

    if (data.size() > 512)
    {
        cmd |= COMPRESSED;
        data = qCompress(data);
    }

    QDataStream ds(&packet, QIODevice::WriteOnly);
    ds.setVersion(DATASTREAM_VERSION);
    ds << uint16_t(0) << msg.id_.value_or(0) << cmd << data;

    ds.device()->seek(0);
    ds << qChecksum(packet.constData() + 2, 7);

//    qCDebug(DetailLog) << "CMD OUT" << (cmd & ~ALL_FLAGS) << "SIZE" << data.size() << "WRITE" << buffer.size();
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

void Protocol::process_wait_list()
{
    std::vector<Message_Item> messages = pop_waiting_messages();

    Time_Point now = std::chrono::system_clock::now();
    for (Message_Item& msg: messages)
    {
        if (msg.end_time_ > now)
        {
            send_message(std::move(msg));
        }
        else
        {
            if (msg.timeout_func_)
            {
                msg.timeout_func_();
            }
        }
    }
}

void Protocol::process_stream()
{
    bool checksum_ok;
    uint8_t msg_id;
    uint16_t checksum, cmd, flags;
    uint32_t buffer_size;
    uint64_t pos;

    while (device_.bytesAvailable() >= 9)
    {
        pos = device_.pos();

        // Using QDataStream for auto swap bytes for big or little endian
        msg_stream_ >> checksum >> msg_id >> cmd >> buffer_size;
        checksum_ok = checksum == qChecksum(device_.buffer().constData() + pos + 2, 7);

        if (buffer_size == 0xffffffff)
        {
            buffer_size = 0;
        }

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

        try
        {
            internal_process_message(msg_id, cmd, flags, device_.buffer().constData() + pos + 9, buffer_size);
        }
        catch(const std::exception& e)
        {
            qCCritical(Log) << "EXCEPTION: process_stream" << cmd << e.what();
        }
        catch(...)
        {
            qCCritical(Log) << "EXCEPTION Unknown: process_stream" << cmd;
        }

        device_.seek(pos + 9 + buffer_size);
    }
}

bool Protocol::is_lost_message(uint8_t msg_id)
{
    for (auto it = lost_msg_list_.begin(); it != lost_msg_list_.end(); ++it)
    {
        if (it->second == msg_id)
        {
            lost_msg_list_.erase(it);
            return true;
        }
    }
    return false;
}

void Protocol::fill_lost_msg(uint8_t msg_id)
{
    Time_Point now = std::chrono::system_clock::now();

    for (auto it = lost_msg_list_.begin(); it != lost_msg_list_.end(); )
    {
        if ((now - it->first) > std::chrono::minutes(4) || (it->second >= next_rx_msg_id_ && it->second < msg_id))
        {
            it = lost_msg_list_.erase(it);
        }
        else
        {
            ++it;
        }
    }

    while (msg_id != next_rx_msg_id_)
    {
        lost_msg_list_.push_back(std::make_pair(now, next_rx_msg_id_++));
    }
}

void Protocol::internal_process_message(uint8_t msg_id, quint16 cmd, quint16 flags, const char *data_ptr, quint32 data_size)
{
//    std::cout << "internal_process_message " << int(msg_id) << "(" << int(next_rx_msg_id_) << ") cmd: " << (cmd & ~ALL_FLAGS) << std::endl;
    if (msg_id < next_rx_msg_id_)
    {
        if (!is_lost_message(msg_id))
        {
            return;
        }
    }
    else
    {
        if (msg_id > next_rx_msg_id_)
        {
            if ((msg_id - next_rx_msg_id_) > std::numeric_limits<int8_t>::max())
            {
                std::swap(msg_id, next_rx_msg_id_);
            }

            fill_lost_msg(msg_id);
        }

        next_rx_msg_id_ = msg_id + 1;
    }

    // If COMPRESSED or FRAGMENT flag is setted, then data_size can not be zero.
    if (!data_size && (flags & (COMPRESSED | FRAGMENT)))
    {
        return;
    }

    QByteArray data = flags & COMPRESSED ?
                qUncompress(reinterpret_cast<const uchar*>(data_ptr), data_size) :
                QByteArray(data_ptr, data_size);

    if (flags & FRAGMENT)
    {
        uint32_t full_size, pos;
        parse_out(data, full_size, pos);

        std::vector<Fragmented_Message>::iterator it = std::find(fragmented_messages_.begin(), fragmented_messages_.end(), msg_id);

        if (full_size >= MAX_MESSAGE_SIZE)
        {
            fragmented_messages_.erase(it);
            return;
        }

        if (it == fragmented_messages_.end())
        {
            uint32_t max_fragment_size = pos > 0 ? pos : MAX_MESSAGE_DATA_SIZE;
            it = fragmented_messages_.emplace(fragmented_messages_.end(), Fragmented_Message{ msg_id, cmd, max_fragment_size });
        }
        Fragmented_Message &msg = *it;
        if ((data.size() - 8) > 0)
        {
            msg.data_device_->seek(pos);
            msg.data_device_->write(data.constData() + 8, data.size() - 8);
        }

        if (msg.data_device_->pos() == full_size)
        {
//            msg.data_device_->close();
            process_message(cmd, msg.data_device_.get());
            fragmented_messages_.erase(it);
        }
        else
        {
            lost_msg_list_.push_back(std::make_pair(std::chrono::system_clock::now(), msg_id));
            send(cmd, FRAGMENT_QUERY) << msg_id << static_cast<uint32_t>(msg.data_device_->pos()) << msg.max_fragment_size_;
        }
    }
    else if (flags & FRAGMENT_QUERY)
    {
        uint8_t fragmanted_msg_id;
        uint32_t pos, fragmanted_size;
        parse_out(data, fragmanted_msg_id, pos, fragmanted_size);

        Message_Item msg = pop_waiting_message([fragmanted_msg_id](const Message_Item &item){ return item.id_.value_or(0) == fragmanted_msg_id; });
        if (msg.data_device_)
        {
            send_message(std::move(msg), pos, fragmanted_size);
        }
    }
    else
    {
        if (flags & ANSWER)
        {
            Message_Item msg = pop_waiting_message([cmd](const Message_Item &item){ return item.cmd_ == cmd; });
            if (msg.answer_func_)
            {
                QBuffer buffer(&data);
                msg.answer_func_(&buffer);
            }
        }
        else
        {
            if (cmd == Cmd::Ping)
            {
                send_cmd(Cmd::Ping, ANSWER);
            }
            else
            {
                QBuffer buffer(&data);
                process_message(cmd, &buffer);
            }
        }
    }
}

void Protocol::add_to_waiting(Time_Point time_point, Message_Item &&message)
{
    std::lock_guard lock(waiting_messages_mutex_);
    waiting_messages_.emplace(time_point, std::move(message));
}

std::vector<Message_Item> Protocol::pop_waiting_messages()
{
    std::vector<Message_Item> messages;
    std::lock_guard lock(waiting_messages_mutex_);

    Time_Point now = std::chrono::system_clock::now() + std::chrono::milliseconds(20);
    for (auto it = waiting_messages_.begin(); it != waiting_messages_.end(); )
    {
        if (it->first > now)
        {
            break;
        }

        messages.push_back(std::move(it->second));
        it = waiting_messages_.erase(it);
    }
    return messages;
}

Message_Item Protocol::pop_waiting_message(std::function<bool (const Message_Item &)> check_func)
{
    std::lock_guard lock(waiting_messages_mutex_);
    for (auto it = waiting_messages_.begin(); it != waiting_messages_.end(); ++it)
    {
        if (check_func(it->second))
        {
            Message_Item msg = std::move(it->second);
            waiting_messages_.erase(it);
            return std::move(msg);
        }
    }
    return {};
}

} // namespace Network
} // namespace Helpz
