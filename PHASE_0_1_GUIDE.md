# Phase 0 & Phase 1 Implementation Guide

This document provides a roadmap for implementing the foundational verification and infrastructure layers of Narwhal++.

## 📋 Phase 0: Formal Verification Foundation

### Objectives
- Establish formal specifications for consensus safety and liveness
- Implement property-based testing infrastructure
- Set up continuous verification in CI/CD

### Tasks

#### 1. Complete TLA+ Specifications
- [x] Create Mysticeti skeleton (`formal/tla/Mysticeti.tla`)
- [ ] Add full DAG construction logic
- [ ] Model vote aggregation and quorum checks
- [ ] Add Byzantine behavior models
- [ ] Create Shoal++ specification
- [ ] Create Tusk specification

**How to verify:**
```bash
cd formal/tla
# Download TLA+ tools from https://github.com/tlaplus/tlaplus/releases
java -jar tla2tools.jar -config Mysticeti.cfg Mysticeti.tla
```

#### 2. Property-Based Testing with RapidCheck
- [x] Set up RapidCheck infrastructure (`tests/property_tests.cpp`)
- [x] Implement basic property tests:
  - Certificate digest determinism
  - Round monotonicity
  - No equivocation
  - Quorum intersection
  - Serialization round-trip
- [ ] Add advanced properties:
  - Causal ordering preservation
  - Byzantine fault tolerance (up to f nodes)
  - Liveness under network delays

**How to build and run:**
```bash
# Install RapidCheck
git clone https://github.com/emil-e/rapidcheck.git
cd rapidcheck && mkdir build && cd build
cmake .. && make && sudo make install

# Build Narwhal++ with property tests
cmake -B build -S . -DBUILD_PROPERTY_TESTS=ON
cmake --build build --config Release
./build/property_tests
```

#### 3. CI/CD Integration
- [ ] Add GitHub Actions workflow for TLA+ model checking
- [ ] Add property test runs to CI pipeline
- [ ] Set up coverage reporting for property tests
- [ ] Create nightly fuzzing jobs

---

## 🚀 Phase 1: Infrastructure Professionalization

### Objectives
- Replace mocks with production-grade implementations
- Add observability and metrics
- Implement async networking with Boost.Asio

### Tasks

#### 1. Async Network Layer
- [x] Design architecture (`include/narwhal/async_network.hpp`)
- [x] Implement core networking (`src/async_network.cpp`)
- [ ] Add connection pooling and peer discovery
- [ ] Implement automatic reconnection with exponential backoff
- [ ] Add network partition simulation for testing
- [ ] Performance benchmarking (compare with mock)

**Key Features:**
- TLS 1.3 enforcement
- Thread pool for I/O operations
- Message framing with length-prefixed protocol
- Graceful shutdown and connection cleanup

**Integration:**
```cpp
#include "narwhal/async_network.hpp"

narwhal::network::AsyncNetwork::Config config{
    .listen_port = 8000,
    .cert_file = "cert.pem",
    .key_file = "key.pem",
    .io_threads = 4
};

auto network = std::make_shared<narwhal::network::AsyncNetwork>(config);
network->on_certificate([](const auto& cert) {
    // Handle incoming certificate
});
network->start();
```

#### 2. Real Cryptography (Ed25519)
- [ ] Integrate libsodium for Ed25519 signatures
- [ ] Implement batch signature verification
- [ ] Add signature caching to avoid redundant verification
- [ ] Benchmark: single vs batch verification

**Expected Performance:**
- Single verification: ~50,000 sigs/sec
- Batch verification (100 sigs): ~200,000 sigs/sec

#### 3. Persistent Storage (RocksDB)
- [ ] Replace in-memory maps with RocksDB
- [ ] Implement Write-Ahead Log (WAL) for crash recovery
- [ ] Add column families for different data types:
  - `certificates`: Certificate storage
  - `dag`: DAG structure
  - `state`: Consensus state
- [ ] Implement garbage collection for old rounds
- [ ] Add snapshot/restore functionality

**Schema Design:**
```
Column Family: certificates
  Key: digest (32 bytes)
  Value: serialized Certificate

Column Family: dag
  Key: round (8 bytes) || author (32 bytes)
  Value: digest (32 bytes)

Column Family: state
  Key: "last_committed_round"
  Value: round number (8 bytes)
```

#### 4. Observability & Metrics
- [ ] Integrate Prometheus C++ client
- [ ] Add key metrics:
  - `narwhal_certificates_committed_total`
  - `narwhal_consensus_latency_seconds`
  - `narwhal_network_bytes_sent_total`
  - `narwhal_dag_size_rounds`
- [ ] Create Grafana dashboard templates
- [ ] Add OpenTelemetry distributed tracing
- [ ] Implement structured logging (spdlog)

**Metrics Endpoint:**
```cpp
// Expose metrics on /metrics endpoint
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

prometheus::Exposer exposer{"127.0.0.1:9090"};
auto registry = std::make_shared<prometheus::Registry>();
exposer.RegisterCollectable(registry);
```

---

## 🧪 Testing Strategy

### Unit Tests
- Test each component in isolation
- Mock dependencies where necessary
- Aim for >80% code coverage

### Integration Tests
- Test full consensus flow with real networking
- Simulate Byzantine nodes
- Test crash recovery with RocksDB

### Property Tests (RapidCheck)
- Run 10,000+ random test cases per property
- Shrink failing cases to minimal reproducers
- Add regression tests for discovered bugs

### Performance Tests
- Benchmark throughput (certificates/sec)
- Measure latency (p50, p95, p99)
- Compare mock vs real implementations

---

## 📊 Success Criteria

### Phase 0
- [ ] TLA+ model checker finds no violations for N=4, F=1, MaxRound=10
- [ ] All property tests pass with 10,000 iterations
- [ ] CI pipeline runs verification on every commit

### Phase 1
- [ ] Async network achieves >10,000 certs/sec with 4 nodes
- [ ] Ed25519 batch verification implemented and benchmarked
- [ ] RocksDB integration with WAL and crash recovery
- [ ] Prometheus metrics exposed and Grafana dashboard created
- [ ] OpenTelemetry traces show end-to-end request flow

---

## 🔗 Dependencies

### Required Libraries
- **Boost.Asio**: Async I/O and networking
- **OpenSSL**: TLS 1.3 support
- **libsodium**: Ed25519 signatures
- **RocksDB**: Persistent storage
- **RapidCheck**: Property-based testing
- **Prometheus C++ Client**: Metrics
- **OpenTelemetry C++**: Distributed tracing

### Installation (Ubuntu/Debian)
```bash
sudo apt-get install -y \
    libboost-all-dev \
    libssl-dev \
    libsodium-dev \
    librocksdb-dev

# RapidCheck (from source)
git clone https://github.com/emil-e/rapidcheck.git
cd rapidcheck && mkdir build && cd build
cmake .. && make && sudo make install

# Prometheus C++ (from source)
git clone https://github.com/jupp0r/prometheus-cpp.git
cd prometheus-cpp && mkdir build && cd build
cmake .. && make && sudo make install
```

---

## 📅 Timeline Estimate

| Phase | Task | Estimated Time |
|-------|------|----------------|
| 0 | Complete TLA+ specs | 2-3 weeks |
| 0 | Property tests | 1 week |
| 0 | CI/CD integration | 3-5 days |
| 1 | Async networking | 2 weeks |
| 1 | Real crypto | 1 week |
| 1 | RocksDB + WAL | 1-2 weeks |
| 1 | Observability | 1 week |

**Total: ~8-10 weeks for Phase 0 + Phase 1**

---

## 🎯 Next Steps

1. **Immediate**: Complete Mysticeti TLA+ specification
2. **Short-term**: Implement async network layer
3. **Medium-term**: Integrate RocksDB and real crypto
4. **Long-term**: Full observability stack

For questions or contributions, see the main [README.md](../README.md).
