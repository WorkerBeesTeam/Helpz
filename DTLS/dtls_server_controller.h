#ifndef DTLS_SERVER_CONTROLLER_H
#define DTLS_SERVER_CONTROLLER_H

#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <boost/thread/shared_mutex.hpp>

#include <Helpz/dtls_controller.h>
#include <Helpz/dtls_server_node.h>

namespace Helpz {
namespace DTLS {

typedef std::function<std::shared_ptr<Network::Protocol>(const std::vector<std::string> &, std::string*)> Create_Server_Protocol_Func_T;

class Server_Controller final : public Controller
{
public:
    Server_Controller(Tools* dtls_tools, Socket* socket, Create_Server_Protocol_Func_T&& create_protocol_func, int record_thread_count = 5);
    ~Server_Controller();

    Socket* socket();

    std::shared_ptr<Node> get_node(const udp::endpoint& remote_endpoint) override;
    void process_data(std::shared_ptr<Node> &node, std::unique_ptr<uint8_t[]> &&data, std::size_t size) override;

    void remove_copy(Network::Protocol* client);
    bool check_copy(Network::Protocol* client);

    void remove_frozen_clients(std::chrono::seconds frozen_timeout);
    bool check_frozen_clients(std::chrono::seconds frozen_timeout);

    std::shared_ptr<Server_Node> find_client(const udp::endpoint& remote_endpoint) const;
    std::shared_ptr<Server_Node> find_client(std::function<bool(const Network::Protocol *)> check_protocol_func) const;
private:
    std::shared_ptr<Server_Node> create_client(const udp::endpoint& remote_endpoint);
public:
    void remove_client(const udp::endpoint& remote_endpoint);

    std::shared_ptr<Network::Protocol> create_protocol(const std::vector<std::string> &client_protos, std::string* choose_out);

    void add_received_record(const udp::endpoint& remote_endpoint, std::unique_ptr<uint8_t[]>&& buffer, std::size_t size);
private:
    void records_thread_run();

    mutable boost::shared_mutex clients_mutex_;
    std::map<udp::endpoint, std::shared_ptr<Server_Node>> clients_;

    Socket* socket_;
    Create_Server_Protocol_Func_T create_protocol_func_;

    struct Record_Item
    {
        udp::endpoint remote_endpoint_;
        std::unique_ptr<uint8_t[]> buffer_;
        std::size_t size_;
    };
    bool records_thread_break_flag_;
    std::vector<std::thread> records_thread_list_;
    std::queue<Record_Item> records_queue_;
    std::condition_variable records_cond_;
    std::mutex records_mutex_;
};

} // namespace DTLS
} // namespace Helpz

#endif // DTLS_SERVER_CONTROLLER_H
