#ifndef HELPZ_NETWORK_PROTOCOL_WRITER_H
#define HELPZ_NETWORK_PROTOCOL_WRITER_H

#include <Helpz/net_message_item.h>

namespace Helpz {
namespace Net {

class Protocol;
class Protocol_Writer
{
public:
    virtual ~Protocol_Writer() = default;

    const QString& title() const;
    void set_title(const QString& title);

    std::chrono::time_point<std::chrono::system_clock> last_msg_recv_time() const;
    void set_last_msg_recv_time(std::chrono::time_point<std::chrono::system_clock> value);

    virtual void write(const QByteArray& data) = 0;
    virtual void write(std::shared_ptr<Message_Item> message) = 0;
    virtual void add_timeout_at(std::chrono::system_clock::time_point time_point, void* data = nullptr) = 0;

    virtual std::shared_ptr<Protocol> protocol() = 0;
private:
    QString title_;
    std::chrono::time_point<std::chrono::system_clock> last_msg_recv_time_;
};

} // namespace Net
} // namespace Helpz

#endif // HELPZ_NETWORK_PROTOCOL_WRITER_H
