#include "narwhal/network.hpp"
#include "narwhal/store.hpp"
#include "narwhal/common.hpp"
#include <iostream>

#ifndef USE_INTERNAL_MOCKS
#include <boost/asio.hpp>
#endif

using namespace narwhal;

int main(int argc, char* argv[]) {
    uint16_t port = 8001;
    std::string db_path = "./db_worker";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--db" && i + 1 < argc) {
            db_path = argv[++i];
        }
    }

    std::cout << "Starting Narwhal Worker Node (" 
#ifdef USE_INTERNAL_MOCKS
              << "MOCK MODE"
#else
              << "FULL MODE"
#endif
              << ") on port " << port << "..." << std::endl;

#ifndef USE_INTERNAL_MOCKS
    boost::asio::io_context io_context;
#else
    int io_context = 0;
#endif

    store::Store store(db_path);
    network::TlsNetwork network(io_context, port, "worker_cert.pem", "worker_key.pem");

    std::cout << "Worker Node initialized successfully." << std::endl;

#ifndef USE_INTERNAL_MOCKS
    io_context.run();
#endif

    return 0;
}
