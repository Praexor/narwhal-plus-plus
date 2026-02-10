#include "narwhal/store.hpp"
#include <iostream>
#include <map>
#include <stdexcept>

namespace narwhal::store {

#ifdef USE_INTERNAL_MOCKS
static std::map<std::vector<uint8_t>, std::vector<uint8_t>> mock_db;
#endif

Store::Store(const std::string& path) : path_(path) {
#ifdef USE_INTERNAL_MOCKS
    std::cout << "[MOCK] Store opened at " << path << std::endl;
#else
    options_.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options_, path, &db_);
    if (!status.ok()) {
        throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
    }
#endif
}

Store::~Store() {
#ifndef USE_INTERNAL_MOCKS
    delete db_;
#endif
}

void Store::write(const std::vector<uint8_t>& key, const std::vector<uint8_t>& value) {
#ifdef USE_INTERNAL_MOCKS
    mock_db[key] = value;
#else
    rocksdb::Slice k(reinterpret_cast<const char*>(key.data()), key.size());
    rocksdb::Slice v(reinterpret_cast<const char*>(value.data()), value.size());
    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), k, v);
    if (!status.ok()) {
        throw std::runtime_error("RocksDB write failed: " + status.ToString());
    }
#endif
}

std::optional<std::vector<uint8_t>> Store::read(const std::vector<uint8_t>& key) {
#ifdef USE_INTERNAL_MOCKS
    auto it = mock_db.find(key);
    if (it != mock_db.end()) return it->second;
    return std::nullopt;
#else
    rocksdb::Slice k(reinterpret_cast<const char*>(key.data()), key.size());
    std::string value;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), k, &value);
    
    if (status.ok()) {
        return std::vector<uint8_t>(value.begin(), value.end());
    } else if (status.IsNotFound()) {
        return std::nullopt;
    } else {
        throw std::runtime_error("RocksDB read failed: " + status.ToString());
    }
#endif
}

void Store::remove(const std::vector<uint8_t>& key) {
#ifdef USE_INTERNAL_MOCKS
    mock_db.erase(key);
#else
    rocksdb::Slice k(reinterpret_cast<const char*>(key.data()), key.size());
    rocksdb::Status status = db_->Delete(rocksdb::WriteOptions(), k);
    if (!status.ok()) {
        throw std::runtime_error("RocksDB delete failed: " + status.ToString());
    }
#endif
}

} // namespace narwhal::store
