#pragma once

#include "narwhal/crypto.hpp"
#include "narwhal/config.hpp"
#include "narwhal/serializable.hpp"
#include <vector>
#include <map>
#include <unordered_map>
#include <optional>

namespace narwhal::consensus {

using Round = uint64_t;
using Stake = config::Stake;

struct Header : public utils::Serializable {
    crypto::PublicKey author;
    Round round;
    std::vector<crypto::Digest> parents;
    std::unordered_map<crypto::Digest, uint32_t> payload;

    std::vector<uint8_t> serialize() const override;
};

struct Certificate : public utils::Serializable {
    Header header;
    std::vector<std::pair<crypto::PublicKey, crypto::Signature>> votes;

    crypto::Digest digest() const;
    crypto::PublicKey origin() const { return header.author; }
    Round round() const { return header.round; }

    std::vector<uint8_t> serialize() const override;
};

using dag_t = std::map<Round, std::unordered_map<crypto::PublicKey, std::pair<crypto::Digest, Certificate>>>;

struct State {
    Round last_committed_round;
    std::unordered_map<crypto::PublicKey, Round> last_committed;
    dag_t dag;

    State(const std::vector<Certificate>& genesis);
    void update(const Certificate& certificate, Round gc_depth);
};

// Abstract Consensus Engine
class ConsensusEngine {
public:
    virtual ~ConsensusEngine() = default;
    virtual std::vector<Certificate> process_round(Round round, dag_t& dag, State& state, const config::Committee& committee) = 0;
};

// Tusk Implementation (Classic)
class TuskEngine : public ConsensusEngine {
public:
    std::vector<Certificate> process_round(Round round, dag_t& dag, State& state, const config::Committee& committee) override;

private:
    std::optional<std::pair<crypto::Digest, Certificate>> leader(Round round, const dag_t& dag, const config::Committee& committee);
    std::vector<Certificate> order_leaders(const Certificate& leader, const State& state, const dag_t& dag, const config::Committee& committee);
    bool linked(const Certificate& leader, const Certificate& prev_leader, const dag_t& dag);
    std::vector<Certificate> order_dag(const Certificate& leader, const State& state);
};

// Shoal++ Implementation (High Performance)
class ShoalPlusPlusEngine : public ConsensusEngine {
public:
    std::vector<Certificate> process_round(Round round, dag_t& dag, State& state, const config::Committee& committee) override;

private:
    // Leader reputation based on past performance
    std::unordered_map<crypto::PublicKey, uint64_t> reputation;
    void update_reputation(const std::vector<Certificate>& committed);
    std::optional<std::pair<crypto::Digest, Certificate>> select_anchor(Round round, const dag_t& dag, const config::Committee& committee);
};

// Mysticeti Implementation (Next-Gen)
class MysticetiEngine : public ConsensusEngine {
public:
    std::vector<Certificate> process_round(Round round, dag_t& dag, State& state, const config::Committee& committee) override;

private:
    std::optional<crypto::PublicKey> get_leader(Round round, const config::Committee& committee);
    bool can_commit(Round round, const dag_t& dag, const config::Committee& committee);
};

} // namespace narwhal::consensus
