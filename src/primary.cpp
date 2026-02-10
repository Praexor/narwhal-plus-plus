#include "narwhal/consensus.hpp"
#include "narwhal/network.hpp"
#include "narwhal/store.hpp"
#include "narwhal/config.hpp"
#include "narwhal/common.hpp"
#include <iostream>
#include <memory>
#include <string>

#ifndef USE_INTERNAL_MOCKS
#include <boost/asio.hpp>
#endif

using namespace narwhal;

int main(int argc, char* argv[]) {
    uint16_t port = 8000;
    std::string db_path = "./db_primary";
    std::string engine_type = "tusk";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--db" && i + 1 < argc) {
            db_path = argv[++i];
        } else if (arg == "--engine" && i + 1 < argc) {
            engine_type = argv[++i];
        }
    }

    std::cout << "Starting Narwhal Primary Node (" 
#ifdef USE_INTERNAL_MOCKS
              << "MOCK MODE"
#else
              << "FULL MODE"
#endif
              << ") on port " << port << " with engine " << engine_type << "..." << std::endl;

#ifndef USE_INTERNAL_MOCKS
    boost::asio::io_context io_context;
#else
    int io_context = 0;
#endif

    config::Committee committee;
    for (int i = 0; i < 4; i++) {
        crypto::PublicKey pk = {0};
        pk[0] = i; // Unique key for each node
        committee.authorities[pk] = {100, "127.0.0.1:" + std::to_string(8000 + i), "127.0.0.1:" + std::to_string(9000 + i)};
    }

    auto rx_primary = std::make_shared<utils::Channel<consensus::Certificate>>();
    auto tx_primary = std::make_shared<utils::Channel<consensus::Certificate>>();
    auto tx_output = std::make_shared<utils::Channel<consensus::Certificate>>();

    store::Store store(db_path);
    network::TlsNetwork network(io_context, port, "cert.pem", "key.pem");

    // Modular Engine Selection
    std::unique_ptr<consensus::ConsensusEngine> engine;
    if (engine_type == "shoal++") {
        engine = std::make_unique<consensus::ShoalPlusPlusEngine>();
    } else if (engine_type == "mysticeti") {
        engine = std::make_unique<consensus::MysticetiEngine>();
    } else {
        engine = std::make_unique<consensus::TuskEngine>();
    }

    consensus::Consensus consensus(committee, 50, rx_primary, tx_primary, tx_output, std::move(engine));

    consensus.spawn();

    // Load Generator for Benchmarking
    bool load_enabled = false;
    for (int i = 1; i < argc; ++i) if (std::string(argv[i]) == "--load") load_enabled = true;

    std::thread load_gen;
    if (load_enabled) {
        load_gen = std::thread([rx_primary, committee]() {
            uint64_t round = 1;
            std::vector<crypto::Digest> previous_round_digests;
            
            // Genesis digests (round 0)
            for (const auto& [pk, auth] : committee.authorities) {
                consensus::Certificate genesis_cert;
                genesis_cert.header.author = pk;
                genesis_cert.header.round = 0;
                previous_round_digests.push_back(genesis_cert.digest());
            }

            while (true) {
                std::vector<crypto::Digest> current_round_digests;
                for (const auto& [pk, auth] : committee.authorities) {
                    consensus::Certificate cert;
                    cert.header.author = pk;
                    cert.header.round = round;
                    cert.header.parents = previous_round_digests;

                    rx_primary->send(cert);
                    current_round_digests.push_back(cert.digest());
                }
                previous_round_digests = std::move(current_round_digests);
                round++;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    }

    auto start_time = std::chrono::steady_clock::now();
    uint64_t commit_count = 0;

    while (true) {
        auto committed = tx_output->receive();
        if (committed) {
            commit_count++;
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
            
            if (elapsed >= 5) { // Log every 5 seconds
                std::cout << "[" << engine_type << "] Perf: " << commit_count / (double)elapsed 
                          << " certificates/sec (Total: " << commit_count << ")" << std::endl;
                // We don't reset start_time/count to get global average, or we could for sliding window.
            }

            if (commit_count % 10 == 0) { // Still log some individual commits but less spammy
                std::cout << "[" << engine_type << "] Committed Round " << committed->round() << std::endl;
            }
        } else break;
    }

#ifndef USE_INTERNAL_MOCKS
    if (io_thread.joinable()) io_thread.join();
#endif

    return 0;
}
