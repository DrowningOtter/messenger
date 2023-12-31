#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <iostream>

#define BUFFER_SIZE 1024 * 5

using namespace boost::asio;
io_service service;
using namespace boost::placeholders;

class talk_to_svr : public boost::enable_shared_from_this<talk_to_svr>, boost::noncopyable
{
    typedef talk_to_svr self_type;
    talk_to_svr(const std::string &username) : sock_(service), started_(true), username_(username) {}
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
        std::cout << "stopping the client\n";
        if (!started_) return;
        started_ = false;
        sock_.close();
    }
    bool started() const { return started_; }
    void on_connect(const error_code &err)
    {
        boost::this_thread::sleep(boost::posix_time::millisec(1000));
        send_message("login:" + username_);
        boost::this_thread::sleep(boost::posix_time::millisec(1000));
        read_the_message();
        std::cout << "on_connect called\n";
    }
    void send_message(std::string msg = "")
    {
        if (!started()) return;
        write_mutex.lock();
        if (msg != "") {
            char tmp[BUFFER_SIZE];
            strncpy(tmp, msg.c_str(), BUFFER_SIZE - 1);
            sock_.async_write_some(buffer(tmp), 
                boost::bind(&self_type::message_sended, shared_from_this(), _1, _2));            
        } else {
            std::cout << "client, default send_message\n";
            sock_.async_write_some(buffer(write_buffer_), 
                boost::bind(&self_type::message_sended, shared_from_this(), _1, _2));
        }
    }
    void message_sended(const error_code &err, size_t bytes)
    {
        write_buffer_[0] = '\0';
        write_mutex.unlock();
        std::cout << "client, message have been sended\n";
        // read_the_message();
        send_message();
    }
    void read_the_message()
    {
        sock_.async_read_some(buffer(read_buffer_),
            boost::bind(&self_type::read_completed, shared_from_this(), _1, _2));
    }
    void read_completed(const error_code &err, size_t bytes)
    {
        if (err) {
            std::cerr << "read_completed: error occured\n";
            stop();
        }
        if (!started_) return;
        std::string msg(read_buffer_, bytes);
        // std::string screen_buffer_string(screen_buffer);
        // screen_buffer_string += username_ + msg + "\n";
        std::cout << "received: " << msg << std::endl;
        unsigned int index = strlen(screen_buffer);
        strncpy(screen_buffer + index, username_.c_str(), 1000);
        index += strlen(username_.c_str());
        screen_buffer[index++] = ':';
        strncpy(screen_buffer + index, read_buffer_, 1000);
        index = strlen(screen_buffer);
        screen_buffer[index] = '\n';
        screen_buffer[index + 1] = '\0';
        // printf("screen_buffer=%s", screen_buffer);
        fflush(stdout);
        // std::cout << msg;
        // std::cout << "servers message: " << msg;
        // create_new_message();
        // send_message();
        read_the_message();
    }
    void create_new_message()
    {
        std::string msg(write_buffer_);
        // std::cin >> msg;
        // getline(std::cin, msg);
        // ip::tcp::endpoint lend = sock_.local_endpoint();
        // msg = sock_.local_endpoint().address().to_string() + ":" + msg + "\n";
        msg = username_ + ":" + msg + "\n";
        // send_message(msg);
    }
    char* get_read_buffer() { return read_buffer_; }
    char* get_write_buffer() { return write_buffer_; }
    char* get_screen_buffer() { return screen_buffer; }
    boost::interprocess::interprocess_mutex& get_mutex() { return write_mutex; }

private:
    ip::tcp::socket sock_;
    enum { max_msg = BUFFER_SIZE };
    char read_buffer_[max_msg];
    char write_buffer_[max_msg];
    bool started_;
    std::string username_;
    boost::interprocess::interprocess_mutex write_mutex;
    char screen_buffer[BUFFER_SIZE];
};

// int main(int argc, char const *argv[])
// {
//     ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 8001);
//     const std::string name = "Artem";
//     boost::shared_ptr<talk_to_svr> client = talk_to_svr::start(ep, name);
//     service.run();
//     return 0;
// }