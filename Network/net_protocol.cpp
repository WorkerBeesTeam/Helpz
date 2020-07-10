#include <QDebug>

#include "net_protocol.h"

namespace Helpz {
namespace Net {

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
        return Protocol_Sender(ptr->protocol(), cmd);
    return Protocol_Sender(std::shared_ptr<Protocol>(), cmd);
}

Protocol_Sender Protocol::send_answer(uint8_t cmd, std::optional<uint8_t> msg_id)
{
    auto ptr = writer();
    if (ptr)
        return Protocol_Sender(ptr->protocol(), cmd, msg_id);
    return Protocol_Sender(std::shared_ptr<Protocol>(), cmd, msg_id);
}

void Protocol::send_byte(uint8_t cmd, char byte) { send(cmd) << byte; }
void Protocol::send_array(uint8_t cmd, const QByteArray &buff) { send(cmd) << buff; }

void Protocol::send_message(std::shared_ptr<Message_Item> msg)
{
    if (!msg)
        return;

    if (msg->data_device_)
    {
        auto writer_ptr = writer();
        if (writer_ptr)
            writer_ptr->write(std::move(msg));
        else
            qCWarning(Log).noquote() << title() << "Attempt to send message, but writer is not set. cmd:" << int(msg->cmd());
    }
    else
        qCWarning(Log).noquote() << title() << "Attempt to send message without data device. cmd:" << int(msg->cmd());
}

QByteArray Protocol::prepare_packet_to_send(std::shared_ptr<Message_Item> msg_ptr)
{
    if (!msg_ptr)
        return {};

    Message_Item& msg = *msg_ptr;

    if (!msg.data_device_)
    {
        qCWarning(Log).noquote() << title() << "Attempt to prepare packet without data device. cmd:" << int(msg.cmd());
        return {};
    }

    Time_Point tt = msg.end_time_;

    QByteArray packet, data;
    uint8_t flags = msg.flags();

    if (msg.answer_id_ || msg.data_device_->size() > msg.fragment_size())
    {
        QDataStream ds(&data, QIODevice::WriteOnly);
        ds.setVersion(DATASTREAM_VERSION);

        if (msg.answer_id_)
        {
            flags |= ANSWER;
            ds << *msg.answer_id_;
        }

        if (msg.data_device_->size() > msg.fragment_size())
        {
            flags |= FRAGMENT;
            ds << static_cast<uint32_t>(msg.data_device_->size());

            qCDebug(DetailLog).noquote() << title() << "Send fragment msg" << msg.id_.value_or(0)
                                         << "full" << msg.data_device_->size() << "pos" << msg.data_device_->pos() << "size" << msg.fragment_size();

            ds << static_cast<uint32_t>(msg.data_device_->pos());

            if (msg.data_device_->atEnd())
            {
                ds << msg.fragment_size();
                msg.end_time_ = std::chrono::system_clock::now() + std::chrono::seconds(10);
            }
            else
                add_raw_data_to_packet(data, msg.data_device_->pos(), msg.fragment_size(), msg.data_device_.get());
        }
        else
        {
            add_raw_data_to_packet(data, 0, msg.fragment_size(), msg.data_device_.get());
        }
    }
    else
    {
        add_raw_data_to_packet(data, 0, msg.fragment_size(), msg.data_device_.get());
    }

    if (static_cast<uint32_t>(data.size()) > msg.min_compress_size())
    {
        flags |= COMPRESSED;
        data = qCompress(data);
    }

    if (!msg.id_)
        msg.id_ = next_tx_msg_id_++;

    QDataStream ds(&packet, QIODevice::WriteOnly);
    ds.setVersion(DATASTREAM_VERSION);
    ds << uint16_t(0) << *msg.id_ << msg.cmd() << flags << data;

    ds.device()->seek(0);
    ds << qChecksum(packet.constData() + 2, 7);

    Time_Point now = std::chrono::system_clock::now();
    last_msg_send_time_ = now;

    if (DetailLog().isDebugEnabled())
    {
        auto dbg = qDebug(DetailLog).noquote()
                << title() << "SEND id:" << (int)*msg.id_ << "cmd:" << (int)msg.cmd() << "flags:" << (int)flags << "size:" << data.size() << "wait:" << (msg.end_time_ > now);
        if (tt.time_since_epoch().count())
            dbg << "tt:" << std::chrono::duration_cast<std::chrono::milliseconds>(tt - now).count();

        if (flags & REPEATED) dbg << "REPEATED";
        if (flags & FRAGMENT_QUERY) dbg << "FRAGMENT_QUERY";
        if (flags & FRAGMENT) dbg << "FRAGMENT";
        if (flags & ANSWER) dbg << "ANSWER " << int (*data.constData());
        if (flags & COMPRESSED) dbg << "COMPRESSED";
    }

    if (msg.end_time_ > now)
    {
        // Если время до повторной посылки меньше чем до таймаута, то используем его
        Time_Point time_point = msg.resend_timeout_ < (msg.end_time_ - now) ?
                    now + msg.resend_timeout_ :
                    msg.end_time_;

        int msg_cmd = msg.cmd();
        add_to_waiting(time_point, std::move(msg_ptr));

        auto writer_ptr = writer();
        if (writer_ptr)
            writer_ptr->add_timeout_at(std::move(time_point));
        else
            qCWarning(Log).noquote() << title() << "Prepare packet, but writer is not set. cmd:" << msg_cmd;
    }

    return packet;
}

void Protocol::add_raw_data_to_packet(QByteArray& data, uint32_t pos, uint32_t max_data_size, QIODevice* device)
{
    uint32_t raw_size = std::min<uint32_t>(max_data_size, static_cast<uint32_t>(device->size() - pos));
    uint32_t header_pos = data.size();
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

    packet_end_position_.push(size);
    device_.seek(device_.size());
    device_.write(reinterpret_cast<const char*>(data), size);
    device_.seek(0);

    bool is_first_call = true;
    while (!process_stream(is_first_call) && !packet_end_position_.empty())
    {
        /* Функция process_stream возвращает false только если чек-сумма используется и не совпала.
         * И если она не совпала и в текущем device_ хранится несколько пакетов,
         * то нужно удалить из него первый пакет и попробовать заново.
         */

        // Отбрасываем удачно обработанные пакеты
        std::size_t pos = device_.pos();
        while(!packet_end_position_.empty() && packet_end_position_.front() <= pos)
        {
            pos -= packet_end_position_.front();
            packet_end_position_.pop();
        }

        // Отбрасываем не удачно обработанный пакет
        device_.seek(0);
        device_.buffer().remove(0, static_cast<int>(packet_end_position_.front()));
        packet_end_position_.pop();

        if (is_first_call)
            is_first_call = false;
    }

    std::size_t pos = device_.pos();
    while(!packet_end_position_.empty() && packet_end_position_.front() <= pos)
    {
        pos -= packet_end_position_.front();
        packet_end_position_.pop();
    }

    if (pos && !packet_end_position_.empty())
        packet_end_position_.front() -= pos;

    if (device_.bytesAvailable())
        device_.buffer().remove(0, static_cast<int>(device_.pos()));
    else
        device_.buffer().clear();
}

bool Protocol::process_stream(bool is_first_call)
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

        if (DetailLog().isDebugEnabled())
        {
            auto dbg = qDebug(DetailLog).noquote() << title();
            if (!is_first_call)
                dbg << "Research";
            dbg << "RECV id:" << (int)msg_id << "cmd:" << (int)cmd << "flags:" << (int)flags << "size:" << buffer_size << "ok:" << checksum_ok
                    << "avail:" << device_.bytesAvailable() << packet_end_position_.size();

            if (flags & REPEATED) dbg << "REPEATED";
            if (flags & FRAGMENT_QUERY) dbg << "FRAGMENT_QUERY";
            if (flags & FRAGMENT) dbg << "FRAGMENT";
            if (flags & ANSWER) dbg << "ANSWER " << int (*(device_.buffer().constData() + pos + 9));
            if (flags & COMPRESSED) dbg << "COMPRESSED";
        }

        if (buffer_size == 0xffffffff)
            buffer_size = 0;

        if (buffer_size > HELPZ_PROTOCOL_MAX_MESSAGE_SIZE)
        {
            if (is_first_call)
                qCWarning(Log).noquote()
                   << title() << "Message size" << buffer_size << "more then" << HELPZ_PROTOCOL_MAX_MESSAGE_SIZE
                   << "Checksum in packet:" << qChecksum(device_.buffer().constData() + pos + 2, 7)
                   << "expected:" << checksum;

            device_.seek(device_.size());
            return true;
        }
        else if (!checksum_ok) // Drop message if checksum bad or too high size
        {
            device_.seek(pos);

            if (is_first_call)
            {
                qCWarning(Log).noquote() << title() << "Message corrupt, checksum isn't same."
                               << "In packet:" << qChecksum(device_.buffer().constData() + pos + 2, 7)
                               << "expected:" << checksum;
            }
            return false;
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
            return false;
        }
        catch(...)
        {
            qCCritical(Log).noquote() << title() << "EXCEPTION Unknown: process_stream" << int(cmd);
            return false;
        }

        device_.seek(pos + 9 + buffer_size);
    }

    return true;
}

bool Protocol::is_lost_message(uint8_t msg_id)
{
    auto it = lost_msg_list_.find(msg_id);
    if (it == lost_msg_list_.end())
        return false;

    const Time_Point tp = it->second;
    lost_msg_list_.erase(it);

    return std::chrono::system_clock::now() - tp < std::chrono::seconds(10);
}

void Protocol::fill_lost_msg(uint8_t msg_id)
{
    const Time_Point now = std::chrono::system_clock::now();

    const Time_Point max_tp = now - std::chrono::seconds(10);
    const uint32_t min_id = std::numeric_limits<uint8_t>::max() + static_cast<uint8_t>(msg_id - 100);
    const uint32_t max_id = std::numeric_limits<uint8_t>::max() + msg_id;

    for (auto it = lost_msg_list_.begin(); it != lost_msg_list_.end(); )
    {
        const uint32_t tmp_id = std::numeric_limits<uint8_t>::max() + it->first;

        if (max_tp > it->second
            || (min_id > tmp_id && tmp_id > max_id))
        {
            const bool is_fragmented = fragmented_messages_.erase(it->first) > 0;

            if (DetailLog().isDebugEnabled())
            {
                auto dbg = qDebug(DetailLog).noquote() << title() << "Message" << it->first << "is lost";
                if (max_tp > it->second)
                    dbg << "time:" << std::chrono::duration_cast<std::chrono::milliseconds>(max_tp - it->second).count();
                else
                    dbg << "min_id" << min_id << "tmp_id" << tmp_id << "max_id" << max_id;

                if (is_fragmented)
                    dbg << " is_fragmented";
            }

            it = lost_msg_list_.erase(it);
        }
        else
            ++it;
    }

    if (msg_id > (next_rx_msg_id_ + 100))
        next_rx_msg_id_ = static_cast<uint8_t>(msg_id - 100);

    while (msg_id != next_rx_msg_id_)
        lost_msg_list_.emplace(next_rx_msg_id_++, now);
}

void Protocol::internal_process_message(uint8_t msg_id, uint8_t cmd, uint8_t flags, const char *data_ptr, uint32_t data_size)
{
    if (flags & REPEATED
        || (msg_id < next_rx_msg_id_
            && (next_rx_msg_id_ - msg_id) < 100))
    {
        if (is_lost_message(msg_id))
            qCDebug(DetailLog).noquote() << title() << "Find lost message" << msg_id;
        else if (flags & REPEATED)
        {
            if (DetailLog().isDebugEnabled())
            {
                auto dbg = qDebug(DetailLog).noquote() << title() << "Dropped message" << msg_id << "expected" << next_rx_msg_id_
                                                       << "Cmd:" << cmd << "Flags:" << int(flags) << "Size:" << data_size;
                if (flags & REPEATED) dbg << "REPEATED";
                if (flags & FRAGMENT_QUERY) dbg << "FRAGMENT_QUERY";
                if (flags & FRAGMENT) dbg << "FRAGMENT";
                if (flags & ANSWER) dbg << "ANSWER";
                if (flags & COMPRESSED) dbg << "COMPRESSED";
            }
            return;
        }
    }
    else
    {
        if (msg_id > next_rx_msg_id_
            || (next_rx_msg_id_ - msg_id) > 100)
        {
            lost_msg_detected(msg_id, next_rx_msg_id_);

            qCDebug(DetailLog).noquote() << title() << "Packet is lost from" << next_rx_msg_id_ << ". Receive message:" << msg_id;

            fill_lost_msg(msg_id);
        }

        if ((msg_id + 1) >= (next_rx_msg_id_ + 1))
            next_rx_msg_id_ = static_cast<uint8_t>(msg_id + 1);
    }

    // If COMPRESSED or FRAGMENT flag is setted, then data_size can not be zero.
    if (!data_size && (flags & (COMPRESSED | FRAGMENT)))
    {
        qCWarning(Log).noquote() << title() << "COMPRESSED or FRAGMENT flag is setted, but data_size is zero.";
        return;
    }

    QByteArray data;
    if (flags & COMPRESSED)
    {
        data = qUncompress(reinterpret_cast<const uchar*>(data_ptr), data_size);
        if (data.isEmpty())
            throw std::runtime_error("Failed uncompress");
    }
    else
        data.setRawData(data_ptr, data_size);

    if (cmd == Cmd::REMOVE_FRAGMENT)
    {
        uint8_t fragment_id;
        Helpz::parse_out(DATASTREAM_VERSION, data, fragment_id);
        fragmented_messages_.erase(fragment_id);
    }
    else if (flags & FRAGMENT_QUERY)
    {
        apply_parse(data, &Protocol::process_fragment_query);
    }
    else if (flags & (FRAGMENT | ANSWER))
    {
        QDataStream ds(&data, QIODevice::ReadOnly);
        ds.setVersion(DATASTREAM_VERSION);

        uint8_t answer_id;
        if (flags & ANSWER)
            Helpz::parse_out(ds, answer_id);

        if (flags & FRAGMENT)
        {
            uint32_t full_size, pos;
            Helpz::parse_out(ds, full_size, pos);

            uint32_t max_fragment_size = full_size == pos ? Helpz::parse<uint32_t>(ds) : 0;

            std::map<uint8_t, Fragmented_Message>::iterator it = fragmented_messages_.find(msg_id);

            if (full_size >= HELPZ_PROTOCOL_MAX_MESSAGE_SIZE)
            {
                qCCritical(Log).noquote() << title() << "try to receive too big message:" << full_size << "max:" << HELPZ_PROTOCOL_MAX_MESSAGE_SIZE;
                if (fragmented_messages_.end() != it)
                    fragmented_messages_.erase(it);
                return;
            }

            if (it == fragmented_messages_.end())
            {
                if (max_fragment_size == 0 || max_fragment_size > HELPZ_MAX_PACKET_DATA_SIZE)
                    max_fragment_size = HELPZ_MAX_MESSAGE_DATA_SIZE;

                Fragmented_Message msg{msg_id, cmd, max_fragment_size, full_size};
                it = fragmented_messages_.emplace(msg_id, std::move(msg)).first;
            }
            Fragmented_Message &msg = it->second;

            if (!msg.data_device_->open(QIODevice::ReadWrite))
            {
                qCCritical(Log).noquote() << title() << "Failed open tempriorary device:" << msg.data_device_->errorString();
                fragmented_messages_.erase(it);
                return;
            }

            qCDebug(DetailLog).noquote() << title() << "Fragment msg" << msg_id << "full" << full_size << "pos" << pos << "size" << ds.device()->bytesAvailable();

            if (!ds.atEnd())
            {
                uint32_t data_pos = static_cast<uint32_t>(ds.device()->pos());
                msg.add_data(pos, data.constData() + data_pos, data.size() - data_pos);
            }

            auto msg_out = send(cmd);
            msg_out.msg_.set_flags(msg_out.msg_.flags() | FRAGMENT_QUERY, Message_Item::Only_Protocol());
            msg_out << msg_id;

            if (msg.is_parts_empty())
            {
                msg_out << full_size << msg.max_fragment_size_;

                msg.data_device_->seek(0);

                if (flags & ANSWER)
                {
                    std::shared_ptr<Message_Item> waiting_msg = pop_waiting_answer(answer_id, cmd);
                    if (waiting_msg && waiting_msg->answer_func_)
                    {
                        waiting_msg->answer_func_(*msg.data_device_);
                        waiting_msg->answer_func_ = nullptr;
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
                auto emp_it = lost_msg_list_.emplace(msg_id, now);
                if (!emp_it.second)
                    emp_it.first->second = now;
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
            std::shared_ptr<Message_Item> msg = pop_waiting_answer(answer_id, cmd);
            if (msg && msg->answer_func_)
            {
                data.remove(0, 1);

                QBuffer buffer(&data);
                msg->answer_func_(buffer);
                msg->answer_func_ = nullptr;
            }
        }
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
    std::shared_ptr<Message_Item> msg = pop_waiting_fragment(fragmanted_msg_id);
    if (msg && msg->data_device_ && pos < msg->data_device_->size())
    {
        qCDebug(DetailLog).noquote() << title() << "Process fragment query msg" << fragmanted_msg_id << "full" << msg->data_device_->size() << "pos" << pos << "size" << fragmanted_size;

        msg->set_fragment_size(fragmanted_size);
        msg->data_device_->seek(pos);
        send_message(std::move(msg));
    }
    else
    {
        if (msg && msg->answer_func_)
            add_to_waiting(msg->end_time_, msg);

        qCDebug(DetailLog).noquote() << title() << "Send remove unknown fragment" << fragmanted_msg_id;
        send(Cmd::REMOVE_FRAGMENT) << fragmanted_msg_id;
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

            for (auto& it: fragmented_messages_)
            {
                Fragmented_Message& msg = it.second;
                if (!msg.is_parts_empty()
                    && now - msg.last_part_time_ >= std::chrono::milliseconds(1500))
                {
                    msg.max_fragment_size_ /= 2;
                    if (msg.max_fragment_size_ < 32)
                        msg.max_fragment_size_ = 32;

                    lost_msg_list_.emplace(msg.id_, now);
                    msg.last_part_time_ = now;

                    const QPair<uint32_t, uint32_t> next_part = msg.get_next_part();

                    auto msg_out = send(msg.cmd_);
                    msg_out.msg_.set_flags(msg_out.msg_.flags() | FRAGMENT_QUERY, Message_Item::Only_Protocol());
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

    std::vector<std::shared_ptr<Message_Item>> messages = pop_waiting_messages();

    Time_Point now = std::chrono::system_clock::now();
    for (std::shared_ptr<Message_Item>& msg: messages)
    {
        if (!msg)
            continue;

        if (msg->end_time_ > now && msg->data_device_)
        {
            msg->set_fragment_size(msg->fragment_size() / 2);
            msg->set_flags(msg->flags() | REPEATED, Message_Item::Only_Protocol());
            send_message(std::move(msg));
        }
        else
        {
            qCDebug(DetailLog).noquote() << title() << "Message timeout. msg" << msg->id_.value_or(0);

            if (msg->timeout_func_)
                msg->timeout_func_();
        }
    }
}

void Protocol::add_to_waiting(Time_Point time_point, std::shared_ptr<Message_Item> message)
{
    std::lock_guard lock(mutex_);

    uint8_t msg_id = message->id_.value_or(0);
    pop_waiting_message([msg_id](const Message_Item &item) { return item.id_.value_or(0) == msg_id; });

    waiting_messages_.emplace(std::move(time_point), std::move(message));
}

std::vector<std::shared_ptr<Message_Item>> Protocol::pop_waiting_messages()
{
    std::vector<std::shared_ptr<Message_Item>> messages;
    std::lock_guard lock(mutex_);

    Time_Point now = std::chrono::system_clock::now() + std::chrono::milliseconds(20);
    for (auto it = waiting_messages_.begin(); it != waiting_messages_.end(); )
    {
        if (it->first > now)
            break;

        messages.push_back(std::move(it->second));
        it = waiting_messages_.erase(it);
    }
    return messages;
}

std::shared_ptr<Message_Item> Protocol::pop_waiting_answer(uint8_t answer_id, uint8_t cmd)
{
    std::lock_guard lock(mutex_);
    return pop_waiting_message([answer_id, cmd](const Message_Item &item){ return item.id_.value_or(0) == answer_id && item.cmd() == cmd; });
}

std::shared_ptr<Message_Item> Protocol::pop_waiting_fragment(uint8_t fragmanted_msg_id)
{
    std::lock_guard lock(mutex_);
    return pop_waiting_message([fragmanted_msg_id](const Message_Item &item){ return item.id_.value_or(0) == fragmanted_msg_id; });
}

std::shared_ptr<Message_Item> Protocol::pop_waiting_message(std::function<bool (const Message_Item &)> check_func)
{
    for (auto it = waiting_messages_.begin(); it != waiting_messages_.end(); ++it)
    {
        if (check_func(*it->second))
        {
            std::shared_ptr<Message_Item> msg = std::move(it->second);
            waiting_messages_.erase(it);
            return msg;
        }
    }
    return {};
}

} // namespace Net
} // namespace Helpz
