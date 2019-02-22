#ifndef SERVER_H
#define SERVER_H

#include <map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

#include <boost/asio/ip/udp.hpp>

#include "protocol_impl.h"

namespace dtls {

template<typename DataPackT, typename T>
struct thread_iface {
    thread_iface(void(T::*thread_func)(), T* obj) : thread_(thread_func, obj) {}
    std::queue<DataPackT> data_queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::thread thread_;
};

class server_thread
{
public:
    using udp = boost::asio::ip::udp;

    server_thread();

    std::size_t clients_size() const;
    std::shared_ptr<protocol_impl> find_client(const udp::endpoint& remote_endpoint) const;
    std::shared_ptr<protocol_impl> create_client(const udp::endpoint& remote_endpoint);

    void remove_node(const project::base &node);
    void remove_frozen_clients();

//    void add_data(std::shared_ptr<server_node>& node, std::unique_ptr<uint8_t[]>&& data, std::size_t size);


    void remove_same(project::base *proj, server_thread** out_own_thread);
    void add_message(quint16 cmd, QByteArray &&msg_data, protocol_base *protocol, bool is_answer = false);
private:
    void process_message_thread_func();

    struct message_pack {
        uint16_t cmd;
        QByteArray msg_data;
        protocol_impl* protocol;
        bool is_answer;
    };
    thread_iface<message_pack, server_thread> msg_;

    std::map<udp::endpoint, std::shared_ptr<protocol_impl>> clients_;
};

class server
{
public:
    using udp = boost::asio::ip::udp;
    server(boost::asio::io_context& io_context);

    void start_receive();

    void remove_same(project::base *proj);
    void remove_client(server_thread* thread, const project::base& node);
    void close_slot(server_thread* thread, const project::base& node);

    void send(const udp::endpoint& remote_endpoint, const uint8_t* data, std::size_t size);
private:
    void recv_thread_func();

    void handle_receive(const boost::system::error_code& err,
                        std::size_t /*bytes_transferred*/);

    void handle_send(const udp::endpoint& remote_endpoint,
                     std::unique_ptr<uint8_t[]> &data, std::size_t size,
                     const boost::system::error_code& error,
                     const std::size_t & bytes_transferred);

    bool check_error(const boost::system::error_code& error);

    udp::socket socket_;
    udp::endpoint remote_endpoint_;

//    thread_iface<data_pack, server> rx_, tx_;


    std::time_t last_clean_time_;

    std::mutex clients_mutex_;
    std::vector<server_thread> threads_;
    std::map<uint32_t, server_thread*> clients_;
};

} // namespace dtls

#endif // SERVER_H
