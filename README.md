# Narwhal++ (narwhal-cpp)

A high-performance C++20 port of the **Narwhal** and **Tusk** DAG-based mempool and consensus engine, featuring modular support for **Shoal++** and **Mysticeti**.

This project is designed as a modular, high-fidelity implementation of state-of-the-art consensus protocols, focusing on low latency and horizontal scalability.

## 🚀 Key Features

- **Modular Consensus Engines**: Switch seamlessly between:
  - **Tusk**: High-throughput DAG-based consensus.
  - **Shoal++**: Advanced anchor selection with reputation systems.
  - **Mysticeti**: Ultra-low latency "3-hop" consensus path.
- **DAG-based Architecture**: Decouples data availability from transaction ordering.
- **High-Performance C++20**: Utilizing modern C++ features for maximum efficiency.
- **Windows-Friendly**: Integrated PowerShell scripts for local cluster simulation and benchmarking.
- **Pluggable Backend**: Support for both production-ready dependencies (RocksDB, Sodium, Boost.Asio) and internal mocks for rapid testing/CI.

## 🛠 Tech Stack

- **Core**: C++20
- **Build System**: CMake 3.20+
- **Asynchronous I/O**: Boost.Asio
- **Storage**: RocksDB
- **Cryptography**: Libsodium (Ed25519)
- **Networking**: TLS 1.3 / Noise Protocol

## 🏁 Getting Started

### Prerequisites

- A modern C++ compiler (MSVC 2022+ or GCC 11+)
- CMake 3.20 or higher

### Build

```powershell
# Create build directory and configure
cmake -B build -S .

# Build in Release mode
cmake --build build --config Release
```

### Run a Local Cluster

Launch a 4-node local cluster with the Mysticeti engine:

```powershell
./run_cluster.ps1 -Engine mysticeti -Load
```

Possible engines: `tusk`, `shoal++`, `mysticeti`.

## 📊 Comparison summary

| Feature | Tusk | Shoal++ | Mysticeti |
| :--- | :--- | :--- | :--- |
| **Commit Path** | 2-rounds | 2-rounds (Anchor) | 1-round (Fast path) |
| **Latency** | Medium | Medium-Low | Ultra-Low |
| **Throughput** | High | Very High | Very High |
| **Reputation** | No | Yes | No |

## 🗺 Roadmap

- [x] Core DAG Logic & Tusk Implementation
- [x] Shoal++ & Mysticeti engines
- [ ] **Phase 1**: Infrastructure Professionalization (Asynchronous Multi-threaded Networking)
- [ ] **Phase 2**: Advanced BFT Liveness (Pacemaker & Timeout Management)
- [ ] **Phase 3**: Scalable Data Availability (Separate Worker-Layer)
- [ ] **Phase 4**: Hardening (Byzantine Simulation & Fuzzing)

## 📄 License

This project is licensed under the MIT License.
