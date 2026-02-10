#include "narwhal/crypto.hpp"
#include "narwhal/common.hpp"

#ifdef USE_INTERNAL_MOCKS
#include <iostream>
#else
#include <sodium.h>
#endif
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace narwhal::crypto {

#ifndef USE_INTERNAL_MOCKS
struct SodiumInit {
    SodiumInit() {
        if (sodium_init() < 0) {
            throw std::runtime_error("libsodium initialization failed");
        }
    }
};
static SodiumInit global_init;
#endif

Signature Ed25519::sign(const std::vector<uint8_t>& message, const std::vector<uint8_t>& secret_key) {
#ifdef USE_INTERNAL_MOCKS
    Signature sig = {0};
    return sig;
#else
    if (secret_key.size() != crypto_sign_SECRETKEYBYTES) {
        throw std::invalid_argument("Invalid secret key size");
    }
    Signature sig;
    crypto_sign_detached(sig.data(), nullptr, message.data(), message.size(), secret_key.data());
    return sig;
#endif
}

bool Ed25519::verify(const std::vector<uint8_t>& message, const Signature& signature, const PublicKey& public_key) {
#ifdef USE_INTERNAL_MOCKS
    return true;
#else
    if (public_key.size() != crypto_sign_PUBLICKEYBYTES) {
        return false;
    }
    return crypto_sign_verify_detached(signature.data(), message.data(), message.size(), public_key.data()) == 0;
#endif
}

Digest Hash::compute(const std::vector<uint8_t>& data) {
    Digest digest = {0};
#ifdef USE_INTERNAL_MOCKS
    // Simple XOR mock hash
    for (size_t i = 0; i < data.size(); ++i) {
        digest[i % 32] ^= data[i];
    }
#else
    crypto_generichash(digest.data(), digest.size(), data.data(), data.size(), nullptr, 0);
#endif
    return digest;
}

std::string Hash::to_hex(const Digest& digest) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (auto byte : digest) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

} // namespace narwhal::crypto
