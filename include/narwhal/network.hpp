#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

#ifndef USE_INTERNAL_MOCKS
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#endif

namespace narwhal::network {

using Message = std::vector<uint8_t>;

#ifndef USE_INTERNAL_MOCKS
namespace asio = boost::asio;
using tcp = asio::ip::tcp;
namespace ssl = asio::ssl;
#endif

class Network {
public:
    virtual ~Network() = default;
    virtual void send(const std::string& address, const Message& message) = 0;
    virtual void broadcast(const std::vector<std::string>& addresses, const Message& message) = 0;
    virtual void on_receive(std::function<void(const Message&, const std::string&)> callback) = 0;
};

class TlsNetwork : public Network {
public:
#ifndef USE_INTERNAL_MOCKS
    TlsNetwork(asio::io_context& io_context, uint16_t port, const std::string& cert_file, const std::string& key_file);
#else
    TlsNetwork(int dummy_io, uint16_t port, const std::string& cert_file, const std::string& key_file);
#endif
    
    void send(const std::string& address, const Message& message) override;
    void broadcast(const std::vector<std::string>& addresses, const Message& message) override;
    void on_receive(std::function<void(const Message&, const std::string&)> callback) override;

private:
#ifndef USE_INTERNAL_MOCKS
    void start_accept();
    void handle_receive(std::shared_ptr<ssl::stream<tcp::socket>> stream);

    asio::io_context& io_context_;
    tcp::acceptor acceptor_;
    ssl::context ssl_context_;
#endif
    std::function<void(const Message&, const std::string&)> receive_callback_;
};

} // namespace narwhal::network
