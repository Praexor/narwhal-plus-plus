#pragma once

#include "narwhal/crypto.hpp"
#include <map>
#include <string>
#include <vector>

namespace narwhal::config {

using Stake = uint32_t;

struct Authority {
    Stake stake;
    std::string primary_address;
    std::string worker_address;
};

class Committee {
public:
    std::map<crypto::PublicKey, Authority> authorities;

    Committee() = default;

    Stake total_stake() const {
        Stake total = 0;
        for (const auto& [_, auth] : authorities) {
            total += auth.stake;
        }
        return total;
    }

    Stake quorum_threshold() const {
        return (total_stake() * 2) / 3 + 1;
    }

    Stake validity_threshold() const {
        return (total_stake() - 1) / 3 + 1;
    }

    size_t size() const {
        return authorities.size();
    }

    Stake get_stake(const crypto::PublicKey& name) const {
        auto it = authorities.find(name);
        if (it != authorities.end()) {
            return it->second.stake;
        }
        return 0;
    }
};

} // namespace narwhal::config
