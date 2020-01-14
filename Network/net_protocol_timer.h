#ifndef HELPZ_NET_PROTOCOL_TIMER_H
#define HELPZ_NET_PROTOCOL_TIMER_H

#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <boost/asio/ip/udp.hpp>

namespace Helpz {
namespace Network {

class Protocol_Timer_Emiter
{
public:
    virtual ~Protocol_Timer_Emiter() = default;
    virtual void on_protocol_timeout(boost::asio::ip::udp::endpoint endpoint, void* data) = 0;
};

class Protocol_Timer
{
public:
    Protocol_Timer(Protocol_Timer_Emiter* emiter);
    virtual ~Protocol_Timer();

    typedef std::chrono::time_point<std::chrono::system_clock> Time_Point;

    void add(Time_Point time_point, boost::asio::ip::udp::endpoint endpoint, void* data);
private:
    void run();

    Protocol_Timer_Emiter *emiter_;

    bool break_flag_, new_timeout_flag_;
    std::thread* thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

    struct Item
    {
        boost::asio::ip::udp::endpoint endpoint_;
        void* data_;
    };
    std::map<Time_Point, Item> items_;
};

} // namespace Network
} // namespace Helpz

#endif // HELPZ_NET_PROTOCOL_TIMER_H
