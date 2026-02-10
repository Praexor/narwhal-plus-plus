#pragma once

#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <functional>

namespace narwhal::crypto {

using Digest = std::array<uint8_t, 32>;
using PublicKey = std::array<uint8_t, 32>;
using Signature = std::array<uint8_t, 64>;

class HashInterface {
public:
    virtual ~HashInterface() = default;
    virtual Digest hash(const std::vector<uint8_t>& data) const = 0;
};

class Ed25519 {
public:
    static Signature sign(const std::vector<uint8_t>& message, const std::vector<uint8_t>& secret_key);
    static bool verify(const std::vector<uint8_t>& message, const Signature& signature, const PublicKey& public_key);
};

struct Hash {
    static Digest compute(const std::vector<uint8_t>& data);
    static std::string to_hex(const Digest& digest);
};

} // namespace narwhal::crypto

// Hash specialization for std::array (needed for unordered_map)
namespace std {
    template<size_t N>
    struct hash<std::array<uint8_t, N>> {
        size_t operator()(const std::array<uint8_t, N>& a) const {
            size_t h = 0;
            for (auto x : a) {
                h ^= std::hash<uint8_t>{}(x) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
            return h;
        }
    };
}
