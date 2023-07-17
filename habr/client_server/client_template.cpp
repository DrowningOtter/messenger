#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <iostream>

using namespace boost::asio;
io_service service;


class talk_to_svr: public boost::enable_shared_from_this<talk_to_svr>, boost::noncopyable
{
    typedef talk_to_svr self_type;
    talk_to_svr(const std::string &username) : sock_(service), started_(true), username_(username), timer_(service) {}
    void start(ip::tcp::endpoint ep)
    {
        sock_.async_connect(ep, boost::bind(&self_type::on_connect, shared_from_this(), _1));
    }
public:
    typedef boost::system::error_code error_code;
    typedef boost::shared_ptr<talk_to_svr> ptr;
    static ptr start(ip::tcp::endpoint ep, const std::string &username)
    {
        ptr new_(new talk_to_svr(username));
        new_->start(ep);
        return new_;
    }
    void stop()
    {
        if (!started_) return;
        started_ = false;
        sock_.close();
    }
    bool started() { return started_; }

    void on_connect(const error_code &err)
    {
        if (!err) do_write("login" + username_ + "\n");
        else stop();
    }
    void on_read(const error_code &err, size_t bytes)
    {
        if (err) stop();
        if (!started()) return;
        //process the msg
        std::string msg(read_buffer_, bytes);
        if (msg.find("login") == 0) on_login();
        else if (msg.find("ping") == 0) on_ping(msg);
        else if (msg.find("clients") == 0) on_clients(msg);
    }
    void on_login()
    {
        do_ask_clients();
    }
    void on_ping(const std::string &msg)
    {
        std::istringstream in(msg);
        std::string answer;
        in >> answer >> answer;
        if (answer == "client_list_changed") do_ask_clients();
        else postpone_ping();
    }
    void on_clients(const std::string &msg)
    {
        std::string clients = msg.substr(8);
        std::cout << username_ << ", new client list:" << clients;
        postpone_ping();
    }

    void do_ping() { do_write("ping\n"); }
    void postpone_ping()
    {
        timer_.expires_from_now(boost::posix_time::millisec(rand() % 7000));
        timer_.async_wait(boost::bind(&self_type::do_ping, shared_from_this()));
    }
    void do_ask_clients() { do_write("ask_clients\n"); }
    void on_write(const error_code &err, size_t bytes) { do_read(); }
    void do_read()
    {
        async_read(sock_, buffer(read_buffer_),
            boost::bind(&self_type::read_complete, shared_from_this(), _1, _2),
            boost::bind(&self_type::on_read, shared_from_this(), _1, _2));
    }
    void do_write(const std::string &msg)
    {
        if (!started()) return;
        std::copy(msg.begin(), msg.end(), write_buffer_);
        sock_.async_write_some(buffer(write_buffer_, msg.size()), boost::bind(&self_type::on_write, shared_from_this(), _1, _2));
    }
private:
    size_t read_complete(const boost::system::error_code &err, size_t bytes)
    {
        if (err) return 0;
        bool found = std::find(read_buffer_, read_buffer_ + bytes, '\n') < read_buffer_ + bytes;
        return found ? 0 : 1;
    }
private:
    ip::tcp::socket sock_;
    enum { max_msg = 1024 };
    char read_buffer_[max_msg];
    char write_buffer_[max_msg];
    bool started_;
    std::string username_;
    deadline_timer timer_;
};


int main(int argc, char const *argv[])
{
    ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 8001);
    const char* names[] = { "John"/*, "James", "Lucy", "Tracy", "Frank", "Abby"*/, 0 };
    for (const char **name = names; *name; ++name) {
        talk_to_svr::start(ep, *name);
        boost::this_thread::sleep(boost::posix_time::millisec(100));
    }
    service.run();

    return 0;
}

