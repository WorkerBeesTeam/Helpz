#include <QDebug>

#include "net_protocol.h"

namespace Helpz {
namespace Network {

Q_LOGGING_CATEGORY(Log, "net")
Q_LOGGING_CATEGORY(DetailLog, "net.detail", QtInfoMsg)

Protocol::Protocol() :
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

void Protocol::reset_msg_id()
{
    next_rx_msg_id_ = 0;
    next_tx_msg_id_ = 0;
}

std::shared_ptr<Protocol_Writer> Protocol::writer()
{
    return protocol_writer_;
}

std::shared_ptr<const Protocol_Writer> Protocol::writer() const
{
    return protocol_writer_;
}

void Protocol::set_writer(std::shared_ptr<Protocol_Writer> protocol_writer)
{
    protocol_writer_ = std::move(protocol_writer);
}

Protocol::Time_Point Protocol::last_msg_send_time() const
{
    return last_msg_send_time_.load();
}

Protocol_Sender Protocol::send(uint8_t cmd)
{
    auto ptr = writer();
    if (ptr)
    {
        return Protocol_Sender(ptr->protocol(), cmd);
    }
    return Protocol_Sender(std::shared_ptr<Protocol>(), cmd);
}

Protocol_Sender Protocol::send_answer(uint8_t cmd, std::optional<uint8_t> msg_id)
{
    auto ptr = writer();
    if (ptr)
    {
        return Protocol_Sender(ptr->protocol(), cmd, msg_id);
    }
    return Protocol_Sender(std::shared_ptr<Protocol>(), cmd, msg_id);
}

void Protocol::send_byte(uint8_t cmd, char byte) { send(cmd) << byte; }
void Protocol::send_array(uint8_t cmd, const QByteArray &buff) { send(cmd) << buff; }

void Protocol::send_message(Message_Item msg)
{
    if (msg.data_device_)
    {
        auto writer_ptr = writer();
        if (writer_ptr)
            writer_ptr->write(std::move(msg));
        else
            qCWarning(Log).noquote() << title() << "Attempt to send message, but writer is not set. cmd:" << int(msg.cmd_);
    }
    else
        qCWarning(Log).noquote() << title() << "Attempt to send message without data device. cmd:" << int(msg.cmd_);
}

QByteArray Protocol::prepare_packet_to_send(Message_Item&& msg)
{
    if (!msg.data_device_)
    {
        qCWarning(Log).noquote() << title() << "Attempt to prepare packet without data device. cmd:" << int(msg.cmd_);
        return {};
    }

    QByteArray packet, data;
    uint8_t flags = msg.flags_;

    if (msg.answer_id_ || msg.data_device_->size() > msg.fragment_size_)
    {
        QDataStream ds(&data, QIODevice::WriteOnly);
        ds.setVersion(DATASTREAM_VERSION);

        if (msg.answer_id_)
        {
            flags |= ANSWER;
            ds << *msg.answer_id_;
        }

        if (msg.data_device_->size() > msg.fragment_size_)
        {
            flags |= FRAGMENT;
            ds << static_cast<uint32_t>(msg.data_device_->size());

            qCDebug(DetailLog).noquote() << title() << "Send fragment msg" << msg.id_.value_or(0)
                                         << "full" << msg.data_device_->size() << "pos" << msg.data_device_->pos() << "size" << msg.fragment_size_;

            if (msg.data_device_->atEnd())
            {
                ds << msg.fragment_size_;

                msg.end_time_ = std::chrono::system_clock::now() + std::chrono::minutes(3);
            }
            else
            {
                ds << static_cast<uint32_t>(msg.data_device_->pos());
                add_raw_data_to_packet(data, msg.data_device_->pos(), msg.fragment_size_, msg.data_device_.get());
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
        flags |= COMPRESSED;
        data = qCompress(data);
    }

    if (!msg.id_)
        msg.id_ = next_tx_msg_id_++;

    QDataStream ds(&packet, QIODevice::WriteOnly);
    ds.setVersion(DATASTREAM_VERSION);
    ds << uint16_t(0) << *msg.id_ << msg.cmd_ << flags << data;

    ds.device()->seek(0);
    ds << qChecksum(packet.constData() + 2, 7);

    Time_Point now = std::chrono::system_clock::now();
    last_msg_send_time_ = now;

    if (msg.end_time_ > now)
    {
        // Если время до повторной посылки меньше чем до таймаута, то используем его
        Time_Point time_point = msg.resend_timeout_ < (msg.end_time_ - now) ?
                    now + msg.resend_timeout_ :
                    msg.end_time_;

        add_to_waiting(time_point, std::move(msg));

        auto writer_ptr = writer();
        if (writer_ptr)
            writer_ptr->add_timeout_at(std::move(time_point));
        else
            qCWarning(Log).noquote() << title() << "Prepare packet, but writer is not set. cmd:" << int(msg.cmd_);
    }

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

void Protocol::process_bytes(const uint8_t* data, size_t size)
{
    if (size == 0)
        return;

    {
        auto writer_ptr = writer();
        if (writer_ptr)
            writer_ptr->set_last_msg_recv_time(std::chrono::system_clock::now());
    }

    if (device_.size() != 0)
        qDebug(Log) << "POSIBLE FRAGMENTED TLS BECOSE ALREADY SIZE:" << device_.size() << "AND NEW PACKET:" << size;

//    packet_end_position_.push(size);
    device_.seek(device_.size());
    device_.write(reinterpret_cast<const char*>(data), size);
    device_.seek(0);
    while (!process_stream() /* && !packet_end_position_.empty() */)
    {
        /* Функция process_stream возвращает false только если чек-сумма используется и не совпала.
         * И если она не совпала и в текущем device_ хранится несколько пакетов,
         * то нужно удалить из него первый пакет и попробовать заново.
         */
        break;
    }

    if (device_.bytesAvailable())
    {
        device_.buffer().remove(0, device_.pos());
    }
    else
        device_.buffer().clear();
}

bool Protocol::process_stream()
{
    /* TODO:
     * Нужно обрабатывать входящие пакеты отдельно, а не складывать их.
     * Т.е. если сообщение состоит из двух пакетов, пришел первый, второй потерялся,
     * а потом приходит первый пакет нового сообщения, то новое сообщение не будет обработано.
     * Нужно хранить все пакеты отдельно или хотя бы позиции начала пакета, что бы если первое сообщение неудачное,
     * то мы удаляем первый пакет, и пробуем заново.
     *
     * Но если протокол использует шифрование то это не актуально.
     */

    /* TODO:
     * Нужно сделать чек-сумму на заголовки однобайтную,
     * и добавить двух или трёх-байтную чек-сумму на тело сообщения
     * если стоит флаг "Использовать чек-сумму".
     */

    bool checksum_ok;
    uint8_t msg_id, cmd, flags;
    uint16_t checksum;
    uint32_t buffer_size;
    uint64_t pos;

    while (device_.bytesAvailable() >= 9)
    {
        pos = device_.pos();

        // Using QDataStream for auto swap bytes for big or little endian
        msg_stream_ >> checksum >> msg_id >> cmd >> flags >> buffer_size;
        checksum_ok = checksum == qChecksum(device_.buffer().constData() + pos + 2, 7);

        if (buffer_size == 0xffffffff)
        {
            buffer_size = 0;
        }

        if (!checksum_ok || buffer_size > HELPZ_PROTOCOL_MAX_MESSAGE_SIZE) // Drop message if checksum bad or too high size
        {
            if (!device_.atEnd())
            {
                device_.seek(device_.size());
            }
            if (!checksum_ok)
            {
                qCWarning(Log) << "Message corrupt, checksum isn't same."
                               << "In packet:" << qChecksum(device_.buffer().constData() + pos + 2, 7)
                               << "expected:" << checksum;
            }
            return checksum_ok;
        }
        else if (device_.bytesAvailable() < buffer_size) // else if all ok, but not enough bytes
        {
            device_.seek(pos);
            return true; // Wait more bytes
        }

        try
        {
            internal_process_message(msg_id, cmd, flags, device_.buffer().constData() + pos + 9, buffer_size);
        }
        catch(const std::exception& e)
        {
            qCCritical(Log).noquote() << title() << "EXCEPTION: process_stream" << int(cmd) << e.what();
        }
        catch(...)
        {
            qCCritical(Log).noquote() << title() << "EXCEPTION Unknown: process_stream" << int(cmd);
        }

        device_.seek(pos + 9 + buffer_size);
    }

    return true;
}

bool Protocol::is_lost_message(uint8_t msg_id)
{
    bool finded = false;
    for (auto it = lost_msg_list_.begin(); it != lost_msg_list_.end(); )
    {
        if (it->second == msg_id)
        {
            qCDebug(DetailLog).noquote() << title() << "Find lost message" << msg_id;
            it = lost_msg_list_.erase(it);
            finded = true;
        }
        else
            ++it;
    }
    return finded;
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

void Protocol::internal_process_message(uint8_t msg_id, uint8_t cmd, uint8_t flags, const char *data_ptr, uint32_t data_size)
{
    if (flags & REPEATED || (msg_id < next_rx_msg_id_ && uint8_t(next_rx_msg_id_ - msg_id) < std::numeric_limits<int8_t>::max()))
    {
        if (!is_lost_message(msg_id))
        {
            qCDebug(DetailLog).noquote() << title() << "Dropped message" << msg_id << "expected" << next_rx_msg_id_ << ". Cmd:" << int(cmd) << "Flags:" << int(flags) << "Size:" << data_size;
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
        qCWarning(Log).noquote() << title() << "COMPRESSED or FRAGMENT flag is setted, but data_size is zero.";
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

            if (full_size >= HELPZ_PROTOCOL_MAX_MESSAGE_SIZE)
            {
                qCCritical(Log).noquote() << title() << "try to receive too big message:" << full_size << "max:" << HELPZ_PROTOCOL_MAX_MESSAGE_SIZE;
                fragmented_messages_.erase(it);
                return;
            }

            if (it == fragmented_messages_.end())
            {
                uint32_t max_fragment_size = pos > 0 ? pos : HELPZ_MAX_MESSAGE_DATA_SIZE;
                it = fragmented_messages_.emplace(fragmented_messages_.end(), msg_id, cmd, max_fragment_size, full_size);
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
                msg.add_data(pos, data.constData() + data_pos, data.size() - data_pos);
            }

            auto msg_out = send(cmd);
            msg_out.msg_.flags_ |= FRAGMENT_QUERY;
            msg_out << msg_id;

            if (msg.is_parts_empty())
            {
                msg_out << full_size << msg.max_fragment_size_;

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

                Time_Point now = std::chrono::system_clock::now();
                lost_msg_list_.push_back(std::make_pair(now, msg_id));
                msg.last_part_time_ = now;

                const QPair<uint32_t, uint32_t> next_part = msg.get_next_part();
                msg_out << next_part;

                auto writer_ptr = writer();
                if (writer_ptr)
                {
                    intptr_t value = FRAGMENT;
                    writer_ptr->add_timeout_at(now + std::chrono::milliseconds(1505), reinterpret_cast<void*>(value));
                }

                qCDebug(DetailLog).noquote() << title() << "Send fragment query msg" << msg_id
                                             << "full" << full_size << "part" << next_part;
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
        msg.data_device_->seek(pos);
        send_message(std::move(msg));
    }
}

void Protocol::process_wait_list(void *data)
{
    if (data)
    {
        intptr_t value = reinterpret_cast<intptr_t>(data);
        if (value == FRAGMENT)
        {
            Time_Point now = std::chrono::system_clock::now();
            auto writer_ptr = writer();

            for (Fragmented_Message& msg: fragmented_messages_)
            {
                if (!msg.is_parts_empty()
                    && now - msg.last_part_time_ >= std::chrono::milliseconds(1500))
                {
                    msg.max_fragment_size_ /= 2;
                    if (msg.max_fragment_size_ < 32)
                        msg.max_fragment_size_ = 32;

                    lost_msg_list_.push_back(std::make_pair(now, msg.id_));
                    msg.last_part_time_ = now;

                    const QPair<uint32_t, uint32_t> next_part = msg.get_next_part();

                    auto msg_out = send(msg.cmd_);
                    msg_out.msg_.cmd_ |= FRAGMENT_QUERY;
                    msg_out << msg.id_ << next_part;

                    if (writer_ptr)
                    {
                        intptr_t value = FRAGMENT;
                        writer_ptr->add_timeout_at(now + std::chrono::milliseconds(1505), reinterpret_cast<void*>(value));
                    }

                    qCDebug(DetailLog).noquote() << title() << "Send fragment query msg" << msg.id_ << "part" << next_part;
                }
            }
            return;
        }
    }

    std::vector<Message_Item> messages = pop_waiting_messages();

    Time_Point now = std::chrono::system_clock::now();
    for (Message_Item& msg: messages)
    {
        if (msg.end_time_ > now && msg.data_device_)
        {
            msg.fragment_size_ /= 2;
            if (msg.fragment_size_ < 32)
            {
                msg.fragment_size_ = 32;
            }
            msg.flags_ |= REPEATED;
            send_message(std::move(msg));
        }
        else
        {
            qCDebug(DetailLog).noquote() << title() << "Message timeout. msg" << msg.id_.value_or(0);

            if (msg.timeout_func_)
                msg.timeout_func_();
        }
    }
}

void Protocol::add_to_waiting(Time_Point time_point, Message_Item &&message)
{
    std::lock_guard lock(mutex_);
    waiting_messages_.emplace(std::move(time_point), std::move(message));
}

std::vector<Message_Item> Protocol::pop_waiting_messages()
{
    std::vector<Message_Item> messages;
    std::lock_guard lock(mutex_);

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

Message_Item Protocol::pop_waiting_answer(uint8_t answer_id, uint8_t cmd)
{
    return pop_waiting_message([answer_id, cmd](const Message_Item &item){ return item.id_.value_or(0) == answer_id && item.cmd_ == cmd; });
}

Message_Item Protocol::pop_waiting_message(std::function<bool (const Message_Item &)> check_func)
{
    std::lock_guard lock(mutex_);
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
