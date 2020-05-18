#include "net_protocol_writer.h"

namespace Helpz {
namespace Net {

const QString &Protocol_Writer::title() const { return title_; }
void Protocol_Writer::set_title(const QString &title) { title_ = title; }

std::chrono::time_point<std::chrono::system_clock> Protocol_Writer::last_msg_recv_time() const { return last_msg_recv_time_; }
void Protocol_Writer::set_last_msg_recv_time(std::chrono::time_point<std::chrono::system_clock> value) { last_msg_recv_time_ = value; }

} // namespace Net
} // namespace Helpz
