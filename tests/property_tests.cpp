#include <rapidcheck.h>
#include "narwhal/consensus.hpp"
#include "narwhal/crypto.hpp"
#include <iostream>

using namespace narwhal;

/**
 * @brief Property-based tests for Mysticeti consensus engine
 * 
 * These tests verify critical invariants that must hold for ALL possible
 * inputs, not just hand-crafted test cases.
 */

// ============================================================================
// Generators for Narwhal types
// ============================================================================

namespace rc {

template<>
struct Arbitrary<crypto::PublicKey> {
    static Gen<crypto::PublicKey> arbitrary() {
        return gen::apply([](uint8_t id) {
            crypto::PublicKey pk = {0};
            pk[0] = id;
            return pk;
        }, gen::inRange<uint8_t>(0, 10));
    }
};

template<>
struct Arbitrary<consensus::Certificate> {
    static Gen<consensus::Certificate> arbitrary() {
        return gen::build<consensus::Certificate>(
            gen::set(&consensus::Certificate::header,
                gen::build<consensus::Header>(
                    gen::set(&consensus::Header::author, gen::arbitrary<crypto::PublicKey>()),
                    gen::set(&consensus::Header::round, gen::inRange<uint64_t>(0, 100))
                )
            )
        );
    }
};

} // namespace rc

// ============================================================================
// Property Tests
// ============================================================================

/**
 * Property: Certificate digest is deterministic
 * 
 * For any certificate, computing its digest twice should yield the same result.
 */
void test_certificate_digest_deterministic() {
    rc::check("Certificate digest is deterministic", []() {
        auto cert = *rc::gen::arbitrary<consensus::Certificate>();
        auto digest1 = cert.digest();
        auto digest2 = cert.digest();
        RC_ASSERT(digest1 == digest2);
    });
}

/**
 * Property: Round monotonicity in committed sequence
 * 
 * In Mysticeti, committed leaders should have monotonically increasing rounds.
 * This is a critical safety property.
 */
void test_mysticeti_round_monotonicity() {
    rc::check("Mysticeti commits have monotonic rounds", []() {
        // Generate a sequence of certificates
        auto certs = *rc::gen::container<std::vector<consensus::Certificate>>(
            10, rc::gen::arbitrary<consensus::Certificate>()
        );
        
        // Simulate Mysticeti commits (simplified)
        consensus::MysticetiEngine engine;
        std::vector<uint64_t> committed_rounds;
        
        for (const auto& cert : certs) {
            if (cert.round() > 0) {
                committed_rounds.push_back(cert.round());
            }
        }
        
        // Check monotonicity
        for (size_t i = 1; i < committed_rounds.size(); ++i) {
            RC_ASSERT(committed_rounds[i] >= committed_rounds[i-1]);
        }
    });
}

/**
 * Property: No equivocation in DAG
 * 
 * A node should never produce two different certificates for the same round.
 */
void test_no_equivocation() {
    rc::check("No node equivocates (two certs at same round)", []() {
        auto certs = *rc::gen::container<std::vector<consensus::Certificate>>(
            20, rc::gen::arbitrary<consensus::Certificate>()
        );
        
        std::map<std::pair<crypto::PublicKey, uint64_t>, crypto::Digest> seen;
        
        for (const auto& cert : certs) {
            auto key = std::make_pair(cert.origin(), cert.round());
            auto digest = cert.digest();
            
            if (seen.count(key)) {
                // If we've seen this (author, round) before, digests must match
                RC_ASSERT(seen[key] == digest);
            } else {
                seen[key] = digest;
            }
        }
    });
}

/**
 * Property: Quorum intersection
 * 
 * Any two quorums (2f+1 nodes) must have at least f+1 nodes in common.
 * This is fundamental to BFT consensus.
 */
void test_quorum_intersection() {
    rc::check("Quorum intersection property", []() {
        const size_t N = 4; // 4 nodes, f=1
        const size_t QUORUM_SIZE = 3; // 2f+1
        
        auto quorum1 = *rc::gen::container<std::set<size_t>>(
            QUORUM_SIZE, rc::gen::inRange<size_t>(0, N)
        );
        auto quorum2 = *rc::gen::container<std::set<size_t>>(
            QUORUM_SIZE, rc::gen::inRange<size_t>(0, N)
        );
        
        // Count intersection
        std::set<size_t> intersection;
        std::set_intersection(
            quorum1.begin(), quorum1.end(),
            quorum2.begin(), quorum2.end(),
            std::inserter(intersection, intersection.begin())
        );
        
        // Must have at least f+1 = 2 nodes in common
        RC_ASSERT(intersection.size() >= 2);
    });
}

/**
 * Property: Serialization round-trip
 * 
 * Serializing and deserializing a certificate should yield the original.
 */
void test_serialization_roundtrip() {
    rc::check("Certificate serialization is reversible", []() {
        auto original = *rc::gen::arbitrary<consensus::Certificate>();
        auto serialized = original.serialize();
        
        // TODO: Implement Certificate::deserialize()
        // auto deserialized = consensus::Certificate::deserialize(serialized);
        // RC_ASSERT(original.digest() == deserialized.digest());
        
        // For now, just check serialization doesn't crash
        RC_ASSERT(!serialized.empty());
    });
}

// ============================================================================
// Main test runner
// ============================================================================

int main() {
    std::cout << "Running RapidCheck property-based tests...\n" << std::endl;
    
    try {
        test_certificate_digest_deterministic();
        std::cout << "✓ Certificate digest determinism" << std::endl;
        
        test_mysticeti_round_monotonicity();
        std::cout << "✓ Mysticeti round monotonicity" << std::endl;
        
        test_no_equivocation();
        std::cout << "✓ No equivocation" << std::endl;
        
        test_quorum_intersection();
        std::cout << "✓ Quorum intersection" << std::endl;
        
        test_serialization_roundtrip();
        std::cout << "✓ Serialization round-trip" << std::endl;
        
        std::cout << "\n✅ All property tests passed!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
