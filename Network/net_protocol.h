#ifndef HELPZ_NETWORK_PROTOCOL_H
#define HELPZ_NETWORK_PROTOCOL_H

#include <chrono>
#include <mutex>
#include <queue>
#include <atomic>

#include <QBuffer>
#include <QLoggingCategory>

#include <Helpz/apply_parse.h>
#include <Helpz/net_protocol_writer.h>
#include <Helpz/net_protocol_sender.h>
#include <Helpz/net_fragmented_message.h>

namespace Helpz {
namespace Net {

Q_DECLARE_LOGGING_CATEGORY(Log)
Q_DECLARE_LOGGING_CATEGORY(DetailLog)

namespace Cmd {
enum ReservedCommands {
    ZERO = 0,
    PING,
    REMOVE_FRAGMENT,
    CLOSE,

    USER_COMMAND = 16
};
}

/**
 * @brief The Protocol class
 *
 * Protocol structure:
 * [2bytes Checksum][2bytes [13bits cmd][3bits flags]][4bytes data][Nbytes... data]
 */
class Protocol
{
public:

    enum { DATASTREAM_VERSION = QDataStream::Qt_5_6 };

    enum Flags {
        RESERVED                    = 0x04,

        REPEATED                    = 0x08,
        FRAGMENT_QUERY              = 0x10,
        FRAGMENT                    = 0x20,
        ANSWER                      = 0x40,
        COMPRESSED                  = 0x80,

        FLAGS_ALL                   = RESERVED | REPEATED | FRAGMENT_QUERY | FRAGMENT | ANSWER | COMPRESSED
    };

    template<typename... Args>
    void parse_out(QIODevice& data_dev, Args&... args)
    {
        std::unique_ptr<QDataStream> ds = parse_open_device(data_dev, DATASTREAM_VERSION);
        Helpz::parse_out(*ds, args...);
    }

    template<typename... Args>
    void parse_out(const QByteArray& data, Args&... args)
    {
        QDataStream ds(data);
        ds.setVersion(DATASTREAM_VERSION);
        Helpz::parse_out(ds, args...);
    }

    template<typename RetType, class T, typename... FArgs, typename... Args>
    RetType apply_parse(QDataStream &ds, RetType(T::*__f)(FArgs...), Args&&... args)
    {
        return apply_parse_impl<RetType, decltype(__f), T, FArgs...>(ds, __f, static_cast<T*>(this), std::forward<Args&&>(args)...);
    }

    template<typename RetType, class T, typename... FArgs, typename... Args>
    RetType apply_parse(QDataStream &ds, RetType(T::*__f)(FArgs...) const, Args&&... args) const
    {
        return apply_parse_impl<RetType, decltype(__f), T, FArgs...>(ds, __f, static_cast<T*>(this), std::forward<Args&&>(args)...);
    }

    template<typename RetType, class T, typename... FArgs, typename... Args>
    RetType apply_parse(QIODevice& data_dev, RetType(T::*__f)(FArgs...) const, Args&&... args)
    {
        std::unique_ptr<QDataStream> ds = parse_open_device(data_dev, DATASTREAM_VERSION);
        return apply_parse_impl<RetType, decltype(__f), T, FArgs...>(*ds, __f, static_cast<T*>(this), std::forward<Args&&>(args)...);
    }

    template<typename RetType, class T, typename... FArgs, typename... Args>
    RetType apply_parse(QIODevice& data_dev, RetType(T::*__f)(FArgs...), Args&&... args)
    {
        std::unique_ptr<QDataStream> ds = parse_open_device(data_dev, DATASTREAM_VERSION);
        return apply_parse_impl<RetType, decltype(__f), T, FArgs...>(*ds, __f, static_cast<T*>(this), std::forward<Args&&>(args)...);
    }

    template<typename RetType, class T, typename... FArgs, typename... Args>
    RetType apply_parse(const QByteArray& data, RetType(T::*__f)(FArgs...) const, Args&&... args)
    {
        QDataStream ds(data);
        ds.setVersion(DATASTREAM_VERSION);
        return apply_parse_impl<RetType, decltype(__f), T, FArgs...>(ds, __f, static_cast<T*>(this), std::forward<Args&&>(args)...);
    }

    template<typename RetType, class T, typename... FArgs, typename... Args>
    RetType apply_parse(const QByteArray& data, RetType(T::*__f)(FArgs...), Args&&... args)
    {
        QDataStream ds(data);
        ds.setVersion(DATASTREAM_VERSION);
        return apply_parse_impl<RetType, decltype(__f), T, FArgs...>(ds, __f, static_cast<T*>(this), std::forward<Args&&>(args)...);
    }

public:
    Protocol();
    virtual ~Protocol() = default;

    virtual void before_remove_copy() {}

    QString title() const;

    void reset_msg_id();

    virtual bool operator ==(const Protocol&) const { return false; }

    std::shared_ptr<Protocol_Writer> writer();
    std::shared_ptr<const Protocol_Writer> writer() const;
    void set_writer(std::shared_ptr<Protocol_Writer> protocol_writer);

    typedef std::chrono::time_point<std::chrono::system_clock> Time_Point;

    Time_Point last_msg_send_time() const;

    Protocol_Sender send(uint8_t cmd);
    Protocol_Sender send_answer(uint8_t cmd, std::optional<uint8_t> msg_id);
    void send_byte(uint8_t cmd, char byte);
    void send_array(uint8_t cmd, const QByteArray &buff);
    void send_message(std::shared_ptr<Message_Item> msg);

public:
    QByteArray prepare_packet_to_send(std::shared_ptr<Message_Item> msg_ptr);
    void add_raw_data_to_packet(QByteArray& data, uint32_t pos, uint32_t max_data_size, QIODevice* device);
    void process_bytes(const uint8_t* data, size_t size);

    /**
     * @brief ready_write
     * Called when a connection is established.
     */
    virtual void ready_write() {}
    virtual void closed() {}
    virtual void lost_msg_detected(uint8_t /*msg_id*/, uint8_t /*expected*/) {}
protected:

    virtual void process_message(uint8_t msg_id, uint8_t cmd, QIODevice& data_dev) = 0;
    virtual void process_answer_message(uint8_t msg_id, uint8_t cmd, QIODevice& data_dev) = 0;

//    friend class Protocol_Sender;
private:
    bool process_stream(bool is_first_call);
    bool is_lost_message(uint8_t msg_id);
    void fill_lost_msg(uint8_t msg_id);
    void internal_process_message(uint8_t msg_id, uint8_t cmd, uint8_t flags, const char* data_ptr, uint32_t data_size);
    void process_fragment_query(uint8_t fragmanted_msg_id, uint32_t pos, uint32_t fragmanted_size);

public:
    void process_wait_list(void* data);

private:
    void add_to_waiting(Time_Point time_point, std::shared_ptr<Message_Item> message);
    std::vector<std::shared_ptr<Message_Item>> pop_waiting_messages();
    std::shared_ptr<Message_Item> pop_waiting_answer(uint8_t answer_id, uint8_t cmd);
    std::shared_ptr<Message_Item> pop_waiting_fragment(uint8_t fragmanted_msg_id);
    std::shared_ptr<Message_Item> pop_waiting_message(std::function<bool(const Message_Item&)> check_func);

    std::atomic<uint8_t> next_rx_msg_id_, next_tx_msg_id_;
    std::map<uint8_t, Time_Point> lost_msg_list_;

    QBuffer device_;
    QDataStream msg_stream_;

    std::atomic<Time_Point> last_msg_send_time_;

    std::map<uint8_t, Fragmented_Message> fragmented_messages_;

    std::map<Time_Point, std::shared_ptr<Message_Item>> waiting_messages_;
    mutable std::recursive_mutex mutex_;

    std::shared_ptr<Protocol_Writer> protocol_writer_;
    std::queue<std::size_t> packet_end_position_;
};

} // namespace Net
} // namespace Helpz

#endif // HELPZ_NETWORK_PROTOCOL_H
