#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <vector>

#define BUFFER_SIZE 1024 * 5

using namespace boost::asio;
using namespace boost::placeholders;
io_service service;

class talk_to_client;
typedef boost::shared_ptr<talk_to_client> client_ptr;
std::vector<client_ptr> clients;

class talk_to_client : public boost::enable_shared_from_this<talk_to_client>, boost::noncopyable
{
    typedef talk_to_client self_type;
    talk_to_client() : sock_(service), started_(false), clients_changed_(false) {}
public:
    typedef boost::system::error_code error_code;
    typedef boost::shared_ptr<talk_to_client> ptr;
    void start()
    {
        started_ = true;
        clients.push_back(shared_from_this());
        std::cout << "client fucking started\n";
        reading();
    }
    static ptr new_()
    {
        ptr new_(new talk_to_client);
        return new_;
    }
    void stop()
    {
        std::cout << "server, stop()\n";
        if (!started_) return;
        started_ = false;
        sock_.close();
        auto it = std::find(clients.begin(), clients.end(), shared_from_this());
        clients.erase(it);
        update_clients_changed();
    }
    ip::tcp::socket& sock() { return sock_; }
    std::string username() const { return username_; }
    void set_clients_changed() { clients_changed_ = true; }
    void send_message_public(std::string msg) { send_message(msg); }

private:
    void reading()
    {
        sock_.async_read_some(buffer(read_buffer_), 
            boost::bind(&self_type::read_completed, shared_from_this(), _1, _2));
    }
    void read_completed(const error_code &err, size_t bytes)
    {
        if (err) stop();
        if (!started_) {
            std::cerr << "server has been stopped in read_completed\n";
            return;
        }
        // printf("read_buffer: %s\n", read_buffer_);
        std::string msg(read_buffer_, bytes);
        std::cout << "server received: " << msg << std::endl;
        if (msg.find("login:") == 0) {
            std::string username = msg.substr(6);
            username_ = username;
            send_message("hello, " + username + "!");
        } else {
            for (auto it = clients.begin(), e = clients.end(); it != e; ++it)
            {
                // std::cout << "server, entered rassylka\n";
                std::cout << (*it)->username();
                // printf("%s\n", (*it)->username());
                // fflush(stdout);
                (*it)->send_message_public(msg);
            }
            std::cout << std::endl;
            // send_message(msg);
        }
    }
    void send_message(std::string msg)
    {
        if (!started_) return;
        std::copy(msg.begin(), msg.end(), write_buffer_);
        sock_.async_write_some(buffer(write_buffer_, msg.size()), 
            boost::bind(&self_type::message_sended, shared_from_this(), _1, _2));
    }
    void message_sended(const error_code &err, size_t bytes)
    {
        reading();
    }
    void update_clients_changed()
    {
        for(auto b = clients.begin(), e = clients.end(); b != e; ++b)
        {
            (*b)->set_clients_changed();
        }
    }

private:
    ip::tcp::socket sock_;
    bool started_;
    enum { max_msg = BUFFER_SIZE };
    char read_buffer_[max_msg];
    char write_buffer_[max_msg];
    std::string username_;
    bool clients_changed_;
};

ip::tcp::acceptor acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), 8001));
void handle_accept(talk_to_client::ptr client, const talk_to_client::error_code &err)
{
    client->start();
    talk_to_client::ptr new_client = talk_to_client::new_();
    acceptor.async_accept(new_client->sock(), boost::bind(handle_accept, new_client, _1));
}

int main(int argc, char const *argv[])
{
    talk_to_client::ptr client = talk_to_client::new_();
    acceptor.async_accept(client->sock(), boost::bind(handle_accept, client, _1));
    service.run();
    return 0;
}