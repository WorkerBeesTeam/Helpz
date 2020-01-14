#ifndef HELPZ_NETWORK_PROTOCOL_WRITER_H
#define HELPZ_NETWORK_PROTOCOL_WRITER_H

#include <memory>
#include <chrono>

#include <QString>

namespace Helpz {
namespace Network {

class Protocol;
class Protocol_Writer
{
public:
    virtual ~Protocol_Writer() = default;

    const QString& title() const;
    void set_title(const QString& title);

    std::chrono::time_point<std::chrono::system_clock> last_msg_recv_time() const;
    void set_last_msg_recv_time(std::chrono::time_point<std::chrono::system_clock> value);

    virtual void write(QByteArray&& data) = 0;
    virtual void add_timeout_at(std::chrono::time_point<std::chrono::system_clock> time_point, void* data = nullptr) = 0;

    virtual std::shared_ptr<Protocol> protocol() = 0;
private:
    QString title_;
    std::chrono::time_point<std::chrono::system_clock> last_msg_recv_time_;
};

} // namespace Network
} // namespace Helpz

#endif // HELPZ_NETWORK_PROTOCOL_WRITER_H
