/**
 * @file content_manager.cpp
 * @brief Core content management system orchestrating processors, validators, and storage.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=8; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=4, C=1, H=2, M=5, L=0
 * @note Status: Production Ready; Core orchestration complete; advanced caching deferred
 * @note This block is auto-generated and will be overwritten.
 */
#include "content/content_manager.h"
#include "content/content_type.h"
#include "content/content_processor.h"
#include "content/archive_processor.h"
#include "content/html_processor.h"
#include "content/markdown_processor.h"
#include "content/content_validator.h"
#include "content/ocr_processor.h"
#include "utils/logger.h"
#include "storage/key_schema.h"
#include "utils/zstd_codec.h"
#include "utils/hkdf_helper.h"
#include "utils/hkdf_cache.h"

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <exception>
#include <random>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <unordered_set>
#include <queue>
#include <set>
#include <sstream>
#include <fstream>
#include <thread>
#include <stdexcept>

namespace themis {
namespace content {

using namespace std::chrono;

namespace {
constexpr std::string_view kFulltextChunkTable = "chunk";
constexpr std::string_view kFulltextChunkTextColumn = "text";
} // namespace

// ---------------------------------------------------------------------------
// Pipeline retry helper
// ---------------------------------------------------------------------------
// Calls fn(error_out) up to 1 + max_retries times, waiting retry_delay_ms
// between attempts.  Returns true as soon as fn returns true; otherwise
// returns false after all attempts are exhausted.
// attempts_out receives the total number of calls made.
namespace {
template <typename Fn>
bool executeWithRetry(Fn&& fn, int max_retries, int retry_delay_ms,
                      std::string& error_out, int& attempts_out) {
    attempts_out = 0;
    // i=0 is the initial attempt; i=1..max_retries are the retries.
    // Using i <= max_retries is intentional: max_retries=0 means "no retries" (one attempt total).
    for (int i = 0; i <= max_retries; ++i) {
        if (i > 0 && retry_delay_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
        }
        ++attempts_out;
        error_out.clear();
        if (fn(error_out)) return true;
    }
    return false;
}
} // anonymous namespace
// ---------------------------------------------------------------------------

[[maybe_unused]] static std::string toHex(const std::string& in) {
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(in.size() * 2);
    for (unsigned char c : in) {
        out.push_back(hex[c >> 4]);
        out.push_back(hex[c & 0x0F]);
    }
    return out;
}

static std::string computeImageDedupHash(const std::string& blob) {
    if (blob.empty()) {
        return {};
    }

    uint64_t hash = 1469598103934665603ULL;
    for (unsigned char byte : blob) {
        hash ^= static_cast<uint64_t>(byte);
        hash *= 1099511628211ULL;
    }

    std::ostringstream oss;
    oss << std::hex << std::nouppercase << std::setfill('0') << std::setw(16) << hash;
    return oss.str();
}

// Helper: parse category string to enum
static std::optional<ContentCategory> parseCategory(const std::string& s) {
    std::string up = s;
    std::transform(up.begin(), up.end(), up.begin(), ::toupper);
    if (up == "TEXT") return ContentCategory::TEXT;
    if (up == "IMAGE") return ContentCategory::IMAGE;
    if (up == "GEO") return ContentCategory::GEO;
    if (up == "CAD") return ContentCategory::CAD;
    if (up == "AUDIO") return ContentCategory::AUDIO;
    if (up == "STRUCTURED") return ContentCategory::STRUCTURED;
    if (up == "BINARY") return ContentCategory::BINARY;
    return std::nullopt;
}

static bool hasSearchFilterConstraints(const json& filters) {
    if (!filters.is_object() || filters.empty()) {
        return false;
    }
    for (auto it = filters.begin(); it != filters.end(); ++it) {
        if (it.key() != "scoring") {
            return true;
        }
    }
    return false;
}

static bool whitelistContainsChunkPk(
    const std::unordered_set<std::string>& whitelist_set,
    const std::string& chunk_pk
) {
    if (whitelist_set.empty()) {
        return true;
    }
    if (whitelist_set.count(chunk_pk) > 0) {
        return true;
    }
    return whitelist_set.count(std::string("chunks:") + chunk_pk) > 0;
}

// Helper: convert category enum to string
[[maybe_unused]] static std::string categoryToString(ContentCategory cat) {
    switch (cat) {
        case ContentCategory::TEXT: return "TEXT";
        case ContentCategory::IMAGE: return "IMAGE";
        case ContentCategory::GEO: return "GEO";
        case ContentCategory::CAD: return "CAD";
        case ContentCategory::AUDIO: return "AUDIO";
        case ContentCategory::STRUCTURED: return "STRUCTURED";
        case ContentCategory::BINARY: return "BINARY";
        default: return "UNKNOWN";
    }
}

// Build whitelist of chunk PKs ("chunks:<id>") based on filters
static const json* jsonPathRef(const json& j, const std::string& path) {
    // dotted path (no arrays)
    const json* cur = &j;
    size_t start = 0;
    while (start <= path.size()) {
        size_t dot = path.find('.', start);
        std::string key = dot == std::string::npos ? path.substr(start) : path.substr(start, dot - start);
        if (!cur->is_object() || !cur->contains(key)) return nullptr;
        cur = &((*cur)[key]);
        if (dot == std::string::npos) break;
        start = dot + 1;
    }
    return cur;
}

static std::vector<std::string> buildChunkWhitelist(
    RocksDBWrapper& storage,
    const json& filters
) {
    std::set<ContentCategory> allowedCats;
    std::unordered_set<std::string> allowedMimes;
    std::unordered_map<std::string, json> wantedMeta;
    std::unordered_set<std::string> wantedTags;
    bool hasAnyFilter = false;
    bool hasEffectiveConstraint = false;
    bool hasMalformedConstraint = false;
    std::optional<int64_t> dateFrom;
    std::optional<int64_t> dateTo;

    try {
        if (filters.contains("category")) {
            hasAnyFilter = true;
            const auto& category = filters["category"];
            if (category.is_string()) {
                auto cat = parseCategory(category.get<std::string>());
                if (cat) {
                    allowedCats.insert(*cat);
                    hasEffectiveConstraint = true;
                } else {
                    hasMalformedConstraint = true;
                }
            } else if (category.is_number_integer()) {
                int ci = category.get<int>();
                if (ci >= 0 && ci <= static_cast<int>(ContentCategory::BINARY)) {
                    allowedCats.insert(static_cast<ContentCategory>(ci));
                    hasEffectiveConstraint = true;
                } else {
                    hasMalformedConstraint = true;
                }
            } else if (category.is_array()) {
                bool added = false;
                for (const auto& v : filters["category"]) {
                    if (v.is_string()) {
                        auto cat = parseCategory(v.get<std::string>());
                        if (cat) {
                            allowedCats.insert(*cat);
                            added = true;
                        }
                    } else if (v.is_number_integer()) {
                        int ci = v.get<int>();
                        if (ci >= 0 && ci <= static_cast<int>(ContentCategory::BINARY)) {
                            allowedCats.insert(static_cast<ContentCategory>(ci));
                            added = true;
                        }
                    }
                }
                hasEffectiveConstraint = hasEffectiveConstraint || added;
                if (!added) {
                    hasMalformedConstraint = true;
                }
            } else {
                hasMalformedConstraint = true;
            }
        }
        if (filters.contains("mime_type")) {
            hasAnyFilter = true;
            const auto& mime = filters["mime_type"];
            if (mime.is_string()) {
                allowedMimes.insert(mime.get<std::string>());
                hasEffectiveConstraint = true;
            } else if (mime.is_array()) {
                bool added = false;
                for (const auto& v : filters["mime_type"]) {
                    if (v.is_string()) {
                        allowedMimes.insert(v.get<std::string>());
                        added = true;
                    }
                }
                hasEffectiveConstraint = hasEffectiveConstraint || added;
                if (!added) {
                    hasMalformedConstraint = true;
                }
            } else {
                hasMalformedConstraint = true;
            }
        }
        if (filters.contains("metadata")) {
            hasAnyFilter = true;
            if (filters["metadata"].is_object() && !filters["metadata"].empty()) {
                for (auto it = filters["metadata"].begin(); it != filters["metadata"].end(); ++it) {
                    wantedMeta[it.key()] = it.value();
                }
                hasEffectiveConstraint = true;
            } else {
                hasMalformedConstraint = true;
            }
        }
        if (filters.contains("tags")) {
            hasAnyFilter = true;
            const auto& tags = filters["tags"];
            if (tags.is_string()) {
                wantedTags.insert(tags.get<std::string>());
                hasEffectiveConstraint = true;
            } else if (tags.is_array()) {
                bool added = false;
                for (const auto& t : filters["tags"]) {
                    if (t.is_string()) {
                        wantedTags.insert(t.get<std::string>());
                        added = true;
                    }
                }
                hasEffectiveConstraint = hasEffectiveConstraint || added;
                if (!added) {
                    hasMalformedConstraint = true;
                }
            } else {
                hasMalformedConstraint = true;
            }
        }
        if (filters.contains("date_from")) {
            hasAnyFilter = true;
            if (filters["date_from"].is_number_integer()) {
                dateFrom = filters["date_from"].get<int64_t>();
                hasEffectiveConstraint = true;
            } else {
                hasMalformedConstraint = true;
            }
        }
        if (filters.contains("date_to")) {
            hasAnyFilter = true;
            if (filters["date_to"].is_number_integer()) {
                dateTo = filters["date_to"].get<int64_t>();
                hasEffectiveConstraint = true;
            } else {
                hasMalformedConstraint = true;
            }
        }
    } catch (const json::exception&) {
        hasMalformedConstraint = true;
    } catch (...) {
        hasMalformedConstraint = true;
    }

    if (!hasAnyFilter || !hasEffectiveConstraint || hasMalformedConstraint) return {};

    // Load optional filter schema from config
    // Key: config:content_filter_schema, example: { "field_map": { "dataset": "user_metadata.dataset", "region": "user_metadata.region" } }
    std::unordered_map<std::string, std::string> fieldMap;
    try {
        auto v = storage.get("config:content_filter_schema");
        if (v) {
            std::string s(v->begin(), v->end());
            json sc = json::parse(s);
            if (sc.contains("field_map") && sc["field_map"].is_object()) {
                for (auto it = sc["field_map"].begin(); it != sc["field_map"].end(); ++it) {
                    if (it.value().is_string()) fieldMap[it.key()] = it.value().get<std::string>();
                }
            }
        }
    } catch (const json::exception&) {
        // Ignore malformed schema config.
    } catch (...) {
        // Ignore malformed schema config.
    }

    auto jsonPathEq = [](const json& j, const std::string& path, const json& expected) -> bool {
        auto cur = jsonPathRef(j, path);
        if (!cur) return false;
        try {
            return cur->dump() == expected.dump();
        } catch (const json::exception&) {
            return false;
        } catch (...) {
            return false;
        }
    };

    std::vector<std::string> whitelist;
    // Scan all content metas
    storage.scanPrefix("content:", [&]([[maybe_unused]] std::string_view key, std::string_view val){
        // Ignore non-meta keys like content:chunks lists by checking JSON
        try {
            std::string s(val);
            json j = json::parse(s);
            if (!j.is_object()) return true;
            // It is a meta object if it has mime_type/size_bytes etc.
            bool looksMeta = j.contains("mime_type") && j.contains("size_bytes");
            if (!looksMeta) return true;
            ContentMeta m = ContentMeta::fromJson(j);
            if (dateFrom.has_value() && m.created_at < *dateFrom) return true;
            if (dateTo.has_value() && m.created_at > *dateTo) return true;
            // category filter
            if (!allowedCats.empty() && allowedCats.count(m.category) == 0) return true;
            // mime filter
            if (!allowedMimes.empty() && allowedMimes.count(m.mime_type) == 0) return true;
            // metadata filter: only top-level user_metadata exact matches
            if (!wantedMeta.empty()) {
                bool allMatch = true;
                for (const auto& kv : wantedMeta) {
                    if (!m.user_metadata.contains(kv.first)) { allMatch = false; break; }
                    const auto& v = m.user_metadata[kv.first];
                    if (v.type() != kv.second.type()) {
                        // allow string/numeric loose comparison fallback
                        try {
                            if (v.dump() != kv.second.dump()) { allMatch = false; break; }
                        } catch (const json::exception&) {
                            allMatch = false;
                            break;
                        } catch (...) {
                            allMatch = false;
                            break;
                        }
                    } else {
                        if (v.dump() != kv.second.dump()) { allMatch = false; break; }
                    }
                }
                if (!allMatch) return true;
            }
            // tags filter: any match
            if (!wantedTags.empty()) {
                bool any = false;
                for (const auto& t : m.tags) { if (wantedTags.count(t)) { any = true; break; } }
                if (!any) return true;
            }
            // custom filters via schema mapping: for any key present in filters but not reserved
            for (auto it = filters.begin(); it != filters.end(); ++it) {
                const std::string keyName = it.key();
                if (keyName == "category" || keyName == "mime_type" || keyName == "metadata" || keyName == "tags"
                    || keyName == "scoring" || keyName == "date_from" || keyName == "date_to") continue;
                auto fmap = fieldMap.find(keyName);
                if (fmap == fieldMap.end()) return true; // unknown key → reject (fail-closed)
                const std::string& jpath = fmap->second;
                const json* vptr = jsonPathRef(j, jpath);
                if (!vptr) return true; // missing → reject

                const json& cond = it.value();
                bool match = false;
                try {
                    if (cond.is_array()) {
                        // IN semantics: any equals
                        for (const auto& c : cond) {
                            if (vptr->dump() == c.dump()) { match = true; break; }
                        }
                    } else if (cond.is_object() && (cond.contains("min") || cond.contains("max"))) {
                        // RANGE semantics (numeric). Convert vptr to number if possible.
                        double numeric_val = 0.0; bool ok = false;
                        if (vptr->is_number()) { numeric_val = vptr->get<double>(); ok = true; }
                        else if (vptr->is_string()) {
                            try {
                                numeric_val = std::stod(vptr->get<std::string>());
                                ok = true;
                            } catch (const std::invalid_argument&) {
                                ok = false;
                            } catch (const std::out_of_range&) {
                                ok = false;
                            }
                        }
                        if (ok) {
                            double vmin = -std::numeric_limits<double>::infinity();
                            double vmax =  std::numeric_limits<double>::infinity();
                            if (cond.contains("min")) {
                                if (cond["min"].is_number()) vmin = cond["min"].get<double>();
                                else if (cond["min"].is_string()) {
                                    try {
                                        vmin = std::stod(cond["min"].get<std::string>());
                                    } catch (const std::invalid_argument&) {
                                        // Keep default bound.
                                    } catch (const std::out_of_range&) {
                                        // Keep default bound.
                                    }
                                }
                            }
                            if (cond.contains("max")) {
                                if (cond["max"].is_number()) vmax = cond["max"].get<double>();
                                else if (cond["max"].is_string()) {
                                    try {
                                        vmax = std::stod(cond["max"].get<std::string>());
                                    } catch (const std::invalid_argument&) {
                                        // Keep default bound.
                                    } catch (const std::out_of_range&) {
                                        // Keep default bound.
                                    }
                                }
                            }
                            match = (numeric_val >= vmin && numeric_val <= vmax);
                        } else {
                            match = false;
                        }
                    } else {
                        // default: equality
                        match = (vptr->dump() == cond.dump());
                    }
                } catch (const json::exception&) {
                    match = false;
                } catch (...) {
                    match = false;
                }
                if (!match) return true; // mismatch → reject
            }
            // This content matches → add all its chunks to whitelist
            std::string id = m.id;
            std::string lkey = std::string("content_chunks:") + id;
            auto lv = storage.get(lkey);
            if (lv) {
                try {
                    std::string ls(lv->begin(), lv->end());
                    json lj = json::parse(ls);
                    if (lj.contains("ids") && lj["ids"].is_array()) {
                        for (const auto& cid : lj["ids"]) {
                            if (cid.is_string()) whitelist.push_back(std::string("chunks:") + cid.get<std::string>());
                        }
                    }
                } catch (const json::exception&) {
                    // Ignore malformed chunk-id list.
                } catch (...) {
                    // Ignore malformed chunk-id list.
                }
            }
        } catch (const json::exception&) {
            // ignore parsing errors
        } catch (...) {
            // ignore parsing errors
        }
        return true;
    });
    return whitelist;
}

json ContentMeta::toJson() const {
    return json{
        {"id", id},
        {"mime_type", mime_type},
        {"category", static_cast<int>(category)},
        {"original_filename", original_filename},
        {"size_bytes", size_bytes},
        {"compressed", compressed},
        {"compression_type", compression_type},
        {"encrypted", encrypted},
        {"encryption_type", encryption_type},
        {"created_at", created_at},
        {"modified_at", modified_at},
        {"hash_sha256", hash_sha256},
        {"text_extracted", text_extracted},
        {"chunked", chunked},
        {"indexed", indexed},
        {"chunk_count", chunk_count},
        {"embedding_dim", embedding_dim},
        {"extracted_metadata", extracted_metadata},
        {"user_metadata", user_metadata},
        {"tags", tags},
        {"parent_id", parent_id},
        {"child_ids", child_ids},
        {"virtual_path", virtual_path},
        {"is_directory", is_directory}
    };
}

ContentMeta ContentMeta::fromJson(const json& j) {
    auto parseCategory = [](const json& value) -> ContentCategory {
        if (value.is_number_integer()) {
            return static_cast<ContentCategory>(value.get<int>());
        }
        if (value.is_string()) {
            std::string category = value.get<std::string>();
            std::transform(category.begin(), category.end(), category.begin(),
                           [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
            if (category == "TEXT") return ContentCategory::TEXT;
            if (category == "IMAGE") return ContentCategory::IMAGE;
            if (category == "AUDIO") return ContentCategory::AUDIO;
            if (category == "VIDEO") return ContentCategory::VIDEO;
            if (category == "GEO") return ContentCategory::GEO;
            if (category == "CAD") return ContentCategory::CAD;
            if (category == "ARCHIVE") return ContentCategory::ARCHIVE;
            if (category == "STRUCTURED") return ContentCategory::STRUCTURED;
            if (category == "BINARY") return ContentCategory::BINARY;
            if (category == "UNKNOWN") return ContentCategory::UNKNOWN;
        }
        return ContentCategory::UNKNOWN;
    };

    ContentMeta m;
    m.id = j.value("id", "");
    m.mime_type = j.value("mime_type", "");
    m.category = j.contains("category") ? parseCategory(j["category"]) : ContentCategory::UNKNOWN;
    m.original_filename = j.value("original_filename", "");
    m.size_bytes = j.value("size_bytes", 0LL);
    m.compressed = j.value("compressed", false);
    m.compression_type = j.value("compression_type", "");
    m.encrypted = j.value("encrypted", false);
    m.encryption_type = j.value("encryption_type", "");
    m.created_at = j.value("created_at", 0LL);
    m.modified_at = j.value("modified_at", 0LL);
    m.hash_sha256 = j.value("hash_sha256", "");
    m.text_extracted = j.value("text_extracted", false);
    m.chunked = j.value("chunked", false);
    m.indexed = j.value("indexed", false);
    m.chunk_count = j.value("chunk_count", 0);
    m.embedding_dim = j.value("embedding_dim", 0);
    m.extracted_metadata = j.value("extracted_metadata", json::object());
    m.user_metadata = j.value("user_metadata", json::object());
    m.tags = j.value("tags", std::vector<std::string>{});
    m.parent_id = j.value("parent_id", "");
    m.child_ids = j.value("child_ids", std::vector<std::string>{});
    m.virtual_path = j.value("virtual_path", "");
    m.is_directory = j.value("is_directory", false);
    return m;
}

json ChunkMeta::toJson() const {
    return json{
        {"id", id},
        {"content_id", content_id},
        {"seq_num", seq_num},
        {"chunk_type", chunk_type},
        {"text", text},
        {"data", data},
        {"blob_ref", blob_ref},
        {"start_offset", start_offset},
        {"end_offset", end_offset},
        {"embedding", embedding},
        {"embedding_indexed", embedding_indexed},
        {"created_at", created_at}
    };
}

ChunkMeta ChunkMeta::fromJson(const json& j) {
    ChunkMeta c;
    c.id = j.value("id", "");
    c.content_id = j.value("content_id", "");
    c.seq_num = j.value("seq_num", 0);
    c.chunk_type = j.value("chunk_type", "");
    c.text = j.value("text", "");
    c.data = j.value("data", json::object());
    c.blob_ref = j.value("blob_ref", "");
    c.start_offset = j.value("start_offset", 0);
    c.end_offset = j.value("end_offset", 0);
    c.embedding = j.value("embedding", std::vector<float>{});
    c.embedding_indexed = j.value("embedding_indexed", false);
    c.created_at = j.value("created_at", 0LL);
    return c;
}

ContentManager::ContentManager(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<VectorIndexManager> vector_index,
        std::shared_ptr<GraphIndexManager> graph_index,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        std::shared_ptr<FieldEncryption> field_encryption
) : storage_(std::move(storage))
    , vector_index_(std::move(vector_index))
    , graph_index_(std::move(graph_index))
    , secondary_index_(std::move(secondary_index))
    , field_encryption_(std::move(field_encryption))
{}

const ContentManager::Metrics& ContentManager::getMetrics() const {
    return metrics_;
}

void ContentManager::setMalwareFilter(std::shared_ptr<themis::security::MalwareFilterManager> malware_filter) {
    malware_filter_ = std::move(malware_filter);
}

std::shared_ptr<themis::security::MalwareFilterManager> ContentManager::getMalwareFilter() const {
    return malware_filter_;
}

void ContentManager::setDeduplicationChecker(std::shared_ptr<DeduplicationChecker> checker) {
    dedup_checker_ = std::move(checker);
}

std::shared_ptr<DeduplicationChecker> ContentManager::getDeduplicationChecker() const {
    return dedup_checker_;
}

void ContentManager::setProcessorChainConfig(const ProcessorChainConfig& config) {
    processor_chain_config_ = config;
}

const ProcessorChainConfig& ContentManager::getProcessorChainConfig() const {
    return processor_chain_config_;
}

void ContentManager::registerProcessor(std::unique_ptr<IContentProcessor> processor) {
    if (!processor) return;
    auto cats = processor->getSupportedCategories();
    if (cats.empty()) return;
    // Insert for the first supported category (current processors use single category)
    processors_[cats.front()] = std::move(processor);
}

std::string ContentManager::generateUuid() {
    static thread_local std::mt19937_64 rng{static_cast<uint64_t>(steady_clock::now().time_since_epoch().count()) ^ 0x9e3779b97f4a7c15ULL};
    auto u64 = rng();
    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << u64
        << std::hex << std::setw(16) << std::setfill('0') << rng();
    return oss.str();
}

std::string ContentManager::normalizeId(const std::string& id, const std::string& prefix) {
    if (id.rfind(prefix, 0) == 0) return id.substr(prefix.size());
    return id;
}

std::string ContentManager::computeSHA256(const std::string& blob) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(blob.data()), blob.size(), digest);
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(digest[i]);
    }
    return oss.str();
}

std::optional<std::string> ContentManager::checkDuplicateByHash(const std::string& hash) {
    // Simple secondary lookup: store mapping hash -> content_id list (first only)
    std::string key = std::string("content_hash:") + hash;
    auto v = storage_->get(key);
    if (!v) return std::nullopt;
    try {
        std::string s(v->begin(), v->end());
        json j = json::parse(s);
        if (j.contains("ids") && j["ids"].is_array() && !j["ids"].empty()) {
            return j["ids"][0].get<std::string>();
        }
    } catch (const json::exception&) {
    } catch (...) {
    }
    return std::nullopt;
}

[[maybe_unused]] static ContentCategory detectCategory(const std::string& mime, const std::string& blob) {
    auto& reg = ContentTypeRegistry::instance();
    ContentType ct;
    if (!mime.empty()) {
        auto t = reg.getByMimeType(mime);
        if (t) ct = *t;
    }
    if (ct.mime_type.empty()) {
        auto t = reg.detectFromBlob(blob);
        if (t) ct = *t;
    }
    return ct.mime_type.empty() ? ContentCategory::UNKNOWN : ct.category;
}

/// Returns true for MIME types that are text-based and have no binary magic bytes.
/// Used by ingestStream() to skip the format/magic-bytes check for streaming
/// text types while still running it for binary formats.
static bool isTextBasedMime(const std::string& mime) {
    return mime.find("text/") == 0 ||
           mime.find("ndjson") != std::string::npos ||
           mime.find("jsonlines") != std::string::npos;
}

Status ContentManager::importContent(const json& spec, const std::optional<std::string>& blob, const std::string& user_context) {
    try {
        if (!spec.is_object() || !spec.contains("content") || !spec["content"].is_object()) {
            return Status::Error("spec.content missing or invalid");
        }
        ContentMeta meta = ContentMeta::fromJson(spec["content"]);
        // ID vergeben falls nicht vorhanden
        if (meta.id.empty()) meta.id = generateUuid();
        
        // Malware scan before storing blob (Audit Compliance: BSI C5 OPS-12, ISO 27001 A.12.2.1)
        if (blob.has_value() && malware_filter_) {
            const std::string& blob_data = *blob;
            auto scan_result = malware_filter_->scan(
                blob_data,
                meta.original_filename,
                meta.mime_type,
                meta.id
            );
            
            if (malware_filter_->shouldBlock(scan_result)) {
                std::string threat_info = "Malware detected: ";
                if (!scan_result.scanner_results.empty()) {
                    for (const auto& r : scan_result.scanner_results) {
                        if (!r.clean) {
                            threat_info += r.threat_name + " (" + r.scanner_name + "); ";
                        }
                    }
                }
                THEMIS_WARN("Content import blocked due to malware: {} - {}", meta.id, threat_info);
                return Status::Error("Content blocked: " + threat_info);
            }
            
            if (!scan_result.clean) {
                THEMIS_WARN("Content {} passed with warnings: threat_level={}", 
                           meta.id, 
                           security::threatLevelToString(scan_result.highest_threat));
            }
        }
        
        // Blob optional speichern
        if (blob.has_value()) {
            std::string bkey = std::string("content_blob:") + meta.id;
            const std::string& bb = *blob;
            // Load content config from DB: key config:content
            bool compress = false;
            int zstd_level = 19;
            std::vector<std::string> skip_mimes = {"image/", "video/", "application/zip", "application/gzip"};
            try {
                if (auto cfgv = storage_->get("config:content")) {
                    std::string s(cfgv->begin(), cfgv->end());
                    json cj = json::parse(s);
                    compress = cj.value("compress_blobs", false);
                    zstd_level = cj.value("compression_level", 19);
                    if (cj.contains("skip_compressed_mimes") && cj["skip_compressed_mimes"].is_array()) {
                        skip_mimes.clear();
                        for (const auto& mv : cj["skip_compressed_mimes"]) if (mv.is_string()) skip_mimes.push_back(mv.get<std::string>());
                    }
                }
            } catch (const json::exception&) {
            } catch (...) {
            }

            std::string matched_skip_prefix;
            auto should_compress = [&](const std::string& mime, size_t size) -> bool {
                if (!compress) return false;
                if (size <= 4096) return false; // Skip small blobs (<4KB)
                // Skip if MIME starts with any of the skip prefixes
                for (const auto& p : skip_mimes) {
                    if (!p.empty()) {
                        if (p.back()=='/' && mime.rfind(p, 0) == 0) {
                            matched_skip_prefix = p;
                            return false; // prefix like image/
                        }
                        if (mime == p) {
                            matched_skip_prefix = p;
                            return false; // exact
                        }
                    }
                }
                return true;
            };

            std::vector<uint8_t> to_store;
            size_t original_size = bb.size();
            [[maybe_unused]] size_t compressed_size = original_size;
            [[maybe_unused]] float compression_ratio = 1.0f;
            
            if (should_compress(meta.mime_type, bb.size())) {
#ifdef THEMIS_HAS_ZSTD
                auto comp = utils::zstd_compress(reinterpret_cast<const uint8_t*>(bb.data()), bb.size(), zstd_level);
                // Only use compressed version if it actually reduces size (avoid decompression overhead for incompressible data)
                if (!comp.empty() && comp.size() < bb.size()) {
                    to_store = std::move(comp);
                    compressed_size = to_store.size();
                    compression_ratio = static_cast<float>(original_size) / static_cast<float>(compressed_size);
                    meta.compressed = true;
                    meta.compression_type = "zstd";
                    THEMIS_INFO("Content blob {} compressed: {}B -> {}B (ratio: {:.2f}x)", 
                               meta.id, original_size, compressed_size, compression_ratio);
                    // Update metrics
                    metrics_.compressed_bytes_total.fetch_add(static_cast<uint64_t>(compressed_size));
                    metrics_.uncompressed_bytes_total.fetch_add(static_cast<uint64_t>(original_size));
                    // compression ratio tracking
                    uint64_t ratio_milli = static_cast<uint64_t>(compression_ratio * 1000.0f);
                    metrics_.comp_ratio_sum_milli.fetch_add(ratio_milli);
                    metrics_.comp_ratio_count.fetch_add(1);
                    // place into per-bucket (non-cumulative)
                    if (compression_ratio <= 1.0f) metrics_.comp_ratio_le_1.fetch_add(1);
                    else if (compression_ratio <= 1.5f) metrics_.comp_ratio_le_1_5.fetch_add(1);
                    else if (compression_ratio <= 2.0f) metrics_.comp_ratio_le_2.fetch_add(1);
                    else if (compression_ratio <= 3.0f) metrics_.comp_ratio_le_3.fetch_add(1);
                    else if (compression_ratio <= 5.0f) metrics_.comp_ratio_le_5.fetch_add(1);
                    else if (compression_ratio <= 10.0f) metrics_.comp_ratio_le_10.fetch_add(1);
                    else if (compression_ratio <= 100.0f) metrics_.comp_ratio_le_100.fetch_add(1);
                    else metrics_.comp_ratio_le_inf.fetch_add(1);
                } else {
                    // Fallback to raw (compression failed or increased size)
                    to_store.assign(bb.begin(), bb.end());
                    meta.compressed = false;
                    meta.compression_type.clear();
                    if (!comp.empty()) {
                        THEMIS_INFO("Content blob {} skipped compression (would increase size: {}B -> {}B)", 
                                   meta.id, original_size, comp.size());
                    }
                }
#else
                // ZSTD not available at build time → store raw
                to_store.assign(bb.begin(), bb.end());
                meta.compressed = false;
                meta.compression_type.clear();
#endif
            } else {
                to_store.assign(bb.begin(), bb.end());
                meta.compressed = false;
                meta.compression_type.clear();
                // If compression was enabled but skipped due to MIME prefix, record skip metrics
                if (compress && !matched_skip_prefix.empty()) {
                    metrics_.compression_skipped_total.fetch_add(1);
                    if (matched_skip_prefix == "image/" || matched_skip_prefix.rfind("image/",0)==0) metrics_.compression_skipped_image_total.fetch_add(1);
                    else if (matched_skip_prefix == "video/" || matched_skip_prefix.rfind("video/",0)==0) metrics_.compression_skipped_video_total.fetch_add(1);
                    else if (matched_skip_prefix == "application/zip" || matched_skip_prefix == "application/gzip") metrics_.compression_skipped_zip_total.fetch_add(1);
                }
            }

            // Optional encryption of blob based on config:content_encryption_schema and user_context
            bool encrypt_blob = false;
            std::string encryption_key_id;
            try {
                if (auto encv = storage_->get("config:content_encryption_schema")) {
                    std::string es(encv->begin(), encv->end());
                    json ej = json::parse(es);
                    // Example schema: {"enabled":true, "key_id":"content_blob", "context":"user"}
                    encrypt_blob = ej.value("enabled", false);
                    encryption_key_id = ej.value("key_id", "content_blob");
                }
            } catch (const json::exception&) {
            } catch (...) {
            }
            if (encrypt_blob && field_encryption_) {
                // Kontextuelle Ableitung via HKDF (salt = user_context) – nutzt aktuelle Key-Version.
                // Falls user_context leer, verwende "anonymous" als Fallback
                std::string ctx = user_context.empty() ? "anonymous" : user_context;
                try {
                    auto kp = field_encryption_->getKeyProvider();
                    auto key_bytes = kp->getKey(encryption_key_id); // latest active
                    auto meta_info = kp->getKeyMetadata(encryption_key_id, 0);
                    // HKDF ableiten (info = "content_blob")
                    std::vector<uint8_t> salt(ctx.begin(), ctx.end());
                    auto derived_key = themis::utils::HKDFCache::threadLocal().derive_cached(key_bytes, salt, "content_blob", key_bytes.size());
                    auto blobEnc = field_encryption_->encryptWithKey(std::string(to_store.begin(), to_store.end()), encryption_key_id, meta_info.version, derived_key);
                    json bjson = blobEnc.toJson();
                    std::string benc = bjson.dump();
                    to_store.assign(benc.begin(), benc.end());
                    meta.encrypted = true;
                    meta.encryption_type = "AES-256-GCM";
                } catch (const std::exception& e) {
                    THEMIS_WARN("blob encryption failed: {}", e.what());
                }
            }
            if (!storage_->put(bkey, to_store)) {
                return Status::Error("failed to store blob");
            }
            meta.size_bytes = static_cast<int64_t>(bb.size());
            // If compression was not applied but compression enabled, still record uncompressed bytes total
            if (compress) {
                // Only add uncompressed total if we didn't already add it for compressed path
                if (!meta.compressed) metrics_.uncompressed_bytes_total.fetch_add(static_cast<uint64_t>(original_size));
            }
        }
        // Chunks verarbeiten
        std::vector<std::string> chunk_ids;
        int embedding_dim = 0;
        std::vector<float> first_chunk_embedding;  // For content-level emb:<id> storage
        
        // Load fulltext index configuration from DB: key config:content
        bool auto_fulltext_index = false;
        SecondaryIndexManager::FulltextConfig fulltext_config;
        try {
            if (auto cfgv = storage_->get("config:content")) {
                std::string s(cfgv->begin(), cfgv->end());
                json cj = json::parse(s);
                auto_fulltext_index = cj.value("auto_fulltext_index", false);
                
                // Parse fulltext config if present
                if (cj.contains("fulltext_config") && cj["fulltext_config"].is_object()) {
                    auto ftcfg = cj["fulltext_config"];
                    fulltext_config.stemming_enabled = ftcfg.value("stemming_enabled", false);
                    fulltext_config.language = ftcfg.value("language", "none");
                    fulltext_config.stopwords_enabled = ftcfg.value("stopwords_enabled", false);
                    fulltext_config.normalize_umlauts = ftcfg.value("normalize_umlauts", false);
                    if (ftcfg.contains("stopwords") && ftcfg["stopwords"].is_array()) {
                        for (const auto& sw : ftcfg["stopwords"]) {
                            if (sw.is_string()) fulltext_config.stopwords.push_back(sw.get<std::string>());
                        }
                    }
                }
            }
        } catch (const nlohmann::json::exception& e) {
            // Config parsing failed - continue with defaults (auto_fulltext_index = false)
            // This is acceptable as the feature is opt-in
            THEMIS_DEBUG("Failed to parse content config for fulltext index: {}", e.what());
        }
        
        // Ensure fulltext index exists if auto-indexing is enabled
        if (auto_fulltext_index && secondary_index_) {
            if (!secondary_index_->hasFulltextIndex("chunk", "text")) {
                auto fulltext_create_result = secondary_index_->createFulltextIndex("chunk", "text", fulltext_config);
                if (fulltext_create_result.ok) {
                    THEMIS_INFO("Created fulltext index for chunk.text with language={}, stemming={}", 
                               fulltext_config.language, fulltext_config.stemming_enabled);
                } else {
                    THEMIS_WARN("Failed to create fulltext index for chunk.text: {}", fulltext_create_result.message);
                }
            }
        }
        
        if (spec.contains("chunks") && spec["chunks"].is_array()) {
            int64_t now = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
            for (const auto& jc : spec["chunks"]) {
                ChunkMeta c = ChunkMeta::fromJson(jc);
                if (c.id.empty()) c.id = generateUuid();
                if (c.content_id.empty()) c.content_id = meta.id;
                if (c.created_at == 0) c.created_at = now;

                // EmbeddingStage: generate embedding for text chunks that do
                // not yet carry one, when the pipeline is attached and enabled.
                // Failures are tracked by EmbeddingPipeline::getFailureCount()
                // and, when EmbeddingPipelineConfig::metrics is set, also
                // propagated to ContentMetrics::recordEmbeddingFailure().
                // Respect the "__embedding_enabled" hint set by ingestRawBlob
                // when the ProcessorChainConfig has embedding disabled.
                const bool embedding_stage_enabled = spec.value("__embedding_enabled", true);
                if (embedding_stage_enabled && embedding_pipeline_ && embedding_pipeline_->isEnabled() &&
                    c.embedding.empty() && !c.text.empty()) {
                    auto emb = embedding_pipeline_->generateEmbedding(c.text);
                    if (!emb.empty()) {
                        c.embedding = std::move(emb);
                    }
                }

                chunk_ids.push_back(c.id);
                // In RocksDB ablegen
                std::string ckey = std::string("chunk:") + c.id;
                std::string cjson = c.toJson().dump();
                if (!storage_->put(ckey, std::vector<uint8_t>(cjson.begin(), cjson.end()))) {
                    return Status::Error("failed to store chunk meta");
                }
                
                // Add to fulltext index if enabled and text is present
                // Note: BaseEntity for fulltext uses chunk ID as PK and includes text field
                if (auto_fulltext_index && secondary_index_ && !c.text.empty()) {
                    BaseEntity chunk_entity = BaseEntity::fromFields(
                        c.id,
                        BaseEntity::FieldMap{
                            {"content_id", c.content_id},
                            {"text", c.text},
                            {"seq_num", static_cast<int64_t>(c.seq_num)},
                            {"chunk_type", c.chunk_type}
                        }
                    );
                    auto fulltext_put_result = secondary_index_->put(std::string(kFulltextChunkTable), chunk_entity);
                    if (!fulltext_put_result.ok) {
                        THEMIS_WARN("Failed to index chunk {} in fulltext index: {}", c.id, fulltext_put_result.message);
                    }
                }
                
                // Embedding in VectorIndex einfügen (falls vorhanden)
                // Note: BaseEntity for vector index uses "chunks:" prefix and includes embedding
                if (!c.embedding.empty() && vector_index_) {
                    if (vector_index_->getDimension() == 0) {
                        (void)vector_index_->init("chunks", static_cast<int>(c.embedding.size()), VectorIndexManager::Metric::COSINE);
                    }
                    if (vector_index_->getDimension() == static_cast<int>(c.embedding.size())) {
                        BaseEntity e = BaseEntity::fromFields(
                            std::string("chunks:") + c.id,
                            BaseEntity::FieldMap{{"content_id", c.content_id}, {"seq_num", static_cast<int64_t>(c.seq_num)}, {"mime_type", meta.mime_type}, {"chunk_type", c.chunk_type}, {"embedding", c.embedding}}
                        );
                        auto st = vector_index_->addEntity(e);
                        if (!st.ok) THEMIS_WARN("Vector index addEntity failed: {}", st.message);
                        embedding_dim = static_cast<int>(c.embedding.size());
                    }
                    // Keep the first chunk's embedding for content-level storage.
                    if (first_chunk_embedding.empty()) {
                        first_chunk_embedding = c.embedding;
                    }
                }
            }
    }
        // Store content-level embedding under emb:<content_id> for direct lookup
        // by ContentId (spec: "cf_embeddings keyed by ContentId").
        if (!first_chunk_embedding.empty()) {
            json emb_json = first_chunk_embedding;
            std::string emb_str = emb_json.dump();
            std::string emb_key = std::string("emb:") + meta.id;
            if (!storage_->put(emb_key, std::vector<uint8_t>(emb_str.begin(), emb_str.end()))) {
                THEMIS_WARN("Failed to store content-level embedding for content:{}", meta.id);
            }
        }

        // Optionale Verschlüsselung von Metadaten-Feldern (vector metadata encryption)
        try {
            bool meta_encrypt_enabled = false;
            std::vector<std::string> meta_fields;
            if (auto mev = storage_->get("config:vector_metadata_encryption")) {
                std::string ms(mev->begin(), mev->end());
                auto mcfg = json::parse(ms);
                meta_encrypt_enabled = mcfg.value("enabled", false);
                if (meta_encrypt_enabled && mcfg.contains("fields") && mcfg["fields"].is_array()) {
                    for (const auto& f : mcfg["fields"]) if (f.is_string()) meta_fields.push_back(f.get<std::string>());
                }
            }
            if (meta_encrypt_enabled && field_encryption_) {
                // Derive base DEK once and then per-field HKDF
                auto kp = field_encryption_->getKeyProvider();
                // Salt = user_context (oder "anonymous")
                std::string ctx = user_context.empty() ? std::string("anonymous") : user_context;
                std::vector<uint8_t> salt(ctx.begin(), ctx.end());
                for (const auto& f : meta_fields) {
                    // Mapping: Feldname auf ContentMeta Struktur
                    // Unterstützte Felder: extracted_metadata, user_metadata, tags
                    //
                    // Use a unique_ptr to own the heap-allocated JSON object for the
                    // "tags" field.  For the other two fields we point at an existing
                    // ContentMeta member — no heap allocation needed.  This eliminates
                    // the previous raw-new / manual-delete pattern (CWE-401 / RAII).
                    std::unique_ptr<nlohmann::json> tags_json_owner;
                    nlohmann::json* target = nullptr;
                    std::optional<nlohmann::json> tags_json_holder;
                    if (f == "extracted_metadata") target = &meta.extracted_metadata;
                    else if (f == "user_metadata") target = &meta.user_metadata;
                    else if (f == "tags") {
                        // tags als Array -> JSON konvertieren
                        tags_json_holder = nlohmann::json(meta.tags);
                        target = &(*tags_json_holder);
                    }
                    if (!target) continue;
                    try {
                        if (target->is_null() || (target->is_object() && target->empty()) || (target->is_array() && target->empty())) {
                            continue; // nichts zu verschlüsseln
                        }
                        std::string plain = target->dump();
                        // HKDF ableiten: info = "vector_meta:" + feld
                        auto dek = kp->getKey("dek");
                        auto meta_info = kp->getKeyMetadata("dek", 0);
                        std::string info = std::string("vector_meta:") + f;
                        auto derived = themis::utils::HKDFCache::threadLocal().derive_cached(dek, salt, info, dek.size());
                        auto blobEnc = field_encryption_->encryptWithKey(plain, std::string("dek"), meta_info.version, derived);
                        std::string enc_b64 = blobEnc.toBase64();
                        // Metadaten ersetzen: <f>_encrypted + <f>_enc Flag, Original entfernen/neutralisieren
                        if (f == "extracted_metadata") {
                            meta.extracted_metadata = json::object(); // leeren
                        } else if (f == "user_metadata") {
                            meta.user_metadata = json::object();
                        } else if (f == "tags") {
                            meta.tags.clear();
                        }
                        // tags_json_owner auto-deleted at end of loop iteration.
                        // Hänge verschlüsselte Strings in eine Zusatzliste (wird später gemerged)
                        // Wir lagern verschlüsselte Meta-Felder im allgemeinen Meta-JSON als Platzhalter unter reserved key
                        // Da ContentMeta::toJson() Felder fix zusammenstellt, hängen wir Zusatzfelder erst nachher an (siehe unten mjsonPatch)
                        // Temporär speichern in map structure
                        // Hänge verschlüsselte Strings in eine Zusatzliste (wird später gemerged)
                        if (!meta.extracted_metadata.contains("__enc_meta")) {
                            meta.extracted_metadata["__enc_meta"] = json::object();
                        }
                        meta.extracted_metadata["__enc_meta"][f + "_encrypted"] = enc_b64;
                        meta.extracted_metadata["__enc_meta"][f + "_enc"] = true;
                    } catch (const std::exception& ex) {
                        THEMIS_WARN("vector metadata encryption field {} failed: {}", f, ex.what());
                    }
                }
            }
        } catch (const std::exception& ex) {
            THEMIS_WARN("vector metadata encryption processing error: {}", ex.what());
        }

    // Content-Meta aktualisieren/speichern (verschlüsselte Felder markiert)
        meta.chunk_count = static_cast<int>(chunk_ids.size());
        meta.chunked = meta.chunk_count > 0;
        if (meta.created_at == 0) meta.created_at = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
        meta.modified_at = meta.created_at;
        if (meta.embedding_dim == 0) meta.embedding_dim = embedding_dim;
        std::string mkey = std::string("content:") + meta.id;
        std::string mjson = meta.toJson().dump();
        if (!storage_->put(mkey, std::vector<uint8_t>(mjson.begin(), mjson.end()))) {
            return Status::Error("failed to store content meta");
        }
        // Persist SHA-256 hash → content_id mapping for exact-duplicate detection.
        if (!meta.hash_sha256.empty()) {
            std::string hkey = std::string("content_hash:") + meta.hash_sha256;
            json hj = json{{"ids", json::array({meta.id})}};
            std::string hstr = hj.dump();
            // Only store if no entry exists yet (first writer wins).
            if (!storage_->get(hkey)) {
                storage_->put(hkey, std::vector<uint8_t>(hstr.begin(), hstr.end()));
            }
        }
        // Chunk-Liste speichern
        std::string lkey = std::string("content_chunks:") + meta.id;
        json lj = json{{"ids", chunk_ids}};
        std::string lstr = lj.dump();
        storage_->put(lkey, std::vector<uint8_t>(lstr.begin(), lstr.end()));
        // Optionale Kanten übernehmen
        if (spec.contains("edges") && spec["edges"].is_array() && graph_index_) {
            for (const auto& je : spec["edges"]) {
                if (!je.is_object()) continue;
                BaseEntity::FieldMap fm;
                for (auto it = je.begin(); it != je.end(); ++it) {
                    const std::string key = it.key();
                    const auto& val = it.value();
                    if (val.is_string()) fm[key] = val.get<std::string>();
                    else if (val.is_number_integer()) fm[key] = static_cast<int64_t>(val.get<int64_t>());
                    else if (val.is_number_float()) fm[key] = val.get<double>();
                    else if (val.is_boolean()) fm[key] = val.get<bool>();
                }
                BaseEntity edge = BaseEntity::fromFields(std::string("graph:edge:"), std::move(fm));
                auto st = graph_index_->addEdge(edge);
                if (!st.ok) THEMIS_WARN("graph addEdge failed: {}", st.message);
            }
        }
        return Status::OK();
    } catch (const std::exception& e) {
        return Status::Error(std::string("import error: ") + e.what());
    }
}

std::optional<ContentMeta> ContentManager::getContentMeta(const std::string& content_id, const std::string& user_context) {
    std::string id = normalizeId(content_id, "content:");
    std::string key = std::string("content:") + id;
    auto v = storage_->get(key);
    if (!v) return std::nullopt;
    try {
        std::string s(v->begin(), v->end());
        json j = json::parse(s);
        // Entschlüsselung verschlüsselter Metadaten falls vorhanden
        if (j.contains("_encrypted_meta") && field_encryption_) {
            bool meta_encrypt_enabled = false;
            std::vector<std::string> meta_fields;
            try {
                if (auto mev = storage_->get("config:vector_metadata_encryption")) {
                    std::string cfgs(mev->begin(), mev->end());
                    auto mcfg = json::parse(cfgs);
                    meta_encrypt_enabled = mcfg.value("enabled", false);
                    if (meta_encrypt_enabled && mcfg.contains("fields") && mcfg["fields"].is_array()) {
                        for (const auto& f : mcfg["fields"]) if (f.is_string()) meta_fields.push_back(f.get<std::string>());
                    }
                }
            } catch (const json::exception&) {
                meta_encrypt_enabled = false;
            } catch (...) {
                meta_encrypt_enabled = false;
            }
            if (meta_encrypt_enabled) {
                auto enc_section = j["_encrypted_meta"];
                std::string ctx = user_context.empty() ? std::string("anonymous") : user_context;
                std::vector<uint8_t> salt(ctx.begin(), ctx.end());
                auto kp = field_encryption_->getKeyProvider();
                auto dek = kp->getKey("dek");
                for (const auto& f : meta_fields) {
                    std::string enc_key = f + std::string("_encrypted");
                    if (!enc_section.contains(enc_key)) continue;
                    try {
                        std::string b64 = enc_section[enc_key].get<std::string>();
                        auto blob = EncryptedBlob::fromBase64(b64);
                        std::string info = std::string("vector_meta:") + f;
                        auto derived = themis::utils::HKDFCache::threadLocal().derive_cached(dek, salt, info, dek.size());
                        std::string plain = field_encryption_->decryptWithKey(blob, derived);
                        auto pj = json::parse(plain);
                        j[f] = pj; // wiederherstellen
                    } catch (const std::exception& exf) {
                        THEMIS_WARN("vector metadata decrypt field {} failed: {}", f, exf.what());
                    }
                }
                j.erase("_encrypted_meta");
            }
        }
        return ContentMeta::fromJson(j);
    } catch (const json::exception&) {
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}
std::optional<std::string> ContentManager::getContentBlob(const std::string& content_id, const std::string& user_context) {
    std::string id = normalizeId(content_id, "content:");
    std::string key = std::string("content_blob:") + id;
    auto v = storage_->get(key);
    if (!v) return std::nullopt;
    // Inspect meta for compression
    auto m = getContentMeta(id);
    if (m && m->encrypted && field_encryption_) {
        // Falls user_context leer, verwende "anonymous" als Fallback für HKDF
        std::string ctx = user_context.empty() ? "anonymous" : user_context;
        try {
            // Attempt decrypt
            std::string raw(v->begin(), v->end());
            json bj = json::parse(raw);
            auto blob = EncryptedBlob::fromJson(bj);
            // Fetch key and derive contextual key same way
            auto kp = field_encryption_->getKeyProvider();
            auto key_bytes = kp->getKey(blob.key_id, blob.key_version);
            std::vector<uint8_t> salt(ctx.begin(), ctx.end());
            auto derived_key = themis::utils::HKDFCache::threadLocal().derive_cached(key_bytes, salt, "content_blob", key_bytes.size());
            std::string plain = field_encryption_->decryptWithKey(blob, derived_key);
            
            // Lazy Re-Encryption: Check if blob uses outdated key version
            bool needs_reencryption = false;
            uint32_t latest_version = 0;
            try {
                auto latest_meta = kp->getKeyMetadata(blob.key_id, 0); // version=0 -> latest
                latest_version = latest_meta.version;
                if (blob.key_version < latest_version) {
                    needs_reencryption = true;
                    THEMIS_INFO("Content blob {} uses outdated key version {} (latest: {}), triggering re-encryption", 
                                id, blob.key_version, latest_version);
                }
            } catch (...) {
                // If metadata check fails, skip re-encryption
            }
            
            if (needs_reencryption) {
                try {
                    // Re-encrypt with latest key version
                    auto latest_key = kp->getKey(blob.key_id); // no version arg -> latest
                    auto latest_derived = themis::utils::HKDFCache::threadLocal().derive_cached(latest_key, salt, "content_blob", latest_key.size());
                    auto new_blob = field_encryption_->encryptWithKey(plain, blob.key_id, latest_version, latest_derived);
                    json new_bj = new_blob.toJson();
                    std::string new_raw = new_bj.dump();
                    std::vector<uint8_t> new_store(new_raw.begin(), new_raw.end());
                    // Update storage with new encrypted version
                    if (storage_->put(key, new_store)) {
                        THEMIS_INFO("Content blob {} successfully re-encrypted to version {}", id, latest_version);
                    } else {
                        THEMIS_WARN("Failed to update storage after re-encryption for blob {}", id);
                    }
                } catch (const std::exception& re_ex) {
                    THEMIS_WARN("Re-encryption failed for blob {}: {}", id, re_ex.what());
                }
            }
            
            // Handle compression after decryption (ciphertext stored compressed? We encrypted post-compression so meta.compressed indicates original compression state)
            if (m->compressed && m->compression_type == "zstd") {
#ifdef THEMIS_HAS_ZSTD
                // Stored was compressed before encryption; decrypt returns compressed bytes now
                std::vector<uint8_t> tmp(plain.begin(), plain.end());
                auto decomp = utils::zstd_decompress(tmp);
                if (!decomp.empty()) return std::string(decomp.begin(), decomp.end());
#endif
            }
            return plain;
        } catch (const std::exception& e) {
            THEMIS_WARN("blob decrypt failed: {}", e.what());
        }
    }
    if (m && m->compressed && m->compression_type == "zstd") {
#ifdef THEMIS_HAS_ZSTD
        auto decomp = utils::zstd_decompress(*v);
        if (!decomp.empty()) return std::string(decomp.begin(), decomp.end());
        // Fallback on failure: return raw
        return std::string(v->begin(), v->end());
#else
        // Build without ZSTD: return raw bytes (client may handle)
        return std::string(v->begin(), v->end());
#endif
    }
    return std::string(v->begin(), v->end());
}

std::vector<ChunkMeta> ContentManager::getContentChunks(const std::string& content_id) {
    std::vector<ChunkMeta> out;
    std::string id = normalizeId(content_id, "content:");
    std::string lkey = std::string("content_chunks:") + id;
    auto lv = storage_->get(lkey);
    if (!lv) return out;
    std::vector<std::string> ids;
    try {
        std::string s(lv->begin(), lv->end());
        json j = json::parse(s);
        if (j.contains("ids")) ids = j["ids"].get<std::vector<std::string>>();
    } catch (const json::exception&) {
        return out;
    } catch (...) {
        return out;
    }
    for (const auto& cid : ids) {
        auto v = storage_->get(std::string("chunk:") + cid);
        if (!v) continue;
        try {
            std::string s(v->begin(), v->end());
            json j = json::parse(s);
            out.push_back(ChunkMeta::fromJson(j));
        } catch (const json::exception&) {
            continue;
        } catch (...) {
            continue;
        }
    }
    std::sort(out.begin(), out.end(), [](const ChunkMeta& a, const ChunkMeta& b){ return a.seq_num < b.seq_num; });
    return out;
}

std::optional<ChunkMeta> ContentManager::getChunk(const std::string& chunk_id) {
    std::string id = normalizeId(chunk_id, "chunk:");
    auto v = storage_->get(std::string("chunk:") + id);
    if (!v) return std::nullopt;
    try {
        std::string s(v->begin(), v->end());
        json j = json::parse(s);
        return ChunkMeta::fromJson(j);
    } catch (const json::exception&) {
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

// ===================== Content Assembly & Navigation =====================

std::optional<ChunkMeta> ContentAssembly::getChunkBySeqNum(int seq_num) const {
    for (const auto& chunk : chunks) {
        if (chunk.seq_num == seq_num) {
            return chunk;
        }
    }
    return std::nullopt;
}

std::optional<ContentAssembly> ContentManager::assembleContent(const std::string& content_id, bool include_text) {
    // Get metadata
    auto meta = getContentMeta(content_id);
    if (!meta.has_value()) {
        return std::nullopt;
    }
    
    // Get all chunks
    auto chunks = getContentChunks(content_id);
    
    // Build assembly
    ContentAssembly assembly;
    assembly.metadata = *meta;
    assembly.chunks = chunks;
    assembly.total_size_bytes = 0;
    
    // Calculate total size
    for (const auto& chunk : chunks) {
        assembly.total_size_bytes += chunk.text.size();
    }
    
    // Optionally assemble full text
    if (include_text && !chunks.empty()) {
        std::string full_text;
        full_text.reserve(assembly.total_size_bytes);
        
        for (const auto& chunk : chunks) {
            full_text += chunk.text;
        }
        
        assembly.assembled_text = std::move(full_text);
    }
    
    return assembly;
}

std::optional<ChunkMeta> ContentManager::getNextChunk(const std::string& chunk_id) {
    // Get current chunk
    auto current = getChunk(chunk_id);
    if (!current.has_value()) {
        return std::nullopt;
    }
    
    // Get all chunks for this content
    auto chunks = getContentChunks(current->content_id);
    
    // Find next chunk by seq_num
    int next_seq = current->seq_num + 1;
    for (const auto& chunk : chunks) {
        if (chunk.seq_num == next_seq) {
            return chunk;
        }
    }
    
    return std::nullopt;
}

std::optional<ChunkMeta> ContentManager::getPreviousChunk(const std::string& chunk_id) {
    // Get current chunk
    auto current = getChunk(chunk_id);
    if (!current.has_value()) {
        return std::nullopt;
    }
    
    // Get all chunks for this content
    auto chunks = getContentChunks(current->content_id);
    
    // Find previous chunk by seq_num
    int prev_seq = current->seq_num - 1;
    if (prev_seq < 0) {
        return std::nullopt;
    }
    
    for (const auto& chunk : chunks) {
        if (chunk.seq_num == prev_seq) {
            return chunk;
        }
    }
    
    return std::nullopt;
}

std::vector<ChunkMeta> ContentManager::getChunkRange(const std::string& content_id, int start_seq, int count) {
    std::vector<ChunkMeta> result;
    
    if (count <= 0) {
        return result;
    }
    
    // Get all chunks
    auto all_chunks = getContentChunks(content_id);
    
    // Filter by sequence range
    int end_seq = start_seq + count - 1;
    for (const auto& chunk : all_chunks) {
        if (chunk.seq_num >= start_seq && chunk.seq_num <= end_seq) {
            result.push_back(chunk);
        }
    }
    
    // Already sorted by seq_num from getContentChunks
    return result;
}

std::vector<std::pair<std::string, float>> ContentManager::searchContent(
    const std::string& query_text, int k, const json& filters
) {
    std::vector<std::pair<std::string, float>> res;
    if (!vector_index_ || vector_index_->getDimension() <= 0) return res;

    // Simple text embedding via TextProcessor if available
    auto it = processors_.find(ContentCategory::TEXT);
    if (it == processors_.end()) return res;
    std::vector<float> q = it->second->generateEmbedding(query_text);
    // Optional: Build whitelist from filters to pre-filter vector search
    std::vector<std::string> whitelist = buildChunkWhitelist(*storage_, filters);
    if (hasSearchFilterConstraints(filters) && whitelist.empty()) {
        return res;
    }
    const std::vector<std::string>* wptr = whitelist.empty() ? nullptr : &whitelist;
    auto [st, results] = vector_index_->searchKnn(q, static_cast<size_t>(k), wptr);
    if (!st.ok) return res;
    for (const auto& r : results) {
        res.emplace_back(r.pk, r.distance);
    }
    return res;
}

std::vector<std::pair<std::string, float>> ContentManager::searchContentHybrid(
    const std::string& query_text,
    int k,
    const json& filters,
    float vector_weight,
    float fulltext_weight,
    float rrf_k
) {
    std::vector<std::pair<std::string, float>> result;
    
    // Step 1: Vector Search (HNSW)
    std::unordered_map<std::string, float> vector_scores;
    std::unordered_map<std::string, size_t> vector_ranks;
    const bool has_filter_constraints = hasSearchFilterConstraints(filters);
    std::vector<std::string> whitelist = has_filter_constraints
                                       ? buildChunkWhitelist(*storage_, filters)
                                       : std::vector<std::string>{};
    if (has_filter_constraints && whitelist.empty()) {
        return result;
    }
    const std::vector<std::string>* whitelist_ptr = whitelist.empty() ? nullptr : &whitelist;
    std::unordered_set<std::string> whitelist_set(whitelist.begin(), whitelist.end());
    
    if (vector_index_ && vector_index_->getDimension() > 0 && vector_weight > 0.0f) {
        auto it = processors_.find(ContentCategory::TEXT);
        if (it != processors_.end()) {
            std::vector<float> q = it->second->generateEmbedding(query_text);
            
            // Retrieve more results for better RRF fusion (k*2)
            size_t fetch_k = static_cast<size_t>(k * 2);
            auto [st, results] = vector_index_->searchKnn(q, fetch_k, whitelist_ptr);
            
            if (st.ok) {
                size_t rank = 1;
                for (const auto& r : results) {
                    // Convert distance to similarity score
                    // For COSINE: similarity = 1 - distance
                    // For L2: similarity = 1 / (1 + distance)
                    float similarity = 0.0f;
                    auto metric = vector_index_->getMetric();
                    if (metric == VectorIndexManager::Metric::COSINE) {
                        similarity = std::max(0.0f, 1.0f - r.distance);
                    } else {
                        similarity = 1.0f / (1.0f + r.distance);
                    }
                    
                    vector_scores[r.pk] = similarity;
                    vector_ranks[r.pk] = rank++;
                }
            }
        }
    }
    
    // Step 2: Fulltext Search (BM25)
    std::unordered_map<std::string, float> fulltext_scores;
    std::unordered_map<std::string, size_t> fulltext_ranks;
    
    if (secondary_index_ && fulltext_weight > 0.0f) {
        if (secondary_index_->hasFulltextIndex(kFulltextChunkTable, kFulltextChunkTextColumn)) {
            size_t fetch_k = static_cast<size_t>(k * 2);
            auto [st, ft_results] = secondary_index_->scanFulltextWithScores(
                kFulltextChunkTable,
                kFulltextChunkTextColumn,
                query_text, 
                fetch_k
            );
            
            if (st.ok) {
                size_t rank = 1;
                for (const auto& r : ft_results) {
                    if (has_filter_constraints && !whitelistContainsChunkPk(whitelist_set, r.pk)) {
                        continue;
                    }
                    
                    fulltext_scores[r.pk] = static_cast<float>(r.score);
                    fulltext_ranks[r.pk] = rank++;
                }
            }
        }
    }
    
    // Step 3: Reciprocal Rank Fusion (RRF)
    // RRF formula: score = sum_i [ weight_i / (k + rank_i) ]
    // where k is typically 60, rank_i is the rank in result set i
    
    std::unordered_map<std::string, float> rrf_scores;
    
    // Combine vector scores
    for (const auto& [chunk_id, rank] : vector_ranks) {
        float rrf_score = vector_weight / (rrf_k + static_cast<float>(rank));
        rrf_scores[chunk_id] += rrf_score;
    }
    
    // Combine fulltext scores
    for (const auto& [chunk_id, rank] : fulltext_ranks) {
        float rrf_score = fulltext_weight / (rrf_k + static_cast<float>(rank));
        rrf_scores[chunk_id] += rrf_score;
    }
    
    // Step 4: Sort by combined RRF score and return top-k
    for (const auto& [chunk_id, score] : rrf_scores) {
        result.emplace_back(chunk_id, score);
    }
    
    std::sort(result.begin(), result.end(), 
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    if (result.size() > static_cast<size_t>(k)) {
        result.resize(k);
    }
    
    return result;
}

std::vector<std::pair<std::string, float>> ContentManager::searchWithExpansion(
    const std::string& query_text, int k, int expansion_hops, const json& filters
) {
    // 1) Basissuche (Vector Top-K)
    auto base = searchContent(query_text, k, filters);
    if (base.empty()) return base;

    // Scoring-Parameter (optional aus filters.scoring)
    double alpha = 1.0; // Gewicht für Vektor-Ähnlichkeit
    double beta = 0.0;  // reserviert (Graph-Distanz, z. B. Dijkstra)
    double gamma = 0.1; // Hop-Strafterm
    try {
        if (filters.contains("scoring")) {
            const auto& sc = filters["scoring"];
            if (sc.contains("alpha")) alpha = sc["alpha"].get<double>();
            if (sc.contains("beta")) beta = sc["beta"].get<double>();
            if (sc.contains("gamma")) gamma = sc["gamma"].get<double>();
        }
    } catch (const nlohmann::json::exception&) {
    } catch (...) {
    }

    // Erzeuge Map pk->score und Queue für Expansion
    std::unordered_map<std::string, double> bestScore; bestScore.reserve(base.size()*2);
    struct QItem { std::string origin; std::string node; int hop = 0; };  // CON-015: explicit default for hop
    std::queue<QItem> q;

    // Metrik beachten: COSINE liefert distance = 1 - cosine → similarity = 1 - d
    auto metric = vector_index_ ? vector_index_->getMetric() : VectorIndexManager::Metric::COSINE;
    auto toSim = [&](float distance) -> double {
        if (metric == VectorIndexManager::Metric::COSINE) return 1.0 - static_cast<double>(distance);
        // L2: invertiert, grob normalisiert
        return -static_cast<double>(distance);
    };

    for (const auto& [pk, dist] : base) {
        double sim = toSim(dist);
        double score = alpha * sim; // basis ohne Graphanteil
        auto it = bestScore.find(pk);
        if (it == bestScore.end() || score > it->second) bestScore[pk] = score;
        if (expansion_hops > 0) q.push(QItem{pk, pk, 0});
    }

    // 2) Graph-Expansion (BFS bis expansion_hops)
    if (graph_index_ && expansion_hops > 0) {
        std::unordered_set<std::string> seen;
        while (!q.empty()) {
            QItem qi = q.front(); q.pop();
            if (qi.hop >= expansion_hops) continue;
            auto [st, neigh] = graph_index_->outNeighbors(qi.node);
            if (!st.ok) continue;
            int nextHop = qi.hop + 1;
            for (const auto& nb : neigh) {
                // optional: allow revisits if better path from different origin
                // Compute Dijkstra distance from origin to nb (weighted graph)
                double distCost = 0.0;
                if (beta != 0.0) {
                    auto pr = graph_index_->dijkstra(qi.origin, nb);
                    if (pr.first.ok) distCost = pr.second.totalCost; else distCost = static_cast<double>(nextHop);
                } else {
                    distCost = static_cast<double>(nextHop);
                }
                double expandedScore = - gamma * static_cast<double>(nextHop) - beta * distCost;
                auto it2 = bestScore.find(nb);
                if (it2 == bestScore.end() || expandedScore > it2->second) bestScore[nb] = expandedScore;
                q.push(QItem{qi.origin, nb, nextHop});
            }
        }
    }

    // 3) Ergebnisse zusammenstellen und nach Score sortieren
    std::vector<std::pair<std::string, float>> out;
    out.reserve(bestScore.size());
    for (const auto& kv : bestScore) {
        out.emplace_back(kv.first, static_cast<float>(kv.second));
    }
    // Optional: Post-Filter gegen erlaubte Whitelist (sichert, dass Expansion nicht aus Filters ausbricht)
    try {
        std::vector<std::string> allow = buildChunkWhitelist(*storage_, filters);
        if (!allow.empty()) {
            std::unordered_set<std::string> allowed(allow.begin(), allow.end());
            out.erase(std::remove_if(out.begin(), out.end(), [&](const auto& p){ return allowed.find(p.first) == allowed.end(); }), out.end());
        }
    } catch (...) {
    }

    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b){ return a.second > b.second; });
    if (out.size() > static_cast<size_t>(k)) out.resize(static_cast<size_t>(k));
    return out;
}

Status ContentManager::deleteContent(const std::string& content_id) {
    std::string id = normalizeId(content_id, "content:");
    // Load chunks
    auto chunks = getContentChunks(id);
    for (const auto& c : chunks) {
        storage_->del(std::string("chunk:") + c.id);
        // Remove vector object if present
        if (vector_index_) {
            vector_index_->removeByPk(std::string("chunks:") + c.id);
        }
        // Remove from fulltext index if present
        if (secondary_index_ && secondary_index_->hasFulltextIndex("chunk", "text")) {
            auto fulltext_erase_result = secondary_index_->erase("chunk", c.id);
            if (!fulltext_erase_result.ok) {
                THEMIS_WARN("Failed to remove chunk {} from fulltext index: {}", c.id, fulltext_erase_result.message);
            }
        }
    }
    storage_->del(std::string("content_chunks:") + id);
    storage_->del(std::string("content_blob:") + id);
    storage_->del(std::string("content:") + id);
    return Status::OK();
}

IContentProcessor* ContentManager::getProcessor(ContentCategory category) {
    auto it = processors_.find(category);
    if (it == processors_.end()) return nullptr;
    return it->second.get();
}

// ===================== Virtual Filesystem =====================

std::optional<std::string> ContentManager::resolvePath(const std::string& virtual_path) {
    std::optional<std::string> result;
    
    // Normalize path (remove trailing slash, ensure leading slash)
    std::string normalized = virtual_path;
    if (!normalized.empty() && normalized.back() == '/') {
        normalized.pop_back();
    }
    if (normalized.empty() || normalized[0] != '/') {
        normalized = "/" + normalized;
    }
    
    // Scan all content items for matching virtual_path
    storage_->scanPrefix("content:", [&](std::string_view key, std::string_view value) {
        try {
            json j = json::parse(value);
            if (j.contains("virtual_path") && j["virtual_path"].get<std::string>() == normalized) {
                // Extract content ID from key "content:<uuid>"
                std::string key_str(key);
                if (key_str.size() > 8) {
                    result = key_str.substr(8); // Skip "content:"
                }
                return false; // Stop scanning
            }
        } catch (const nlohmann::json::exception&) {
        } catch (...) {
        }
        return true; // Continue scanning
    });
    
    return result;
}

std::vector<ContentMeta> ContentManager::listDirectory(const std::string& virtual_path) {
    std::vector<ContentMeta> results;
    
    // Normalize directory path
    std::string dir_path = virtual_path;
    if (!dir_path.empty() && dir_path.back() == '/') {
        dir_path.pop_back();
    }
    if (dir_path.empty()) {
        dir_path = "/";
    }
    
    // Find all content with this directory as parent
    // Strategy: Find directory content by path, then get children
    auto dir_id = resolvePath(dir_path);
    
    if (dir_id.has_value()) {
        // List children by parent_id
        storage_->scanPrefix("content:", [&](std::string_view /*key*/, std::string_view value) {
            try {
                json j = json::parse(value);
                if (j.contains("parent_id") && j["parent_id"].get<std::string>() == *dir_id) {
                    results.push_back(ContentMeta::fromJson(j));
                }
            } catch (const nlohmann::json::exception&) {
            } catch (...) {
            }
            return true;
        });
    } else {
        // If directory doesn't exist, list items directly under this path
        std::string prefix = dir_path;
        if (prefix != "/") prefix += "/";
        
        storage_->scanPrefix("content:", [&](std::string_view /*key*/, std::string_view value) {
            try {
                json j = json::parse(value);
                if (j.contains("virtual_path")) {
                    std::string vpath = j["virtual_path"].get<std::string>();
                    // Check if this is a direct child
                    if (vpath.size() > prefix.size() && vpath.rfind(prefix, 0) == 0) {
                        std::string remainder = vpath.substr(prefix.size());
                        // Direct child if no more slashes
                        if (remainder.find('/') == std::string::npos) {
                            results.push_back(ContentMeta::fromJson(j));
                        }
                    }
                }
            } catch (const nlohmann::json::exception&) {
            } catch (...) {
            }
            return true;
        });
    }
    
    return results;
}

Status ContentManager::createDirectory(const std::string& virtual_path, bool recursive) {
    // Normalize path
    std::string normalized = virtual_path;
    if (!normalized.empty() && normalized.back() == '/') {
        normalized.pop_back();
    }
    if (normalized.empty() || normalized[0] != '/') {
        normalized = "/" + normalized;
    }
    
    // Check if already exists
    auto existing = resolvePath(normalized);
    if (existing.has_value()) {
        return Status::Error("Directory already exists: " + normalized);
    }
    
    // If recursive, create parent directories first
    if (recursive) {
        size_t pos = normalized.rfind('/');
        if (pos > 0) {
            std::string parent = normalized.substr(0, pos);
            auto parent_create_result = createDirectory(parent, true);
            if (!parent_create_result.ok) {
                return parent_create_result;
            }
        }
    }
    
    // Create directory entry
    ContentMeta meta;
    meta.id = generateUuid();
    meta.virtual_path = normalized;
    meta.is_directory = true;
    meta.mime_type = "inode/directory";
    meta.category = ContentCategory::BINARY;
    meta.original_filename = normalized.substr(normalized.rfind('/') + 1);
    meta.size_bytes = 0;
    meta.created_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    meta.modified_at = meta.created_at;
    meta.text_extracted = false;
    meta.chunked = false;
    meta.indexed = false;
    meta.chunk_count = 0;
    meta.embedding_dim = 0;
    
    // Set parent_id if not root
    size_t parent_pos = normalized.rfind('/');
    if (parent_pos > 0) {
        std::string parent_path = normalized.substr(0, parent_pos);
        auto parent_id = resolvePath(parent_path);
        if (parent_id.has_value()) {
            meta.parent_id = *parent_id;
        }
    }
    
    // Store directory metadata
    std::string key = "content:" + meta.id;
    std::string value = meta.toJson().dump();
    if (!storage_->put(key, value)) {
        return Status::Error("Failed to store directory metadata");
    }
    
    return Status::OK();
}

Status ContentManager::registerPath(const std::string& content_id, const std::string& virtual_path) {
    // Normalize path
    std::string normalized = virtual_path;
    if (!normalized.empty() && normalized.back() == '/') {
        normalized.pop_back();
    }
    if (normalized.empty() || normalized[0] != '/') {
        normalized = "/" + normalized;
    }
    
    // Check if path already taken
    auto existing = resolvePath(normalized);
    if (existing.has_value() && *existing != content_id) {
        return Status::Error("Path already exists: " + normalized);
    }
    
    // Load content metadata
    std::string key = "content:" + content_id;
    auto val = storage_->get(key);
    if (!val.has_value()) {
        return Status::Error("Content not found: " + content_id);
    }
    
    try {
        json j = json::parse(*val);
        j["virtual_path"] = normalized;
        
        // Set parent_id based on path hierarchy
        size_t parent_pos = normalized.rfind('/');
        if (parent_pos > 0) {
            std::string parent_path = normalized.substr(0, parent_pos);
            auto parent_id = resolvePath(parent_path);
            if (parent_id.has_value()) {
                j["parent_id"] = *parent_id;
            }
        }
        
        // Update stored metadata
        if (!storage_->put(key, j.dump())) {
            return Status::Error("Failed to update content metadata");
        }
        
        return Status::OK();
    } catch (const json::exception& e) {
        return Status::Error(std::string("JSON error: ") + e.what());
    }
}

ContentManager::IngestResult ContentManager::ingestRawBlob(
    const std::string& blob,
    const std::string& filename,
    const std::string& mime_type,
    const std::string& user_context,
    const json& config
) {
    IngestResult result;
    result.success = false;

    // Validate filename for path traversal and other security issues
    if (!filename.empty()) {
        ContentValidator upload_validator;
        auto fn_err = upload_validator.validateFilename(filename);
        if (fn_err.failed()) {
            result.error_message = fn_err.message;
            return result;
        }
    }
    
    // Detect content type and category
    std::string detected_mime = mime_type;
    if (detected_mime.empty()) {
        auto& registry = ContentTypeRegistry::instance();
        auto type = registry.detectFromBlob(blob);
        if (type) {
            detected_mime = type->mime_type;
        } else {
            // Fallback to filename extension
            auto ext_pos = filename.find_last_of('.');
            if (ext_pos != std::string::npos) {
                std::string ext = filename.substr(ext_pos);
                type = registry.getByExtension(ext);
                if (type) detected_mime = type->mime_type;
            }
        }
    }
    
    if (detected_mime.empty()) {
        result.error_message = "Unable to detect content type";
        return result;
    }
    
    // Get category
    auto& registry = ContentTypeRegistry::instance();
    auto type = registry.getByMimeType(detected_mime);
    ContentCategory category = type ? type->category : ContentCategory::UNKNOWN;
    
    // Handle archives specially (ONLY if archive processor is registered - plugin design)
    if (category == ContentCategory::ARCHIVE) {
        // Try to get archive processor (may not be available - plugin design)
        auto proc = getProcessor(category);
        if (!proc) {
            // No archive processor available - fall back to storing as blob with metadata only
            THEMIS_INFO("Archive processor not available (plugin not loaded), storing as blob with metadata");
            
            // Store as regular content without extraction
            std::string content_id = generateUuid();
            
            ContentMeta meta;
            meta.id = content_id;
            meta.mime_type = detected_mime;
            meta.category = category;
            meta.original_filename = filename;
            meta.size_bytes = static_cast<int64_t>(blob.size());
            meta.created_at = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
            meta.modified_at = meta.created_at;
            meta.hash_sha256 = computeSHA256(blob);
            meta.extracted_metadata = json{
                {"archive_processing", "not_available"},
                {"note", "Archive processor plugin not loaded"}
            };
            
            json spec = {
                {"content", meta.toJson()}
            };
            
            auto import_result = importContent(spec, blob, user_context);
            if (!import_result.ok) {
                result.error_message = import_result.message;
                return result;
            }
            
            result.success = true;
            result.primary_content_id = content_id;
            result.metadata = json{
                {"content_id", content_id},
                {"mime_type", detected_mime},
                {"category", static_cast<int>(category)},
                {"archive_processing", "disabled"}
            };
            
            return result;
        }
        
        // Archive processor is available - use it
        auto* archive_proc = dynamic_cast<content::ArchiveProcessor*>(proc);
        if (!archive_proc) {
            result.error_message = "Archive processor not properly registered";
            return result;
        }
        
        // Configure archive processor from config
        auto proc_config = archive_proc->getConfig();
        if (config.contains("archive_strategy")) {
            std::string strategy = config["archive_strategy"].get<std::string>();
            if (strategy == "EXTRACT_AND_INGEST") {
                proc_config.strategy = content::ArchiveStrategy::EXTRACT_AND_INGEST;
            } else if (strategy == "METADATA_ONLY") {
                proc_config.strategy = content::ArchiveStrategy::METADATA_ONLY;
            } else if (strategy == "REJECT") {
                proc_config.strategy = content::ArchiveStrategy::REJECT;
            }
        }
        
        if (config.contains("encrypted_policy")) {
            std::string policy = config["encrypted_policy"].get<std::string>();
            if (policy == "REJECT") {
                proc_config.encrypted_policy = content::EncryptedArchivePolicy::REJECT;
            } else if (policy == "METADATA_ONLY") {
                proc_config.encrypted_policy = content::EncryptedArchivePolicy::METADATA_ONLY;
            } else if (policy == "REQUIRE_PASSWORD") {
                proc_config.encrypted_policy = content::EncryptedArchivePolicy::REQUIRE_PASSWORD;
                if (config.contains("password")) {
                    proc_config.password = config["password"].get<std::string>();
                }
            }
        }
        
        archive_proc->setConfig(proc_config);
        
        // Process archive
        auto proc_result = archive_proc->process(blob, detected_mime, filename);
        if (!proc_result.success) {
            result.error_message = proc_result.error_message;
            return result;
        }
        
        // Generate archive content ID
        std::string archive_id = generateUuid();
        result.primary_content_id = archive_id;
        
        // Create archive metadata
        ContentMeta archive_meta;
        archive_meta.id = archive_id;
        archive_meta.mime_type = detected_mime;
        archive_meta.category = ContentCategory::ARCHIVE;
        archive_meta.original_filename = filename;
        archive_meta.size_bytes = static_cast<int64_t>(blob.size());
        archive_meta.compressed = false;
        archive_meta.encrypted = false;
        archive_meta.created_at = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
        archive_meta.modified_at = archive_meta.created_at;
        archive_meta.hash_sha256 = computeSHA256(blob);
        archive_meta.text_extracted = false;
        archive_meta.chunked = false;
        archive_meta.indexed = false;
        archive_meta.chunk_count = 0;
        archive_meta.extracted_metadata = proc_result.metadata;
        
        // Store archive metadata
        json archive_spec = {
            {"content", archive_meta.toJson()}
        };
        
        auto import_result = importContent(archive_spec, blob, user_context);
        if (!import_result.ok) {
            result.error_message = "Failed to store archive: " + import_result.message;
            return result;
        }
        
        // Handle extraction strategy
        if (proc_config.strategy == content::ArchiveStrategy::EXTRACT_AND_INGEST && 
            proc_result.metadata.contains("extracted_files")) {
            
            auto extracted_files = proc_result.metadata["extracted_files"];
            std::string temp_dir = proc_result.metadata.value("temp_directory", "");
            
            // Ingest each extracted file
            for (const auto& file_path : extracted_files) {
                std::string path_str = file_path.get<std::string>();
                
                // Read extracted file
                std::ifstream file(path_str, std::ios::binary);
                if (!file) continue;
                
                std::string file_blob(
                    (std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>()
                );
                // std::ifstream closed automatically (RAII) at end of loop iteration.
                
                // Get relative path within archive
                std::string relative_path = path_str;
                if (!temp_dir.empty() && path_str.find(temp_dir) == 0) {
                    relative_path = path_str.substr(temp_dir.size());
                    if (!relative_path.empty() && relative_path[0] == '/') {
                        relative_path = relative_path.substr(1);
                    }
                }
                
                // Extract just the filename
                auto filename_pos = relative_path.find_last_of('/');
                std::string extracted_filename = (filename_pos != std::string::npos) 
                    ? relative_path.substr(filename_pos + 1) 
                    : relative_path;
                
                // Recursively ingest (handles nested archives too)
                auto nested_result = ingestRawBlob(file_blob, extracted_filename, "", user_context, config);
                
                if (nested_result.success) {
                    result.extracted_content_ids.push_back(nested_result.primary_content_id);
                    
                    // Create graph edge: archive -> extracted_file (only if graph_index available)
                    if (graph_index_) {
                        try {
                            // Create edge entity using BaseEntity::FieldMap
                            BaseEntity::FieldMap edge_fields;
                            edge_fields["id"] = "edge:" + archive_id + ":" + nested_result.primary_content_id;
                            edge_fields["_from"] = "content:" + archive_id;
                            edge_fields["_to"] = "content:" + nested_result.primary_content_id;
                            edge_fields["_label"] = "CONTAINS";
                            edge_fields["original_path"] = relative_path;
                            edge_fields["extraction_order"] = static_cast<int64_t>(result.extracted_content_ids.size() - 1);
                            
                            BaseEntity edge = BaseEntity::fromFields(
                                "edge:" + archive_id + ":" + nested_result.primary_content_id,
                                edge_fields
                            );
                            
                            graph_index_->addEdge(edge);
                        } catch (const std::exception& e) {
                            THEMIS_WARN("Failed to create graph edge for archive member: {}", e.what());
                        }
                    }
                    
                    // Update extracted file's parent_id
                    auto member_meta = getContentMeta(nested_result.primary_content_id);
                    if (member_meta) {
                        member_meta->parent_id = archive_id;
                        member_meta->virtual_path = "/" + filename + "/" + relative_path;
                        
                        // Update stored metadata
                        std::string meta_key = std::string("content:") + nested_result.primary_content_id;
                        std::string meta_json = member_meta->toJson().dump();
                        storage_->put(meta_key, std::vector<uint8_t>(meta_json.begin(), meta_json.end()));
                    }
                }
            }
            
            // Update archive metadata with child IDs
            archive_meta.child_ids = result.extracted_content_ids;
            std::string archive_key = std::string("content:") + archive_id;
            std::string archive_json = archive_meta.toJson().dump();
            storage_->put(archive_key, std::vector<uint8_t>(archive_json.begin(), archive_json.end()));
            
            // Cleanup temporary directory
            if (!temp_dir.empty()) {
                content::ArchiveProcessor::cleanupTempDirectory(temp_dir);
            }
        }
        
        result.success = true;
        result.metadata = proc_result.metadata;
        result.metadata["archive_id"] = archive_id;
        result.metadata["extracted_count"] = result.extracted_content_ids.size();
        
        return result;
    }
    
    // For non-archive content, create a simple import spec
    std::string content_id = generateUuid();
    
    ContentMeta meta;
    meta.id = content_id;
    meta.mime_type = detected_mime;
    meta.category = category;
    meta.original_filename = filename;
    meta.size_bytes = static_cast<int64_t>(blob.size());
    meta.created_at = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    meta.modified_at = meta.created_at;
    meta.hash_sha256 = computeSHA256(blob);

    // ---- Exact-duplicate detection via SHA-256 hash ----
    // Check before any processing; no dedup_checker or policy gate required.
    {
        auto existing_id = checkDuplicateByHash(meta.hash_sha256);
        if (existing_id) {
            THEMIS_INFO("Dedup (SHA-256): exact duplicate of '{}' detected for '{}'",
                        *existing_id, filename);
            result.success = true;
            result.primary_content_id = *existing_id;
            result.metadata = json{
                {"content_id",   *existing_id},
                {"duplicate_of", *existing_id},
                {"sha256",       meta.hash_sha256},
                {"mime_type",    detected_mime}
            };
            return result;
        }
    }

    // Determine the effective processing stage configuration for this content type.
    const ContentTypePipelineConfig stage_cfg =
        processor_chain_config_.getEffectiveConfig(detected_mime, category);

    // ---- Perceptual deduplication (opt-in via ContentPolicy::enable_deduplication) ----
    // Callers pass `config["enable_deduplication"] = policy.enable_deduplication`.
    // The default is false (opt-in, not opt-out): dedup is skipped unless the caller
    // explicitly enables it.  ProcessorChainConfig can still disable it globally per
    // MIME/category by setting deduplication.enabled=false; both conditions must be
    // true for dedup to run.
    // Compute pHash (image) or MinHash (text) once; reuse for both the duplicate
    // check and the post-storage registration to avoid redundant computation.
    const bool dedup_policy_enabled =
        config.value("enable_deduplication", false) && stage_cfg.deduplication.enabled;
    const bool dedup_is_image = (category == ContentCategory::IMAGE);
    const bool dedup_is_text  = (category == ContentCategory::TEXT);
    std::string cached_phash;
    std::vector<uint32_t> cached_minhash;

    if (dedup_policy_enabled && dedup_checker_ && (dedup_is_image || dedup_is_text)) {
        metrics_.dedup_checks_total.fetch_add(1);

        std::optional<DuplicateOf> dup;
        if (dedup_is_image) {
            cached_phash = computeImageDedupHash(blob);
            if (!cached_phash.empty()) {
                dup = dedup_checker_->isDuplicateImage(cached_phash);
            }
        } else {
            cached_minhash = TextProcessor::computeMinHash(blob);
            if (!cached_minhash.empty()) {
                dup = dedup_checker_->isDuplicateText(cached_minhash);
            }
        }

        if (dup) {
            metrics_.dedup_hits_total.fetch_add(1);
            THEMIS_INFO("Dedup: near-duplicate of '{}' detected for '{}' (similarity={:.3f})",
                        dup->existing_id, filename, dup->similarity);
            result.success = true;
            result.primary_content_id = dup->existing_id;
            result.metadata = json{
                {"content_id",   dup->existing_id},
                {"duplicate_of", dup->existing_id},
                {"similarity",   dup->similarity},
                {"mime_type",    detected_mime}
            };
            return result;
        }
    }
    json chunks_json = json::array();
    if (stage_cfg.extraction.enabled &&
        (detected_mime == "text/html" || detected_mime == "application/xhtml+xml")) {
        HtmlProcessor html_proc;
        ContentType ct;
        ct.mime_type = detected_mime;
        ct.category  = category;
        auto extraction = html_proc.extract(blob, ct);
        if (extraction.ok) {
            meta.text_extracted      = true;
            meta.extracted_metadata  = extraction.metadata;
            if (stage_cfg.chunking.enabled) {
                std::string extraction_error;
                int extraction_attempts = 0;
                bool extraction_ok = executeWithRetry(
                    [&](std::string& err) -> bool {
                        chunks_json = json::array();
                        HtmlProcessor retry_html_proc;
                        ContentType retry_ct;
                        retry_ct.mime_type = detected_mime;
                        retry_ct.category  = category;
                        auto extraction_retry = retry_html_proc.extract(blob, retry_ct);
                        if (!extraction_retry.ok) {
                            err = extraction_retry.error_message;
                            return false;
                        }

                        meta.text_extracted     = true;
                        meta.extracted_metadata = extraction_retry.metadata;
                        int chunk_size = config.value("chunk_size", 512);
                        int overlap    = config.value("chunk_overlap", 50);
                        auto raw_chunks = retry_html_proc.chunk(extraction_retry, chunk_size, overlap);
                        int64_t now = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
                        for (const auto& rc : raw_chunks) {
                            ChunkMeta cm;
                            cm.id         = generateUuid();
                            cm.content_id = content_id;
                            cm.seq_num    = rc.value("seq_num", 0);
                            cm.chunk_type = rc.value("chunk_type", std::string("text"));
                            cm.text       = rc.value("text", std::string{});
                            cm.created_at = now;
                            chunks_json.push_back(cm.toJson());
                        }
                        meta.chunk_count = static_cast<int>(chunks_json.size());
                        meta.chunked     = !chunks_json.empty();
                        return true;
                    },
                    stage_cfg.extraction.max_retries,
                    stage_cfg.extraction.retry_delay_ms,
                    extraction_error,
                    extraction_attempts
                );

                IngestResult::StageOutcome extraction_outcome;
                extraction_outcome.stage_name    = "extraction";
                extraction_outcome.succeeded     = extraction_ok;
                extraction_outcome.attempts      = extraction_attempts;
                extraction_outcome.error_message = extraction_error;

                if (extraction_ok) {
                    THEMIS_INFO("HTML processor: extracted {} tokens, {} chunks from '{}'",
                                meta.extracted_metadata.value("token_count", 0),
                                chunks_json.size(), filename);
                } else {
                    THEMIS_WARN("HTML extraction failed for '{}' after {} attempt(s): {}",
                                filename, extraction_attempts, extraction_error);
                    if (!stage_cfg.extraction.continue_on_error) {
                        result.error_message = "Extraction failed: " + extraction_error;
                        result.stage_outcomes.push_back(std::move(extraction_outcome));
                        return result;
                    }
                    extraction_outcome.skipped = true;
                }
                result.stage_outcomes.push_back(std::move(extraction_outcome));
            }
        } else {
            THEMIS_WARN("HTML processor extraction failed for '{}': {}", filename, extraction.error_message);
        }
    }

    // Markdown: parse frontmatter and extract plain text
    if (stage_cfg.extraction.enabled && detected_mime == "text/markdown") {
        MarkdownProcessor md_proc;
        ContentType ct;
        ct.mime_type = detected_mime;
        ct.category  = category;
        auto extraction = md_proc.extract(blob, ct);
        if (extraction.ok) {
            meta.text_extracted      = true;
            meta.extracted_metadata  = extraction.metadata;
            if (stage_cfg.chunking.enabled) {
                std::string extraction_error;
                int extraction_attempts = 0;
                bool extraction_ok = executeWithRetry(
                    [&](std::string& err) -> bool {
                        chunks_json = json::array();
                        MarkdownProcessor retry_md_proc;
                        ContentType retry_ct;
                        retry_ct.mime_type = detected_mime;
                        retry_ct.category  = category;
                        auto extraction_retry = retry_md_proc.extract(blob, retry_ct);
                        if (!extraction_retry.ok) {
                            err = extraction_retry.error_message;
                            return false;
                        }

                        meta.text_extracted     = true;
                        meta.extracted_metadata = extraction_retry.metadata;
                        int chunk_size = config.value("chunk_size", 512);
                        int overlap    = config.value("chunk_overlap", 50);
                        auto raw_chunks = retry_md_proc.chunk(extraction_retry, chunk_size, overlap);
                        int64_t now = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
                        for (const auto& rc : raw_chunks) {
                            ChunkMeta cm;
                            cm.id         = generateUuid();
                            cm.content_id = content_id;
                            cm.seq_num    = rc.value("seq_num", 0);
                            cm.chunk_type = rc.value("chunk_type", std::string("text"));
                            cm.text       = rc.value("text", std::string{});
                            cm.created_at = now;
                            chunks_json.push_back(cm.toJson());
                        }
                        meta.chunk_count = static_cast<int>(chunks_json.size());
                        meta.chunked     = !chunks_json.empty();
                        return true;
                    },
                    stage_cfg.extraction.max_retries,
                    stage_cfg.extraction.retry_delay_ms,
                    extraction_error,
                    extraction_attempts
                );

                IngestResult::StageOutcome extraction_outcome;
                extraction_outcome.stage_name    = "extraction";
                extraction_outcome.succeeded     = extraction_ok;
                extraction_outcome.attempts      = extraction_attempts;
                extraction_outcome.error_message = extraction_error;

                if (extraction_ok) {
                    THEMIS_INFO("Markdown processor: extracted {} tokens, {} chunks from '{}'",
                                meta.extracted_metadata.value("token_count", 0),
                                chunks_json.size(), filename);
                } else {
                    THEMIS_WARN("Markdown extraction failed for '{}' after {} attempt(s): {}",
                                filename, extraction_attempts, extraction_error);
                    if (!stage_cfg.extraction.continue_on_error) {
                        result.error_message = "Extraction failed: " + extraction_error;
                        result.stage_outcomes.push_back(std::move(extraction_outcome));
                        return result;
                    }
                    extraction_outcome.skipped = true;
                }
                result.stage_outcomes.push_back(std::move(extraction_outcome));
            }
        } else {
            THEMIS_WARN("Markdown processor extraction failed for '{}': {}", filename, extraction.error_message);
        }
    }

    // ---- OCR extraction (opt-in via ContentPolicy::ocrEnabled() → MimeDetector::shouldTriggerOcr()) ----
    // Triggered when the caller sets config["ocr_enabled"]=true AND the MIME type is
    // one of the OCR-capable image formats: image/png, image/jpeg, image/tiff.
    // ContentPolicy::ocrEnabled() is wired to MimeDetector via shouldTriggerOcr(mime, bool) so that
    // all routing decisions go through MimeDetector — thread-safe, no shared state mutation.
    const bool ocr_enabled = config.value("ocr_enabled", false);
    if (stage_cfg.extraction.enabled &&
        mime_detector_.shouldTriggerOcr(detected_mime, ocr_enabled)) {

        const std::string ocr_language = config.value("ocr_language", std::string("eng"));
        std::vector<uint8_t> image_bytes(blob.begin(), blob.end());
        std::string ocr_text = OcrProcessor::performOcr(image_bytes, ocr_language);

        if (!ocr_text.empty()) {
            meta.text_extracted = true;
            meta.extracted_metadata["content_ocr_text"] = ocr_text;

            if (stage_cfg.chunking.enabled) {
                OcrProcessor::Config ocr_cfg;
                ocr_cfg.language = ocr_language;
                OcrProcessor ocr_proc(std::move(ocr_cfg));

                // Build a synthetic ExtractionResult from the OCR text
                ExtractionResult ocr_extraction;
                ocr_extraction.ok   = true;
                ocr_extraction.text = ocr_text;
                ocr_extraction.metadata["content_ocr_text"] = ocr_text;

                int chunk_size = config.value("chunk_size", 512);
                int overlap    = config.value("chunk_overlap", 50);
                auto raw_chunks = ocr_proc.chunk(ocr_extraction, chunk_size, overlap);

                int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                for (const auto& rc : raw_chunks) {
                    ChunkMeta cm;
                    cm.id         = generateUuid();
                    cm.content_id = content_id;
                    cm.seq_num    = rc.value("sequence", 0);
                    cm.chunk_type = rc.value("type", std::string("ocr_text"));
                    cm.text       = rc.value("text", std::string{});
                    cm.created_at = now;
                    chunks_json.push_back(cm.toJson());
                }
                meta.chunk_count = static_cast<int>(chunks_json.size());
                meta.chunked     = !chunks_json.empty();
            }

            THEMIS_INFO("OCR extraction: {} chars, {} chunks from '{}' ({})",
                        ocr_text.size(), chunks_json.size(), filename, detected_mime);
        } else {
            THEMIS_INFO("OCR extraction returned no text for '{}' ({})",
                        filename, detected_mime);
        }
    }

    json spec = {
        {"content", meta.toJson()}
    };
    if (!chunks_json.empty()) {
        spec["chunks"] = chunks_json;
    }
    // Pass the embedding stage flag to importContent via the spec.
    // The "__embedding_enabled" key is an internal hint consumed by importContent;
    // it is never persisted to storage.
    //
    // ContentPolicy::embedding_model gates the stage per collection:
    //   - If config["embedding_model"] is present and non-empty → stage is activated
    //     (provided the global ProcessorChainConfig also has it enabled).
    //   - If config["embedding_model"] is present but empty     → stage is disabled
    //     for this ingestion regardless of the global setting.
    //   - If config["embedding_model"] is absent                → fall back to the
    //     ProcessorChainConfig default (backward-compatible).
    {
        bool embedding_active = stage_cfg.embedding.enabled;
        if (config.contains("embedding_model")) {
            const std::string policy_model = config.value("embedding_model", std::string{});
            embedding_active = stage_cfg.embedding.enabled && !policy_model.empty();
        }
        spec["__embedding_enabled"] = embedding_active;
    }

    // Storage stage: importContent with per-stage retry.
    {
        std::string storage_error;
        int storage_attempts = 0;
        bool stored = executeWithRetry(
            [&](std::string& err) -> bool {
                auto status = importContent(spec, blob, user_context);
                if (!status.ok) { err = status.message; return false; }
                return true;
            },
            stage_cfg.storage.max_retries,
            stage_cfg.storage.retry_delay_ms,
            storage_error,
            storage_attempts
        );

        IngestResult::StageOutcome storage_outcome;
        storage_outcome.stage_name    = "storage";
        storage_outcome.succeeded     = stored;
        storage_outcome.attempts      = storage_attempts;
        storage_outcome.error_message = storage_error;
        result.stage_outcomes.push_back(std::move(storage_outcome));

        if (!stored) {
            if (storage_attempts > 1) {
                THEMIS_WARN("Storage stage failed for '{}' after {} attempt(s): {}",
                            filename, storage_attempts, storage_error);
            }
            result.error_message = storage_error;
            return result;
        }
    }

    // Register with the deduplication index after successful storage.
    // Reuse cached_phash / cached_minhash computed above (no redundant DCT/hash).
    if (dedup_policy_enabled && dedup_checker_ && (dedup_is_image || dedup_is_text)) {
        if (dedup_is_image && !cached_phash.empty()) {
            dedup_checker_->registerImage(content_id, cached_phash);
            meta.extracted_metadata["phash_hex"] = cached_phash;
        } else if (dedup_is_text && !cached_minhash.empty()) {
            dedup_checker_->registerText(content_id, cached_minhash);
        }
    }

    result.success = true;
    result.primary_content_id = content_id;
    result.metadata = json{
        {"content_id", content_id},
        {"mime_type", detected_mime},
        {"category", static_cast<int>(category)}
    };
    if (!chunks_json.empty()) {
        result.metadata["chunk_count"] = static_cast<int>(chunks_json.size());
    }
    
    return result;
}

ContentManager::IngestResult ContentManager::ingestStream(
    std::istream& stream,
    const std::string& filename,
    const std::string& mime_type,
    const std::string& user_context,
    const json& config
) {
    IngestResult result;
    result.success = false;

    if (!stream.good()) {
        result.error_message = "Invalid or unreadable input stream";
        return result;
    }

    static constexpr size_t kDefaultChunkSizeBytes  = 4 * 1024 * 1024;    // 4 MB
    static constexpr size_t kDefaultMaxBufferedBytes = 256 * 1024 * 1024;  // 256 MB
    static constexpr size_t kHeaderSize              = 8192;               // 8 KB for detection

    const size_t chunk_size_bytes  = config.value("chunk_size_bytes",  kDefaultChunkSizeBytes);
    const size_t max_buffered_bytes = config.value("max_buffered_bytes", kDefaultMaxBufferedBytes);
    const int    text_chunk_chars  = config.value("chunk_size", 512);

    // --- Read header for MIME type detection ---
    std::string header_buf;
    header_buf.resize(std::min(kHeaderSize, chunk_size_bytes));
    stream.read(header_buf.data(), static_cast<std::streamsize>(header_buf.size()));
    size_t header_read = static_cast<size_t>(stream.gcount());
    header_buf.resize(header_read);

    if (header_read == 0) {
        result.error_message = "Empty stream";
        return result;
    }

    // --- Detect MIME type ---
    std::string detected_mime = mime_type;
    if (detected_mime.empty()) {
        auto& registry = ContentTypeRegistry::instance();
        auto type = registry.detectFromBlob(header_buf);
        if (type) {
            detected_mime = type->mime_type;
        } else {
            auto ext_pos = filename.find_last_of('.');
            if (ext_pos != std::string::npos) {
                std::string ext = filename.substr(ext_pos);
                type = registry.getByExtension(ext);
                if (type) detected_mime = type->mime_type;
            }
        }
    }
    if (detected_mime.empty()) {
        detected_mime = "application/octet-stream";
    }

    // --- Validate MIME type and header format (security gate) ---
    // Uses content_validator.cpp to enforce MIME type validity and magic-bytes
    // consistency before any data is stored. Streaming-capable text types skip
    // the magic-bytes check (they have no binary header signature).
    {
        ContentValidator hdr_validator;
        auto mime_err = hdr_validator.validateMimeType(detected_mime);
        if (mime_err.failed()) {
            result.error_message = "MIME type validation failed: " + mime_err.message;
            return result;
        }
        // Format / magic-bytes check on the header for non-text MIME types.
        // Text/*, NDJSON and jsonlines have no binary magic bytes – skip.
        if (!isTextBasedMime(detected_mime)) {
            auto fmt_err = hdr_validator.validateFormat(header_buf, detected_mime);
            if (fmt_err.failed()) {
                THEMIS_WARN("ingestStream: format validation failed for '{}' ({}): {}",
                            filename, detected_mime, fmt_err.message);
                result.error_message = "Content format validation failed: " + fmt_err.message;
                return result;
            }
        }
    }

    // --- Small file: stream already exhausted after header read ---
    if (stream.eof() || !stream.good()) {
        return ingestRawBlob(header_buf, filename, detected_mime, user_context, config);
    }

    // --- Determine if MIME type supports line-oriented streaming ---
    auto isStreamingCapable = [](const std::string& m) -> bool {
        return m == "text/plain"        ||
               m == "text/csv"          ||
               m == "text/markdown"     ||
               m == "text/x-markdown"   ||
               m.find("ndjson")     != std::string::npos ||
               m.find("jsonlines")  != std::string::npos;
    };

    // --- Non-streaming path: buffer up to max_buffered_bytes ---
    if (!isStreamingCapable(detected_mime)) {
        std::string buffer = header_buf;
        std::vector<char> read_buf(chunk_size_bytes);
        while (stream.good()) {
            stream.read(read_buf.data(), static_cast<std::streamsize>(chunk_size_bytes));
            size_t n = static_cast<size_t>(stream.gcount());
            if (n == 0) break;
            if (buffer.size() + n > max_buffered_bytes) {
                result.error_message =
                    "File exceeds max_buffered_bytes (" + std::to_string(max_buffered_bytes) +
                    ") for non-streaming content type '" + detected_mime + "'";
                return result;
            }
            buffer.append(read_buf.data(), n);
        }
        return ingestRawBlob(buffer, filename, detected_mime, user_context, config);
    }

    // =========================================================================
    // Streaming text ingestion path
    // Chunks are stored directly to RocksDB as they are produced, keeping peak
    // memory well below the full file size (at most two read-buffer windows).
    // =========================================================================

    const int64_t now = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    const std::string content_id = generateUuid();
    int64_t total_bytes = static_cast<int64_t>(header_read);
    int seq_num = 0;
    std::vector<std::string> chunk_ids;

    // Determine the effective processing stage configuration for this content type.
    const ContentCategory streaming_category = [&]() {
        auto& reg = ContentTypeRegistry::instance();
        auto t = reg.getByMimeType(detected_mime);
        return t ? t->category : ContentCategory::UNKNOWN;
    }();
    const ContentTypePipelineConfig stream_stage_cfg =
        processor_chain_config_.getEffectiveConfig(detected_mime, streaming_category);
    // ContentPolicy::embedding_model gates embedding generation per collection.
    // If config["embedding_model"] is present:
    //   - non-empty → embedding active (subject to stream_stage_cfg.embedding.enabled)
    //   - empty     → embedding disabled for this ingestion regardless of global config
    // If absent → fall back to the ProcessorChainConfig default (backward-compatible).
    const bool stream_embedding_active = [&]() -> bool {
        if (config.contains("embedding_model")) {
            const std::string policy_model = config.value("embedding_model", std::string{});
            return stream_stage_cfg.embedding.enabled && !policy_model.empty();
        }
        return stream_stage_cfg.embedding.enabled;
    }();
    // Incremental SHA-256 hash over all streamed bytes.
    // RAII wrapper — EVP_MD_CTX_free() is called automatically when the unique_ptr
    // is reset or goes out of scope, guarding against exception-induced leaks.
    using EvpCtxPtr = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
    EvpCtxPtr sha256_ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (sha256_ctx) {
        if (EVP_DigestInit_ex(sha256_ctx.get(), EVP_sha256(), nullptr) != 1) {
            THEMIS_WARN("ingestStream: EVP_DigestInit_ex failed; SHA-256 dedup disabled for '{}'", filename);
            sha256_ctx.reset();
        } else {
            EVP_DigestUpdate(sha256_ctx.get(), header_buf.data(), header_buf.size());
        }
    } else {
        THEMIS_WARN("ingestStream: EVP_MD_CTX_new failed; SHA-256 dedup disabled for '{}'", filename);
    }
    // updateHash is unused for SHA-256 (raw bytes are hashed directly in the
    // stream read loop below); the lambda exists as a no-op to avoid changing
    // the storeTextChunk call sites.
    auto updateHash = [](const std::string&) {};

    // --- Load indexing config (mirrors importContent) ---
    bool auto_fulltext_index = false;
    SecondaryIndexManager::FulltextConfig fulltext_config;
    try {
        if (auto cfgv = storage_->get("config:content")) {
            std::string s(cfgv->begin(), cfgv->end());
            json cj = json::parse(s);
            auto_fulltext_index = cj.value("auto_fulltext_index", false);
            if (cj.contains("fulltext_config") && cj["fulltext_config"].is_object()) {
                auto ftcfg = cj["fulltext_config"];
                fulltext_config.stemming_enabled   = ftcfg.value("stemming_enabled", false);
                fulltext_config.language           = ftcfg.value("language", "none");
                fulltext_config.stopwords_enabled  = ftcfg.value("stopwords_enabled", false);
                fulltext_config.normalize_umlauts  = ftcfg.value("normalize_umlauts", false);
                if (ftcfg.contains("stopwords") && ftcfg["stopwords"].is_array()) {
                    for (const auto& sw : ftcfg["stopwords"])
                        if (sw.is_string()) fulltext_config.stopwords.push_back(sw.get<std::string>());
                }
            }
        }
    } catch (const nlohmann::json::exception&) {
    } catch (...) {
    }

    if (auto_fulltext_index && secondary_index_) {
        if (!secondary_index_->hasFulltextIndex("chunk", "text"))
            secondary_index_->createFulltextIndex("chunk", "text", fulltext_config);
    }

    // --- Helper: store one text segment as a chunk ---
    auto storeTextChunk = [&](const std::string& text) {
        if (text.empty()) return;
        updateHash(text);
        if (!stream_stage_cfg.chunking.enabled) return;
        ChunkMeta cm;
        cm.id         = generateUuid();
        cm.content_id = content_id;
        cm.seq_num    = seq_num++;
        cm.chunk_type = "text";
        cm.text       = text;
        cm.created_at = now;

        if (stream_embedding_active && embedding_pipeline_ && embedding_pipeline_->isEnabled()) {
            auto emb = embedding_pipeline_->generateEmbedding(text);
            if (!emb.empty()) cm.embedding = std::move(emb);
        }

        std::string ckey = std::string("chunk:") + cm.id;
        std::string cjson = cm.toJson().dump();
        storage_->put(ckey, std::vector<uint8_t>(cjson.begin(), cjson.end()));

        if (auto_fulltext_index && secondary_index_) {
            BaseEntity chunk_entity = BaseEntity::fromFields(
                cm.id,
                BaseEntity::FieldMap{
                    {"content_id", cm.content_id},
                    {"text",       cm.text},
                    {"seq_num",    static_cast<int64_t>(cm.seq_num)},
                    {"chunk_type", cm.chunk_type}
                }
            );
            secondary_index_->put("chunk", chunk_entity);
        }

        if (!cm.embedding.empty() && vector_index_) {
            if (vector_index_->getDimension() == 0)
                vector_index_->init("chunks", static_cast<int>(cm.embedding.size()), VectorIndexManager::Metric::COSINE);
            if (vector_index_->getDimension() == static_cast<int>(cm.embedding.size())) {
                BaseEntity e = BaseEntity::fromFields(
                    std::string("chunks:") + cm.id,
                    BaseEntity::FieldMap{
                        {"content_id", cm.content_id},
                        {"seq_num",    static_cast<int64_t>(cm.seq_num)},
                        {"mime_type",  detected_mime},
                        {"chunk_type", cm.chunk_type},
                        {"embedding",  cm.embedding}
                    }
                );
                vector_index_->addEntity(e);
            }
        }

        chunk_ids.push_back(cm.id);
    };

    // --- Helper: split carry buffer into text segments ---
    std::string carry;  // incomplete segment carried over between read iterations
    auto flushCarry = [&](bool force) {
        size_t pos = 0;
        while (pos < carry.size()) {
            size_t remaining = carry.size() - pos;
            if (!force && remaining < static_cast<size_t>(text_chunk_chars))
                break;  // wait for more data
            size_t end = pos + std::min(static_cast<size_t>(text_chunk_chars), remaining);
            // Prefer to split on newline boundary when close
            if (end < carry.size()) {
                size_t nl = carry.find('\n', end > 0 ? end - 1 : 0);
                if (nl != std::string::npos && nl < pos + static_cast<size_t>(text_chunk_chars) * 2)
                    end = nl + 1;
            }
            storeTextChunk(carry.substr(pos, end - pos));
            pos = end;
        }
        carry = carry.substr(pos);
    };

    // Process header chunk
    carry = header_buf;
    flushCarry(false);

    // Read and process remaining stream
    std::vector<char> read_buf(chunk_size_bytes);
    while (stream.good()) {
        stream.read(read_buf.data(), static_cast<std::streamsize>(chunk_size_bytes));
        size_t n = static_cast<size_t>(stream.gcount());
        if (n == 0) break;
        total_bytes += static_cast<int64_t>(n);
        if (sha256_ctx) {
            if (EVP_DigestUpdate(sha256_ctx.get(), read_buf.data(), n) != 1) {
                THEMIS_WARN("ingestStream: EVP_DigestUpdate failed; disabling SHA-256 for '{}'", filename);
                sha256_ctx.reset();
            }
        }
        carry.append(read_buf.data(), n);
        flushCarry(false);
    }
    // Flush remaining carry
    flushCarry(true);

    // --- Finalize: write content metadata ---
    ContentMeta meta;
    meta.id               = content_id;
    meta.mime_type        = detected_mime;
    meta.category         = ContentCategory::TEXT;
    meta.original_filename = filename;
    meta.size_bytes       = total_bytes;
    meta.created_at       = now;
    meta.modified_at      = now;
    meta.hash_sha256      = [&]() -> std::string {
        if (!sha256_ctx) return {};
        unsigned char digest[SHA256_DIGEST_LENGTH];
        unsigned int  digest_len = 0;
        int ok = EVP_DigestFinal_ex(sha256_ctx.get(), digest, &digest_len);
        sha256_ctx.reset();
        if (!ok) return {};
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (unsigned int i = 0; i < digest_len; ++i)
            oss << std::setw(2) << static_cast<unsigned int>(digest[i]);
        return oss.str();
    }();
    meta.text_extracted   = true;
    meta.chunk_count      = static_cast<int>(chunk_ids.size());
    meta.chunked          = meta.chunk_count > 0;

    std::string mkey = std::string("content:") + meta.id;
    std::string mjson = meta.toJson().dump();
    if (!storage_->put(mkey, std::vector<uint8_t>(mjson.begin(), mjson.end()))) {
        // Rollback: remove chunks already written
        for (const auto& cid : chunk_ids)
            storage_->del(std::string("chunk:") + cid);
        result.error_message = "Failed to store content metadata";
        return result;
    }

    // Write chunk list
    std::string lkey = std::string("content_chunks:") + meta.id;
    json lj = json{{"ids", chunk_ids}};
    std::string lstr = lj.dump();
    storage_->put(lkey, std::vector<uint8_t>(lstr.begin(), lstr.end()));

    // Persist SHA-256 hash → content_id mapping for exact-duplicate detection.
    if (!meta.hash_sha256.empty()) {
        std::string hkey = std::string("content_hash:") + meta.hash_sha256;
        json hj = json{{"ids", json::array({meta.id})}};
        std::string hstr = hj.dump();
        if (!storage_->get(hkey)) {
            storage_->put(hkey, std::vector<uint8_t>(hstr.begin(), hstr.end()));
        }
    }

    THEMIS_INFO("Streaming ingestion completed: {} bytes, {} text chunks, file '{}'",
                total_bytes, chunk_ids.size(), filename);

    result.success            = true;
    result.primary_content_id = content_id;
    result.metadata = json{
        {"content_id",  content_id},
        {"mime_type",   detected_mime},
        {"category",    static_cast<int>(ContentCategory::TEXT)},
        {"chunk_count", static_cast<int>(chunk_ids.size())},
        {"total_bytes", total_bytes},
        {"streaming",   true}
    };
    return result;
}

ContentManager::Stats ContentManager::getStats() {
    Stats s{};
    s.total_content_items = 0;
    s.total_chunks = 0;
    s.total_embeddings = 0;
    s.total_storage_bytes = static_cast<int64_t>(storage_->getApproximateSize());

    // naive count via scan
    storage_->scanPrefix("content:", [&](std::string_view, std::string_view){ s.total_content_items++; return true; });
    storage_->scanPrefix("chunk:", [&](std::string_view, std::string_view){ s.total_chunks++; return true; });
    // embeddings equal vector_index count if initialized
    if (vector_index_) s.total_embeddings = static_cast<int>(vector_index_->getVectorCount());
    return s;
}

} // namespace content
} // namespace themis
