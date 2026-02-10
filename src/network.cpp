#include "narwhal/network.hpp"
#include "narwhal/common.hpp"
#include <iostream>

namespace narwhal::network {

#ifndef USE_INTERNAL_MOCKS
TlsNetwork::TlsNetwork(asio::io_context& io_context, uint16_t port, const std::string& cert_file, const std::string& key_file)
    : io_context_(io_context),
      acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
      ssl_context_(ssl::context::tls_server) {
    
    ssl_context_.set_options(ssl::context::default_workarounds | ssl::context::no_sslv2 | ssl::context::no_sslv3 | ssl::context::no_tlsv1 | ssl::context::no_tlsv1_1 | ssl::context::no_tlsv1_2 | ssl::context::single_dh_use);
    start_accept();
}

void TlsNetwork::send(const std::string& address, const Message& message) {
    auto resolver = std::make_shared<tcp::resolver>(io_context_);
    auto socket = std::make_shared<tcp::socket>(io_context_);
    auto client_context = std::make_shared<ssl::context>(ssl::context::tls_client);
    client_context->set_options(ssl::context::no_sslv2 | ssl::context::no_sslv3 | ssl::context::no_tlsv1 | ssl::context::no_tlsv1_1 | ssl::context::no_tlsv1_2);
    auto stream = std::make_shared<ssl::stream<tcp::socket>>(io_context_, *client_context);
    size_t colon_pos = address.find(':');
    std::string host = address.substr(0, colon_pos);
    std::string port = address.substr(colon_pos + 1);
    resolver->async_resolve(host, port, [stream, message](const boost::system::error_code& ec, tcp::resolver::results_type results) {
        if (!ec) {
            asio::async_connect(stream->lowest_layer(), results, [stream, message](const boost::system::error_code& ec, const tcp::endpoint& endpoint) {
                if (!ec) {
                    stream->async_handshake(ssl::stream_base::client, [stream, message](const boost::system::error_code& ec) {
                        if (!ec) asio::async_write(*stream, asio::buffer(message), [stream](const boost::system::error_code& ec, std::size_t){});
                    });
                }
            });
        }
    });
}
#else
TlsNetwork::TlsNetwork(int, uint16_t port, const std::string&, const std::string&) {
    std::cout << "[MOCK] Network listening on port " << port << " (Simulated)" << std::endl;
}

void TlsNetwork::send(const std::string& address, const Message& message) {
    std::cout << "[MOCK] Sending " << message.size() << " bytes to " << address << " (Simulated)" << std::endl;
}
#endif

void TlsNetwork::broadcast(const std::vector<std::string>& addresses, const Message& message) {
    for (const auto& addr : addresses) send(addr, message);
}

void TlsNetwork::on_receive(std::function<void(const Message&, const std::string&)> callback) {
    receive_callback_ = std::move(callback);
}

#ifndef USE_INTERNAL_MOCKS
void TlsNetwork::start_accept() {
    auto socket = std::make_shared<tcp::socket>(io_context_);
    acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& ec) {
        if (!ec) {
            auto stream = std::make_shared<ssl::stream<tcp::socket>>(std::move(*socket), ssl_context_);
            stream->async_handshake(ssl::stream_base::server, [this, stream](const boost::system::error_code& ec) {
                if (!ec) handle_receive(stream);
                start_accept();
            });
        } else start_accept();
    });
}

void TlsNetwork::handle_receive(std::shared_ptr<ssl::stream<tcp::socket>> stream) {
    auto message = std::make_shared<Message>(4096);
    stream->async_read_some(asio::buffer(*message), [this, stream, message](const boost::system::error_code& ec, std::size_t length) {
        if (!ec) {
            message->resize(length);
            if (receive_callback_) receive_callback_(*message, stream->lowest_layer().remote_endpoint().address().to_string());
            handle_receive(stream);
        }
    });
}
#endif

} // namespace narwhal::network
