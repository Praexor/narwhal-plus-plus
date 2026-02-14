#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <memory>
#include <functional>
#include <unordered_map>
#include <queue>
#include <mutex>
#include "narwhal/consensus.hpp"

namespace narwhal::network {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;
namespace ssl = asio::ssl;

/**
 * @brief Message types for the Narwhal protocol
 */
enum class MessageType : uint8_t {
    CERTIFICATE = 0x01,
    BATCH = 0x02,
    VOTE = 0x03,
    SYNC_REQUEST = 0x04,
    SYNC_RESPONSE = 0x05
};

/**
 * @brief Wire protocol message header
 * 
 * Format: [magic:4][version:1][type:1][length:4] = 10 bytes
 */
struct MessageHeader {
    static constexpr uint32_t MAGIC = 0x4E415257; // "NARW"
    static constexpr uint8_t VERSION = 0x01;
    
    uint32_t magic;
    uint8_t version;
    MessageType type;
    uint32_t length;
    
    std::vector<uint8_t> serialize() const;
    static MessageHeader deserialize(const std::vector<uint8_t>& data);
};

/**
 * @brief Async network connection to a peer
 */
class Connection : public std::enable_shared_from_this<Connection> {
public:
    using MessageHandler = std::function<void(MessageType, std::vector<uint8_t>)>;
    
    Connection(asio::io_context& io_context, ssl::context& ssl_context);
    
    tcp::socket& socket() { return socket_.lowest_layer(); }
    
    void start(MessageHandler handler);
    void send(MessageType type, const std::vector<uint8_t>& payload);
    void close();
    
private:
    void do_handshake();
    void do_read_header();
    void do_read_body(const MessageHeader& header);
    void do_write();
    
    ssl::stream<tcp::socket> socket_;
    MessageHandler message_handler_;
    
    std::vector<uint8_t> read_buffer_;
    std::queue<std::vector<uint8_t>> write_queue_;
    std::mutex write_mutex_;
};

/**
 * @brief Async network manager for Narwhal
 * 
 * Architecture:
 * - Single io_context with thread pool (configurable size)
 * - TLS 1.3 enforced for all connections
 * - Automatic reconnection with exponential backoff
 * - Connection pooling and peer management
 */
class AsyncNetwork {
public:
    using CertificateHandler = std::function<void(const consensus::Certificate&)>;
    
    struct Config {
        uint16_t listen_port;
        std::string cert_file;
        std::string key_file;
        size_t io_threads = 4;
        size_t max_connections = 100;
        std::chrono::seconds reconnect_interval{5};
    };
    
    explicit AsyncNetwork(const Config& config);
    ~AsyncNetwork();
    
    // Start the network layer (non-blocking)
    void start();
    
    // Stop the network layer (blocking until all connections close)
    void stop();
    
    // Send a certificate to a specific peer
    void send_certificate(const std::string& peer_address, 
                         const consensus::Certificate& cert);
    
    // Broadcast a certificate to all known peers
    void broadcast_certificate(const consensus::Certificate& cert);
    
    // Register handler for incoming certificates
    void on_certificate(CertificateHandler handler);
    
    // Add a peer to the known peers list
    void add_peer(const std::string& address);
    
    // Get network statistics
    struct Stats {
        size_t active_connections;
        size_t messages_sent;
        size_t messages_received;
        size_t bytes_sent;
        size_t bytes_received;
    };
    Stats get_stats() const;
    
private:
    void do_accept();
    void connect_to_peer(const std::string& address);
    void handle_message(const std::string& peer, MessageType type, 
                       const std::vector<uint8_t>& data);
    
    Config config_;
    asio::io_context io_context_;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
    ssl::context ssl_context_;
    tcp::acceptor acceptor_;
    
    std::vector<std::thread> io_threads_;
    std::unordered_map<std::string, std::shared_ptr<Connection>> connections_;
    std::mutex connections_mutex_;
    
    CertificateHandler certificate_handler_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    Stats stats_;
};

} // namespace narwhal::network
