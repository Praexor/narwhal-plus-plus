#pragma once

#include "narwhal/consensus_engines.hpp"
#include "narwhal/utils.hpp"
#include <memory>
#include <thread>
#include <atomic>

namespace narwhal::consensus {

class Consensus {
private:
    config::Committee committee;
    Round gc_depth;
    
    std::shared_ptr<utils::Channel<Certificate>> rx_primary;
    std::shared_ptr<utils::Channel<Certificate>> tx_primary;
    std::shared_ptr<utils::Channel<Certificate>> tx_output;

    std::atomic<bool> running{false};
    std::thread worker_thread;
    
    std::unique_ptr<ConsensusEngine> engine;

public:
    Consensus(config::Committee committee, Round gc_depth,
              std::shared_ptr<utils::Channel<Certificate>> rx,
              std::shared_ptr<utils::Channel<Certificate>> tx_p,
              std::shared_ptr<utils::Channel<Certificate>> tx_o,
              std::unique_ptr<ConsensusEngine> engine = std::make_unique<TuskEngine>());
    
    ~Consensus();

    void spawn();
    void run();

    static std::vector<Certificate> genesis(const config::Committee& committee);
};

} // namespace narwhal::consensus
