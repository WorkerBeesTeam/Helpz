#include <iostream>
#include <queue>

#include "net_protocol_timer.h"

namespace Helpz {
namespace Net {

Protocol_Timer::Protocol_Timer(Protocol_Timer_Emiter *emiter) :
    emiter_(emiter),
    break_flag_(false), new_timeout_flag_(false)
{
    thread_ = new std::thread(&Protocol_Timer::run, this);
}

Protocol_Timer::~Protocol_Timer()
{
    if (thread_->joinable())
    {
        {
            std::lock_guard lock(mutex_);
            break_flag_ = true;
            cond_.notify_one();
        }

        thread_->join();
    }
    delete thread_;
}

void Protocol_Timer::add(Time_Point time_point, boost::asio::ip::udp::endpoint endpoint, void *data)
{
    std::lock_guard lock(mutex_);
    items_.emplace(time_point, Item{endpoint, data});
    new_timeout_flag_ = true;
    cond_.notify_one();
}

void Protocol_Timer::run()
{
    std::queue<Item> clients;
    auto pred_func = [this]() { return break_flag_ || new_timeout_flag_; };
    bool timeout;

    std::unique_lock lock(mutex_, std::defer_lock);
    while (!break_flag_)
    {
        timeout = false;

        lock.lock();
        if (items_.size())
        {
            timeout = !cond_.wait_until(lock, items_.begin()->first, pred_func);
        }
        else
        {
            cond_.wait(lock, pred_func);
        }

        if (break_flag_)
        {
            break;
        }

        if (new_timeout_flag_)
        {
            new_timeout_flag_ = false;
        }

        if (timeout && items_.size())
        {
            auto expires_close = std::chrono::system_clock::now() + std::chrono::milliseconds(10);
            for (auto it = items_.begin(); it != items_.end(); )
            {
                if (expires_close >= it->first)
                {
                    clients.push(std::move(it->second));
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
            emiter_->on_protocol_timeout(clients.front().endpoint_, clients.front().data_);
            clients.pop();
        }
    }
}

} // namespace Net
} // namespace Helpz
