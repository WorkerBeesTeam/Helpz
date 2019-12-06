#include <QDebug>

#include "net_protocol.h"

namespace Helpz {
namespace Network {

Q_LOGGING_CATEGORY(Log, "net")
Q_LOGGING_CATEGORY(DetailLog, "net.detail", QtInfoMsg)

const QString &Protocol_Writer::title() const { return title_; }
void Protocol_Writer::set_title(const QString &title) { title_ = title; }

std::chrono::time_point<std::chrono::system_clock> Protocol_Writer::last_msg_recv_time() const { return last_msg_recv_time_; }
void Protocol_Writer::set_last_msg_recv_time(std::chrono::time_point<std::chrono::system_clock> value) { last_msg_recv_time_ = value; }

// ----------------------------------------------------------------------------------

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

QString Protocol::title() const
{
    return protocol_writer_ ? protocol_writer_->title() : QString{};
}

std::shared_ptr<Protocol_Writer> Protocol::writer_pointer()
{
    return writer_pointer_;
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

const Protocol_Writer* Protocol::writer() const
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

Protocol_Sender Protocol::send(uint16_t cmd) { return Protocol_Sender(this, cmd); }
Protocol_Sender Protocol::send_answer(uint16_t cmd, std::optional<uint8_t> msg_id) { return Protocol_Sender(this, cmd, msg_id); }
void Protocol::send_byte(uint16_t cmd, char byte) { send(cmd) << byte; }
void Protocol::send_array(uint16_t cmd, const QByteArray &buff) { send(cmd) << buff; }

void Protocol::send_message(Message_Item message, uint32_t pos)
{
    if (!protocol_writer_)
    {
        return;
    }

    last_msg_send_time_ = std::chrono::system_clock::now();

    bool is_new_message = !message.id_;
    if (is_new_message)
    {
        message.id_ = next_tx_msg_id_++;
    }

    QByteArray packet = prepare_packet(message, pos);
    if (!packet.size())
    {
        if (is_new_message)
            --next_tx_msg_id_;
        qCDebug(DetailLog).noquote() << title() << "Attempt to send empty message. cmd:" << message.cmd_;
        return;
    }

    Time_Point now = std::chrono::system_clock::now();
    if (message.end_time_ > now)
    {
        Time_Point time_point;
        if ((message.end_time_ - now) > message.resend_timeout_)
        {
            time_point = now + message.resend_timeout_;
        }
        else
        {
            time_point = message.end_time_;
        }
        add_to_waiting(time_point, std::move(message));
        protocol_writer_->add_timeout_at(time_point);
    }

    protocol_writer_->write(std::move(packet));
}

QByteArray Protocol::prepare_packet(const Message_Item &msg, uint32_t pos)
{
    if (!msg.data_device_ || (pos > msg.data_device_->size() && pos != std::numeric_limits<uint32_t>::max()))
    {
        return {};
    }

    QByteArray packet, data;
    uint16_t cmd = msg.cmd_;

    if (msg.answer_id_ || msg.data_device_->size() > msg.fragment_size_ || pos != 0)
    {
        QDataStream ds(&data, QIODevice::WriteOnly);
        ds.setVersion(DATASTREAM_VERSION);

        if (msg.answer_id_)
        {
            cmd |= ANSWER;
            ds << *msg.answer_id_;
        }

        if (msg.data_device_->size() > msg.fragment_size_ || pos != 0)
        {
            cmd |= FRAGMENT;
            ds << static_cast<uint32_t>(msg.data_device_->size());

            if (pos != std::numeric_limits<uint32_t>::max())
            {
                ds << pos;
                add_raw_data_to_packet(data, pos, msg.fragment_size_, msg.data_device_.get());
            }
            else
            {
                ds << msg.fragment_size_;
            }
        }
        else
        {
            add_raw_data_to_packet(data, 0, msg.fragment_size_, msg.data_device_.get());
        }
    }
    else
    {
        add_raw_data_to_packet(data, 0, msg.fragment_size_, msg.data_device_.get());
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

    return packet;
}

void Protocol::add_raw_data_to_packet(QByteArray& data, uint32_t pos, uint32_t max_data_size, QIODevice* device)
{
    uint32_t raw_size = std::min<uint32_t>(max_data_size, device->size() - pos);
    uint8_t header_pos = data.size();
    data.resize(header_pos + raw_size);
    device->seek(pos);
    device->read(data.data() + header_pos, raw_size);
}

void Protocol::process_bytes(std::shared_ptr<Protocol_Writer> self_pointer, const uint8_t* data, size_t size)
{
    if (size == 0)
    {
        return;
    }

    writer_pointer_ = std::move(self_pointer);
    struct Clear_Pointer {
        Clear_Pointer(std::shared_ptr<Protocol_Writer>& p) : p_(p) {}
        ~Clear_Pointer() { p_.reset(); }
        std::shared_ptr<Protocol_Writer>& p_;
    } clear_pointer(writer_pointer_);

    if (protocol_writer_)
        protocol_writer_->set_last_msg_recv_time(std::chrono::system_clock::now());

    packet_end_position_.push(size);
    if (device_.size() != 0)
        qDebug() << "SIZE:" << device_.size();
    device_.seek(device_.size());
    device_.write(reinterpret_cast<const char*>(data), size);
    device_.seek(0);
    process_stream();

    if (device_.bytesAvailable())
    {
        device_.buffer().remove(0, device_.pos());
    }
    else
        device_.buffer().clear();
}

void Protocol::process_stream()
{
    /* TODO:
     * Нужно обрабатывать входящие пакеты отдельно, а не складывать их.
     * Т.е. если сообщение состоит из двух пакетов, пришел первый, второй потерялся,
     * а потом приходит первый пакет нового сообщения, то новое сообщение не будет обработано.
     * Нужно хранить все пакеты отдельно или хотя бы позиции начала пакета, что бы если первое сообщение неудачное,
     * то мы удаляем первый пакет, и пробуем заново.
     */

    // TODO: Нужно сделать чек-сумму на заголовки однобайтную, и добавить двубайтную чек-сумму на тело сообщения.

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
            qCCritical(Log).noquote() << title() << "EXCEPTION: process_stream" << cmd << e.what();
        }
        catch(...)
        {
            qCCritical(Log).noquote() << title() << "EXCEPTION Unknown: process_stream" << cmd;
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
    bool is_fragment_msg_deleted;
    for (auto it = lost_msg_list_.begin(); it != lost_msg_list_.end(); )
    {
        if (uint8_t(next_rx_msg_id_ - it->second) > (std::numeric_limits<int8_t>::max() / 2))
        {
            is_fragment_msg_deleted = false;
            fragmented_messages_.erase(std::remove_if(fragmented_messages_.begin(), fragmented_messages_.end(), [&is_fragment_msg_deleted, &it](const Fragmented_Message& msg)
            {
                if (msg == it->second)
                {
                    is_fragment_msg_deleted = true;
                    return true;
                }
                return false;
            }), fragmented_messages_.end());
            it = lost_msg_list_.erase(it);

            qCDebug(DetailLog).noquote() << title() << "Message" << it->second << "is lost. Is fragmented message:" << is_fragment_msg_deleted;
        }
        else
        {
            ++it;
        }
    }

    Time_Point now = std::chrono::system_clock::now();
    while (msg_id != next_rx_msg_id_)
    {
        lost_msg_list_.push_back(std::make_pair(now, next_rx_msg_id_++));
    }
}

void Protocol::internal_process_message(uint8_t msg_id, uint16_t cmd, uint16_t flags, const char *data_ptr, uint32_t data_size)
{
    if (msg_id < next_rx_msg_id_ && uint8_t(next_rx_msg_id_ - msg_id) < std::numeric_limits<int8_t>::max())
    {
        if (!is_lost_message(msg_id))
        {
            return;
        }
    }
    else
    {
        if (msg_id > next_rx_msg_id_ || uint8_t(next_rx_msg_id_ - msg_id) > std::numeric_limits<int8_t>::max())
        {
            lost_msg_detected(msg_id, next_rx_msg_id_);

            qCDebug(DetailLog).noquote() << title() << "Packet is lost from" << next_rx_msg_id_ << ". Receive message:" << msg_id;

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

    if (flags & (FRAGMENT | ANSWER))
    {
        QDataStream ds(&data, QIODevice::ReadOnly);
        ds.setVersion(DATASTREAM_VERSION);

        uint8_t answer_id;
        if (flags & ANSWER)
        {
            Helpz::parse_out(ds, answer_id);
        }

        if (flags & FRAGMENT)
        {
            uint32_t full_size, pos;
            Helpz::parse_out(ds, full_size, pos);

            std::vector<Fragmented_Message>::iterator it = std::find(fragmented_messages_.begin(), fragmented_messages_.end(), msg_id);

            if (full_size >= MAX_MESSAGE_SIZE)
            {
                qCCritical(Log).noquote() << title() << "try to receive too big message:" << full_size << "max:" << MAX_MESSAGE_SIZE;
                fragmented_messages_.erase(it);
                return;
            }

            if (it == fragmented_messages_.end())
            {
                uint32_t max_fragment_size = pos > 0 ? pos : MAX_MESSAGE_DATA_SIZE;
                it = fragmented_messages_.emplace(fragmented_messages_.end(), msg_id, cmd, max_fragment_size, full_size < 1000000);
            }
            Fragmented_Message &msg = *it;

            if (!msg.data_device_->open(QIODevice::ReadWrite))
            {
                qCCritical(Log).noquote() << title() << "Failed open tempriorary device:" << msg.data_device_->errorString();
                fragmented_messages_.erase(it);
                return;
            }

            qCDebug(DetailLog).noquote() << title() << "Fragment msg" << msg_id << "full" << full_size << "pos" << pos << "size" << ds.device()->bytesAvailable();

            if (!ds.atEnd())
            {
                uint32_t data_pos = ds.device()->pos();
                msg.data_device_->seek(pos);
                msg.data_device_->write(data.constData() + data_pos, data.size() - data_pos);
            }

            lost_msg_list_.push_back(std::make_pair(std::chrono::system_clock::now(), msg_id));
            auto msg_out = send(cmd);
            msg_out.msg_.cmd_ |= FRAGMENT_QUERY;
            msg_out << msg_id << static_cast<uint32_t>(msg.data_device_->pos()) << msg.max_fragment_size_;

            if (msg.data_device_->pos() == full_size)
            {
                msg.data_device_->seek(0);
                if (flags & ANSWER)
                {
                    Message_Item waiting_msg = pop_waiting_answer(answer_id, cmd);
                    if (waiting_msg.answer_func_)
                    {
                        waiting_msg.answer_func_(*msg.data_device_);
                    }
                    else
                    {
//                    msg.data_device_->close(); // most process_message call with closed devices
                        process_answer_message(msg_id, cmd, *msg.data_device_);
                    }
                }
                else
                {
                    process_message(msg_id, cmd, *msg.data_device_);
                }
                fragmented_messages_.erase(it);
            }
            else
            {
                msg.data_device_->close();
            }
        }
        else
        {
            Message_Item msg = pop_waiting_answer(answer_id, cmd);
            if (msg.answer_func_)
            {
                data.remove(0, 1);

                QBuffer buffer(&data);
                msg.answer_func_(buffer);
            }
        }
    }
    else if (flags & FRAGMENT_QUERY)
    {
        apply_parse(data, &Protocol::process_fragment_query);
    }
    else
    {
        if (cmd == Cmd::PING)
        {
            send_answer(cmd, msg_id);
        }
        else
        {
            QBuffer buffer(&data);
            process_message(msg_id, cmd, buffer);
        }
    }
}

void Protocol::process_fragment_query(uint8_t fragmanted_msg_id, uint32_t pos, uint32_t fragmanted_size)
{
    Message_Item msg = pop_waiting_message([fragmanted_msg_id](const Message_Item &item){ return item.id_.value_or(0) == fragmanted_msg_id; });
    if (msg.data_device_ && pos < msg.data_device_->size())
    {
        qCDebug(DetailLog).noquote() << title() << "Send fragment msg" << fragmanted_msg_id << "full" << msg.data_device_->size() << "pos" << pos << "size" << fragmanted_size;

        msg.fragment_size_ = fragmanted_size;
        send_message(std::move(msg), pos);
    }
}

void Protocol::process_wait_list()
{
    std::vector<Message_Item> messages = pop_waiting_messages();

    Time_Point now = std::chrono::system_clock::now();
    for (Message_Item& msg: messages)
    {
        if (msg.end_time_ > now)
        {
            uint32_t pos = msg.data_device_ ? msg.data_device_->pos() : 0;
            msg.fragment_size_ /= 2;
            if (msg.fragment_size_ < 32)
            {
                msg.fragment_size_ = 32;
            }
            send_message(std::move(msg), pos);
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

Message_Item Protocol::pop_waiting_answer(uint8_t answer_id, uint16_t cmd)
{
    return pop_waiting_message([answer_id, cmd](const Message_Item &item){ return item.id_.value_or(0) == answer_id && item.cmd_ == cmd; });
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
            return msg;
        }
    }
    return {};
}

} // namespace Network
} // namespace Helpz
