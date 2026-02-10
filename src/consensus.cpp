#include "narwhal/consensus.hpp"
#include <algorithm>
#include <iostream>
#include <unordered_set>

namespace narwhal::consensus {

// --- Serializable Implementations ---

std::vector<uint8_t> Header::serialize() const {
    std::vector<uint8_t> buf;
    utils::Packer::pack_bytes(buf, author.data(), author.size());
    utils::Packer::pack_u64(buf, round);
    utils::Packer::pack_u64(buf, parents.size());
    for (const auto& p : parents) {
        utils::Packer::pack_bytes(buf, p.data(), p.size());
    }
    utils::Packer::pack_u64(buf, payload.size());
    for (const auto& p : payload) {
        utils::Packer::pack_bytes(buf, p.first.data(), p.first.size());
        utils::Packer::pack_u64(buf, p.second);
    }
    return buf;
}

std::vector<uint8_t> Certificate::serialize() const {
    std::vector<uint8_t> buf = header.serialize();
    utils::Packer::pack_u64(buf, votes.size());
    for (const auto& v : votes) {
        utils::Packer::pack_bytes(buf, v.first.data(), v.first.size());
        utils::Packer::pack_bytes(buf, v.second.data(), v.second.size());
    }
    return buf;
}

crypto::Digest Certificate::digest() const {
    return crypto::Hash::compute(header.serialize());
}

// --- State Implementation ---

State::State(const std::vector<Certificate>& genesis_certs) {
    last_committed_round = 0;
    std::unordered_map<crypto::PublicKey, std::pair<crypto::Digest, Certificate>> genesis_map;
    
    for (const auto& cert : genesis_certs) {
        crypto::PublicKey origin = cert.origin();
        genesis_map[origin] = {cert.digest(), cert};
        last_committed[origin] = cert.round();
    }
    
    dag[0] = std::move(genesis_map);
}

void State::update(const Certificate& certificate, Round gc_depth) {
    crypto::PublicKey origin = certificate.origin();
    last_committed[origin] = std::max(last_committed[origin], certificate.round());

    Round max_round = 0;
    for (const auto& p : last_committed) {
        max_round = std::max(max_round, p.second);
    }
    last_committed_round = max_round;

    for (auto it = dag.begin(); it != dag.end(); ) {
        if (it->first + gc_depth < last_committed_round) {
            it = dag.erase(it);
        } else {
            ++it;
        }
    }
}

// --- Consensus Implementation ---

Consensus::Consensus(config::Committee committee, Round gc_depth,
                    std::shared_ptr<utils::Channel<Certificate>> rx,
                    std::shared_ptr<utils::Channel<Certificate>> tx_p,
                    std::shared_ptr<utils::Channel<Certificate>> tx_o,
                    std::unique_ptr<ConsensusEngine> engine)
    : committee(committee), gc_depth(gc_depth), rx_primary(rx), tx_primary(tx_p), tx_output(tx_o), engine(std::move(engine)) {}

Consensus::~Consensus() {
    running = false;
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
}

void Consensus::spawn() {
    running = true;
    worker_thread = std::thread(&Consensus::run, this);
}

void Consensus::run() {
    State state(Consensus::genesis(committee));

    while (running) {
        auto cert_opt = rx_primary->receive();
        if (!cert_opt) break;

        const Certificate& certificate = *cert_opt;
        Round round = certificate.round();

        state.dag[round][certificate.origin()] = {certificate.digest(), certificate};

        std::vector<Certificate> sequence = engine->process_round(round, state.dag, state, committee);

        for (const auto& cert : sequence) {
            tx_primary->send(cert);
            tx_output->send(cert);
            state.update(cert, gc_depth);
        }
    }
}

std::vector<Certificate> Consensus::genesis(const config::Committee& committee) {
    std::vector<Certificate> certs;
    for (const auto& p : committee.authorities) {
        Certificate cert;
        cert.header.author = p.first;
        cert.header.round = 0;
        certs.push_back(cert);
    }
    return certs;
}

// --- Tusk Engine Implementation ---

std::vector<Certificate> TuskEngine::process_round(Round round, dag_t& dag, State& state, const config::Committee& committee) {
    if (round < 4 || (round - 1) % 2 != 0) return {};

    Round r = round - 1;
    Round leader_round = r - 2;

    if (leader_round <= state.last_committed_round) return {};

    auto leader_opt = leader(leader_round, dag, committee);
    if (!leader_opt) return {};

    const auto& leader_digest = leader_opt->first;
    const auto& leader_cert = leader_opt->second;

    Stake stake = 0;
    auto rit = dag.find(r - 1);
    if (rit != dag.end()) {
        for (const auto& p : rit->second) {
            const auto& cert = p.second.second;
            for (const auto& parent : cert.header.parents) {
                if (parent == leader_digest) {
                    stake += committee.get_stake(cert.origin());
                    break;
                }
            }
        }
    }

    if (stake < committee.validity_threshold()) return {};

    auto leaders = order_leaders(leader_cert, state, dag, committee);
    std::reverse(leaders.begin(), leaders.end());

    std::vector<Certificate> sequence;
    for (const auto& l : leaders) {
        for (const auto& x : order_dag(l, state)) {
            sequence.push_back(x);
        }
    }
    return sequence;
}

std::optional<std::pair<crypto::Digest, Certificate>> TuskEngine::leader(Round round, const dag_t& dag, const config::Committee& committee) {
    auto it = dag.find(round);
    if (it == dag.end() || it->second.empty()) return std::nullopt;

    std::vector<crypto::PublicKey> keys;
    for (const auto& p : it->second) keys.push_back(p.first);
    std::sort(keys.begin(), keys.end());

    crypto::PublicKey leader_key = keys[round % keys.size()];
    auto cert_it = it->second.find(leader_key);
    if (cert_it != it->second.end()) return cert_it->second;
    return std::nullopt;
}

std::vector<Certificate> TuskEngine::order_leaders(const Certificate& leader_cert, const State& state, const dag_t& dag, const config::Committee& committee) {
    std::vector<Certificate> to_commit = {leader_cert};
    const Certificate* current = &leader_cert;

    for (Round r = current->round() - 2; r > state.last_committed_round; r -= 2) {
        auto prev = leader(r, dag, committee);
        if (!prev) continue;
        if (linked(*current, prev->second, dag)) {
            to_commit.push_back(prev->second);
            current = &to_commit.back();
        }
        if (r < 2) break;
    }
    return to_commit;
}

bool TuskEngine::linked(const Certificate& leader, const Certificate& prev_leader, const dag_t& dag) {
    std::vector<const Certificate*> current_round = {&leader};
    for (Round r = leader.round(); r > prev_leader.round(); --r) {
        auto it = dag.find(r - 1);
        if (it == dag.end()) return false;
        std::vector<const Certificate*> next_parents;
        for (const auto& p : it->second) {
            for (const auto* curr : current_round) {
                bool found = false;
                for (const auto& parent_digest : curr->header.parents) {
                    if (parent_digest == p.second.first) { found = true; break; }
                }
                if (found) {
                    next_parents.push_back(&p.second.second);
                    break;
                }
            }
        }
        current_round = std::move(next_parents);
        if (current_round.empty()) return false;
    }
    for (const auto* c : current_round) if (c->digest() == prev_leader.digest()) return true;
    return false;
}

std::vector<Certificate> TuskEngine::order_dag(const Certificate& leader, const State& state) {
    std::vector<Certificate> ordered;
    std::unordered_set<std::string> seen;
    std::vector<const Certificate*> buffer = {&leader};

    while (!buffer.empty()) {
        const Certificate* x = buffer.back();
        buffer.pop_back();
        ordered.push_back(*x);
        if (x->round() == 0) continue;
        auto it = state.dag.find(x->round() - 1);
        if (it == state.dag.end()) continue;
        for (const auto& p_digest : x->header.parents) {
            for (const auto& p : it->second) {
                if (p.second.first == p_digest) {
                    std::string hex = crypto::Hash::to_hex(p_digest);
                    bool skip = seen.count(hex) || (state.last_committed.count(p.second.second.origin()) && state.last_committed.at(p.second.second.origin()) == p.second.second.round());
                    if (!skip) {
                        buffer.push_back(&p.second.second);
                        seen.insert(hex);
                    }
                    break;
                }
            }
        }
    }
    std::sort(ordered.begin(), ordered.end(), [](const Certificate& a, const Certificate& b) { return a.round() < b.round(); });
    return ordered;
}

// --- Shoal++ Engine Implementation ---

std::vector<Certificate> ShoalPlusPlusEngine::process_round(Round round, dag_t& dag, State& state, const config::Committee& committee) {
    Round leader_round = round - 1; 
    if (leader_round <= state.last_committed_round) return {};

    auto anchor_opt = select_anchor(leader_round, dag, committee);
    if (!anchor_opt) return {};

    const auto& anchor_digest = anchor_opt->first;
    const auto& anchor_cert = anchor_opt->second;

    Stake stake = 0;
    auto rit = dag.find(round); 
    if (rit != dag.end()) {
        for (const auto& p : rit->second) {
            for (const auto& parent : p.second.second.header.parents) {
                if (parent == anchor_digest) {
                    stake += committee.get_stake(p.second.second.origin());
                    break;
                }
            }
        }
    }

    if (stake < committee.validity_threshold()) return {};

    std::vector<Certificate> sequence;
    sequence.push_back(anchor_cert);
    
    update_reputation(sequence);
    return sequence;
}

std::optional<std::pair<crypto::Digest, Certificate>> ShoalPlusPlusEngine::select_anchor(Round round, const dag_t& dag, const config::Committee& committee) {
    auto it = dag.find(round);
    if (it == dag.end() || it->second.empty()) return std::nullopt;

    std::vector<crypto::PublicKey> candidates;
    for (const auto& p : it->second) candidates.push_back(p.first);
    std::sort(candidates.begin(), candidates.end());

    uint64_t total_reputation = 0;
    for (const auto& pk : candidates) total_reputation += reputation[pk] + 1;
    
    uint64_t choice = round % total_reputation;
    uint64_t current = 0;
    for (const auto& pk : candidates) {
        current += reputation[pk] + 1;
        if (current > choice) {
            auto cert_it = it->second.find(pk);
            return cert_it->second;
        }
    }

    return std::nullopt;
}

void ShoalPlusPlusEngine::update_reputation(const std::vector<Certificate>& committed) {
    for (const auto& cert : committed) {
        reputation[cert.origin()]++;
    }
    
    if (reputation.size() > 100) {
        for (auto& p : reputation) p.second /= 2;
    }
}

// --- Mysticeti Engine Implementation ---

std::vector<Certificate> MysticetiEngine::process_round(Round round, dag_t& dag, State& state, const config::Committee& committee) {
    if (round < 3) return {};

    Round leader_round = round - 2;
    if (leader_round <= state.last_committed_round) return {};

    auto leader_pk_opt = get_leader(leader_round, committee);
    if (!leader_pk_opt) return {};
    auto leader_pk = *leader_pk_opt;

    auto lit = dag.find(leader_round);
    if (lit == dag.end() || lit->second.find(leader_pk) == lit->second.end()) return {};

    const auto& leader_digest = lit->second.at(leader_pk).first;
    const auto& leader_cert = lit->second.at(leader_pk).second;

    Stake votes = 0;
    auto rit = dag.find(leader_round + 1);
    if (rit != dag.end()) {
        for (const auto& p : rit->second) {
            const auto& cert = p.second.second;
            for (const auto& parent_digest : cert.header.parents) {
                if (parent_digest == leader_digest) {
                    votes += committee.get_stake(cert.origin());
                    break;
                }
            }
        }
    }

    if (votes < committee.validity_threshold()) return {};

    std::cout << "[Mysticeti] Leader at round " << leader_round << " committed with " << votes << " votes." << std::endl;
    return {leader_cert};
}

std::optional<crypto::PublicKey> MysticetiEngine::get_leader(Round round, const config::Committee& committee) {
    std::vector<crypto::PublicKey> keys;
    for (const auto& p : committee.authorities) keys.push_back(p.first);
    std::sort(keys.begin(), keys.end());
    if (keys.empty()) return std::nullopt;
    return keys[round % keys.size()];
}

bool MysticetiEngine::can_commit(Round round, const dag_t& dag, const config::Committee& committee) {
    return true; 
}

} // namespace narwhal::consensus
