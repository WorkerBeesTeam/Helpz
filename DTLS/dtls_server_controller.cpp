#include <mutex>

#include "dtls_server_controller.h"

namespace Helpz {
namespace DTLS {

Server_Controller::Server_Controller(Tools *dtls_tools, Socket *socket, const Create_Protocol_Func_T &create_protocol_func, int record_thread_count) :
    Controller{ dtls_tools },
    socket_(socket),
    create_protocol_func_(create_protocol_func),
    records_thread_break_flag_(false)
{
    for (int i = 0; i < record_thread_count; ++i)
    {
        records_thread_list_.emplace_back(std::thread(&Server_Controller::records_thread_run, this));
    }
}

Server_Controller::~Server_Controller()
{
    records_thread_break_flag_ = true;
    records_cond_.notify_all();
    for (std::thread& t: records_thread_list_)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
}

Socket *Server_Controller::socket()
{
    return socket_;
}

void Server_Controller::process_data(const udp::endpoint &remote_endpoint, std::unique_ptr<uint8_t[]> &&data, std::size_t size)
{
    std::unique_lock lock(clients_mutex_);
    auto node = get_or_create_client(remote_endpoint);
    if (!node)
    {
        return;
    }

    std::lock_guard node_lock(node->mutex_);
    lock.unlock();
    node->process_received_data(std::move(data), size);
}

void Server_Controller::remove_copy(Network::Protocol *client)
{
    if (check_copy(client))
    {
        std::lock_guard lock(clients_mutex_);
        for(auto it = clients_.begin(); it != clients_.end();)
        {
            if (it->second->protocol() && it->second->protocol() != client && *it->second->protocol() == *client)
            {
                std::cout << it->second->title() << " same. Erase it." << std::endl;
                it = clients_.erase(it);
            }
            else
                ++it;
        }
    }
}

bool Server_Controller::check_copy(Network::Protocol *client)
{
    boost::shared_lock lock(clients_mutex_);
    for (const std::pair<udp::endpoint, std::shared_ptr<Server_Node>>& it: clients_)
    {
        if (it.second->protocol() && it.second->protocol() != client && *it.second->protocol() == *client)
        {
            return true;
        }
    }
    return false;
}

void Server_Controller::remove_frozen_clients(std::chrono::seconds frozen_timeout)
{
    if (check_frozen_clients(frozen_timeout))
    {
        std::lock_guard lock(clients_mutex_);
        auto now = std::chrono::system_clock::now();

        for(auto it = clients_.begin(); it != clients_.end();)
        {
            if ((now - it->second->last_msg_recv_time()) > frozen_timeout)
            {
                std::cout << it->second->title() << " timeout. Erase it." << std::endl;
                it = clients_.erase(it);
            }
            else
                ++it;
        }
    }
}

bool Server_Controller::check_frozen_clients(std::chrono::seconds frozen_timeout)
{
    boost::shared_lock lock(clients_mutex_);
    auto now = std::chrono::system_clock::now();
    for (const std::pair<udp::endpoint, std::shared_ptr<Server_Node>>& it: clients_)
    {
        if ((now - it.second->last_msg_recv_time()) > frozen_timeout)
        {
            return true;
        }
    }
    return false;
}

std::shared_ptr<Server_Node> Server_Controller::find_client(const udp::endpoint &remote_endpoint) const
{
    boost::shared_lock lock(clients_mutex_);
    auto it = clients_.find(remote_endpoint);
    if (it != clients_.cend())
    {
        return it->second;
    }
    return {};
}

std::shared_ptr<Server_Node> Server_Controller::find_client(std::function<bool (const Network::Protocol *)> check_protocol_func) const
{
    boost::shared_lock lock(clients_mutex_);
    for (const std::pair<udp::endpoint, std::shared_ptr<Server_Node>>& it: clients_)
    {
        if (it.second->protocol() && check_protocol_func(it.second->protocol()))
        {
            return it.second;
        }
    }
    return {};
}

std::shared_ptr<Server_Node> Server_Controller::get_or_create_client(const udp::endpoint &remote_endpoint)
{
    auto it = clients_.find(remote_endpoint);
    if (it != clients_.cend())
    {
        return it->second;
    }

    auto create_pair = clients_.emplace(remote_endpoint, std::make_shared<Server_Node>(this, remote_endpoint));
    if (create_pair.second)
    {
        return create_pair.first->second;
    }
    return {};
}

void Server_Controller::remove_client(const udp::endpoint &remote_endpoint)
{
    std::lock_guard lock(clients_mutex_);
    clients_.erase(remote_endpoint);
}

Network::Protocol *Server_Controller::create_protocol(const std::vector<std::string> &client_protos, std::string *choose_out)
{
    return create_protocol_func_(client_protos, choose_out);
}

void Server_Controller::add_received_record(const udp::endpoint &remote_endpoint, std::unique_ptr<uint8_t[]> &&buffer, std::size_t size)
{
    std::lock_guard lock(records_mutex_);
    records_queue_.push(Record_Item{remote_endpoint, std::move(buffer), size});
    records_cond_.notify_one();
}

void Server_Controller::add_timeout_at(const boost::asio::ip::udp::endpoint &remote_endpoint, std::chrono::time_point<std::chrono::system_clock> time_point)
{
    protocol_timer_.add(time_point, remote_endpoint);
}

void Server_Controller::on_protocol_timeout(boost::asio::ip::udp::endpoint remote_endpoint)
{
    auto node = find_client(remote_endpoint);
    if (node)
    {
        node->protocol()->process_wait_list();
    }
}

void Server_Controller::records_thread_run()
{
    std::unique_lock lock(records_mutex_, std::defer_lock);
    while (true)
    {
        lock.lock();
        records_cond_.wait(lock, [this](){ return records_queue_.size() || records_thread_break_flag_; });
        if (records_thread_break_flag_)
        {
            break;
        }

        Record_Item record{std::move(records_queue_.front())};
        records_queue_.pop();

        auto node = find_client(record.remote_endpoint_);
        if (!node || !node->protocol())
        {
            lock.unlock();
            continue;
        }

        std::lock_guard node_lock(node->record_mutex_);
        lock.unlock();

        node->protocol()->process_bytes(record.buffer_.get(), record.size_);
    }
}

} // namespace DTLS
} // namespace Helpz
