#include "sharding/shard_transfer_store.h"

#include <algorithm>
#include <array>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <rocksdb/db.h>
#include <rocksdb/write_batch.h>

namespace themis::sharding {
namespace {

using Json = nlohmann::json;
constexpr std::size_t kDigestChars = 64;
constexpr std::size_t kMaxFieldBytes = 1024;
constexpr uint64_t kMaxTransferBytes = 1024ull * 1024 * 1024;
constexpr uint64_t kMaxChunks = 65'536;
constexpr auto kIdentityKey = "shard-transfer:v1:local-shard";

bool validField(std::string_view field) {
    if (field.empty() || field.size() > kMaxFieldBytes || field.find('\0') != std::string_view::npos) {
        return false;
    }
    if (std::all_of(field.begin(), field.end(), [](unsigned char c) { return c < 0x80; })) {
        return true;
    }
    try {
        (void) Json(std::string(field)).dump(); // Strict UTF-8 validation for non-ASCII fields.
        return true;
    } catch (const Json::type_error&) {
        return false;
    }
}

bool validDigest(std::string_view digest) {
    return digest.size() == kDigestChars && std::all_of(digest.begin(), digest.end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}

bool validSize(uint64_t chunks, uint64_t bytes) {
    return chunks <= kMaxChunks && bytes <= kMaxTransferBytes &&
           chunks <= bytes && bytes <= chunks * ShardTransferStore::kMaxChunkBytes;
}

bool validManifest(const ShardTransferManifest& m) {
    return validField(m.id) && validField(m.source) && validField(m.target) &&
           validField(m.watermark) && m.source != m.target &&
           m.range_start < m.range_end && validSize(m.chunks, m.bytes) &&
           validDigest(m.sha256);
}

void checkStatus(const rocksdb::Status& status) {
    if (!status.ok()) {
        throw std::runtime_error("Shard transfer storage: " + status.ToString());
    }
}

std::string recordKey(const std::string& id) {
    return "m" + id;
}

std::string chunkPrefix(const std::string& id) {
    return "c" + id + '\0';
}

std::string chunkEnd(const std::string& id) {
    return "c" + id + '\1';
}

std::string chunkKey(const std::string& prefix, uint64_t index) {
    auto key = prefix;
    // Big endian index preserves transfer order in RocksDB's bytewise key ordering.
    for (int shift = 56; shift >= 0; shift -= 8) {
        key.push_back(static_cast<char>((index >> shift) & 0xff));
    }
    return key;
}

class Sha256 {
public:
    Sha256() {
        if (!context_ || EVP_DigestInit_ex(context_.get(), EVP_sha256(), nullptr) != 1) {
            throw std::runtime_error("Unable to initialize transfer digest");
        }
    }

    void update(std::string_view bytes) {
        if (EVP_DigestUpdate(context_.get(), bytes.data(), bytes.size()) != 1) {
            throw std::runtime_error("Unable to update transfer digest");
        }
    }

    std::string finish() {
        std::array<unsigned char, 32> digest{};
        unsigned int size = 0;
        if (EVP_DigestFinal_ex(context_.get(), digest.data(), &size) != 1 || size != digest.size()) {
            throw std::runtime_error("Unable to finalize transfer digest");
        }
        constexpr auto hex = "0123456789abcdef";
        std::string result;
        result.reserve(kDigestChars);
        for (auto byte : digest) {
            result.push_back(hex[byte >> 4]);
            result.push_back(hex[byte & 0xf]);
        }
        return result;
    }

private:
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context_{
        EVP_MD_CTX_new(), EVP_MD_CTX_free};
};

std::string hash(std::string_view bytes) {
    Sha256 digest;
    digest.update(bytes);
    return digest.finish();
}

// Each payload carries its admitted digest so damaged persisted content is distinct
// from an intact stream that does not match the manifest.
std::string_view checkedPayload(std::string_view value) {
    if (value.size() <= kDigestChars ||
        value.size() - kDigestChars > ShardTransferStore::kMaxChunkBytes) {
        throw std::runtime_error("Invalid persisted shard transfer chunk size");
    }
    const auto payload = value.substr(kDigestChars);
    if (hash(payload) != value.substr(0, kDigestChars)) {
        throw std::runtime_error("Persisted shard transfer chunk digest mismatch");
    }
    return payload;
}

enum class Phase : uint64_t { assigned, prepared, verified, aborted };

struct Record {
    ShardTransferManifest manifest;
    Phase phase = Phase::assigned;
    uint64_t next = 0;
    uint64_t received = 0;
};

std::string encode(const Record& record) {
    const auto& m = record.manifest;
    return Json{{"id", m.id}, {"source", m.source}, {"target", m.target},
                {"watermark", m.watermark}, {"sha256", m.sha256},
                {"range_start", m.range_start}, {"range_end", m.range_end}, {"epoch", m.epoch},
                {"chunks", m.chunks}, {"bytes", m.bytes},
                {"phase", static_cast<uint64_t>(record.phase)},
                {"next", record.next}, {"received", record.received}}.dump();
}

Record decode(const std::string& value, const std::string& id, const std::string& local_shard) {
    Record record;
    try {
        const auto data = Json::parse(value);
        const auto string = [&data](const char* key) { return data.at(key).get<std::string>(); };
        const auto number = [&data](const char* key) {
            const auto& field = data.at(key);
            if (!field.is_number_unsigned()) {
                throw std::runtime_error("Invalid persisted shard transfer number");
            }
            return field.get<uint64_t>();
        };
        record.manifest = {string("id"), string("source"), string("target"), string("watermark"),
                           string("sha256"), number("range_start"), number("range_end"),
                           number("epoch"), number("chunks"), number("bytes")};
        record.phase = static_cast<Phase>(number("phase"));
        record.next = number("next");
        record.received = number("received");
    } catch (const Json::exception&) {
        throw std::runtime_error("Invalid persisted shard transfer record");
    }
    const auto& m = record.manifest;
    const bool reset = record.phase == Phase::assigned || record.phase == Phase::aborted;
    if (!validManifest(m) || m.id != id || m.target != local_shard ||
        record.phase > Phase::aborted ||
        record.next > m.chunks || record.received > m.bytes ||
        !validSize(record.next, record.received) ||
        !validSize(m.chunks - record.next, m.bytes - record.received) ||
        (reset && (record.next != 0 || record.received != 0)) ||
        (record.phase == Phase::verified && (record.next != m.chunks || record.received != m.bytes))) {
        throw std::runtime_error("Inconsistent persisted shard transfer record");
    }
    return record;
}

} // namespace

class ShardTransferStore::Impl {
public:
    Impl(const std::string& path, std::string shard,
         std::function<bool(const ShardTransferManifest&)> predicate)
        : local_shard(std::move(shard)), current_assignment(std::move(predicate)) {
        if (path.empty() || path.find('\0') != std::string::npos ||
            !validField(local_shard) || !current_assignment) {
            throw std::invalid_argument("Invalid shard transfer store configuration");
        }
        rocksdb::Options options;
        options.create_if_missing = true;
        options.wal_recovery_mode = rocksdb::WALRecoveryMode::kAbsoluteConsistency;
        rocksdb::DB* raw = nullptr;
        checkStatus(rocksdb::DB::Open(options, path, &raw));
        db.reset(raw);

        const auto identity = read(kIdentityKey);
        if (identity) {
            if (*identity != local_shard) {
                throw std::logic_error("Shard transfer store is bound to another shard");
            }
            return;
        }
        const std::unique_ptr<rocksdb::Iterator> entries(db->NewIterator(rocksdb::ReadOptions{}));
        entries->SeekToFirst();
        checkStatus(entries->status());
        if (entries->Valid()) {
            throw std::runtime_error("Shard transfer store identity is missing");
        }
        rocksdb::WriteBatch batch;
        checkStatus(batch.Put(kIdentityKey, local_shard));
        write(batch);
    }

    std::optional<std::string> read(const std::string& key) const {
        std::string value;
        const auto status = db->Get(rocksdb::ReadOptions{}, key, &value);
        if (status.IsNotFound()) {
            return std::nullopt;
        }
        checkStatus(status);
        return value;
    }

    void write(rocksdb::WriteBatch& batch) {
        rocksdb::WriteOptions options;
        options.sync = true;
        checkStatus(db->Write(options, &batch));
    }

    void persist(const Record& record, rocksdb::WriteBatch& batch) {
        checkStatus(batch.Put(recordKey(record.manifest.id), encode(record)));
        write(batch);
    }

    void authorize(const ShardTransferManifest& m, const std::string& principal) const {
        if (principal != m.source || m.target != local_shard) {
            throw std::logic_error("Shard transfer principal or destination is not authorized");
        }
        bool current = false;
        try {
            current = current_assignment(m);
        } catch (...) {
            throw std::runtime_error("Unable to check current shard transfer assignment");
        }
        if (!current) {
            throw std::logic_error("Shard transfer assignment is not current");
        }
    }

    Record loadAuthorized(const std::string& id, const std::string& principal) const {
        if (!validField(id)) {
            throw std::invalid_argument("Invalid shard transfer ID");
        }
        const auto value = read(recordKey(id));
        if (!value) {
            throw std::logic_error("Shard transfer is not assigned");
        }
        auto record = decode(*value, id, local_shard);
        authorize(record.manifest, principal);
        return record;
    }

    void validateStream(const Record& record) const {
        const auto& m = record.manifest;
        const auto prefix = chunkPrefix(m.id);
        const auto end = chunkEnd(m.id);
        const rocksdb::Slice end_slice(end);
        rocksdb::ReadOptions options;
        options.iterate_upper_bound = &end_slice;
        options.fill_cache = false;
        const std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(options));
        it->Seek(prefix);
        Sha256 stream;
        uint64_t received = 0;
        for (uint64_t index = 0; index < m.chunks; ++index, it->Next()) {
            checkStatus(it->status());
            if (!it->Valid() || it->key() != chunkKey(prefix, index)) {
                throw std::runtime_error("Missing or out-of-order persisted shard transfer chunk");
            }
            const auto payload = checkedPayload(it->value().ToStringView());
            received += payload.size();
            if (received > m.bytes) {
                throw std::runtime_error("Persisted shard transfer byte count exceeds manifest");
            }
            stream.update(payload);
        }
        checkStatus(it->status());
        if (it->Valid() || received != m.bytes) {
            throw std::runtime_error("Persisted shard transfer length does not match checkpoint");
        }
        if (stream.finish() != m.sha256) {
            if (record.phase == Phase::verified) {
                throw std::runtime_error("Previously verified shard transfer digest changed");
            }
            throw std::logic_error("Staged shard transfer digest does not match manifest");
        }
    }

    const std::string local_shard;
    const std::function<bool(const ShardTransferManifest&)> current_assignment;
    std::mutex mutex;
    std::unique_ptr<rocksdb::DB> db;
};

ShardTransferStore::ShardTransferStore(
    const std::string& path, std::string local_shard,
    std::function<bool(const ShardTransferManifest&)> current_assignment)
    : impl_(std::make_unique<Impl>(path, std::move(local_shard), std::move(current_assignment))) {}

ShardTransferStore::~ShardTransferStore() = default;

void ShardTransferStore::assign(const ShardTransferManifest& m, const std::string& principal) {
    if (!validManifest(m)) {
        throw std::invalid_argument("Invalid shard transfer manifest");
    }
    const std::lock_guard lock(impl_->mutex);
    impl_->authorize(m, principal);
    if (const auto value = impl_->read(recordKey(m.id))) {
        const auto existing = decode(*value, m.id, impl_->local_shard);
        if (existing.manifest != m || existing.phase == Phase::aborted) {
            throw std::logic_error("Shard transfer ID already assigned or aborted");
        }
        return;
    }
    rocksdb::WriteBatch batch;
    impl_->persist(Record{m}, batch);
}

uint64_t ShardTransferStore::prepare(const ShardTransferManifest& m, const std::string& principal) {
    if (!validManifest(m)) {
        throw std::invalid_argument("Invalid shard transfer manifest");
    }
    const std::lock_guard lock(impl_->mutex);
    auto record = impl_->loadAuthorized(m.id, principal);
    if (record.manifest != m || record.phase == Phase::aborted) {
        throw std::logic_error("Shard transfer manifest differs from assignment or is aborted");
    }
    if (record.phase == Phase::assigned) {
        record.phase = Phase::prepared;
        rocksdb::WriteBatch batch;
        impl_->persist(record, batch);
    }
    return record.next;
}

uint64_t ShardTransferStore::append(const std::string& id, const std::string& principal,
                                    uint64_t index, const std::string& bytes, const std::string& sha256) {
    if (bytes.empty() || bytes.size() > kMaxChunkBytes || !validDigest(sha256)) {
        throw std::invalid_argument("Invalid shard transfer chunk");
    }
    const std::lock_guard lock(impl_->mutex);
    auto record = impl_->loadAuthorized(id, principal);
    if ((record.phase != Phase::prepared && record.phase != Phase::verified) ||
        index > record.next || index >= record.manifest.chunks) {
        throw std::logic_error("Shard transfer is not accepting this chunk index");
    }
    if (hash(bytes) != sha256) {
        throw std::invalid_argument("Shard transfer chunk digest mismatch");
    }
    const auto key = chunkKey(chunkPrefix(id), index);
    const auto staged = impl_->read(key);
    if (index < record.next) {
        if (!staged) {
            throw std::runtime_error("Missing persisted shard transfer chunk during replay");
        }
        if (checkedPayload(*staged) != bytes) {
            throw std::logic_error("Shard transfer replay differs from staged chunk");
        }
        return record.next;
    }
    if (staged) {
        throw std::runtime_error("Unexpected persisted shard transfer chunk beyond checkpoint");
    }
    const auto new_size = record.received + bytes.size();
    const auto& m = record.manifest;
    if (new_size > m.bytes || !validSize(m.chunks - record.next - 1, m.bytes - new_size)) {
        throw std::logic_error("Shard transfer chunk size is inconsistent with manifest");
    }
    ++record.next;
    record.received = new_size;
    rocksdb::WriteBatch batch;
    checkStatus(batch.Put(key, sha256 + bytes));
    impl_->persist(record, batch);
    return record.next;
}

void ShardTransferStore::verify(const std::string& id, const std::string& principal) {
    const std::lock_guard lock(impl_->mutex);
    auto record = impl_->loadAuthorized(id, principal);
    if ((record.phase != Phase::prepared && record.phase != Phase::verified) ||
        record.next != record.manifest.chunks || record.received != record.manifest.bytes) {
        throw std::logic_error("Shard transfer is not complete");
    }
    impl_->validateStream(record);
    impl_->authorize(record.manifest, principal);
    if (record.phase != Phase::verified) {
        record.phase = Phase::verified;
        rocksdb::WriteBatch batch;
        impl_->persist(record, batch);
    }
}

void ShardTransferStore::abort(const std::string& id, const std::string& principal) {
    const std::lock_guard lock(impl_->mutex);
    auto record = impl_->loadAuthorized(id, principal);
    if (record.phase != Phase::aborted) {
        rocksdb::WriteBatch batch;
        checkStatus(batch.DeleteRange(chunkPrefix(id), chunkEnd(id)));
        record.phase = Phase::aborted;
        record.next = 0;
        record.received = 0;
        impl_->persist(record, batch);
    }
}

} // namespace themis::sharding
