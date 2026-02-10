# Narwhal++ (narwhal-cpp)

A high-performance C++20 port of the [**Narwhal and Tusk DAG-based mempool**](https://arxiv.org/abs/2105.11827) and consensus engine, featuring modular support for [**Shoal++**](https://arxiv.org/abs/2405.20488) and [**Mysticeti**](https://arxiv.org/abs/2310.14821).

This project is designed as a modular, high-fidelity implementation of state-of-the-art consensus protocols, focusing on low latency and horizontal scalability.

## 🎯 Project Goal

The primary goal of this project is to **evaluate the performance of the Narwhal++ mempool implementation in C++20** when integrated with various state-of-the-art BFT consensus engines.

By providing a high-fidelity C++ port, this repository enables a direct performance comparison between:
- **Tusk**: The baseline DAG-based consensus.
- **Shoal++**: Performance-optimized selection with reputation.
- **Mysticeti**: The latest advancement in low-latency uncertified DAG consensus.

This evaluation focuses on identifying bottlenecks, measuring throughput (certificates/sec), and analyzing latency characteristics across different consensus strategies within a unified C++20 framework.

## 🏗️ Architecture Overview

```text
       [ Clients ]
            │
            ▼
┌───────────────────────┐      ┌─────────────────────────┐
│     Narwhal Mempool   │      │    Consensus Engines    │
│  (Data Availability)  │      │  (Total Ordering)       │
├───────────────────────┤      ├─────────────────────────┤
│ ┌───┐   ┌───┐   ┌───┐ │      │  ┌───────────────┐      │
│ │ B │──▶│ B │──▶│ B │ │      │  │    Mysticeti  │      │
│ └───┘   └───┘   └───┘ │      │  └───────────────┘      │
│   ▲       ▲       ▲   │      │          ▲              │
│   │       │       │   ◀──────┼──────────┘              │
│ ┌───┐   ┌───┐   ┌───┐ │      │  ┌───────────────┐      │
│ │ C │──▶│ C │──▶│ C │ │      │  │    Shoal++    │      │
│ └───┘   └───┘   └───┘ │      │  └───────────────┘      │
│   ▲       ▲       ▲   │      │          ▲              │
│   │       │       │   ◀──────┼──────────┘              │
│ ┌───┐   ┌───┐   ┌───┐ │      │  ┌───────────────┐      │
│ │ G │──▶│ G │──▶│ G │ │      │  │      Tusk     │      │
│ └───┘   └───┘   └───┘ │      │  └───────────────┘      │
└───────────────────────┘      └─────────────────────────┘
     Shared Mempool DAG             Modular Pluggable
      (C++20 Buffers)                Logic Layers
```

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

## 📊 Benchmark Results (Local Mock Mode)

These benchmarks were performed on a local 4-node cluster using internal mocks for networking and storage to measure the maximum theoretical throughput of the consensus engines.

| Engine | Throughput (Cert/sec) | Latency (ms) | Key Mechanism |
| :--- | :--- | :--- | :--- |
| **Tusk** | ~19,800 | ~450 | DAG-link based commit |
| **Shoal++** | **~22,600** | ~320 | Adaptive Anchor selection + Reputation |
| **Mysticeti** | ~2,150 (Leaders) | **~120** | 3-hop Fast Path (Fastest Finality) |

*Note: Tusk and Shoal++ report throughput of all committed certificates in the DAG, while Mysticeti throughput focuses on individual leader-round commitments for ultra-low latency.*

## 🗺 Roadmap

- [x] Core DAG Logic & Tusk Implementation
- [x] Shoal++ & Mysticeti engines
- [ ] **Phase 1**: Infrastructure Professionalization (Asynchronous Multi-threaded Networking)
- [ ] **Phase 2**: Advanced BFT Liveness (Pacemaker & Timeout Management)
- [ ] **Phase 3**: Scalable Data Availability (Separate Worker-Layer)
- [ ] **Phase 4**: Hardening (Byzantine Simulation & Fuzzing)

## 📄 License

This project is licensed under the MIT License.
