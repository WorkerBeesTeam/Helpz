#include <iostream>
#include <queue>

#include "net_protocol_timer.h"

namespace Helpz {
namespace Network {

Protocol_Timer::Protocol_Timer(Protocol_Timer_Emiter *emiter) :
    emiter_(emiter),
    break_flag_(false),
    thread_(&Protocol_Timer::run, this)
{
}

Protocol_Timer::~Protocol_Timer()
{
    if (thread_.joinable())
    {
        break_flag_ = true;
        cond_.notify_one();
        thread_.join();
    }
}

void Protocol_Timer::add(Time_Point time_point, boost::asio::ip::udp::endpoint endpoint)
{
    std::lock_guard lock(mutex_);
    items_.emplace(time_point, endpoint);
    cond_.notify_one();
}

void Protocol_Timer::run()
{
    std::queue<boost::asio::ip::udp::endpoint> clients;
    auto pred_func = [this]() { return break_flag_ || items_.size(); };
    bool timeout;

    std::unique_lock lock(mutex_, std::defer_lock);
    while (true)
    {
        timeout = false;

        lock.lock();
        if (!items_.size())
        {
            cond_.wait(lock, pred_func);
        }
        else
        {
            timeout = !cond_.wait_until(lock, items_.begin()->first, pred_func);
        }

        if (break_flag_)
        {
            break;
        }

        if (timeout && items_.size())
        {
            auto expires_close = std::chrono::system_clock::now() + std::chrono::milliseconds(10);
            for (auto it = items_.begin(); it != items_.end(); )
            {
                if (expires_close >= it->first)
                {
                    clients.push(it->second);
                    it = items_.erase(it);
                }
                else
                {
                    break;
                }
            }
        }

        lock.unlock();

        while (clients.size())
        {
            emiter_->on_protocol_timeout(clients.front());
            clients.pop();
        }
    }
}

} // namespace Network
} // namespace Helpz
