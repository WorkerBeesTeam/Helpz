#include "server.h"

namespace dtls {

std::time_t time_since_epoch()
{
    std::chrono::time_point<std::chrono::system_clock> now =
        std::chrono::system_clock::now();
    return std::chrono::system_clock::to_time_t(now);
}

server_thread::server_thread() :
    msg_(&server_thread::process_message_thread_func, this)
{
}

std::size_t server_thread::clients_size() const { return clients_.size(); }

std::shared_ptr<protocol_impl> server_thread::find_client(const boost::asio::ip::udp::endpoint &remote_endpoint) const
{
    auto it = clients_.find(remote_endpoint);
    if (it == clients_.cend())
        return {};
    return it->second;
}

std::shared_ptr<protocol_impl> server_thread::create_client(const boost::asio::ip::udp::endpoint &remote_endpoint)
{
    try
    {
        std::shared_ptr<protocol_impl> protocol(new protocol_impl{remote_endpoint});
        protocol->set_thread(this);
        //        std::shared_ptr<server_node> node = std::make_shared<server_node>(std::move(protocol));
        //        node->send.connect(std::bind(&server::send, dtls_server, std::placeholders::_1,
        //                                     std::placeholders::_2, std::placeholders::_3));
        //        node->closed_signal.connect(std::bind(&server_thread::remove_node, this, std::placeholders::_1));
        //        node->dtls_.reset(new Botan::TLS::Server{ *node, *dtls_tools->session_manager, *dtls_tools->creds,
        //                                                  *dtls_tools->policy, *dtls_tools->rng, true });
        clients_.emplace(remote_endpoint, protocol);
        return protocol;
    }
    catch(std::exception& e) { std::cerr << remote_endpoint << " Fail init DTLS. " << e.what() << std::endl; }
    catch(...) { std::cerr << remote_endpoint << " Fail create DTLS" << std::endl; }
    return {};
}

void server_thread::remove_node(const project::base &node)
{
    std::cout << node.title() << " removed. ";
    auto it = clients_.find(node.remote_endpoint());
    if (it != clients_.end())
        clients_.erase(it);
    std::cout << "Client count: " << clients_.size() << std::endl;
}

void server_thread::remove_frozen_clients()
{
    std::size_t size = clients_.size();
    for(auto it = clients_.begin(); it != clients_.end();)
    {
        if(it->second->check_timeout())
        {
            std::cout << it->second->title() << " timeout" << std::endl;
            it = clients_.erase(it);
        }
        else
            ++it;
    }
    if (size != clients_.size())
        std::cout << "Frozen clients removed. Client count: " << clients_.size() << std::endl;
}

void server_thread::remove_same(project::base *proj, server_thread **out_own_thread)
{
    std::size_t size = clients_.size();
    for(auto it = clients_.begin(); it != clients_.end();)
    {
        if (it->second->name() == proj->name()) {
            if (it->first != proj->remote_endpoint()) {
                it = clients_.erase(it);
                continue;
            } else if (*out_own_thread == nullptr)
                *out_own_thread = this;
        }
        ++it;
    }
    if (size != clients_.size())
        std::cout << "Same removed. Client count: " << clients_.size() << ' '
                  << (size - clients_.size()) << " same removed" << std::endl;
}

void server_thread::add_message(quint16 cmd, QByteArray &&msg_data, protocol_base *protocol, bool is_answer)
{
    std::lock_guard lock(msg_.mutex_);
    msg_.data_queue_.push(message_pack{cmd, std::move(msg_data), dynamic_cast<protocol_impl*>(protocol), is_answer});
    msg_.cond_.notify_one();
}

void server_thread::process_message_thread_func()
{
    QElapsedTimer e_timer; std::size_t msg_size; qint64 e_time;

    std::unique_lock lock(msg_.mutex_, std::defer_lock);
    while (true)
    {
        lock.lock();
        msg_.cond_.wait(lock, [this](){ return msg_.data_queue_.size(); });
        std::queue<message_pack> data_queue = std::move(msg_.data_queue_);
        lock.unlock();

        e_timer.start(); msg_size = data_queue.size();
        while (data_queue.size())
        {
            message_pack data{std::move(data_queue.front())};
            data_queue.pop();

            QDataStream ds(&data.msg_data, QIODevice::ReadWrite);
            ds.setVersion(protocol_base::DATASTREAM_VERSION);

            try {
                data.protocol->process_message_stream(data.cmd, ds);
            } catch(const std::exception& e) {
                std::cerr << data.protocol->title() << " EXCEPTION: proc msg " << data.cmd << ' ' << e.what() << std::endl;
            } catch(...) {
                std::cerr << data.protocol->title() << " EXCEPTION Unknown: proc msg " << data.cmd << std::endl;
            }
        }

        e_time = e_timer.restart();
        if (e_time > 1000)
            std::cout << "Processed " << msg_size << " messages time: " << e_time << ' ' << std::endl;
        e_timer.invalidate();
    }
    //    db.reset();
}

server::server(boost::asio::io_context &io_context) :
    socket_(io_context, udp::endpoint(udp::v4(), serv_conf->port)),
    last_clean_time_(time_since_epoch()),
    //    rx_(&server::recv_thread_func, this),
    //    tx_(&server::send_thread_func, this),
    threads_(5)
{
    std::cout << "dtls server listening on port " << serv_conf->port << std::endl;
    //    start_receive();
}

void server::start_receive()
{
    socket_.async_receive_from(
                boost::asio::null_buffers(), remote_endpoint_,
                std::bind(&server::handle_receive, this,
                          std::placeholders::_1,   // boost::asio::placeholders::error,
                          std::placeholders::_2)); // boost::asio::placeholders::bytes_transferred
}

void server::remove_same(project::base *proj)
{
    std::time_t now = time_since_epoch();
    bool clean_delta_expired = (now - last_clean_time_) > 180;

    std::lock_guard lock(clients_mutex_);

    if (clean_delta_expired)
        last_clean_time_ = now;

    server_thread* thread = nullptr;
    for (server_thread& th: threads_) {
        th.remove_same(proj, &thread);
        if (clean_delta_expired)
            th.remove_frozen_clients();
    }

    if (thread)
        clients_[proj->id()] = thread;
}

void server::remove_client(server_thread *thread, const project::base &node)
{
    std::lock_guard lock(clients_mutex_);
    clients_.erase(node.id());
    thread->remove_node(node);
}

void server::close_slot(server_thread *thread, const project::base &node)
{
    std::thread(&server::remove_client, this, thread, node).detach();
}

void server::send(const boost::asio::ip::udp::endpoint &remote_endpoint, const uint8_t *data, std::size_t size)
{
    uint8_t* buffer = new uint8_t[size];
    memcpy(buffer, data, size);

    socket_.async_send_to(boost::asio::buffer(buffer, size), remote_endpoint,
                          std::bind(&server::handle_send, this, remote_endpoint,
                                    std::unique_ptr<uint8_t[]>(buffer), size,
                                    std::placeholders::_1,   // boost::asio::placeholders::error,
                                    std::placeholders::_2)); // boost::asio::placeholders::bytes_transferred
}

void server::handle_receive(const boost::system::error_code &err, std::size_t)
{
    if (check_error(err))
        return;

    udp::socket::bytes_readable readable(true);
    boost::system::error_code error;
    socket_.io_control(readable, error);
    if (check_error(error))
        return;

    std::size_t size = readable.get();
    std::unique_ptr<uint8_t[]> recv_buffer(new uint8_t[size]);
    udp::endpoint remote_endpoint;
    socket_.receive_from(boost::asio::buffer(recv_buffer.get(), size), remote_endpoint, 0, error);

    if (!check_error(error))
    {
        std::shared_ptr<protocol_impl> node;

        {
            std::size_t size;
            quint32 min_size = -1;
            server_thread* thread = nullptr;

            std::lock_guard lock(clients_mutex_);
            start_receive(); // Start next receive

            for (server_thread& th: threads_) {
                node = th.find_client(remote_endpoint);
                if (node) {
                    thread = &th;
                    break;
                } else {
                    size = th.clients_size();
                    if (!thread || size < min_size) {
                        min_size = size;
                        thread = &th;
                    }
                }
            }

            if (!node) {
                node = thread->create_client(remote_endpoint);
                std::cout << remote_endpoint << " create " << std::endl;
                if (!node)
                    return;

                //                node->closed_signal.connect(std::bind(&server::close_slot, this, thread, std::placeholders::_1));
            }

            node->mutex_.lock();
        }
        std::unique_lock lock(node->mutex_, std::adopt_lock);
        node->process_data(recv_buffer.get(), size);
    } else
        start_receive();
}

void server::handle_send(const boost::asio::ip::udp::endpoint &remote_endpoint, std::unique_ptr<uint8_t[]> &data, std::size_t size, const boost::system::error_code &error, const std::size_t &bytes_transferred)
{
    // TODO: maybe ping if timeout
    if (error.value() != 0)
        std::cerr << remote_endpoint << " SEND ERROR " << error.category().name() << ": "
                  << error.message() << " size: " << size << std::endl;
    else if (size != bytes_transferred)
        std::cerr << remote_endpoint << " SEND ERROR wrong size " << size
                  << " transfered: " << bytes_transferred << std::endl;
    else if (!data)
        std::cerr << remote_endpoint << " SEND ERROR empty data pointer" << std::endl;
}

bool server::check_error(const boost::system::error_code &error)
{
    if (error)
        std::cerr << "RECV ERROR " << error.category().name() << ": " << error.message() << std::endl;
    return (bool)error;
}

} // namespace dtls
