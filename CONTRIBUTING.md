# Contributing to Narwhal++

Thank you for your interest in contributing to Narwhal++! This document provides guidelines for contributing to the project.

## 🎯 Project Vision

Narwhal++ is a research and evaluation platform for comparing state-of-the-art BFT consensus engines (Tusk, Shoal++, Mysticeti) in a unified C++20 framework. Our goal is to provide a high-fidelity implementation that prioritizes **correctness through formal verification** before performance optimization.

## 🚀 Getting Started

### Prerequisites
- C++20 compatible compiler (MSVC 2022+, GCC 11+, Clang 14+)
- CMake 3.20+
- Git

### Building the Project
```bash
# Clone the repository
git clone https://github.com/Praexor/narwhal-plus-plus.git
cd narwhal-plus-plus

# Build in mock mode (no external dependencies)
cmake -B build -S .
cmake --build build --config Release

# Run a local cluster
./run_cluster.ps1 -Engine mysticeti -Load
```

## 📋 How to Contribute

### 1. Find an Issue
- Check the [Issues](https://github.com/Praexor/narwhal-plus-plus/issues) page
- Look for issues labeled `good first issue` or `help wanted`
- Comment on the issue to let others know you're working on it

### 2. Fork and Branch
```bash
# Fork the repository on GitHub, then:
git clone https://github.com/YOUR_USERNAME/narwhal-plus-plus.git
cd narwhal-plus-plus
git checkout -b feature/your-feature-name
```

### 3. Make Your Changes
- Follow the [Code Style](#code-style) guidelines
- Add tests for new functionality
- Update documentation as needed
- Run existing tests to ensure nothing breaks

### 4. Commit and Push
```bash
git add .
git commit -m "Add feature: brief description

- Detailed point 1
- Detailed point 2"
git push origin feature/your-feature-name
```

### 5. Create a Pull Request
- Go to the original repository on GitHub
- Click "New Pull Request"
- Select your fork and branch
- Fill out the PR template with details about your changes

## 📝 Code Style

### C++ Guidelines
- **Standard:** C++20
- **Naming:**
  - Classes/Structs: `PascalCase`
  - Functions/Methods: `snake_case`
  - Variables: `snake_case`
  - Constants: `UPPER_SNAKE_CASE`
  - Namespaces: `lowercase`
- **Formatting:** Use 4 spaces for indentation (no tabs)
- **Headers:** Use `#pragma once` for header guards
- **Includes:** Group in order: standard library, third-party, project headers

### Example
```cpp
#pragma once

#include <vector>
#include <memory>
#include <boost/asio.hpp>
#include "narwhal/consensus.hpp"

namespace narwhal::network {

class AsyncNetwork {
public:
    void send_certificate(const std::string& peer_address,
                         const consensus::Certificate& cert);
    
private:
    std::vector<std::shared_ptr<Connection>> connections_;
};

} // namespace narwhal::network
```

## 🧪 Testing

### Running Tests
```bash
# Build with tests enabled
cmake -B build -S . -DBUILD_TESTS=ON
cmake --build build --config Release

# Run all tests
ctest --test-dir build
```

### Property-Based Tests
```bash
# Build with RapidCheck
cmake -B build -S . -DBUILD_PROPERTY_TESTS=ON
cmake --build build --config Release
./build/property_tests
```

### Adding New Tests
- Unit tests: Add to `tests/` directory
- Property tests: Add to `tests/property_tests.cpp`
- Integration tests: Create new test file in `tests/integration/`

## 🔍 Formal Verification

If you're contributing to consensus logic, please:

1. **Update TLA+ Specifications**
   - Modify `formal/tla/Mysticeti.tla` (or relevant spec)
   - Run TLC model checker to verify properties
   - Document any new invariants

2. **Add Property Tests**
   - Add RapidCheck tests for new invariants
   - Ensure tests run 10,000+ iterations

3. **Document Assumptions**
   - Clearly state any assumptions in code comments
   - Update formal specifications to reflect assumptions

## 📚 Documentation

### Code Documentation
- Use Doxygen-style comments for public APIs
- Explain **why**, not just **what**
- Document preconditions and postconditions

### Example
```cpp
/**
 * @brief Sends a certificate to a specific peer
 * 
 * @param peer_address The network address of the peer (format: "host:port")
 * @param cert The certificate to send
 * 
 * @pre The network must be started (start() called)
 * @pre peer_address must be a valid, reachable peer
 * @post The certificate is queued for sending (may not be sent immediately)
 * 
 * @throws std::runtime_error if the network is not started
 */
void send_certificate(const std::string& peer_address,
                     const consensus::Certificate& cert);
```

### Documentation Files
- Update `README.md` for user-facing changes
- Update `PHASE_0_1_GUIDE.md` for implementation details
- Add examples to `examples/` directory if applicable

## 🐛 Reporting Bugs

Use the [Bug Report](https://github.com/Praexor/narwhal-plus-plus/issues/new?template=bug_report.md) template and include:
- Clear description of the issue
- Steps to reproduce
- Expected vs actual behavior
- Environment details (OS, compiler, build mode)
- Relevant logs or error messages

## 💡 Feature Requests

Use the [Feature Request](https://github.com/Praexor/narwhal-plus-plus/issues/new) template and include:
- Problem statement (what need does this address?)
- Proposed solution
- Alternatives considered
- Impact on existing functionality

## 🔒 Security

If you discover a security vulnerability, please **do not** open a public issue. Instead:
1. Email the maintainer directly
2. Provide a detailed description of the vulnerability
3. Include steps to reproduce (if applicable)
4. Allow time for a fix before public disclosure

## 📜 License

By contributing to Narwhal++, you agree that your contributions will be licensed under the MIT License.

## 🙏 Recognition

Contributors will be recognized in:
- The project README
- Release notes
- The `CONTRIBUTORS.md` file (coming soon)

## ❓ Questions?

- Open a [Discussion](https://github.com/Praexor/narwhal-plus-plus/discussions)
- Comment on relevant issues
- Check the [documentation](README.md)

Thank you for contributing to Narwhal++! 🦭
