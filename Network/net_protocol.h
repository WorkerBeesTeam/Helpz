#ifndef HELPZ_NETWORK_PROTOCOL_H
#define HELPZ_NETWORK_PROTOCOL_H

#include <chrono>
#include <mutex>

#include <Helpz/apply_parse.h>
#include <Helpz/net_protocol_sender.h>

#include <QBuffer>
#include <QLoggingCategory>

namespace Helpz {
namespace Network {

Q_DECLARE_LOGGING_CATEGORY(Log)
Q_DECLARE_LOGGING_CATEGORY(DetailLog)

namespace Cmd {
enum ReservedCommands {
    Zero = 0,
    Ping,

    UserCommand = 16
};
}

class Protocol;

class Protocol_Writer
{
public:
    const std::string& title() const;
    void set_title(const std::string& title);

    std::chrono::time_point<std::chrono::system_clock> last_msg_recv_time() const;
    void set_last_msg_recv_time(std::chrono::time_point<std::chrono::system_clock> value);

    virtual void write(const quint8* data, std::size_t size) = 0;
    virtual void add_timeout_at(std::chrono::time_point<std::chrono::system_clock> time_point) = 0;
private:
    std::string title_;
    std::chrono::time_point<std::chrono::system_clock> last_msg_recv_time_;
};

/**
 * @brief The Protocol class
 *
 * Protocol structure:
 * [2bytes Checksum][2bytes [13bits cmd][3bits flags]][4bytes data][Nbytes... data]
 */
class Protocol
{
public:
    const quint32 MAX_MESSAGE_SIZE = 2147483648;

    enum { DATASTREAM_VERSION = QDataStream::Qt_5_6 };
#if (QT_VERSION > QT_VERSION_CHECK(5, 12, 0))
#pragma GCC warning "Think about raise used version or if it the same, then raise qt version in this condition."
#endif

    enum Flags {
        REPEAT                      = 0x1000,
        FRAGMENT                    = 0x2000,
        ANSWER                      = 0x4000,
        COMPRESSED                  = 0x8000,
        ALL_FLAGS                   = REPEAT | FRAGMENT | ANSWER | COMPRESSED
    };

    template<typename... Args>
    void parse_out(const QByteArray &data, Args&... args)
    {
        Helpz::parse_out(static_cast<int>(DATASTREAM_VERSION), data, args...);
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

    template<class FT, class T, typename RetType, typename... FArgs, typename... Args>
    RetType apply_parse(const QByteArray &data, RetType(FT::*__f)(FArgs...) const, Args&&... args)
    {
        QDataStream ds(data);
        ds.setVersion(DATASTREAM_VERSION);
        return apply_parse_impl<RetType, decltype(__f), T, FArgs...>(ds, __f, static_cast<T*>(this), std::forward<Args&&>(args)...);
    }

    template<class FT, class T, typename RetType, typename... FArgs, typename... Args>
    RetType apply_parse(const QByteArray &data, RetType(FT::*__f)(FArgs...), Args&&... args)
    {
        QDataStream ds(data);
        ds.setVersion(DATASTREAM_VERSION);
        return apply_parse_impl<RetType, decltype(__f), T, FArgs...>(ds, __f, static_cast<T*>(this), std::forward<Args&&>(args)...);
    }

public:
    Protocol();

    virtual bool operator ==(const Protocol&) const { return false; }

    void set_protocol_writer(Protocol_Writer* protocol_writer);

    typedef std::chrono::time_point<std::chrono::system_clock> Time_Point;

    Time_Point last_msg_send_time() const;

    Protocol_Sender send(quint16 cmd, quint16 flags = 0);
    void send_cmd(quint16 cmd, quint16 flags = 0);
    void send_byte(quint16 cmd, char byte);
    void send_array(quint16 cmd, const QByteArray &buff);
    void send_message(Message_Item message);

public:
    QByteArray prepare_packet(const Message_Item& msg);
    void process_bytes(const quint8 *data, size_t size);
    void process_wait_list();

    /**
     * @brief ready_write
     * Called when a connection is established.
     */
    virtual void ready_write() {}
    virtual void closed() {}
protected:

    virtual void process_message(quint16 cmd, QByteArray&& data, QIODevice* data_dev) = 0;

    Protocol_Writer* protocol_writer_;

//    friend class Protocol_Sender;
private:
    void process_stream();
    void internal_process_message(quint16 cmd, quint16 flags, const char* data_ptr, quint32 data_size);

    void add_to_waiting(Time_Point time_point, Message_Item&& message);
    std::vector<Message_Item> pop_waiting_messages();

    Time_Point calc_wait_for_point(const Time_Point& end_point) const;

    uint8_t next_rx_msg_id_;
    std::atomic<uint8_t> next_tx_msg_id_;
    std::vector<std::pair<uint8_t,Time_Point>> lost_msg_list_;

    QBuffer device_;
    QDataStream msg_stream_;

    std::atomic<Time_Point> last_msg_send_time_;

    std::map<Time_Point,Message_Item> waiting_messages_;
    std::mutex waiting_messages_mutex_;
};

} // namespace Network
} // namespace Helpz

#endif // HELPZ_NETWORK_PROTOCOL_H
