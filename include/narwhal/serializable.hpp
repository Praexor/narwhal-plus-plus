#pragma once

#include <vector>
#include <cstdint>

namespace narwhal::utils {

class Serializable {
public:
    virtual ~Serializable() = default;
    virtual std::vector<uint8_t> serialize() const = 0;
};

// Helper to pack data into bytes
struct Packer {
    static void pack_u64(std::vector<uint8_t>& buf, uint64_t val) {
        for (int i = 0; i < 8; ++i) {
            buf.push_back((val >> (i * 8)) & 0xFF);
        }
    }

    static void pack_bytes(std::vector<uint8_t>& buf, const uint8_t* val, size_t size) {
        buf.insert(buf.end(), val, val + size);
    }

    static void pack_vector_bytes(std::vector<uint8_t>& buf, const std::vector<uint8_t>& val) {
        pack_u64(buf, val.size());
        pack_bytes(buf, val.data(), val.size());
    }
};

} // namespace narwhal::utils
