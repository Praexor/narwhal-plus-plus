#include "narwhal/async_network.hpp"
#include <iostream>

namespace narwhal::network {

// ============================================================================
// MessageHeader Implementation
// ============================================================================

std::vector<uint8_t> MessageHeader::serialize() const {
    std::vector<uint8_t> buffer(10);
    
    // Magic (4 bytes, big-endian)
    buffer[0] = (magic >> 24) & 0xFF;
    buffer[1] = (magic >> 16) & 0xFF;
    buffer[2] = (magic >> 8) & 0xFF;
    buffer[3] = magic & 0xFF;
    
    // Version (1 byte)
    buffer[4] = version;
    
    // Type (1 byte)
    buffer[5] = static_cast<uint8_t>(type);
    
    // Length (4 bytes, big-endian)
    buffer[6] = (length >> 24) & 0xFF;
    buffer[7] = (length >> 16) & 0xFF;
    buffer[8] = (length >> 8) & 0xFF;
    buffer[9] = length & 0xFF;
    
    return buffer;
}

MessageHeader MessageHeader::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 10) {
        throw std::runtime_error("Invalid header size");
    }
    
    MessageHeader header;
    
    header.magic = (static_cast<uint32_t>(data[0]) << 24) |
                   (static_cast<uint32_t>(data[1]) << 16) |
                   (static_cast<uint32_t>(data[2]) << 8) |
                   static_cast<uint32_t>(data[3]);
    
    if (header.magic != MAGIC) {
        throw std::runtime_error("Invalid magic number");
    }
    
    header.version = data[4];
    header.type = static_cast<MessageType>(data[5]);
    
    header.length = (static_cast<uint32_t>(data[6]) << 24) |
                    (static_cast<uint32_t>(data[7]) << 16) |
                    (static_cast<uint32_t>(data[8]) << 8) |
                    static_cast<uint32_t>(data[9]);
    
    return header;
}

// ============================================================================
// Connection Implementation
// ============================================================================

Connection::Connection(asio::io_context& io_context, ssl::context& ssl_context)
    : socket_(io_context, ssl_context) {
    read_buffer_.resize(65536); // 64KB read buffer
}

void Connection::start(MessageHandler handler) {
    message_handler_ = std::move(handler);
    do_handshake();
}

void Connection::do_handshake() {
    auto self = shared_from_this();
    socket_.async_handshake(ssl::stream_base::server,
        [this, self](const boost::system::error_code& ec) {
            if (!ec) {
                do_read_header();
            } else {
                std::cerr << "Handshake failed: " << ec.message() << std::endl;
            }
        });
}

void Connection::do_read_header() {
    auto self = shared_from_this();
    asio::async_read(socket_, asio::buffer(read_buffer_, 10),
        [this, self](const boost::system::error_code& ec, std::size_t) {
            if (!ec) {
                try {
                    auto header = MessageHeader::deserialize(read_buffer_);
                    do_read_body(header);
                } catch (const std::exception& e) {
                    std::cerr << "Header parse error: " << e.what() << std::endl;
                }
            }
        });
}

void Connection::do_read_body(const MessageHeader& header) {
    if (header.length > read_buffer_.size()) {
        read_buffer_.resize(header.length);
    }
    
    auto self = shared_from_this();
    asio::async_read(socket_, asio::buffer(read_buffer_, header.length),
        [this, self, header](const boost::system::error_code& ec, std::size_t) {
            if (!ec) {
                std::vector<uint8_t> payload(read_buffer_.begin(), 
                                            read_buffer_.begin() + header.length);
                message_handler_(header.type, std::move(payload));
                do_read_header(); // Continue reading
            }
        });
}

void Connection::send(MessageType type, const std::vector<uint8_t>& payload) {
    MessageHeader header{
        MessageHeader::MAGIC,
        MessageHeader::VERSION,
        type,
        static_cast<uint32_t>(payload.size())
    };
    
    auto header_bytes = header.serialize();
    std::vector<uint8_t> message;
    message.reserve(header_bytes.size() + payload.size());
    message.insert(message.end(), header_bytes.begin(), header_bytes.end());
    message.insert(message.end(), payload.begin(), payload.end());
    
    {
        std::lock_guard<std::mutex> lock(write_mutex_);
        bool write_in_progress = !write_queue_.empty();
        write_queue_.push(std::move(message));
        if (!write_in_progress) {
            do_write();
        }
    }
}

void Connection::do_write() {
    auto self = shared_from_this();
    asio::async_write(socket_, asio::buffer(write_queue_.front()),
        [this, self](const boost::system::error_code& ec, std::size_t) {
            if (!ec) {
                std::lock_guard<std::mutex> lock(write_mutex_);
                write_queue_.pop();
                if (!write_queue_.empty()) {
                    do_write();
                }
            }
        });
}

void Connection::close() {
    boost::system::error_code ec;
    socket_.lowest_layer().close(ec);
}

// ============================================================================
// AsyncNetwork Implementation
// ============================================================================

AsyncNetwork::AsyncNetwork(const Config& config)
    : config_(config)
    , work_guard_(asio::make_work_guard(io_context_))
    , ssl_context_(ssl::context::tlsv13_server)
    , acceptor_(io_context_, tcp::endpoint(tcp::v4(), config.listen_port))
    , stats_{} {
    
    // Configure SSL context
    ssl_context_.set_options(
        ssl::context::default_workarounds |
        ssl::context::no_sslv2 |
        ssl::context::no_sslv3 |
        ssl::context::no_tlsv1 |
        ssl::context::no_tlsv1_1 |
        ssl::context::no_tlsv1_2 |
        ssl::context::single_dh_use);
    
    ssl_context_.use_certificate_chain_file(config.cert_file);
    ssl_context_.use_private_key_file(config.key_file, ssl::context::pem);
}

AsyncNetwork::~AsyncNetwork() {
    stop();
}

void AsyncNetwork::start() {
    // Start IO threads
    for (size_t i = 0; i < config_.io_threads; ++i) {
        io_threads_.emplace_back([this]() {
            io_context_.run();
        });
    }
    
    // Start accepting connections
    do_accept();
    
    std::cout << "[AsyncNetwork] Started on port " << config_.listen_port 
              << " with " << config_.io_threads << " threads" << std::endl;
}

void AsyncNetwork::stop() {
    work_guard_.reset();
    
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for (auto& [addr, conn] : connections_) {
            conn->close();
        }
        connections_.clear();
    }
    
    io_context_.stop();
    
    for (auto& thread : io_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void AsyncNetwork::do_accept() {
    auto connection = std::make_shared<Connection>(io_context_, ssl_context_);
    
    acceptor_.async_accept(connection->socket(),
        [this, connection](const boost::system::error_code& ec) {
            if (!ec) {
                connection->start([this](MessageType type, std::vector<uint8_t> data) {
                    handle_message("", type, data);
                });
            }
            do_accept(); // Continue accepting
        });
}

void AsyncNetwork::send_certificate(const std::string& peer_address,
                                    const consensus::Certificate& cert) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = connections_.find(peer_address);
    if (it != connections_.end()) {
        auto payload = cert.serialize();
        it->second->send(MessageType::CERTIFICATE, payload);
        
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.messages_sent++;
        stats_.bytes_sent += payload.size();
    }
}

void AsyncNetwork::broadcast_certificate(const consensus::Certificate& cert) {
    auto payload = cert.serialize();
    
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (auto& [addr, conn] : connections_) {
        conn->send(MessageType::CERTIFICATE, payload);
    }
    
    std::lock_guard<std::mutex> stats_lock(stats_mutex_);
    stats_.messages_sent += connections_.size();
    stats_.bytes_sent += payload.size() * connections_.size();
}

void AsyncNetwork::on_certificate(CertificateHandler handler) {
    certificate_handler_ = std::move(handler);
}

void AsyncNetwork::handle_message(const std::string& peer, MessageType type,
                                  const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.messages_received++;
    stats_.bytes_received += data.size();
    
    if (type == MessageType::CERTIFICATE && certificate_handler_) {
        // TODO: Deserialize certificate and call handler
        // This requires implementing Certificate::deserialize()
    }
}

AsyncNetwork::Stats AsyncNetwork::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

} // namespace narwhal::network
