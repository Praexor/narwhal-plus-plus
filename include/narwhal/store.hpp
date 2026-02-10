#pragma once

#include "narwhal/common.hpp"
#include <string>
#include <vector>
#include <optional>

#ifndef USE_INTERNAL_MOCKS
#include <rocksdb/db.hpp>
#endif

namespace narwhal::store {

class Store {
public:
    Store(const std::string& path);
    ~Store();

    void write(const std::vector<uint8_t>& key, const std::vector<uint8_t>& value);
    std::optional<std::vector<uint8_t>> read(const std::vector<uint8_t>& key);
    void remove(const std::vector<uint8_t>& key);

private:
#ifndef USE_INTERNAL_MOCKS
    rocksdb::DB* db_;
    rocksdb::Options options_;
#endif
    std::string path_;
};

} // namespace narwhal::store
