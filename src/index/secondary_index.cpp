/**
 * @file secondary_index.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=29, H=17, M=96, L=1
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/normalizer.h"
#include <stdexcept>
// Secondary index implementation

#include "index/secondary_index.h"
#include "index/index_compression.h"
#include "index/secondary_index_metadata_cache.h"
#include "index/spatial_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/key_schema.h"
#include "storage/base_entity.h"
#include "utils/logger.h"
#include "utils/stemmer.h"
#include "utils/stopwords.h"
#include <nlohmann/json.hpp>

#include <algorithm>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <cmath>
#include <cstdio>
#include <cctype>
#include <chrono>
#include <thread>
#include "utils/geometric_distances.h"

namespace themis {

namespace {
// Convert bytes to vector<uint8_t>
inline std::vector<uint8_t> toBytes(std::string_view sv) {
	return std::vector<uint8_t>(sv.begin(), sv.end());
}
} // namespace
// static
std::string SecondaryIndexManager::makeFulltextTFKey(std::string_view table, std::string_view column, std::string_view token, std::string_view pk) {
	std::string key = {};
	key.reserve(5 + table.size() + 1 + column.size() + 1 + token.size() + 1 + pk.size());
	key += "fttf:";
	key.append(table.data(), table.size());
	key += ":";
	key.append(column.data(), column.size());
	key += ":";
	key.append(token.data(), token.size());
	key += ":";
	key.append(pk.data(), pk.size());
	return key;
}

// static
std::string SecondaryIndexManager::makeFulltextDocLenKey(std::string_view table, std::string_view column, std::string_view pk) {
	std::string key = {};
	key.reserve(7 + table.size() + 1 + column.size() + 1 + pk.size());
	key += "ftdlen:";
	key.append(table.data(), table.size());
	key += ":";
	key.append(column.data(), column.size());
	key += ":";
	key.append(pk.data(), pk.size());
	return key;
}

// static
std::string SecondaryIndexManager::makeFulltextDocLenPrefix(std::string_view table, std::string_view column) {
	std::string key = {};
	key.reserve(7 + table.size() + 1 + column.size() + 1);
	key += "ftdlen:";
	key.append(table.data(), table.size());
	key += ":";
	key.append(column.data(), column.size());
	key += ":";
	return key;
}

SecondaryIndexManager::SecondaryIndexManager(RocksDBWrapper& db) : db_(db) {
	// Default codec — all compression features disabled (opt-in via Config constructor)
	compression_codec_ = std::make_unique<index::IndexCompressionCodec>();
	transactional_put_batch_size_ = 64;
}

SecondaryIndexManager::SecondaryIndexManager(RocksDBWrapper& db, const Config& config)
	: db_(db), compression_config_(config)
{
	index::IndexCompressionCodec::Config codec_cfg;
	if (config.enable_compression) {
		codec_cfg.enable_prefix_compression = config.enable_prefix_compression;
		codec_cfg.enable_delta_encoding     = config.enable_delta_encoding;
		codec_cfg.enable_rle                = config.enable_rle;
		codec_cfg.enable_dict_encoding      = config.enable_dict_encoding;
		codec_cfg.enable_bloom_filter       = config.enable_bloom_filter;
		codec_cfg.algorithm                 = config.compression_algorithm;
		codec_cfg.compression_level         = config.compression_level;
	}
	compression_codec_ = std::make_unique<index::IndexCompressionCodec>(codec_cfg);
	transactional_put_batch_size_ = std::max<size_t>(size_t{1}, config.transactional_put_batch_size);
}

// Phase 4: Set expression evaluator for advanced filtering
void SecondaryIndexManager::setExpressionEvaluator(std::shared_ptr<IExpressionEvaluator> evaluator) {
	expression_evaluator_ = std::move(evaluator);
}

std::shared_ptr<IExpressionEvaluator> SecondaryIndexManager::getExpressionEvaluator() const {
	return expression_evaluator_;
}

// Phase 2: Set spatial index manager for atomic geo index updates
void SecondaryIndexManager::setSpatialIndexManager(index::SpatialIndexManager* spatial_mgr) {
	spatial_index_mgr_ = spatial_mgr;
}

index::SpatialIndexManager* SecondaryIndexManager::getSpatialIndexManager() const {
	return spatial_index_mgr_;
}

// static
std::string SecondaryIndexManager::makeIndexMetaKey(std::string_view table, std::string_view column) {
	std::string key = {};
	key.reserve(8 + table.size() + 1 + column.size());
	key += "idxmeta:";
	key.append(table.data(), table.size());
	key += ":";
	key.append(column.data(), column.size());
	return key;
}

// static
std::string SecondaryIndexManager::makeCompositeIndexMetaKey(std::string_view table, const std::vector<std::string>& columns) {
	size_t total = 8 + table.size() + 1;
	for (size_t i = 0; i < columns.size(); ++i) {
		total += columns[i].size();
		if (i > 0) {
			total += 1;
		}
	}
	std::string key = {};
	key.reserve(total);
	key += "idxmeta:";
	key.append(table.data(), table.size());
	key += ":";
	for (size_t i = 0; i < columns.size(); ++i) {
		if (i > 0) {
		  key += "+";
		}
		key += columns[i];
	}
	return key;
}

// static
std::string SecondaryIndexManager::makeIndexKey(std::string_view table, std::string_view column, std::string_view value, std::string_view pk) {
	return KeySchema::makeSecondaryIndexKey(table, column, value, pk);
}

// static
std::string SecondaryIndexManager::makeCompositeIndexKey(std::string_view table, const std::vector<std::string>& columns, const std::vector<std::string>& values, std::string_view pk) {
	// Format: idx:table:col1+col2:val1:val2:PK
	std::vector<std::string> encoded_values = {};

	encoded_values.reserve(values.size());
	size_t total = 4 + table.size() + 1 + pk.size();
	for (size_t i = 0; i < columns.size(); ++i) {
		total += columns[i].size();
		if (i > 0) {
			total += 1;
		}
	}
	for (const auto& value : values) {
		encoded_values.emplace_back(encodeKeyComponent(value));
		total += encoded_values.back().size() + 1;
	}
	std::string key = {};
	key.reserve(total);
	key += "idx:";
	key.append(table.data(), table.size());
	key += ":";
	for (size_t i = 0; i < columns.size(); ++i) {
		if (i > 0) {
		  key += "+";
		}
		key += columns[i];
	}
	key += ":";
	for (const auto& encoded : encoded_values) {
		key += encoded;
		key += ":";
	}
	key.append(pk.data(), pk.size());
	return key;
}

// static
std::string SecondaryIndexManager::makeCompositeIndexPrefix(std::string_view table, const std::vector<std::string>& columns, const std::vector<std::string>& values) {
	// Gleich wie makeCompositeIndexKey aber ohne PK am Ende
	std::vector<std::string> encoded_values = {};

	encoded_values.reserve(values.size());
	size_t total = 4 + table.size() + 1;
	for (size_t i = 0; i < columns.size(); ++i) {
		total += columns[i].size();
		if (i > 0) {
			total += 1;
		}
	}
	for (const auto& value : values) {
		encoded_values.emplace_back(encodeKeyComponent(value));
		total += encoded_values.back().size() + 1;
	}
	std::string key = {};
	key.reserve(total);
	key += "idx:";
	key.append(table.data(), table.size());
	key += ":";
	for (size_t i = 0; i < columns.size(); ++i) {
		if (i > 0) {
		  key += "+";
		}
		key += columns[i];
	}
	key += ":";
	for (const auto& encoded : encoded_values) {
		key += encoded;
		key += ":";
	}
	return key;
}

// static — unique-constraint sentinel keys for GetForUpdate locking
// Format: "uidx:table:col:encodedVal" (single-column)
std::string SecondaryIndexManager::makeUniqueSentinelKey_(
		std::string_view table, std::string_view col, std::string_view encodedVal) {
	std::string key = {};
	key.reserve(5 + table.size() + col.size() + encodedVal.size());
	key += "uidx:";
	key += table;
	key += ":";
	key += col;
	key += ":";
	key += encodedVal;
	return key;
}

// static — composite unique-constraint sentinel key for GetForUpdate locking
// Format: "uidx:table:col1+col2:encVal1:encVal2"
std::string SecondaryIndexManager::makeCompositeUniqueSentinelKey_(
		std::string_view table,
		const std::vector<std::string>& columns,
		const std::vector<std::string>& values) {
	// Encode values once so we use them for both the reserve hint and the build.
	std::vector<std::string> encodedVals = {};

	encodedVals.reserve(values.size());
	size_t total = 5 + table.size() + 1; // "uidx:" + table + ":"
	for (size_t i = 0; i < columns.size(); ++i) {
		if (i > 0) total += 1; // "+"
		total += columns[i].size();
	}
	for (const auto& v : values) {
		encodedVals.emplace_back(encodeKeyComponent(v));
		total += 1 + encodedVals.back().size(); // ":" + encoded
	}

	std::string key = {};
	key.reserve(total);
	key += "uidx:";
	key += table;
	key += ":";
	for (size_t i = 0; i < columns.size(); ++i) {
		if (i > 0) {
		  key += "+";
		}
		key += columns[i];
	}
	for (const auto& ev : encodedVals) {
		key += ":";
		key += ev;
	}
	return key;
}

// static
std::string SecondaryIndexManager::encodeKeyComponent(std::string_view raw) {
	std::string out = {};
	out.reserve(raw.size());
	// Heuristic: if the component is a positive integer (digits only),
	// encode it as a fixed-width zero-padded field so lexicographic
	// ordering matches numeric ordering for ORDER BY on range indices.
	bool all_digits = !raw.empty();
	for (char ch : raw) { if (!std::isdigit(static_cast<unsigned char>(ch))) { all_digits = false; break; } }
	if (all_digits && raw.size() <= 20) {
		// Pad to 20 characters (large enough for typical integers)
		const size_t width = 20;
		if (raw.size() < width) {
		  out.append(width - raw.size(), '0');
		}
		out.append(raw.data(), raw.size());
		return out;
	}

	for (unsigned char c : raw) {
		if (c == ':' || c == '%') {
			constexpr char kHex[] = "0123456789ABCDEF";
			out.push_back('%');
			out.push_back(kHex[c >> 4]);
			out.push_back(kHex[c & 0x0F]);
		} else {
			out.push_back(static_cast<char>(c));
		}
	}
	return out;
}

// ---------------------------------------------------------------------------
// Backward-compatibility API: createIndex with IndexType enum
// ---------------------------------------------------------------------------
SecondaryIndexManager::Status SecondaryIndexManager::createIndex(std::string_view table, std::string_view column, IndexType type) {
	switch (type) {
		case IndexType::REGULAR:
			return createIndex(table, column, false);
		case IndexType::RANGE:
			return createRangeIndex(table, column);
		case IndexType::SPARSE:
			return createSparseIndex(table, column, false);
		case IndexType::GEO:
			return createGeoIndex(table, column);
		case IndexType::TTL:
			// TTL requires ttl_seconds, cannot infer via this overload
			return Status::Error("createIndex(table,column,TTL) requires TTL seconds; use createTTLIndex(table,column,ttl_seconds)");
		case IndexType::FULLTEXT:
			return createFulltextIndex(table, column);
		case IndexType::PARTIAL:
			// PARTIAL requires a predicate; use createPartialIndex(table, column, predicate) directly
			return Status::Error("createIndex(table,column,PARTIAL) requires a predicate; use createPartialIndex(table,column,predicate)");
		default:
			return Status::Error("Unknown IndexType");
	}
}

// static
std::string SecondaryIndexManager::makeRangeIndexMetaKey(std::string_view table, std::string_view column) {
	return std::string("ridxmeta:") + std::string(table) + ":" + std::string(column);
}

// static
std::string SecondaryIndexManager::makeSparseIndexMetaKey(std::string_view table, std::string_view column) {
	return std::string("sidxmeta:") + std::string(table) + ":" + std::string(column);
}

// static
std::string SecondaryIndexManager::makeGeoIndexMetaKey(std::string_view table, std::string_view column) {
	return std::string("gidxmeta:") + std::string(table) + ":" + std::string(column);
}

// static
std::string SecondaryIndexManager::makeTTLIndexMetaKey(std::string_view table, std::string_view column) {
	return std::string("ttlidxmeta:") + std::string(table) + ":" + std::string(column);
}

// static
std::string SecondaryIndexManager::makeFulltextIndexMetaKey(std::string_view table, std::string_view column) {
	return std::string("ftidxmeta:") + std::string(table) + ":" + std::string(column);
}

// static
std::string SecondaryIndexManager::makeRangeIndexKey(std::string_view table, std::string_view column, std::string_view value, std::string_view pk) {
	std::string key = "ridx:" + std::string(table) + ":" + std::string(column) + ":";
	key += encodeKeyComponent(value);
	key += ":";
	key += std::string(pk);
	return key;
}

// static
std::string SecondaryIndexManager::makeRangeIndexPrefix(std::string_view table, std::string_view column, std::string_view valuePrefix) {
	std::string key = "ridx:" + std::string(table) + ":" + std::string(column) + ":";
	if (!valuePrefix.empty()) {
		key += encodeKeyComponent(valuePrefix);
		key += ":";
	}
	return key;
}

// static
std::string SecondaryIndexManager::makeSparseIndexKey(std::string_view table, std::string_view column, std::string_view value, std::string_view pk) {
	std::string key = "sidx:" + std::string(table) + ":" + std::string(column) + ":";
	key += encodeKeyComponent(value);
	key += ":";
	key += std::string(pk);
	return key;
}

// static
std::string SecondaryIndexManager::makeGeoIndexKey(std::string_view table, std::string_view column, std::string_view geohash, std::string_view pk) {
	std::string key = "gidx:" + std::string(table) + ":" + std::string(column) + ":";
	key += std::string(geohash);
	key += ":";
	key += std::string(pk);
	return key;
}

// static
std::string SecondaryIndexManager::makeGeoIndexPrefix(std::string_view table, std::string_view column, std::string_view geohashPrefix) {
	std::string key = "gidx:" + std::string(table) + ":" + std::string(column) + ":";
	if (!geohashPrefix.empty()) {
		key += std::string(geohashPrefix);
	}
	return key;
}

// static
std::string SecondaryIndexManager::makeTTLIndexKey(std::string_view table, std::string_view column, int64_t expireTimestamp, std::string_view pk) {
	// Format: ttlidx:table:column:timestamp:PK
	// timestamp wird mit führenden Nullen auf 20 Zeichen padded für lexikografische Sortierung
	char buf[32];
	snprintf(buf, sizeof(buf), "%020lld", (long long)expireTimestamp);
	std::string key = "ttlidx:" + std::string(table) + ":" + std::string(column) + ":" + buf + ":" + std::string(pk);
	return key;
}

// static
std::string SecondaryIndexManager::makeTTLIndexPrefix(std::string_view table, std::string_view column) {
	return "ttlidx:" + std::string(table) + ":" + std::string(column) + ":";
}

// static
std::string SecondaryIndexManager::makeFulltextIndexKey(std::string_view table, std::string_view column, std::string_view token, std::string_view pk) {
	std::string key = "ftidx:" + std::string(table) + ":" + std::string(column) + ":";
	key += encodeKeyComponent(token);
	key += ":";
	key += std::string(pk);
	return key;
}

// static
std::string SecondaryIndexManager::makeFulltextIndexPrefix(std::string_view table, std::string_view column, std::string_view token) {
	std::string key = "ftidx:" + std::string(table) + ":" + std::string(column) + ":";
	if (!token.empty()) {
		key += encodeKeyComponent(token) + ":";
	}
	return key;
}

// ────────────────────────────────────────────────────────────────────────────
// Partial (Filtered) Index: Key-Builder
// ────────────────────────────────────────────────────────────────────────────

// static
std::string SecondaryIndexManager::makePartialIndexMetaKey(std::string_view table, std::string_view column) {
	return "pidxmeta:" + std::string(table) + ":" + std::string(column);
}

// static
std::string SecondaryIndexManager::makePartialIndexKey(std::string_view table, std::string_view column, std::string_view value, std::string_view pk) {
	std::string key = "pidx:" + std::string(table) + ":" + std::string(column) + ":";
	key += encodeKeyComponent(value);
	key += ":";
	key += std::string(pk);
	return key;
}

// static
std::string SecondaryIndexManager::makePartialIndexPrefix(std::string_view table, std::string_view column, std::string_view valuePrefix) {
	std::string key = "pidx:" + std::string(table) + ":" + std::string(column) + ":";
	if (!valuePrefix.empty()) {
		key += encodeKeyComponent(valuePrefix);
		key += ":";
	}
	return key;
}

SecondaryIndexManager::Status SecondaryIndexManager::createIndex(std::string_view table, std::string_view column, bool unique) {
	if (table.empty() || column.empty()) {
		return Status::Error("createIndex: table/column darf nicht leer sein");
	}
	if (std::string(table).find(':') != std::string::npos || std::string(column).find(':') != std::string::npos) {
		return Status::Error("createIndex: ':' ist in table/column nicht erlaubt");
	}

	std::string metaKey = makeIndexMetaKey(table, column);
	std::string metaValue = unique ? "unique" : "regular";
	std::vector<uint8_t> marker(metaValue.begin(), metaValue.end());
	if (!db_.put(metaKey, marker)) {
		return Status::Error("createIndex: Schreiben des Metaschlüssels fehlgeschlagen: " + metaKey);
	}
	
	// v1.3.4: Invalidate cache when index structure changes
	SecondaryIndexMetadataCache::instance().invalidate(table);
	
	THEMIS_INFO("Index erstellt: {}.{} (unique={})", table, column, unique);
	return Status::OK();
}

SecondaryIndexManager::Status SecondaryIndexManager::createCompositeIndex(std::string_view table, const std::vector<std::string>& columns, bool unique) {
	if (table.empty() || columns.empty()) {
		return Status::Error("createCompositeIndex: table/columns darf nicht leer sein");
	}
	if (columns.size() < 2) {
		return Status::Error("createCompositeIndex: mindestens 2 Spalten erforderlich (nutze createIndex für Single-Column)");
	}
	for (const auto& col : columns) {
		if (col.empty() || col.find(':') != std::string::npos || col.find('+') != std::string::npos) {
			return Status::Error("createCompositeIndex: ungültiger Spaltenname: " + col);
		}
	}
	
	std::string metaKey = makeCompositeIndexMetaKey(table, columns);
	std::string metaValue = unique ? "unique" : "regular";
	std::vector<uint8_t> marker(metaValue.begin(), metaValue.end());
	if (!db_.put(metaKey, marker)) {
		return Status::Error("createCompositeIndex: Schreiben des Metaschlüssels fehlgeschlagen: " + metaKey);
	}
	SecondaryIndexMetadataCache::instance().invalidate(std::string(table));
	std::string colList = {};
	for (size_t i = 0; i < columns.size(); ++i) {
		if (i > 0) {
		  colList += ", ";
		}
		colList += columns[i];
	}
	THEMIS_INFO("Composite Index erstellt: {}.{{{}}} (unique={})", table, colList, unique);
	return Status::OK();
}

SecondaryIndexManager::Status SecondaryIndexManager::dropIndex(std::string_view table, std::string_view column) {
	if (table.empty() || column.empty()) {
		return Status::Error("dropIndex: table/column darf nicht leer sein");
	}
	std::string metaKey = makeIndexMetaKey(table, column);
	if (!db_.del(metaKey)) {
		return Status::Error("dropIndex: Löschen des Metaschlüssels fehlgeschlagen: " + metaKey);
	}
	
	// v1.3.4: Invalidate cache when index structure changes
	SecondaryIndexMetadataCache::instance().invalidate(table);
	
	THEMIS_INFO("Index gelöscht: {}.{}", table, column);
	return Status::OK();
}

SecondaryIndexManager::Status SecondaryIndexManager::dropCompositeIndex(std::string_view table, const std::vector<std::string>& columns) {
	if (table.empty() || columns.empty()) {
		return Status::Error("dropCompositeIndex: table/columns darf nicht leer sein");
	}
	std::string metaKey = makeCompositeIndexMetaKey(table, columns);
	if (!db_.del(metaKey)) {
		return Status::Error("dropCompositeIndex: Löschen des Metaschlüssels fehlgeschlagen: " + metaKey);
	}
	
	// v1.3.4: Invalidate cache when index structure changes
	SecondaryIndexMetadataCache::instance().invalidate(table);
	
	std::string colList = {};
	for (size_t i = 0; i < columns.size(); ++i) {
		if (i > 0) {
		  colList += ", ";
		}
		colList += columns[i];
	}
	THEMIS_INFO("Composite Index gelöscht: {}.{{{}}}", table, colList);
	return Status::OK();
}

bool SecondaryIndexManager::hasIndex(std::string_view table, std::string_view column) const {
	std::string metaKey = makeIndexMetaKey(table, column);
	return db_.get(metaKey).has_value();
}

bool SecondaryIndexManager::hasCompositeIndex(std::string_view table, const std::vector<std::string>& columns) const {
	std::string metaKey = makeCompositeIndexMetaKey(table, columns);
	return db_.get(metaKey).has_value();
}

SecondaryIndexManager::Status SecondaryIndexManager::createRangeIndex(std::string_view table, std::string_view column) {
	if (table.empty() || column.empty()) {
	  return Status::Error("createRangeIndex: table/column darf nicht leer sein");
	}
	if (std::string(table).find(':') != std::string::npos || std::string(column).find(':') != std::string::npos) {
		return Status::Error("createRangeIndex: ':' ist in table/column nicht erlaubt");
	}
	std::string metaKey = makeRangeIndexMetaKey(table, column);
	std::vector<uint8_t> marker = {1};
	if (!db_.put(metaKey, marker)) {
	  return Status::Error("createRangeIndex: Schreiben des Metaschlüssels fehlgeschlagen: " + metaKey);
	}
	SecondaryIndexMetadataCache::instance().invalidate(table);
	THEMIS_INFO("Range Index erstellt: {}.{}", table, column);
	return Status::OK();
}

SecondaryIndexManager::Status SecondaryIndexManager::dropRangeIndex(std::string_view table, std::string_view column) {
	if (table.empty() || column.empty()) {
	  return Status::Error("dropRangeIndex: table/column darf nicht leer sein");
	}
	std::string metaKey = makeRangeIndexMetaKey(table, column);
	if (!db_.del(metaKey)) {
	  return Status::Error("dropRangeIndex: Löschen des Metaschlüssels fehlgeschlagen: " + metaKey);
	}
	SecondaryIndexMetadataCache::instance().invalidate(table);
	THEMIS_INFO("Range Index gelöscht: {}.{}", table, column);
	return Status::OK();
}

bool SecondaryIndexManager::hasRangeIndex(std::string_view table, std::string_view column) const {
	std::string metaKey = makeRangeIndexMetaKey(table, column);
	return db_.get(metaKey).has_value();
}

// ────────────────────────────────────────────────────────────────────────────
// Sparse-Index: überspringt NULL/leere Werte
// ────────────────────────────────────────────────────────────────────────────

SecondaryIndexManager::Status SecondaryIndexManager::createSparseIndex(std::string_view table, std::string_view column, bool unique) {
	if (table.empty() || column.empty()) {
	  return Status::Error("createSparseIndex: table/column darf nicht leer sein");
	}
	if (table.find(':') != std::string::npos || column.find(':') != std::string::npos) {
		return Status::Error("createSparseIndex: ':' ist in table/column nicht erlaubt");
	}
	std::string metaKey = makeSparseIndexMetaKey(table, column);
	std::string marker = unique ? "unique" : "regular";
	std::vector<uint8_t> markerBytes(marker.begin(), marker.end());
	if (!db_.put(metaKey, markerBytes)) {
	  return Status::Error("createSparseIndex: Schreiben des Metaschlüssels fehlgeschlagen: " + metaKey);
	}
	SecondaryIndexMetadataCache::instance().invalidate(table);
	THEMIS_INFO("Sparse Index erstellt: {}.{} (unique={})", table, column, unique);
	return Status::OK();
}

SecondaryIndexManager::Status SecondaryIndexManager::dropSparseIndex(std::string_view table, std::string_view column) {
	if (table.empty() || column.empty()) {
	  return Status::Error("dropSparseIndex: table/column darf nicht leer sein");
	}
	std::string metaKey = makeSparseIndexMetaKey(table, column);
	if (!db_.del(metaKey)) {
	  return Status::Error("dropSparseIndex: Löschen des Metaschlüssels fehlgeschlagen: " + metaKey);
	}
	SecondaryIndexMetadataCache::instance().invalidate(table);
	THEMIS_INFO("Sparse Index gelöscht: {}.{}", table, column);
	return Status::OK();
}

bool SecondaryIndexManager::hasSparseIndex(std::string_view table, std::string_view column) const {
	std::string metaKey = makeSparseIndexMetaKey(table, column);
	return db_.get(metaKey).has_value();
}

// ────────────────────────────────────────────────────────────────────────────
// Geo-Index: GeoJSON-Punkt-Speicherung mit Geohash
// ────────────────────────────────────────────────────────────────────────────

SecondaryIndexManager::Status SecondaryIndexManager::createGeoIndex(std::string_view table, std::string_view column) {
	if (table.empty() || column.empty()) {
	  return Status::Error("createGeoIndex: table/column darf nicht leer sein");
	}
	if (table.find(':') != std::string::npos || column.find(':') != std::string::npos) {
		return Status::Error("createGeoIndex: ':' ist in table/column nicht erlaubt");
	}
	std::string metaKey = makeGeoIndexMetaKey(table, column);
	std::string marker = "geo";
	std::vector<uint8_t> markerBytes(marker.begin(), marker.end());
	if (!db_.put(metaKey, markerBytes)) {
	  return Status::Error("createGeoIndex: Schreiben des Metaschlüssels fehlgeschlagen: " + metaKey);
	}
	SecondaryIndexMetadataCache::instance().invalidate(table);
	THEMIS_INFO("Geo Index erstellt: {}.{}", table, column);
	return Status::OK();
}

SecondaryIndexManager::Status SecondaryIndexManager::dropGeoIndex(std::string_view table, std::string_view column) {
	if (table.empty() || column.empty()) {
	  return Status::Error("dropGeoIndex: table/column darf nicht leer sein");
	}
	std::string metaKey = makeGeoIndexMetaKey(table, column);
	if (!db_.del(metaKey)) {
	  return Status::Error("dropGeoIndex: Löschen des Metaschlüssels fehlgeschlagen: " + metaKey);
	}
	SecondaryIndexMetadataCache::instance().invalidate(table);
	THEMIS_INFO("Geo Index gelöscht: {}.{}", table, column);
	return Status::OK();
}

bool SecondaryIndexManager::hasGeoIndex(std::string_view table, std::string_view column) const {
	std::string metaKey = makeGeoIndexMetaKey(table, column);
	return db_.get(metaKey).has_value();
}

// ────────────────────────────────────────────────────────────────────────────
// TTL-Index
// ────────────────────────────────────────────────────────────────────────────

SecondaryIndexManager::Status SecondaryIndexManager::createTTLIndex(std::string_view table, std::string_view column, int64_t ttl_seconds) {
	if (table.empty() || column.empty()) {
	  return Status::Error("createTTLIndex: table/column darf nicht leer sein");
	}
	if (ttl_seconds <= 0) {
	  return Status::Error("createTTLIndex: ttl_seconds muss > 0 sein");
	}
	if (table.find(':') != std::string::npos || column.find(':') != std::string::npos) {
		return Status::Error("createTTLIndex: ':' ist in table/column nicht erlaubt");
	}
	std::string metaKey = makeTTLIndexMetaKey(table, column);
	std::string ttlValue = std::to_string(ttl_seconds);
	std::vector<uint8_t> ttlBytes(ttlValue.begin(), ttlValue.end());
	if (!db_.put(metaKey, ttlBytes)) {
	  return Status::Error("createTTLIndex: Schreiben des Metaschlüssels fehlgeschlagen: " + metaKey);
	}
	SecondaryIndexMetadataCache::instance().invalidate(table);
	THEMIS_INFO("TTL Index erstellt: {}.{} (TTL={}s)", table, column, ttl_seconds);
	return Status::OK();
}

SecondaryIndexManager::Status SecondaryIndexManager::dropTTLIndex(std::string_view table, std::string_view column) {
	if (table.empty() || column.empty()) {
	  return Status::Error("dropTTLIndex: table/column darf nicht leer sein");
	}
	std::string metaKey = makeTTLIndexMetaKey(table, column);
	if (!db_.del(metaKey)) {
	  return Status::Error("dropTTLIndex: Löschen des Metaschlüssels fehlgeschlagen: " + metaKey);
	}
	SecondaryIndexMetadataCache::instance().invalidate(table);
	THEMIS_INFO("TTL Index gelöscht: {}.{}", table, column);
	return Status::OK();
}

bool SecondaryIndexManager::hasTTLIndex(std::string_view table, std::string_view column) const {
	std::string metaKey = makeTTLIndexMetaKey(table, column);
	return db_.get(metaKey).has_value();
}

// ────────────────────────────────────────────────────────────────────────────
// Fulltext-Index
// ────────────────────────────────────────────────────────────────────────────

SecondaryIndexManager::Status SecondaryIndexManager::createFulltextIndex(
	std::string_view table, 
	std::string_view column,
	const FulltextConfig& config
) {
	if (table.empty() || column.empty()) {
	  return Status::Error("createFulltextIndex: table/column darf nicht leer sein");
	}
	if (table.find(':') != std::string::npos || column.find(':') != std::string::npos) {
		return Status::Error("createFulltextIndex: ':' ist in table/column nicht erlaubt");
	}
	
	// Serialize config to JSON
	nlohmann::json configJson = {
		{"type", "fulltext"},
		{"stemming_enabled", config.stemming_enabled},
		{"language", config.language},
		{"stopwords_enabled", config.stopwords_enabled},
		{"stopwords", config.stopwords},
		{"normalize_umlauts", config.normalize_umlauts}
	};
	std::string configStr = configJson.dump();
	std::vector<uint8_t> configBytes(configStr.begin(), configStr.end());
	
	std::string metaKey = makeFulltextIndexMetaKey(table, column);
	if (!db_.put(metaKey, configBytes)) {
		return Status::Error("createFulltextIndex: Schreiben des Metaschlüssels fehlgeschlagen: " + metaKey);
	}
	
	SecondaryIndexMetadataCache::instance().invalidate(table);
	
		THEMIS_INFO("Fulltext Index erstellt: {}.{} (stemming={}, lang={}, stopwords_enabled={}, stopwords={}, normalize_umlauts={})", 
			table, column, config.stemming_enabled, config.language, config.stopwords_enabled, config.stopwords.size(), config.normalize_umlauts);
	return Status::OK();
}

// Overload that uses default config
SecondaryIndexManager::Status SecondaryIndexManager::createFulltextIndex(std::string_view table, std::string_view column) {
	return createFulltextIndex(table, column, FulltextConfig{});
}

SecondaryIndexManager::Status SecondaryIndexManager::dropFulltextIndex(std::string_view table, std::string_view column) {
	if (table.empty() || column.empty()) {
	  return Status::Error("dropFulltextIndex: table/column darf nicht leer sein");
	}
	std::string metaKey = makeFulltextIndexMetaKey(table, column);
	if (!db_.del(metaKey)) {
	  return Status::Error("dropFulltextIndex: Löschen des Metaschlüssels fehlgeschlagen: " + metaKey);
	}
	SecondaryIndexMetadataCache::instance().invalidate(table);
	THEMIS_INFO("Fulltext Index gelöscht: {}.{}", table, column);
	return Status::OK();
}

bool SecondaryIndexManager::hasFulltextIndex(std::string_view table, std::string_view column) const {
	std::string metaKey = makeFulltextIndexMetaKey(table, column);
	return db_.get(metaKey).has_value();
}

std::optional<SecondaryIndexManager::FulltextConfig> 
SecondaryIndexManager::getFulltextConfig(std::string_view table, std::string_view column) const {
	std::string metaKey = makeFulltextIndexMetaKey(table, column);
	auto val = db_.get(metaKey);
	if (!val) {
	  return std::nullopt;
	}
	
	try {
		std::string configStr(val->begin(), val->end());
		auto configJson = nlohmann::json::parse(configStr);
		
		FulltextConfig config;
		config.stemming_enabled = configJson.value("stemming_enabled", false);
		config.language = configJson.value("language", std::string("none"));
		config.stopwords_enabled = configJson.value("stopwords_enabled", false);
		if (configJson.contains("stopwords") && configJson["stopwords"].is_array()) {
			config.stopwords.clear();
			for (const auto& s : configJson["stopwords"]) {
				if (s.is_string()) {
				  config.stopwords.emplace_back(s.get<std::string>());
				}
			}
		}
		config.normalize_umlauts = configJson.value("normalize_umlauts", false);
		return config;
	} catch (...) {
		// Legacy format (just "fulltext" marker) - return default config
		return FulltextConfig{};
	}
}

// Lädt alle Spalten, die für eine Tabelle indiziert sind, aus den Metaschlüsseln
std::unordered_set<std::string> SecondaryIndexManager::loadIndexedColumns_(std::string_view table) const {
	std::unordered_set<std::string> cols;
	const std::string prefix = std::string("idxmeta:") + std::string(table) + ":";
	db_.scanPrefix(prefix, [&cols, &prefix](std::string_view key, std::string_view /*value*/) {
		// key = idxmeta:<table>:<column> oder idxmeta:<table>:col1+col2+...
		std::string_view rest = key.substr(prefix.size());
		cols.insert(std::string(rest));
		return true;
	});
	return cols;
}

std::unordered_set<std::string> SecondaryIndexManager::loadRangeIndexedColumns_(std::string_view table) const {
	std::unordered_set<std::string> cols;
	const std::string prefix = std::string("ridxmeta:") + std::string(table) + ":";
	db_.scanPrefix(prefix, [&cols, &prefix](std::string_view key, std::string_view /*value*/) {
		std::string_view rest = key.substr(prefix.size());
		cols.insert(std::string(rest));
		return true;
	});
	return cols;
}

std::unordered_set<std::string> SecondaryIndexManager::loadSparseIndexedColumns_(std::string_view table) const {
	std::unordered_set<std::string> cols;
	const std::string prefix = std::string("sidxmeta:") + std::string(table) + ":";
	db_.scanPrefix(prefix, [&cols, &prefix](std::string_view key, std::string_view /*value*/) {
		std::string_view rest = key.substr(prefix.size());
		cols.insert(std::string(rest));
		return true;
	});
	return cols;
}

std::unordered_set<std::string> SecondaryIndexManager::loadGeoIndexedColumns_(std::string_view table) const {
	std::unordered_set<std::string> cols;
	const std::string prefix = std::string("gidxmeta:") + std::string(table) + ":";
	db_.scanPrefix(prefix, [&cols, &prefix](std::string_view key, std::string_view /*value*/) {
		std::string_view rest = key.substr(prefix.size());
		cols.insert(std::string(rest));
		return true;
	});
	return cols;
}

std::unordered_set<std::string> SecondaryIndexManager::loadTTLIndexedColumns_(std::string_view table) const {
	std::unordered_set<std::string> cols;
	const std::string prefix = std::string("ttlidxmeta:") + std::string(table) + ":";
	db_.scanPrefix(prefix, [&cols, &prefix](std::string_view key, std::string_view /*value*/) {
		std::string_view rest = key.substr(prefix.size());
		cols.insert(std::string(rest));
		return true;
	});
	return cols;
}

std::unordered_set<std::string> SecondaryIndexManager::loadFulltextIndexedColumns_(std::string_view table) const {
	std::unordered_set<std::string> cols;
	const std::string prefix = std::string("ftidxmeta:") + std::string(table) + ":";
	db_.scanPrefix(prefix, [&cols, &prefix](std::string_view key, std::string_view /*value*/) {
		std::string_view rest = key.substr(prefix.size());
		cols.insert(std::string(rest));
		return true;
	});
	return cols;
}

std::unordered_map<std::string, std::string> SecondaryIndexManager::loadPartialIndexedColumns_(std::string_view table) const {
	std::unordered_map<std::string, std::string> result; // column -> predicate
	const std::string prefix = std::string("pidxmeta:") + std::string(table) + ":";
	db_.scanPrefix(prefix, [&result, &prefix](std::string_view key, std::string_view value) {
		std::string col(key.substr(prefix.size()));
		std::string metaVal(value.begin(), value.end());
		// Strip "|unique" suffix to get just the predicate
		auto pipePos = metaVal.find('|');
		std::string predicate = (pipePos != std::string::npos) ? metaVal.substr(0, pipePos) : metaVal;
		result[col] = predicate;
		return true;
	});
	return result;
}

int64_t SecondaryIndexManager::getTTLSeconds_(std::string_view table, std::string_view column) const {
	std::string metaKey = makeTTLIndexMetaKey(table, column);
	auto val = db_.get(metaKey);
	if (!val) {
	  return 0;
	}
	std::string ttlStr(val->begin(), val->end());
	try {
		return std::stoll(ttlStr);
	} catch (...) {
		return 0;
	}
}

bool SecondaryIndexManager::isUniqueIndex_(std::string_view table, std::string_view column) const {
	std::string metaKey = makeIndexMetaKey(table, column);
	auto val = db_.get(metaKey);
	if (!val) {
	  return false;
	}
	std::string metaValue(val->begin(), val->end());
	return metaValue == "unique";
}

bool SecondaryIndexManager::isUniqueCompositeIndex_(std::string_view table, const std::vector<std::string>& columns) const {
	std::string metaKey = makeCompositeIndexMetaKey(table, columns);
	auto val = db_.get(metaKey);
	if (!val) {
	  return false;
	}
	std::string metaValue(val->begin(), val->end());
	return metaValue == "unique";
}

bool SecondaryIndexManager::isSparseIndexUnique_(std::string_view table, std::string_view column) const {
	std::string metaKey = makeSparseIndexMetaKey(table, column);
	auto val = db_.get(metaKey);
	if (!val) {
	  return false;
	}
	std::string metaValue(val->begin(), val->end());
	return metaValue == "unique";
}

bool SecondaryIndexManager::isPartialIndexUnique_(std::string_view table, std::string_view column) const {
	std::string metaKey = makePartialIndexMetaKey(table, column);
	auto val = db_.get(metaKey);
	if (!val) {
	  return false;
	}
	std::string metaValue(val->begin(), val->end());
	return metaValue.find("|unique") != std::string::npos;
}

// ────────────────────────────────────────────────────────────────────────────
// Partial Index: Predicate Evaluator
// ────────────────────────────────────────────────────────────────────────────

// static
bool SecondaryIndexManager::evaluatePartialPredicate_(const BaseEntity& entity, const std::string& predicate) {
	// Trim whitespace helper
	auto trim = [](std::string s) -> std::string {
		size_t start = s.find_first_not_of(" \t\r\n");
		if (start == std::string::npos) {
		  return "";
		}
		size_t end = s.find_last_not_of(" \t\r\n");
		return s.substr(start, end - start + 1);
	};

	std::string expr = trim(predicate);
	if (expr.empty()) return true; // empty predicate = always match

	// Build uppercase copy for keyword matching
	std::string upper = {};
	upper.reserve(expr.size());
	for (unsigned char c : expr) {
	  upper += static_cast<char>(std::toupper(c));
	}

	// IS NOT NULL check
	{
		auto pos = upper.rfind(" IS NOT NULL");
		if (pos != std::string::npos) {
			std::string field = trim(expr.substr(0, pos));
			auto val = entity.extractField(field);
			return val.has_value() && !isNullOrEmpty_(val);
		}
	}

	// IS NULL check
	{
		auto pos = upper.rfind(" IS NULL");
		if (pos != std::string::npos) {
			std::string field = trim(expr.substr(0, pos));
			auto val = entity.extractField(field);
			return !val.has_value() || isNullOrEmpty_(val);
		}
	}

	// Comparison operators - try longest first to avoid partial match (>= before >)
	static const std::pair<std::string, int> ops[] = {
		{">=", 5}, {"<=", 3}, {"!=", 1}, {"=", 0}, {">", 4}, {"<", 2}
	};

	for (const auto& [op, kind] : ops) {
		auto pos = expr.find(op);
		if (pos == std::string::npos) {
		  continue;
		}

		// Ensure '=' match is not part of '>=' or '<=' or '!=' already handled above
		if (op == "=" && pos > 0) {
			char prev = expr[pos - 1];
			if (prev == '>' || prev == '<' || prev == '!') {
			  continue;
			}
		}

		std::string field = trim(expr.substr(0, pos));
		std::string rhs   = trim(expr.substr(pos + op.size()));

		if (field.empty()) {
		  continue;
		}

		// Strip surrounding quotes from string literals
		if (rhs.size() >= 2 &&
		    ((rhs.front() == '\'' && rhs.back() == '\'') ||
		     (rhs.front() == '"'  && rhs.back() == '"'))) {
			rhs = rhs.substr(1, rhs.size() - 2);
		}

		auto fieldVal = entity.extractField(field);
		if (!fieldVal) return false; // field missing → not in index
		const std::string& fv = *fieldVal;

		// Try numeric comparison
		bool numericOk = false;
		double fvNum = 0.0, rhsNum = 0.0;
		try {
			fvNum   = std::stod(fv);
			rhsNum  = std::stod(rhs);
			numericOk = true;
		} catch (...) {}

		switch (kind) {
			case 0: return numericOk ? (fvNum == rhsNum) : (fv == rhs);
			case 1: return numericOk ? (fvNum != rhsNum) : (fv != rhs);
			case 2: return numericOk ? (fvNum <  rhsNum) : (fv <  rhs);
			case 3: return numericOk ? (fvNum <= rhsNum) : (fv <= rhs);
			case 4: return numericOk ? (fvNum >  rhsNum) : (fv >  rhs);
			case 5: return numericOk ? (fvNum >= rhsNum) : (fv >= rhs);
			default: return false;
		}
	}

	// Unparseable predicate - conservative: exclude from index
	THEMIS_WARN("evaluatePartialPredicate_: Prädikat nicht parsierbar: '{}'", predicate);
	return false;
}

// ────────────────────────────────────────────────────────────────────────────
// Partial Index: Lifecycle
// ────────────────────────────────────────────────────────────────────────────

SecondaryIndexManager::Status SecondaryIndexManager::createPartialIndex(
		std::string_view table, std::string_view column,
		std::string_view predicate, bool unique) {
	if (table.empty() || column.empty())
		return Status::Error("createPartialIndex: table/column darf nicht leer sein");
	if (std::string(table).find(':') != std::string::npos ||
	    std::string(column).find(':') != std::string::npos)
		return Status::Error("createPartialIndex: ':' ist in table/column nicht erlaubt");
	if (predicate.empty())
		return Status::Error("createPartialIndex: predicate darf nicht leer sein");

	// Store as "predicate" or "predicate|unique"
	std::string metaValue(predicate);
	if (unique) {
	  metaValue += "|unique";
	}
	std::string metaKey = makePartialIndexMetaKey(table, column);
	std::vector<uint8_t> marker(metaValue.begin(), metaValue.end());
	if (!db_.put(metaKey, marker))
		return Status::Error("createPartialIndex: Schreiben des Metaschlüssels fehlgeschlagen: " + metaKey);

	SecondaryIndexMetadataCache::instance().invalidate(table);
	THEMIS_INFO("Partial Index erstellt: {}.{} predicate='{}' (unique={})", table, column, predicate, unique);
	return Status::OK();
}

SecondaryIndexManager::Status SecondaryIndexManager::dropPartialIndex(
		std::string_view table, std::string_view column) {
	if (table.empty() || column.empty())
		return Status::Error("dropPartialIndex: table/column darf nicht leer sein");
	std::string metaKey = makePartialIndexMetaKey(table, column);
	if (!db_.del(metaKey))
		return Status::Error("dropPartialIndex: Löschen des Metaschlüssels fehlgeschlagen: " + metaKey);

	SecondaryIndexMetadataCache::instance().invalidate(table);
	THEMIS_INFO("Partial Index gelöscht: {}.{}", table, column);
	return Status::OK();
}

bool SecondaryIndexManager::hasPartialIndex(std::string_view table, std::string_view column) const {
	return db_.get(makePartialIndexMetaKey(table, column)).has_value();
}

std::optional<std::string> SecondaryIndexManager::getPartialIndexPredicate(
		std::string_view table, std::string_view column) const {
	auto val = db_.get(makePartialIndexMetaKey(table, column));
	if (!val) {
	  return std::nullopt;
	}
	std::string metaVal(val->begin(), val->end());
	// Strip "|unique" suffix
	auto pipePos = metaVal.find('|');
	if (pipePos != std::string::npos) {
	  metaVal.resize(pipePos);
	}
	return metaVal;
}

std::pair<SecondaryIndexManager::Status, std::vector<std::string>>
SecondaryIndexManager::scanKeysEqualPartial(std::string_view table,
                                             std::string_view column,
                                             std::string_view value) const {
	if (!hasPartialIndex(table, column))
		return {Status::Error("scanKeysEqualPartial: kein partieller Index für " +
		                      std::string(table) + "." + std::string(column)), std::vector<std::string>()};

	const std::string encodedVal = encodeKeyComponent(value);
	const std::string prefix = makePartialIndexPrefix(table, column, encodedVal);
	std::vector<std::string> pks;
	db_.scanPrefix(prefix, [&pks](std::string_view key, std::string_view /*val*/) {
		size_t lastColon = key.rfind(':');
		if (lastColon != std::string_view::npos)
			pks.emplace_back(key.substr(lastColon + 1));
		return true;
	});
	return {Status::OK(), std::move(pks)};
}

SecondaryIndexManager::Status SecondaryIndexManager::put(std::string_view table, const BaseEntity& entity) {
	if (table.empty()) {
	  return Status::Error("put: table darf nicht leer sein");
	}
	const std::string& pk = entity.getPrimaryKey();
	if (pk.empty()) {
	  return Status::Error("put: Entity hat keinen Primary Key");
	}
	if (!db_.isOpen()) {
	  return Status::Error("put: Datenbank ist nicht geöffnet");
	}

	// Atomare Batch-Operation (old-entity read is done inside put(batch) for index cleanup)
	auto batch = db_.createWriteBatch();
	if (!batch) {
	  return Status::Error("put: Konnte WriteBatch nicht erstellen");
	}
	auto st = put(table, entity, *batch);
	if (!st.ok) { batch->rollback(); return st; }
	if (!batch->commit()) {
	  return Status::Error("put: Commit des Batches fehlgeschlagen (atomare Aktualisierung)");
	}
	return Status::OK();
}

SecondaryIndexManager::Status SecondaryIndexManager::erase(std::string_view table, std::string_view pk) {
	if (table.empty()) {
	  return Status::Error("erase: table darf nicht leer sein");
	}
	if (pk.empty()) {
	  return Status::Error("erase: pk darf nicht leer sein");
	}
	if (!db_.isOpen()) {
	  return Status::Error("erase: Datenbank ist nicht geöffnet");
	}

	// old-entity read is done inside erase(batch) for index cleanup
	auto batch = db_.createWriteBatch();
	if (!batch) {
	  return Status::Error("erase: Konnte WriteBatch nicht erstellen");
	}
	auto st = erase(table, pk, *batch);
	if (!st.ok) { batch->rollback(); return st; }
	if (!batch->commit()) {
	  return Status::Error("erase: Commit des Batches fehlgeschlagen (atomare Löschung)");
	}
	return Status::OK();
}

SecondaryIndexManager::Status SecondaryIndexManager::put(std::string_view table, const BaseEntity& entity, RocksDBWrapper::WriteBatchWrapper& batch) {
	if (table.empty()) {
	  return Status::Error("put(tx): table darf nicht leer sein");
	}
	const std::string& pk = entity.getPrimaryKey();
	if (pk.empty()) {
	  return Status::Error("put(tx): Entity hat keinen Primary Key");
	}
	if (!db_.isOpen()) {
	  return Status::Error("put(tx): Datenbank ist nicht geöffnet");
	}

	const std::string relKey = KeySchema::makeRelationalKey(table, pk);
	std::optional<std::vector<uint8_t>> oldBlob = db_.get(relKey);
	std::unique_ptr<BaseEntity> oldEntity = {};

	if (oldBlob) {
		try { oldEntity = std::make_unique<BaseEntity>(BaseEntity::deserialize(pk, *oldBlob)); }
		catch (...) { THEMIS_WARN("put(tx): alte Entity für PK={} nicht deserialisierbar", pk); }
	}

	// Serialize entity once and reuse for both entity write and geo hook
	std::vector<uint8_t> serialized_entity = entity.serialize();
	batch.put(relKey, serialized_entity);

	if (oldEntity) {
		auto st = updateIndexesForDelete_(table, pk, oldEntity.get(), batch);
		if (!st.ok) {
		  return st;
		}
	}
	
	// Phase 2: Atomic geo index update if spatial index manager is available
	if (spatial_index_mgr_) {
		// In modular builds, geo hooks can live in the geo module.
		// Keep entity writes/index updates functional even when hooks are unavailable.
		THEMIS_DEBUG("Spatial manager set for {}:{}, geo hooks deferred in this module", table, pk);
	}
	
	return updateIndexesForPut_(table, pk, entity, batch);
}

SecondaryIndexManager::Status SecondaryIndexManager::erase(std::string_view table, std::string_view pk, RocksDBWrapper::WriteBatchWrapper& batch) {
	if (table.empty()) {
	  return Status::Error("erase(tx): table darf nicht leer sein");
	}
	if (pk.empty()) {
	  return Status::Error("erase(tx): pk darf nicht leer sein");
	}
	if (!db_.isOpen()) {
	  return Status::Error("erase(tx): Datenbank ist nicht geöffnet");
	}

	const std::string relKey = KeySchema::makeRelationalKey(table, pk);
	std::optional<std::vector<uint8_t>> oldBlob = db_.get(relKey);
	std::unique_ptr<BaseEntity> oldEntity = {};

	if (oldBlob) {
		try { oldEntity = std::make_unique<BaseEntity>(BaseEntity::deserialize(std::string(pk), *oldBlob)); }
		catch (...) { THEMIS_WARN("erase(tx): alte Entity für PK={} nicht deserialisierbar", pk); }
	}

	batch.del(relKey);
	return updateIndexesForDelete_(table, pk, oldEntity.get(), batch);
}

// v1.3.4+: Batch Insert API - entities are committed in transaction chunks.
SecondaryIndexManager::Status SecondaryIndexManager::putBatch(std::string_view table, const std::vector<BaseEntity>& entities) {
	return putBatch(table, entities, transactional_put_batch_size_);
}

void SecondaryIndexManager::setTransactionalPutBatchSize([[maybe_unused]] size_t batch_size) {
	transactional_put_batch_size_ = std::max<size_t>(size_t{1}, batch_size);
}

SecondaryIndexManager::Status SecondaryIndexManager::putBatch(std::string_view table, const std::vector<BaseEntity>& entities, size_t transaction_batch_size) {
	if (table.empty()) {
	  return Status::Error("putBatch: table darf nicht leer sein");
	}
	if (entities.empty()) return Status::OK(); // Nothing to do
	if (!db_.isOpen()) {
	  return Status::Error("putBatch: Datenbank ist nicht geöffnet");
	}
	if (transaction_batch_size == 0) {
	  return Status::Error("putBatch: transaction_batch_size darf nicht 0 sein");
	}

	for (size_t chunk_begin = 0; chunk_begin < entities.size(); chunk_begin += transaction_batch_size) {
		const size_t chunk_end = std::min(chunk_begin + transaction_batch_size, entities.size());
		auto batch = db_.createWriteBatch();
		if (!batch) {
		  return Status::Error("putBatch: Konnte WriteBatch nicht erstellen");
		}

		for (size_t i = chunk_begin; i < chunk_end; ++i) {
			const auto& entity = entities[i];
			const std::string pk = entity.getPrimaryKey();
			if (pk.empty()) {
				batch->rollback();
				return Status::Error("putBatch: Entity ohne Primary Key gefunden");
			}

			const std::string relKey = KeySchema::makeRelationalKey(table, pk);

			// Load old entity for index cleanup (if exists)
			std::optional<std::vector<uint8_t>> oldBlob = db_.get(relKey);
			std::unique_ptr<BaseEntity> oldEntity = {};

			if (oldBlob) {
				try {
					oldEntity = std::make_unique<BaseEntity>(BaseEntity::deserialize(pk, *oldBlob));
				}
				catch (...) {
					THEMIS_WARN("putBatch: alte Entity für PK={} nicht deserialisierbar", pk);
				}
			}

			// Write entity
			batch->put(relKey, entity.serialize());

			// Update indexes
			if (oldEntity) {
				auto st = updateIndexesForDelete_(table, pk, oldEntity.get(), *batch);
				if (!st.ok) { batch->rollback(); return st; }
			}

			auto st = updateIndexesForPut_(table, pk, entity, *batch);
			if (!st.ok) { batch->rollback(); return st; }
		}

		// Single commit for one transaction chunk
		if (!batch->commit()) {
			return Status::Error("putBatch: WriteBatch commit fehlgeschlagen");
		}
	}

	return Status::OK();
}

SecondaryIndexManager::Status SecondaryIndexManager::updateIndexesForPut_(std::string_view table,
																		  std::string_view pk,
																		  const BaseEntity& newEntity,
																		  RocksDBWrapper::WriteBatchWrapper& batch) {
	// v1.3.4 OPTIMIZATION: Use metadata cache to avoid repeated DB scans
	auto& cache = SecondaryIndexMetadataCache::instance();
	auto cachedMetadata = cache.get(table);
	const bool hasCachedMetadata = cachedMetadata.has_value();

	std::unordered_map<std::string, bool> regularUniqueCache;
	std::unordered_map<std::string, bool> sparseUniqueCache;
	std::unordered_map<std::string, bool> compositeUniqueCache;
	std::unordered_map<std::string, int64_t> ttlSecondsCache;
	std::unordered_map<std::string, SecondaryIndexMetadataCache::CachedFulltextConfig> fulltextConfigsCache;
	std::unordered_map<std::string, std::string> partialPredicatesCache;
	std::unordered_map<std::string, bool> partialUniqueCache;
	std::vector<std::string> sparseColsCache;
	std::vector<std::string> geoColsCache;
	std::vector<std::string> ttlColsCache;
	std::vector<std::string> fulltextColsCache;
	std::vector<std::string> partialColsOrderCache;
	
	// On cache hit use the precomputed sets directly; on miss load from DB and
	// populate the cache including the precomputed sets for future hits.
	const std::unordered_set<std::string>* indexedColsPtr = nullptr;
	const std::unordered_set<std::string>* rangeColsPtr   = nullptr;
	std::unordered_set<std::string> indexedColsMiss, rangeColsMiss;
	std::unordered_set<std::string> indexedColsCache, rangeColsCache;

	if (hasCachedMetadata) {
		const auto metadata = *cachedMetadata;
		indexedColsCache = metadata.regular_indexes_set;
		rangeColsCache = metadata.range_indexes_set;
		regularUniqueCache = metadata.regular_unique;
		sparseUniqueCache = metadata.sparse_unique;
		compositeUniqueCache = metadata.composite_unique;
		ttlSecondsCache = metadata.ttl_seconds;
		fulltextConfigsCache = metadata.fulltext_configs;
		partialPredicatesCache = metadata.partial_predicates;
		partialUniqueCache = metadata.partial_unique;
		sparseColsCache = metadata.sparse_indexes;
		geoColsCache = metadata.geo_indexes;
		ttlColsCache = metadata.ttl_indexes;
		fulltextColsCache = metadata.fulltext_indexes;
		partialColsOrderCache = metadata.partial_indexes;
		indexedColsPtr = &indexedColsCache;
		rangeColsPtr   = &rangeColsCache;
	} else {
		// Cache miss - load from DB and populate cache
		indexedColsMiss = loadIndexedColumns_(table);
		rangeColsMiss   = loadRangeIndexedColumns_(table);
		
		// Populate cache for next time
		SecondaryIndexMetadataCache::IndexMetadata metadata;
		metadata.regular_indexes = std::vector<std::string>(indexedColsMiss.begin(), indexedColsMiss.end());
		metadata.range_indexes   = std::vector<std::string>(rangeColsMiss.begin(), rangeColsMiss.end());
		metadata.regular_indexes_set = indexedColsMiss;
		metadata.range_indexes_set   = rangeColsMiss;
		for (const auto& col : metadata.regular_indexes) {
			metadata.regular_unique[col] = isUniqueIndex_(table, col);
		}
		
		// Load other index types too
		auto sparseCols = loadSparseIndexedColumns_(table);
		metadata.sparse_indexes = std::vector<std::string>(sparseCols.begin(), sparseCols.end());
		for (const auto& col : metadata.sparse_indexes) {
			metadata.sparse_unique[col] = isSparseIndexUnique_(table, col);
		}
		auto geoCols = loadGeoIndexedColumns_(table);
		metadata.geo_indexes = std::vector<std::string>(geoCols.begin(), geoCols.end());
		auto ttlCols = loadTTLIndexedColumns_(table);
		metadata.ttl_indexes = std::vector<std::string>(ttlCols.begin(), ttlCols.end());
		auto ftCols = loadFulltextIndexedColumns_(table);
		metadata.fulltext_indexes = std::vector<std::string>(ftCols.begin(), ftCols.end());
		auto partialColsMap = loadPartialIndexedColumns_(table);
		for (const auto& [col, pred] : partialColsMap) {
			metadata.partial_indexes.emplace_back(col);
			metadata.partial_predicates[col] = pred;
			metadata.partial_unique[col] = isPartialIndexUnique_(table, col);
		}

		// v1.3.5: cache per-column TTL seconds to avoid db.get on every insert
		for (const auto& tcol : metadata.ttl_indexes) {
			metadata.ttl_seconds[tcol] = getTTLSeconds_(table, tcol);
		}

		// v1.3.5: cache per-column fulltext config to avoid db.get + JSON parse on every insert
		for (const auto& fcol : metadata.fulltext_indexes) {
			auto cfg = getFulltextConfig(table, fcol).value_or(FulltextConfig{});
			SecondaryIndexMetadataCache::CachedFulltextConfig cached;
			cached.stemming_enabled  = cfg.stemming_enabled;
			cached.language          = cfg.language;
			cached.stopwords_enabled = cfg.stopwords_enabled;
			cached.stopwords         = cfg.stopwords;
			cached.normalize_umlauts = cfg.normalize_umlauts;
			metadata.fulltext_configs[fcol] = std::move(cached);
		}

		// v1.3.5: cache composite unique flags to avoid db.get per composite insert
		for (const auto& col : metadata.regular_indexes) {
			if (col.find('+') != std::string::npos) {
				std::vector<std::string> columns;
				columns.reserve(std::count(col.begin(), col.end(), '+') + 1);
				size_t start = 0;
				while (start < col.size()) {
					size_t pos = col.find('+', start);
					if (pos == std::string::npos) { columns.emplace_back(col.substr(start)); break; }
					columns.emplace_back(col.substr(start, pos - start));
					start = pos + 1;
				}
				metadata.composite_unique[col] = isUniqueCompositeIndex_(table, columns);
			}
		}

		cache.set(table, metadata);
		indexedColsPtr = &indexedColsMiss;
		rangeColsPtr   = &rangeColsMiss;
	}

	const auto& indexedCols = *indexedColsPtr;
	const auto& rangeCols   = *rangeColsPtr;

	// Micro-Optimization: compute PK bytes once and reuse
	std::vector<uint8_t> pkBytes = toBytes(pk);

	// Trennen: Single-Column vs. Composite (enthält '+')
	for (const auto& col : indexedCols) {
		if (col.find('+') == std::string::npos) {
			// Single-Column
			auto maybe = newEntity.extractField(col);
			if (!maybe) {
			  continue;
			}
			const std::string encodedVal = encodeKeyComponent(*maybe);
			
			// Unique-Constraint prüfen
			bool uniqueIndex = false;
			if (hasCachedMetadata) {
				auto it = regularUniqueCache.find(col);
				uniqueIndex = (it != regularUniqueCache.end()) ? it->second : isUniqueIndex_(table, col);
			} else {
				uniqueIndex = isUniqueIndex_(table, col);
			}
			if (uniqueIndex) {
				// Prüfe ob bereits ein anderer PK mit diesem Wert existiert
				std::string prefix = std::string("idx:") + std::string(table) + ":" + col + ":" + encodedVal + ":";
				bool conflict = false;
				db_.scanPrefix(prefix, [&pk, &conflict](std::string_view key, std::string_view /*val*/) {
					// Extrahiere PK aus key: idx:table:column:value:PK
					size_t lastColon = key.rfind(':');
					if (lastColon != std::string_view::npos) {
						std::string_view existingPK = key.substr(lastColon + 1);
						if (existingPK != pk) {
							conflict = true;
							return false; // Stop scan
						}
					}
					return true;
				});
				if (conflict) {
					return Status::Error("Unique constraint violation: " + std::string(table) + "." + col + " = " + *maybe);
				}
			}
			
			const std::string idxKey = KeySchema::makeSecondaryIndexKey(table, col, encodedVal, pk);
			batch.put(idxKey, pkBytes);
			
			// Falls Range-Index für diese Spalte existiert, ebenfalls pflegen
			if (rangeCols.find(col) != rangeCols.end()) {
				const std::string rkey = makeRangeIndexKey(table, col, *maybe, pk);
				batch.put(rkey, pkBytes);
			}
		} else {
			// Composite: col = "col1+col2+..."
			// Parse columns
			std::vector<std::string> columns;
			columns.reserve(std::count(col.begin(), col.end(), '+') + 1);
			size_t start = 0;
			while (start < col.size()) {
				size_t pos = col.find('+', start);
				if (pos == std::string::npos) {
					columns.emplace_back(col.substr(start));
					break;
				}
				columns.emplace_back(col.substr(start, pos - start));
				start = pos + 1;
			}
			
			// Extract values
			std::vector<std::string> values = {};

			values.reserve(columns.size());
			bool allPresent = true;
			for (const auto& c : columns) {
				auto maybe = newEntity.extractField(c);
				if (!maybe) {
					allPresent = false;
					break;
				}
				values.emplace_back(*maybe);
			}
			
			if (!allPresent) continue; // Skip wenn nicht alle Felder vorhanden
			
			// Unique-Constraint prüfen für Composite Index
			// Use cache to avoid db.get per composite insert; fall back to DB on cache miss.
			bool compositeUnique = false;
			if (hasCachedMetadata) {
				auto it = compositeUniqueCache.find(col);
				compositeUnique = (it != compositeUniqueCache.end()) && it->second;
			} else {
				compositeUnique = isUniqueCompositeIndex_(table, columns);
			}
			if (compositeUnique) {
				// Prüfe ob bereits ein anderer PK mit dieser Wertekombination existiert
				std::string prefix = makeCompositeIndexPrefix(table, columns, values);
				bool conflict = false;
				db_.scanPrefix(prefix, [&pk, &conflict](std::string_view key, std::string_view /*val*/) {
					// Extrahiere PK aus key (letztes Segment nach ':')
					size_t lastColon = key.rfind(':');
					if (lastColon != std::string_view::npos) {
						std::string_view existingPK = key.substr(lastColon + 1);
						if (existingPK != pk) {
							conflict = true;
							return false; // Stop scan
						}
					}
					return true;
				});
				if (conflict) {
					std::string valueStr = {};
					for (size_t i = 0; i < values.size(); ++i) {
						if (i > 0) {
						  valueStr += ", ";
						}
						valueStr += columns[i] + "=" + values[i];
					}
					return Status::Error("Unique constraint violation: " + std::string(table) + ".{" + valueStr + "}");
				}
			}
			
			const std::string idxKey = makeCompositeIndexKey(table, columns, values, pk);
			batch.put(idxKey, pkBytes);
		}
	}

	// Zusätzlich: Range-Indizes pflegen, die keine Equality-Indizes haben
	for (const auto& rcol : rangeCols) {
		// Wenn diese Spalte bereits im obigen Loop gepflegt wurde (weil Equality-Index existiert), überspringen
		if (indexedCols.find(rcol) != indexedCols.end()) {
		  continue;
		}
		// Nur Single-Column Range-Indizes unterstützen (Composite-Range-Indizes sind nicht implementiert)
		auto maybe = newEntity.extractField(rcol);
		if (!maybe) {
		  continue;
		}
		const std::string rkey = makeRangeIndexKey(table, rcol, *maybe, pk);
		batch.put(rkey, pkBytes);
	}

	// Sparse-Indizes pflegen (v1.3.4: use cache)
	std::vector<std::string> sparseCols = {};

	if (hasCachedMetadata) {
		sparseCols = sparseColsCache;
	} else {
		auto tmp = loadSparseIndexedColumns_(table);
		sparseCols.assign(tmp.begin(), tmp.end());
	}
	
	for (const auto& scol : sparseCols) {
		auto maybe = newEntity.extractField(scol);
		if (!maybe || isNullOrEmpty_(*maybe)) continue; // Skip NULL/empty values
		
		const std::string encodedVal = encodeKeyComponent(*maybe);
		
		// Unique-Constraint prüfen für Sparse Index
		bool sparseUnique = false;
		if (hasCachedMetadata) {
			auto it = sparseUniqueCache.find(scol);
			sparseUnique = (it != sparseUniqueCache.end()) ? it->second : isSparseIndexUnique_(table, scol);
		} else {
			sparseUnique = isSparseIndexUnique_(table, scol);
		}
		if (sparseUnique) {
			std::string prefix = makeSparseIndexKey(table, scol, encodedVal, "");
			bool conflict = false;
			db_.scanPrefix(prefix, [&pk, &conflict](std::string_view key, std::string_view /*val*/) {
				size_t lastColon = key.rfind(':');
				if (lastColon != std::string_view::npos) {
					std::string_view existingPK = key.substr(lastColon + 1);
					if (existingPK != pk) {
						conflict = true;
						return false;
					}
				}
				return true;
			});
			if (conflict) {
				return Status::Error("Sparse unique constraint violation: " + std::string(table) + "." + scol + " = " + *maybe);
			}
		}
		
		const std::string sidxKey = makeSparseIndexKey(table, scol, encodedVal, pk);
		batch.put(sidxKey, pkBytes);
	}

	// Geo-Indizes pflegen (v1.3.4: use cache)
	std::vector<std::string> geoCols = {};

	if (hasCachedMetadata) {
		geoCols = geoColsCache;
	} else {
		auto tmp = loadGeoIndexedColumns_(table);
		geoCols.assign(tmp.begin(), tmp.end());
	}
	
	for (const auto& gcol : geoCols) {
		// Geo-Index erwartet zwei Felder: gcol_lat und gcol_lon (oder einfach lat/lon)
		// Konvention: Spaltenname ist z.B. "location", dann Felder "location_lat" und "location_lon"
		std::string latField = gcol + "_lat";
		std::string lonField = gcol + "_lon";
		
		auto maybeLat = newEntity.extractField(latField);
		auto maybeLon = newEntity.extractField(lonField);
		
		if (!maybeLat || !maybeLon) continue; // Skip wenn Koordinaten fehlen
		
		try {
			double lat = std::stod(*maybeLat);
			double lon = std::stod(*maybeLon);
			
			std::string geohash = encodeGeohash(lat, lon);
			const std::string gidxKey = makeGeoIndexKey(table, gcol, geohash, pk);
			batch.put(gidxKey, pkBytes);
		} catch (...) {
			THEMIS_WARN("updateIndexesForPut_: Ungültige Geo-Koordinaten für {}.{}: lat={}, lon={}", 
					   table, gcol, *maybeLat, *maybeLon);
			continue;
		}
	}

	// TTL-Indizes pflegen (use cache and reuse current timestamp)
	std::vector<std::string> ttlCols = {};

	if (hasCachedMetadata) {
		ttlCols = ttlColsCache;
	} else {
		auto tmp = loadTTLIndexedColumns_(table);
		ttlCols.assign(tmp.begin(), tmp.end());
	}
	// Calculate current timestamp once
	auto now = std::chrono::system_clock::now();
	auto epoch = now.time_since_epoch();
	int64_t currentTimestamp = std::chrono::duration_cast<std::chrono::seconds>(epoch).count();

	for (const auto& tcol : ttlCols) {
		auto maybeValue = newEntity.extractField(tcol);
		if (!maybeValue) {
		  continue;
		}
		// Use cached TTL seconds to avoid db.get on every insert (v1.3.5)
		int64_t ttlSeconds = 0;
		if (hasCachedMetadata) {
			auto it = ttlSecondsCache.find(tcol);
			if (it != ttlSecondsCache.end()) {
			  ttlSeconds = it->second;
			}
		} else {
			ttlSeconds = getTTLSeconds_(table, tcol);
		}
		if (ttlSeconds <= 0) {
		  continue;
		}
		
		int64_t expireTimestamp = currentTimestamp + ttlSeconds;
		const std::string ttlKey = makeTTLIndexKey(table, tcol, expireTimestamp, pk);
		batch.put(ttlKey, pkBytes);
	}

	// Fulltext-Indizes pflegen (use cache)
	std::vector<std::string> fulltextCols = {};

	if (hasCachedMetadata) {
		fulltextCols = fulltextColsCache;
	} else {
		auto tmp = loadFulltextIndexedColumns_(table);
		fulltextCols.assign(tmp.begin(), tmp.end());
	}
	for (const auto& fcol : fulltextCols) {
		auto maybeText = newEntity.extractField(fcol);
		if (!maybeText || isNullOrEmpty_(maybeText)) {
		  continue;
		}
		
		// Use cached fulltext config to avoid db.get + JSON parse on every insert (v1.3.5)
		FulltextConfig config = {};
		if (hasCachedMetadata) {
			auto it = fulltextConfigsCache.find(fcol);
			if (it != fulltextConfigsCache.end()) {
				const auto& c = it->second;
				config.stemming_enabled  = c.stemming_enabled;
				config.language          = c.language;
				config.stopwords_enabled = c.stopwords_enabled;
				config.stopwords         = c.stopwords;
				config.normalize_umlauts = c.normalize_umlauts;
			}
		} else {
			config = getFulltextConfig(table, fcol).value_or(FulltextConfig{});
		}
		auto tokens = tokenize(*maybeText, config);
		
		std::unordered_map<std::string, uint32_t> tf = {};

		tf.reserve(tokens.size());
		for (const auto& t : tokens) { if (!t.empty()) tf[t]++; }
		const std::string dkey = makeFulltextDocLenKey(table, fcol, pk);
		{
			std::string s = std::to_string(tokens.size());
			std::vector<uint8_t> val(s.begin(), s.end());
			batch.put(dkey, val);
		}
		for (const auto& [token, count] : tf) {
			const std::string ftKey = makeFulltextIndexKey(table, fcol, token, pk);
			batch.put(ftKey, pkBytes);
			const std::string tfKey = makeFulltextTFKey(table, fcol, token, pk);
			std::string s = std::to_string(count);
			std::vector<uint8_t> tfVal(s.begin(), s.end());
			batch.put(tfKey, tfVal);
		}
	}

	// Partial (filtered) indexes pflegen
	{
		std::unordered_map<std::string, std::string> partialCols = {};

		if (hasCachedMetadata) {
			for (const auto& col : partialColsOrderCache) {
				auto it = partialPredicatesCache.find(col);
				if (it != partialPredicatesCache.end()) {
					partialCols[col] = it->second;
				}
			}
		} else {
			partialCols = loadPartialIndexedColumns_(table);
		}
		for (const auto& [pcol, ppred] : partialCols) {
			// Only index if entity satisfies the predicate
			if (!evaluatePartialPredicate_(newEntity, ppred)) {
			  continue;
			}
			auto maybe = newEntity.extractField(pcol);
			if (!maybe) {
			  continue;
			}
			const std::string encodedVal = encodeKeyComponent(*maybe);

			// Unique-Constraint prüfen
			bool partialUnique = false;
			if (hasCachedMetadata) {
				auto it = partialUniqueCache.find(pcol);
				partialUnique = (it != partialUniqueCache.end()) ? it->second : isPartialIndexUnique_(table, pcol);
			} else {
				partialUnique = isPartialIndexUnique_(table, pcol);
			}
			if (partialUnique) {
				const std::string checkPrefix = makePartialIndexPrefix(table, pcol, encodedVal);
				bool conflict = false;
				db_.scanPrefix(checkPrefix, [&pk, &conflict](std::string_view key, std::string_view) {
					size_t lastColon = key.rfind(':');
					if (lastColon != std::string_view::npos && key.substr(lastColon + 1) != pk) {
						conflict = true;
						return false;
					}
					return true;
				});
				if (conflict)
					return Status::Error("Partial index unique constraint violation: " +
					                     std::string(table) + "." + pcol + " = " + *maybe);
			}

			const std::string pidxKey = makePartialIndexKey(table, pcol, encodedVal, pk);
			batch.put(pidxKey, pkBytes);
		}
	}

	return Status::OK();
}
SecondaryIndexManager::Status SecondaryIndexManager::updateIndexesForDelete_(std::string_view table,
																			 std::string_view pk,
																			 const BaseEntity* oldEntityOpt,
																			 RocksDBWrapper::WriteBatchWrapper& batch) {
	// Use metadata cache to avoid repeated DB meta-scans on every delete/upsert.
	auto& cache = SecondaryIndexMetadataCache::instance();
	auto cachedMetadata = cache.get(table);
	const bool hasCachedMetadata = cachedMetadata.has_value();

	std::unordered_set<std::string> indexedColsCache;
	std::unordered_set<std::string> rangeColsCache;
	std::unordered_set<std::string> sparseColsCache;
	std::unordered_set<std::string> geoColsCache;
	std::unordered_set<std::string> ttlColsCache;
	std::unordered_set<std::string> fulltextColsCache;
	std::unordered_map<std::string, std::string> partialColsCache;
	std::unordered_map<std::string, SecondaryIndexMetadataCache::CachedFulltextConfig> fulltextConfigsCache = {};

	if (hasCachedMetadata) {
		const auto metadata = *cachedMetadata;
		indexedColsCache = metadata.regular_indexes_set;
		rangeColsCache = metadata.range_indexes_set;
		sparseColsCache = {metadata.sparse_indexes.begin(), metadata.sparse_indexes.end()};
		geoColsCache = {metadata.geo_indexes.begin(), metadata.geo_indexes.end()};
		ttlColsCache = {metadata.ttl_indexes.begin(), metadata.ttl_indexes.end()};
		fulltextColsCache = {metadata.fulltext_indexes.begin(), metadata.fulltext_indexes.end()};
		for (const auto& col : metadata.partial_indexes) {
			auto it = metadata.partial_predicates.find(col);
			partialColsCache[col] = (it != metadata.partial_predicates.end()) ? it->second : "";
		}
		fulltextConfigsCache = metadata.fulltext_configs;
	}

	const auto indexedCols = hasCachedMetadata ? indexedColsCache : loadIndexedColumns_(table);
	const auto rangeCols = hasCachedMetadata ? rangeColsCache : loadRangeIndexedColumns_(table);
	const auto sparseCols = hasCachedMetadata ? sparseColsCache : loadSparseIndexedColumns_(table);
	const auto geoCols = hasCachedMetadata ? geoColsCache : loadGeoIndexedColumns_(table);
	const auto ttlCols = hasCachedMetadata ? ttlColsCache : loadTTLIndexedColumns_(table);
	const auto fulltextCols = hasCachedMetadata ? fulltextColsCache : loadFulltextIndexedColumns_(table);
	const auto partialCols = hasCachedMetadata ? partialColsCache : loadPartialIndexedColumns_(table);

	if (!oldEntityOpt) {
		// Falls keine alte Entity, können wir die spezifischen Index-Keys nicht sicher bestimmen.
		// Defensive strategy: alle Index-Prefixe für diesen PK löschen via Scan.
		for (const auto& col : indexedCols) {
			std::string prefix = {};
			if (col.find('+') == std::string::npos) {
				// Single
				prefix = std::string("idx:") + std::string(table) + ":" + col + ":";
			} else {
				// Composite
				prefix = std::string("idx:") + std::string(table) + ":" + col + ":";
			}
			// W5: Snapshot pk locally; eliminate [this] capture to avoid manager state closure
			const std::string_view pk_snapshot = pk;
			db_.scanPrefix(prefix, [&pk_snapshot, &batch](std::string_view key, std::string_view /*val*/){
				// Prüfen, ob PK am Ende passt (endet mit :PK)
				std::string_view keyView(key);
				size_t lastColon = keyView.rfind(':');
				if (lastColon != std::string_view::npos) {
					std::string_view extractedPK = keyView.substr(lastColon + 1);
					if (extractedPK == pk_snapshot) {
						batch.del(std::string(key));
					}
				}
				return true;
			});
		}
		// Auch alle Range-Index-Einträge mit diesem PK für diese Tabelle entfernen
		for (const auto& rcol : rangeCols) {
			std::string rprefix = std::string("ridx:") + std::string(table) + ":" + rcol + ":";
			db_.scanPrefix(rprefix, [&pk, &batch](std::string_view key, std::string_view /*val*/){
				size_t lastColon = key.rfind(':');
				if (lastColon != std::string_view::npos) {
					std::string_view existingPK = key.substr(lastColon + 1);
					if (existingPK == pk) {
						batch.del(std::string(key));
					}
				}
				return true;
			});
		}
		// Partial index entries löschen (via Scan, da Wert unbekannt)
		for (const auto& [pcol, ppred] : partialCols) {
			std::string pprefix = makePartialIndexPrefix(table, pcol);
			db_.scanPrefix(pprefix, [&pk, &batch](std::string_view key, std::string_view) {
				size_t lastColon = key.rfind(':');
				if (lastColon != std::string_view::npos && key.substr(lastColon + 1) == pk) {
					batch.del(std::string(key));
				}
				return true;
			});
		}
		return Status::OK();
	}

	// Zielgerichtet löschen basierend auf alten Feldwerten.
	// Metadata sets were materialized once above to avoid deferred state captures.

	for (const auto& col : indexedCols) {
		if (col.find('+') == std::string::npos) {
			// Single-Column
			auto maybe = oldEntityOpt->extractField(col);
			if (!maybe) {
			  continue;
			}
			const std::string encodedVal = encodeKeyComponent(*maybe);
			const std::string idxKey = KeySchema::makeSecondaryIndexKey(table, col, encodedVal, pk);
			batch.del(idxKey);
			// Auch Range-Index-Eintrag löschen, falls vorhanden
			if (rangeCols.find(col) != rangeCols.end()) {
				const std::string rkey = makeRangeIndexKey(table, col, *maybe, pk);
				batch.del(rkey);
			}
		} else {
			// Composite
			std::vector<std::string> columns;
			size_t start = 0;
			while (start < col.size()) {
				size_t pos = col.find('+', start);
				if (pos == std::string::npos) {
					columns.emplace_back(col.substr(start));
					break;
				}
				columns.emplace_back(col.substr(start, pos - start));
				start = pos + 1;
			}
			
			std::vector<std::string> values = {};

			values.reserve(columns.size());
			bool allPresent = true;
			for (const auto& c : columns) {
				auto maybe = oldEntityOpt->extractField(c);
				if (!maybe) {
					allPresent = false;
					break;
				}
				values.emplace_back(*maybe);
			}
			
			if (!allPresent) {
			  continue;
			}
			
			const std::string idxKey = makeCompositeIndexKey(table, columns, values, pk);
			batch.del(idxKey);
		}
	}

	// Zusätzlich: Range-Indizes löschen, die keine passenden Equality-Indizes haben
	{
		for (const auto& rcol : rangeCols) {
			if (indexedCols.find(rcol) != indexedCols.end()) continue; // bereits oben behandelt
			auto maybe = oldEntityOpt->extractField(rcol);
			if (!maybe) {
			  continue;
			}
			const std::string rkey = makeRangeIndexKey(table, rcol, *maybe, pk);
			batch.del(rkey);
		}
	}

	// Sparse-Indizes löschen
	{
		for (const auto& scol : sparseCols) {
			auto maybe = oldEntityOpt->extractField(scol);
			if (!maybe || isNullOrEmpty_(*maybe)) continue; // War nicht im Index
			const std::string encodedVal = encodeKeyComponent(*maybe);
			const std::string sidxKey = makeSparseIndexKey(table, scol, encodedVal, pk);
			batch.del(sidxKey);
		}
	}

	// Geo-Indizes löschen
	{
		for (const auto& gcol : geoCols) {
			std::string latField = gcol + "_lat";
			std::string lonField = gcol + "_lon";
			
			auto maybeLat = oldEntityOpt->extractField(latField);
			auto maybeLon = oldEntityOpt->extractField(lonField);
			
			if (!maybeLat || !maybeLon) {
			  continue;
			}
			
			try {
				double lat = std::stod(*maybeLat);
				double lon = std::stod(*maybeLon);
				
				std::string geohash = encodeGeohash(lat, lon);
				const std::string gidxKey = makeGeoIndexKey(table, gcol, geohash, pk);
				batch.del(gidxKey);
			} catch (...) {
				// Koordinaten waren ungültig, wahrscheinlich war kein Index-Eintrag vorhanden
				continue;
			}
		}
	}

	// TTL-Indizes löschen
	{
		for (const auto& tcol : ttlCols) {
			auto maybeValue = oldEntityOpt->extractField(tcol);
			if (!maybeValue) {
			  continue;
			}
			
			// We need to find the TTL index entry, but we don't know the exact timestamp
			// Scan the TTL index prefix and delete matching PKs
			std::string prefix = makeTTLIndexPrefix(table, tcol);
			db_.scanPrefix(prefix, [&pk, &batch](std::string_view key, std::string_view /*val*/) {
				// Extract PK from ttlidx:table:column:timestamp:PK
				size_t lastColon = key.rfind(':');
				if (lastColon != std::string_view::npos) {
					std::string_view extractedPK = key.substr(lastColon + 1);
					if (extractedPK == pk) {
						batch.del(std::string(key));
						return false; // Stop after finding the matching entry
					}
				}
				return true;
			});
		}
	}

	// Fulltext-Indizes löschen
	{
		for (const auto& fcol : fulltextCols) {
			auto maybeText = oldEntityOpt->extractField(fcol);
			if (!maybeText || isNullOrEmpty_(maybeText)) {
			  continue;
			}
			
			// Use cached fulltext config to avoid db.get + JSON parse on every upsert/delete (v1.3.5)
			FulltextConfig config = {};
			if (hasCachedMetadata) {
				auto it = fulltextConfigsCache.find(fcol);
				if (it != fulltextConfigsCache.end()) {
					const auto& c = it->second;
					config.stemming_enabled  = c.stemming_enabled;
					config.language          = c.language;
					config.stopwords_enabled = c.stopwords_enabled;
					config.stopwords         = c.stopwords;
					config.normalize_umlauts = c.normalize_umlauts;
				}
			} else {
				config = getFulltextConfig(table, fcol).value_or(FulltextConfig{});
			}
			auto tokens = tokenize(*maybeText, config);
			
			std::unordered_set<std::string> uniqueTokens(tokens.begin(), tokens.end());
			for (const auto& token : uniqueTokens) {
				if (token.empty()) {
				  continue;
				}
				const std::string ftKey = makeFulltextIndexKey(table, fcol, token, pk);
				batch.del(ftKey);
				const std::string tfKey = makeFulltextTFKey(table, fcol, token, pk);
				batch.del(tfKey);
			}
			// DocLength löschen
			const std::string dkey = makeFulltextDocLenKey(table, fcol, pk);
			batch.del(dkey);
		}
	}

	// Partial (filtered) indexes löschen
	{
		for (const auto& [pcol, ppred] : partialCols) {
			auto maybe = oldEntityOpt->extractField(pcol);
			if (!maybe) {
			  continue;
			}
			// Only the entry was added if the predicate matched at insert time.
			// We attempt to delete it regardless (idempotent).
			const std::string encodedVal = encodeKeyComponent(*maybe);
			const std::string pidxKey = makePartialIndexKey(table, pcol, encodedVal, pk);
			batch.del(pidxKey);
		}
	}

	return Status::OK();
}

std::pair<SecondaryIndexManager::Status, std::vector<std::string>>
SecondaryIndexManager::scanKeysEqual(std::string_view table,
									 std::string_view column,
									 std::string_view value) const {
	// Check for regular index or sparse index
	bool hasRegularIndex = hasIndex(table, column);
	bool hasSparse = hasSparseIndex(table, column);
	
	if (!hasRegularIndex && !hasSparse) {
		return {Status::Error("scanKeysEqual: kein Index für " + std::string(table) + "." + std::string(column)), std::vector<std::string>()};
	}

	const std::string encodedVal = encodeKeyComponent(value);
	std::vector<std::string> pks;
	
	// Scan regular index if exists
	if (hasRegularIndex) {
		const std::string prefix = KeySchema::makeSecondaryIndexKey(table, column, encodedVal, "");
		db_.scanPrefix(prefix, [&pks](std::string_view key, std::string_view /*val*/){
			pks.emplace_back(KeySchema::extractPrimaryKey(key));
			return true;
		});
	}
	
	// Scan sparse index if exists (and no regular index, or as fallback)
	if (hasSparse && !hasRegularIndex) {
		const std::string prefix = makeSparseIndexKey(table, column, encodedVal, "");
		db_.scanPrefix(prefix, [&pks](std::string_view key, std::string_view /*val*/){
			// Extract PK from sidx:table:column:value:PK
			size_t lastColon = key.rfind(':');
			if (lastColon != std::string_view::npos) {
				pks.emplace_back(key.substr(lastColon + 1));
			}
			return true;
		});
	}
	
	return {Status::OK(), std::move(pks)};
}

std::pair<SecondaryIndexManager::Status, std::vector<BaseEntity>>
SecondaryIndexManager::scanEntitiesEqual(std::string_view table,
										 std::string_view column,
										 std::string_view value) const {
	auto [st, keys] = scanKeysEqual(table, column, value);
	if (!st.ok) return {st, std::vector<BaseEntity>()};

	std::vector<BaseEntity> out = {};

	out.reserve(keys.size());
	for (const auto& pk : keys) {
		const std::string relKey = KeySchema::makeRelationalKey(table, pk);
		auto blob = db_.get(relKey);
		if (!blob) {
			THEMIS_WARN("scanEntitiesEqual: Entity nicht gefunden für PK={} (inkonsistenter Index?)", pk);
			continue;
		}
		try {
			out.emplace_back(BaseEntity::deserialize(pk, *blob));
		} catch (...) {
			THEMIS_ERROR("scanEntitiesEqual: Deserialisierung fehlgeschlagen für PK={}", pk);
		}
	}
	return {Status::OK(), std::move(out)};
}

size_t SecondaryIndexManager::estimateCountEqual(std::string_view table,
												 std::string_view column,
												 std::string_view value,
												 size_t maxProbe,
												 bool* capped) const {
	if (capped) {
	  *capped = false;
	}
	if (!hasIndex(table, column)) {
	  return 0;
	}
	const std::string encodedVal = encodeKeyComponent(value);
	const std::string prefix = KeySchema::makeSecondaryIndexKey(table, column, encodedVal, "");
	size_t count = 0;
	db_.scanPrefix(prefix, [&](std::string_view /*key*/, std::string_view /*val*/){
		++count;
		if (count >= maxProbe) {
			if (capped) {
			  *capped = true;
			}
			return false; // stop early
		}
		return true;
	});
	return count;
}

std::pair<SecondaryIndexManager::Status, std::vector<std::string>>
SecondaryIndexManager::scanKeysEqualComposite(std::string_view table,
											  const std::vector<std::string>& columns,
											  const std::vector<std::string>& values) const {
	if (columns.size() != values.size()) {
		return {Status::Error("scanKeysEqualComposite: Anzahl Spalten und Werte stimmt nicht überein"), std::vector<std::string>()};
	}
	if (!hasCompositeIndex(table, columns)) {
		std::string colList = {};
		for (size_t i = 0; i < columns.size(); ++i) {
			if (i > 0) {
			  colList += ", ";
			}
			colList += columns[i];
		}
		return {Status::Error("scanKeysEqualComposite: kein Composite Index für " + std::string(table) + ".{" + colList + "}"), std::vector<std::string>()};
	}
	
	const std::string prefix = makeCompositeIndexPrefix(table, columns, values);
	std::vector<std::string> pks;
	db_.scanPrefix(prefix, [&pks, &prefix](std::string_view key, std::string_view /*val*/){
		// key format: idx:table:col1+col2:val1:val2:PK
		// Der PK folgt nach dem letzten ':'
		std::string_view rest = key.substr(prefix.size());
		pks.emplace_back(rest);
		return true;
	});
	return {Status::OK(), std::move(pks)};
}

std::pair<SecondaryIndexManager::Status, std::vector<BaseEntity>>
SecondaryIndexManager::scanEntitiesEqualComposite(std::string_view table,
												  const std::vector<std::string>& columns,
												  const std::vector<std::string>& values) const {
	auto [st, keys] = scanKeysEqualComposite(table, columns, values);
	if (!st.ok) return {st, std::vector<BaseEntity>()};
	
	std::vector<BaseEntity> out = {};

	out.reserve(keys.size());
	for (const auto& pk : keys) {
		const std::string relKey = KeySchema::makeRelationalKey(table, pk);
		auto blob = db_.get(relKey);
		if (!blob) {
			THEMIS_WARN("scanEntitiesEqualComposite: Entity nicht gefunden für PK={} (inkonsistenter Index?)", pk);
			continue;
		}
		try {
			out.emplace_back(BaseEntity::deserialize(pk, *blob));
		} catch (...) {
			THEMIS_ERROR("scanEntitiesEqualComposite: Deserialisierung fehlgeschlagen für PK={}", pk);
		}
	}
	return {Status::OK(), std::move(out)};
}


size_t SecondaryIndexManager::estimateCountEqualComposite(std::string_view table,
											  const std::vector<std::string>& columns,
											  const std::vector<std::string>& values,
											  size_t maxProbe,
											  bool* capped) const {
	if (capped) {
	  *capped = false;
	}
	if (columns.size() != values.size()) {
	  return 0;
	}
	if (!hasCompositeIndex(table, columns)) {
	  return 0;
	}
	
	const std::string prefix = makeCompositeIndexPrefix(table, columns, values);
	size_t count = 0;
	db_.scanPrefix(prefix, [&](std::string_view /*key*/, std::string_view /*val*/){
		++count;
		if (count >= maxProbe) {
			if (capped) {
			  *capped = true;
			}
			return false;
		}
		return true;
	});
	return count;
}

std::pair<SecondaryIndexManager::Status, std::vector<std::string>> SecondaryIndexManager::scanKeysRange(
    std::string_view table,
    std::string_view column,
    const std::optional<std::string>& lower,
    const std::optional<std::string>& upper,
    bool includeLower,
    bool includeUpper,
    size_t limit,
    bool reversed
) const {
    if (table.empty() || column.empty()) return {Status::Error("scanKeysRange: table/column darf nicht leer sein"), std::vector<std::string>()};
    if (!hasRangeIndex(table, column)) return {Status::Error("scanKeysRange: kein Range-Index vorhanden für " + std::string(table) + "." + std::string(column)), std::vector<std::string>()};

    std::vector<std::string> result;
    std::string startKey, endKey;

    // RocksDB scanRange is [start, end) - exclusive on end
    if (lower.has_value()) {
        startKey = makeRangeIndexPrefix(table, column, *lower);
        // If lower is exclusive, seek past all keys with this value
        if (!includeLower) {
            startKey += '\xFF'; // Skip to next value
        }
    } else {
        startKey = std::string("ridx:") + std::string(table) + ":" + std::string(column) + ":";
    }

    if (upper.has_value()) {
        endKey = makeRangeIndexPrefix(table, column, *upper);
        // If upper is inclusive, extend range to include all keys with this value
        if (includeUpper) {
            endKey += '\xFF';
        }
        // else: scanRange is already exclusive on end, so we're good
    } else {
        endKey = std::string("ridx:") + std::string(table) + ":" + std::string(column) + ":\xFF";
    }

	uint64_t steps = 0;
	if (!reversed) {
		db_.scanRange(startKey, endKey, [&result, limit, &steps](std::string_view key, std::string_view /*value*/){
            if (result.size() >= limit) {
              return false;
            }
            size_t lastColon = key.rfind(':');
            if (lastColon != std::string_view::npos) {
				result.emplace_back(key.substr(lastColon+1));
            }
			++steps;
            return true;
        });
    } else {
        std::vector<std::string> tmp;
		db_.scanRange(startKey, endKey, [&tmp, &steps](std::string_view key, std::string_view /*value*/){
            size_t lastColon = key.rfind(':');
			if (lastColon != std::string_view::npos) {
			  tmp.emplace_back(key.substr(lastColon+1));
			}
			++steps;
            return true;
        });
        std::reverse(tmp.begin(), tmp.end());
        if (tmp.size() > limit) {
          tmp.resize(limit);
        }
        result = std::move(tmp);
    }

	// Update range scan steps metric
	query_metrics_.range_scan_steps_total.fetch_add(steps, std::memory_order_relaxed);

    return {Status::OK(), result};
}

	std::pair<SecondaryIndexManager::Status, std::vector<std::string>>
	SecondaryIndexManager::scanKeysRangeAnchored(
		std::string_view table,
		std::string_view column,
		const std::optional<std::string>& lowerValue,
		const std::optional<std::string>& upperValue,
		bool includeLowerValue,
		bool includeUpperValue,
		size_t limit,
		bool reversed,
		const std::optional<std::pair<std::string, std::string>>& anchor
	) const {
		// Fallback auf normalen Range-Scan, wenn kein Anchor übergeben
		if (!anchor.has_value()) {
			return scanKeysRange(table, column, lowerValue, upperValue, includeLowerValue, includeUpperValue, limit, reversed);
		}

		if (table.empty() || column.empty()) return {Status::Error("scanKeysRangeAnchored: table/column darf nicht leer sein"), std::vector<std::string>()};
		if (!hasRangeIndex(table, column)) return {Status::Error("scanKeysRangeAnchored: kein Range-Index vorhanden für " + std::string(table) + "." + std::string(column)), std::vector<std::string>()};

		const std::string& anchorValue = anchor->first;
		const std::string& anchorPk = anchor->second;

		// Count anchor usage
		query_metrics_.cursor_anchor_hits_total.fetch_add(1, std::memory_order_relaxed);

		std::vector<std::string> out;
		out.reserve(limit);

		// 1) Innerhalb des gleichen Wertes selektieren
		//    Für asc: PKs > anchorPk, für desc: PKs < anchorPk
		{
			std::string prefix = makeRangeIndexPrefix(table, column, anchorValue);
			std::vector<std::string> sameValuePks;
			sameValuePks.reserve(limit);
			db_.scanPrefix(prefix, [&sameValuePks](std::string_view key, std::string_view /*val*/){
				size_t lastColon = key.rfind(':');
				if (lastColon != std::string_view::npos) {
					sameValuePks.emplace_back(key.substr(lastColon+1));
				}
				return true;
			});

			// Add steps equal to scanned keys on the anchor value
			query_metrics_.range_scan_steps_total.fetch_add(static_cast<uint64_t>(sameValuePks.size()), std::memory_order_relaxed);

			if (!reversed) {
				// ascending: > anchorPk
				for (const auto& pk : sameValuePks) {
					if (pk > anchorPk) {
						out.emplace_back(pk);
						if (out.size() >= limit) return {Status::OK(), std::move(out)};
					}
				}
			} else {
				// descending: < anchorPk, in umgekehrter Ordnung
				// sameValuePks ist aktuell aufsteigend; wir iterieren rückwärts
				for (auto it = sameValuePks.rbegin(); it != sameValuePks.rend(); ++it) {
					if (*it < anchorPk) {
						out.emplace_back(*it);
						if (out.size() >= limit) return {Status::OK(), std::move(out)};
					}
				}
			}
		}

		// 2) Über die benachbarten Werte hinaus
		//    Für asc: Werte > anchorValue
		//    Für desc: Werte < anchorValue
		{
			std::optional<std::string> lb = lowerValue;
			std::optional<std::string> ub = upperValue;
			bool il = includeLowerValue;
			bool iu = includeUpperValue;

			// An Bounds anpassen, sodass wir nicht außerhalb des gewünschten Bereichs scannen
			if (!reversed) {
				// Start nach anchorValue
				// Falls upperBound < anchorValue, gibt es nichts mehr
				if (ub.has_value() && *ub <= anchorValue && !iu) {
					return {Status::OK(), std::move(out)};
				}
				// setze lb auf anchorValue und überspringe gleiche Werte
				lb = anchorValue;
				il = false; // exklusiv: Werte NACH anchorValue
			} else {
				// descending: Ende vor anchorValue
				if (lb.has_value() && *lb >= anchorValue && !il) {
					return {Status::OK(), std::move(out)};
				}
				// setze ub auf anchorValue und überspringe gleiche Werte
				ub = anchorValue;
				iu = false; // exklusiv: Werte VOR anchorValue
			}

			// Rest auffüllen
			auto [st2, more] = scanKeysRange(table, column, lb, ub, il, iu, limit - out.size(), reversed);
			if (!st2.ok) return {st2, std::vector<std::string>()};

			// Anhängen
			for (const auto& pk : more) {
				out.emplace_back(pk);
				if (out.size() >= limit) {
				  break;
				}
			}
		}

		return {Status::OK(), std::move(out)};
	}

// ────────────────────────────────────────────────────────────────────────────
// Geo-Index: Geohash-Encoding und Geo-Queries
// ────────────────────────────────────────────────────────────────────────────

std::string SecondaryIndexManager::encodeGeohash(double lat, double lon, int /*precision*/) {
	// Normalize to [0, 1]
	double lat_norm = (lat + 90.0) / 180.0;
	double lon_norm = (lon + 180.0) / 360.0;
	
	// Clamp
	lat_norm = std::max(0.0, std::min(1.0, lat_norm));
	lon_norm = std::max(0.0, std::min(1.0, lon_norm));
	
	// Convert to integer coordinates (use full 32 bits for better precision)
	uint64_t lat_bits = static_cast<uint64_t>(lat_norm * 0xFFFFFFFFULL);
	uint64_t lon_bits = static_cast<uint64_t>(lon_norm * 0xFFFFFFFFULL);
	
	// Interleave bits (Z-Order / Morton Code)
	uint64_t morton = 0;
	for (int i = 0; i < 32; ++i) {
		morton |= ((lat_bits & (1 << i)) << i) | ((lon_bits & (1 << i)) << (i + 1));
	}
	
	// Convert to hex string
	char buf[32];
	snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)morton);
	return std::string(buf);
}

std::pair<double, double> SecondaryIndexManager::decodeGeohash(std::string_view geohash) {
	uint64_t morton = std::strtoull(std::string(geohash).c_str(), nullptr, 16);
	
	uint64_t lat_bits = 0, lon_bits = 0;
	for (int i = 0; i < 32; ++i) {
		if (morton & (1 << (i * 2))) {
		  lat_bits |= (1 << i);
		}
		if (morton & (1 << (i * 2 + 1))) {
		  lon_bits |= (1 << i);
		}
	}
	
	double lat_norm = static_cast<double>(lat_bits) / 0xFFFFFFFFULL;
	double lon_norm = static_cast<double>(lon_bits) / 0xFFFFFFFFULL;
	
	double lat = lat_norm * 180.0 - 90.0;
	double lon = lon_norm * 360.0 - 180.0;
	
	return {lat, lon};
}

double SecondaryIndexManager::haversineDistance(double lat1, double lon1, double lat2, double lon2) {
	return themis::geo::haversine_km(lat1, lon1, lat2, lon2);
}

std::pair<SecondaryIndexManager::Status, std::vector<std::string>>
SecondaryIndexManager::scanGeoBox(
	std::string_view table,
	std::string_view column,
	double minLat, double maxLat,
	double minLon, double maxLon,
	size_t limit) const {
	
	if (!hasGeoIndex(table, column)) {
		return {Status::Error("scanGeoBox: Kein Geo-Index für " + std::string(table) + "." + std::string(column)), std::vector<std::string>()};
	}
	
	// Generate geohash range for bounding box
	std::string minHash = encodeGeohash(minLat, minLon);
	std::string maxHash = encodeGeohash(maxLat, maxLon);
	
	std::vector<std::string> results;
	const std::string prefix = std::string("gidx:") + std::string(table) + ":" + std::string(column) + ":";
	
	size_t count = 0;
	db_.scanPrefix(prefix, [&](std::string_view key, std::string_view /*value*/) {
		if (count >= limit) {
		  return false;
		}
		
		// Extract PK from key: gidx:table:column:geohash:PK
		size_t lastColon = key.rfind(':');
		if (lastColon == std::string_view::npos) {
		  return true;
		}
		
		size_t secondLastColon = key.rfind(':', lastColon - 1);
		if (secondLastColon == std::string_view::npos) {
		  return true;
		}
		
		std::string_view geohash = key.substr(secondLastColon + 1, lastColon - secondLastColon - 1);
		auto [lat, lon] = decodeGeohash(geohash);
		
		// Check if in bounding box
		if (lat >= minLat && lat <= maxLat && lon >= minLon && lon <= maxLon) {
			results.emplace_back(key.substr(lastColon + 1));
			count++;
		}
		return true;
	});
	
	return {Status::OK(), std::move(results)};
}

std::pair<SecondaryIndexManager::Status, std::vector<std::string>>
SecondaryIndexManager::scanGeoRadius(
	std::string_view table,
	std::string_view column,
	double centerLat, double centerLon,
	double radiusKm,
	size_t limit) const {
	
	if (!hasGeoIndex(table, column)) {
		return {Status::Error("scanGeoRadius: Kein Geo-Index für " + std::string(table) + "." + std::string(column)), std::vector<std::string>()};
	}
	
	// Approximate bounding box (1 degree ≈ 111 km at equator)
	double latDelta = radiusKm / 111.0;
	double lonDelta = radiusKm / (111.0 * std::cos(centerLat * 3.14159265358979323846 / 180.0));
	
	double minLat = centerLat - latDelta;
	double maxLat = centerLat + latDelta;
	double minLon = centerLon - lonDelta;
	double maxLon = centerLon + lonDelta;
	
	std::vector<std::string> results;
	const std::string prefix = std::string("gidx:") + std::string(table) + ":" + std::string(column) + ":";
	
	size_t count = 0;
	db_.scanPrefix(prefix, [&](std::string_view key, std::string_view /*value*/) {
		if (count >= limit) {
		  return false;
		}
		
		// Extract geohash and PK
		size_t lastColon = key.rfind(':');
		if (lastColon == std::string_view::npos) {
		  return true;
		}
		
		size_t secondLastColon = key.rfind(':', lastColon - 1);
		if (secondLastColon == std::string_view::npos) {
		  return true;
		}
		
		std::string_view geohash = key.substr(secondLastColon + 1, lastColon - secondLastColon - 1);
		auto [lat, lon] = decodeGeohash(geohash);
		
		// Check if in bounding box first (fast filter)
		if (lat < minLat || lat > maxLat || lon < minLon || lon > maxLon) {
		  return true;
		}
		
		// Precise distance check
		double dist = haversineDistance(centerLat, centerLon, lat, lon);
		if (dist <= radiusKm) {
			results.emplace_back(key.substr(lastColon + 1));
			count++;
		}
		return true;
	});
	
	return {Status::OK(), std::move(results)};
}

// ────────────────────────────────────────────────────────────────────────────
// TTL Cleanup
// ────────────────────────────────────────────────────────────────────────────

std::pair<SecondaryIndexManager::Status, size_t> 
SecondaryIndexManager::cleanupExpiredEntities(std::string_view table, std::string_view column) {
	if (!hasTTLIndex(table, column)) {
		return {Status::Error("cleanupExpiredEntities: Kein TTL-Index für " + std::string(table) + "." + std::string(column)), 0};
	}
	
	// Current timestamp
	auto now = std::chrono::system_clock::now();
	auto epoch = now.time_since_epoch();
	int64_t currentTimestamp = std::chrono::duration_cast<std::chrono::seconds>(epoch).count();
	
	// Scan TTL index for expired entries: ttlidx:table:column:0 bis ttlidx:table:column:{currentTimestamp}
	std::string prefix = makeTTLIndexPrefix(table, column);
	char maxBuf[32];
	snprintf(maxBuf, sizeof(maxBuf), "%020lld", (long long)currentTimestamp);
	std::string upperBound = prefix + maxBuf;
	
	std::vector<std::string> expiredPKs;
	std::vector<std::string> ttlKeys;
	
	db_.scanPrefix(prefix, [&](std::string_view key, std::string_view /*val*/) {
		// Stop if key > upperBound
		if (key > upperBound) {
		  return false;
		}
		
		// Extract PK from ttlidx:table:column:timestamp:PK
		size_t lastColon = key.rfind(':');
		if (lastColon != std::string_view::npos) {
			std::string pk(key.substr(lastColon + 1));
			expiredPKs.emplace_back(std::move(pk));
			ttlKeys.emplace_back(key);
		}
		return true;
	});
	
	// Delete expired entities
	size_t deletedCount = 0;
	for (size_t i = 0; i < expiredPKs.size(); ++i) {
		auto st = erase(table, expiredPKs[i]);
		if (st.ok) {
			deletedCount++;
			// Also remove TTL index entry
			db_.del(ttlKeys[i]);
		} else {
			THEMIS_WARN("cleanupExpiredEntities: Fehler beim Löschen von PK={}: {}", expiredPKs[i], st.message);
		}
	}
	
	if (deletedCount > 0) {
		THEMIS_INFO("TTL Cleanup: {} abgelaufene Entities gelöscht aus {}.{}", deletedCount, table, column);
	}
	
	return {Status::OK(), deletedCount};
}

// ────────────────────────────────────────────────────────────────────────────
// Fulltext Scan
// ────────────────────────────────────────────────────────────────────────────

// Internal helper: computes BM25 scores for fulltext queries
std::pair<SecondaryIndexManager::Status, std::vector<SecondaryIndexManager::FulltextResult>>
SecondaryIndexManager::computeBM25Scores_(
	std::string_view table,
	std::string_view column,
	std::string_view query,
	size_t limit
) const {
	if (!hasFulltextIndex(table, column)) {
		return {Status::Error("computeBM25Scores_: Kein Fulltext-Index für " + std::string(table) + "." + std::string(column)), std::vector<FulltextResult>()};
	}
	
	// Get index config and parse phrases; tokenize query without quoted phrases
	auto config = getFulltextConfig(table, column).value_or(FulltextConfig{});
	auto parsePhrases = [](std::string_view q) {
		std::vector<std::string> phrases = {};

		phrases.reserve(std::max<size_t>(1, q.size() / 16));
		std::string cleaned = {};
		cleaned.reserve(q.size());
		bool in_quotes = false;
		std::string current = {};
		current.reserve(std::min<size_t>(q.size(), 64));
		for (size_t i = 0; i < q.size(); ++i) {
			char c = q[i];
			if (c == '"') {
				if (in_quotes) {
					if (!current.empty()) { phrases.emplace_back(current); current.clear(); }
					in_quotes = false;
				} else {
					in_quotes = true;
				}
				continue;
			}
			if (in_quotes) {
			  current.push_back(c); else cleaned.push_back(c);
			}
		}
		return std::pair{phrases, cleaned};
	};
	auto [phrases, cleanedQuery] = parsePhrases(query);
	auto tokens = tokenize(cleanedQuery, config);
	std::vector<std::unordered_set<std::string>> tokenResults;
	tokenResults.reserve(tokens.size());
	if (tokens.empty() && !phrases.empty()) {
		// Fallback: use tokens from phrases to generate candidates
		std::string concat = {};
		concat.reserve(cleanedQuery.size() + query.size());
		for (size_t i = 0; i < phrases.size(); ++i) {
			if (i) {
			  concat.push_back(' ');
			}
			concat += phrases[i];
		}
		tokens = tokenize(concat, config);
		tokenResults.reserve(tokens.size());
	}
	
	if (tokens.empty()) {
		return {Status::OK(), std::vector<FulltextResult>()};
	}
	
	// For each token, get PKs from inverted index (doc frequency sets)
	for (const auto& token : tokens) {
		std::string prefix = makeFulltextIndexPrefix(table, column, token);
		std::unordered_set<std::string> pks;
		
		db_.scanPrefix(prefix, [&pks](std::string_view key, std::string_view /*val*/) {
			// Extract PK from ftidx:table:column:token:PK
			size_t lastColon = key.rfind(':');
			if (lastColon != std::string_view::npos) {
				pks.insert(std::string(key.substr(lastColon + 1)));
			}
			return true;
		});
		
		tokenResults.emplace_back(std::move(pks));
	}
	
	// Intersect all sets (AND logic)
	if (tokenResults.empty()) {
		return {Status::OK(), std::vector<FulltextResult>()};
	}

	// Intersect smallest sets first to reduce container scans on large candidate sets.
	std::sort(tokenResults.begin(), tokenResults.end(),
	          [](const auto& a, const auto& b) { return a.size() < b.size(); });

	std::unordered_set<std::string> intersectionSet = tokenResults.front();
	for (size_t i = 1; i < tokenResults.size(); ++i) {
		std::unordered_set<std::string> intersection = {};

		intersection.reserve(std::min(intersectionSet.size(), tokenResults[i].size()));
		for (const auto& pk : intersectionSet) {
			if (tokenResults[i].count(pk)) {
				intersection.insert(pk);
			}
		}
		intersectionSet = std::move(intersection);
		if (intersectionSet.empty()) {
			break;
		}
	}

	// Optional phrase verification on original field text (no positions stored)
	if (!phrases.empty()) {
		// Pre-normalize phrases once outside the per-candidate loop to avoid
		// redundant normalization on every outer iteration (O(n²) reduction).
		std::vector<std::string> normalizedPhrases = {};

		normalizedPhrases.reserve(phrases.size());
		for (auto ph : phrases) {
			if (config.normalize_umlauts) {
			  ph = utils::Normalizer::normalizeUmlauts(ph);
			}
			std::transform(ph.begin(), ph.end(), ph.begin(), [](unsigned char c){ return std::tolower(c); });
			normalizedPhrases.push_back(std::move(ph));
		}

		std::vector<std::string> toErase = {};

		toErase.reserve(intersectionSet.size());
		for (const auto& pk : intersectionSet) {
			auto pkey = KeySchema::makeRelationalKey(table, pk);
			auto blob = db_.get(pkey);
			bool keep = false;
			if (blob && !blob->empty()) {
				try {
					// Use BaseEntity deserialization to reliably access field values
					BaseEntity::Blob beBlob(blob->begin(), blob->end());
					BaseEntity entity = BaseEntity::deserialize(pk, beBlob);
					auto maybeVal = entity.extractField(column);
					if (maybeVal.has_value()) {
						std::string field = *maybeVal;
						if (config.normalize_umlauts) {
						  field = utils::Normalizer::normalizeUmlauts(field);
						}
						std::transform(field.begin(), field.end(), field.begin(), [](unsigned char c){ return std::tolower(c); });
						bool allFound = true;
						for (const auto& ph : normalizedPhrases) {
							if (field.find(ph) == std::string::npos) { allFound = false; break; }
						}
						keep = allFound;
					}
				} catch (...) {
					keep = false;
				}
			}
			if (!keep) {
			  toErase.emplace_back(pk);
			}
		}
		for (const auto& pk : toErase) {
		  intersectionSet.erase(pk);
		}
		if (intersectionSet.empty()) return {Status::OK(), std::vector<FulltextResult>()};
	}

	// BM25 Ranking über die Schnittmenge berechnen
	// Annahmen (v1 minimal):
	// - N ~ Anzahl Dokumente im Kandidaten-Universum (Vereinigung aller Token-Sets)
	// - avgdl ~ durchschnittliche DocLength über die Kandidaten (Vereinigung)
	std::unordered_set<std::string> universe;
	size_t universe_hint = 0;
	for (const auto& s : tokenResults) {
		universe_hint += s.size();
	}
	universe.reserve(universe_hint);
	for (const auto& s : tokenResults) {
		for (const auto& pk : s) {
		  universe.insert(pk);
		}
	}
	const double N = static_cast<double>(std::max<size_t>(1, universe.size()));

	// DocLength laden für Kandidaten (für avgdl)
	std::unordered_map<std::string, double> docLen = {};

	docLen.reserve(universe.size());
	double totalLen = 0.0;
	for (const auto& pk : universe) {
		const std::string dkey = makeFulltextDocLenKey(table, column, pk);
		auto v = db_.get(dkey);
		double dl = 0.0;
		if (v && !v->empty()) {
			std::string s(reinterpret_cast<const char*>(v->data()), v->size());
			try { dl = static_cast<double>(std::stoull(s)); } catch (...) { dl = 0.0; }
		}
		docLen.emplace(pk, dl);
		totalLen += dl;
	}
	const double avgdl = (universe.empty() ? 1.0 : std::max(1.0, totalLen / static_cast<double>(universe.size())));

	// Vorbereiten: df je Token
	std::vector<double> dfs = {};

	dfs.reserve(tokens.size());
	for (const auto& s : tokenResults) {
	  dfs.emplace_back(static_cast<double>(s.size()));
	}

	// BM25 Parameter
	const double k1 = 1.2;
	const double b = 0.75;

	// Score für jede PK in der Schnittmenge berechnen
	std::vector<FulltextResult> scored = {};

	scored.reserve(intersectionSet.size());
	for (const auto& pk : intersectionSet) {
		const auto itLen = docLen.find(pk);
		double dl = (itLen != docLen.end()) ? itLen->second : 0.0;
		double s = 0.0;
		for (size_t i = 0; i < tokens.size(); ++i) {
			const auto& token = tokens[i];
			const double df = std::max(1.0, dfs[i]);
			// IDF wie BM25 mit +1 Stabilisierung
			double idf = std::log((N - df + 0.5) / (df + 0.5) + 1.0);
			// tf laden
			const std::string tfKey = makeFulltextTFKey(table, column, token, pk);
			auto tfv = db_.get(tfKey);
			double tf = 0.0;
			if (tfv && !tfv->empty()) {
				std::string sTF(reinterpret_cast<const char*>(tfv->data()), tfv->size());
				try { tf = static_cast<double>(std::stoul(sTF)); } catch (...) { tf = 1.0; }
			} else {
				// Fallback: wenn kein TF gespeichert ist, minimal 1
				tf = 1.0;
			}
			double denom = tf + k1 * (1.0 - b + b * (dl / avgdl));
			if (denom <= 0.0) denom = tf + k1; // Guard
			double term = idf * ((tf * (k1 + 1.0)) / denom);
			s += term;
		}
		scored.emplace_back(FulltextResult{pk, s});
	}

	// Sortieren nach Score absteigend
	std::sort(scored.begin(), scored.end(), [](const FulltextResult& a, const FulltextResult& b){
		return a.score > b.score;
	});

	// Top-k Ergebnisse extrahieren
	std::vector<FulltextResult> finalResults = {};

	const size_t topk = std::min(scored.size(), limit);
	finalResults.reserve(topk);
	finalResults.insert(
		finalResults.end(),
		std::make_move_iterator(scored.begin()),
		std::make_move_iterator(scored.begin() + static_cast<std::ptrdiff_t>(topk))
	);

	return {Status::OK(), std::move(finalResults)};
}

// Public API: returns PKs only (deprecated, use scanFulltextWithScores for scores)
std::pair<SecondaryIndexManager::Status, std::vector<std::string>>
SecondaryIndexManager::scanFulltext(
	std::string_view table,
	std::string_view column,
	std::string_view query,
	size_t limit
) const {
	auto [status, results] = computeBM25Scores_(table, column, query, limit);
	if (!status.ok) {
		return {status, std::vector<std::string>()};
	}
	
	std::vector<std::string> pks = {};

	pks.reserve(results.size());
	for (const auto& result : results) {
		pks.emplace_back(result.pk);
	}
	
	return {Status::OK(), std::move(pks)};
}

// Public API: returns PKs with BM25 scores
std::pair<SecondaryIndexManager::Status, std::vector<SecondaryIndexManager::FulltextResult>>
SecondaryIndexManager::scanFulltextWithScores(
	std::string_view table,
	std::string_view column,
	std::string_view query,
	size_t limit
) const {
	return computeBM25Scores_(table, column, query, limit);
}

// Phrase search: exact phrase matching with position awareness
std::pair<SecondaryIndexManager::Status, std::vector<SecondaryIndexManager::FulltextResult>>
SecondaryIndexManager::scanFulltextPhrase(
	std::string_view table,
	std::string_view column,
	std::string_view phrase,
	size_t limit
) const {
	if (!hasFulltextIndex(table, column)) {
		return {Status::Error("scanFulltextPhrase: No fulltext index for " + std::string(table) + "." + std::string(column)), std::vector<FulltextResult>()};
	}
	
	if (phrase.empty()) {
		return {Status::OK(), std::vector<FulltextResult>()};
	}
	
	// Get index config and tokenize phrase
	auto config = getFulltextConfig(table, column).value_or(FulltextConfig{});
	auto tokens = tokenize(phrase, config);
	
	if (tokens.empty()) {
		return {Status::OK(), std::vector<FulltextResult>()};
	}
	
	// Get candidate documents that contain all tokens
	std::vector<std::unordered_set<std::string>> tokenResults;
	tokenResults.reserve(tokens.size());
	for (const auto& token : tokens) {
		std::string prefix = makeFulltextIndexPrefix(table, column, token);
		std::unordered_set<std::string> pks;
		
		db_.scanPrefix(prefix, [&pks](std::string_view key, std::string_view /*val*/) {
			size_t lastColon = key.rfind(':');
			if (lastColon != std::string_view::npos) {
				pks.insert(std::string(key.substr(lastColon + 1)));
			}
			return true;
		});
		
		tokenResults.emplace_back(std::move(pks));
	}
	
	// Intersect all sets (AND logic)
	if (tokenResults.empty()) {
		return {Status::OK(), std::vector<FulltextResult>()};
	}
	
	std::unordered_set<std::string> candidates = tokenResults[0];
	for (size_t i = 1; i < tokenResults.size(); ++i) {
		std::unordered_set<std::string> intersection = {};

		for (const auto& pk : candidates) {
			if (tokenResults[i].count(pk)) {
				intersection.insert(pk);
			}
		}
		candidates = std::move(intersection);
	}
	
	if (candidates.empty()) {
		return {Status::OK(), std::vector<FulltextResult>()};
	}
	
	// Verify exact phrase match by checking original document
	std::vector<FulltextResult> results;
	std::string phraseNorm = std::string(phrase);
	if (config.normalize_umlauts) {
		phraseNorm = utils::Normalizer::normalizeUmlauts(phraseNorm);
	}
	std::transform(phraseNorm.begin(), phraseNorm.end(), phraseNorm.begin(), 
		[](unsigned char c){ return std::tolower(c); });
	
	for (const auto& pk : candidates) {
		auto pkey = KeySchema::makeRelationalKey(table, pk);
		auto blob = db_.get(pkey);
		
		if (blob && !blob->empty()) {
			try {
				BaseEntity::Blob beBlob(blob->begin(), blob->end());
				BaseEntity entity = BaseEntity::deserialize(pk, beBlob);
				auto maybeVal = entity.extractField(column);
				
				if (maybeVal.has_value()) {
					std::string field = *maybeVal;
					if (config.normalize_umlauts) {
						field = utils::Normalizer::normalizeUmlauts(field);
					}
					std::transform(field.begin(), field.end(), field.begin(), 
						[](unsigned char c){ return std::tolower(c); });
					
					// Check if exact phrase exists in the field
					if (field.find(phraseNorm) != std::string::npos) {
						// Simple scoring: 1.0 for exact match, could be enhanced with position/proximity
						results.emplace_back(FulltextResult{pk, 1.0});
					}
				}
			} catch (...) {
				// Skip documents that fail to deserialize
			}
		}
		
		if (results.size() >= limit) {
			break;
		}
	}
	
	return {Status::OK(), std::move(results)};
}

// Helper function to calculate Levenshtein distance
namespace {
	int levenshteinDistance(const std::string& s1, const std::string& s2) {
		const size_t m = s1.size();
		const size_t n = s2.size();
		
		if (m == 0) {
		  return static_cast<int>(n);
		}
		if (n == 0) {
		  return static_cast<int>(m);
		}
		
		std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));
		
		for (size_t i = 0; i <= m; ++i) {
		  dp[i][0] = static_cast<int>(i);
		}
		for (size_t j = 0; j <= n; ++j) {
		  dp[0][j] = static_cast<int>(j);
		}
		
		for (size_t i = 1; i <= m; ++i) {
			for (size_t j = 1; j <= n; ++j) {
				int cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
				dp[i][j] = std::min({
					dp[i-1][j] + 1,      // deletion
					dp[i][j-1] + 1,      // insertion
					dp[i-1][j-1] + cost  // substitution
				});
			}
		}
		
		return dp[m][n];
	}
}

// Fuzzy search: Levenshtein distance-based similarity matching
std::pair<SecondaryIndexManager::Status, std::vector<SecondaryIndexManager::FulltextResult>>
SecondaryIndexManager::scanFulltextFuzzy(
	std::string_view table,
	std::string_view column,
	std::string_view query,
	int maxDistance,
	size_t limit
) const {
	if (!hasFulltextIndex(table, column)) {
		return {Status::Error("scanFulltextFuzzy: No fulltext index for " + std::string(table) + "." + std::string(column)), std::vector<FulltextResult>()};
	}
	
	if (query.empty() || maxDistance < 0) {
		return {Status::OK(), std::vector<FulltextResult>()};
	}
	
	// Get index config and tokenize query
	auto config = getFulltextConfig(table, column).value_or(FulltextConfig{});
	auto queryTokens = tokenize(query, config);
	
	if (queryTokens.empty()) {
		return {Status::OK(), std::vector<FulltextResult>()};
	}
	
	// For fuzzy search, we need to scan tokens in the index and find similar ones
	// Optimization: Scan index once and check all query tokens
	std::unordered_map<std::string, std::unordered_set<std::string>> tokenToDocs;
	std::unordered_map<std::string, double> pkScores;
	
	// Scan all fulltext index entries for this table/column once
	std::string prefix = "ftidx:" + std::string(table) + ":" + std::string(column) + ":";
	
	// Single scan: collect similar tokens and their documents
	db_.scanPrefix(prefix, [&](std::string_view key, std::string_view /*val*/) {
		// Extract token from ftidx:table:column:token:pk
		std::string keyStr(key);
		size_t thirdColon = keyStr.find(':', prefix.size());
		if (thirdColon != std::string::npos) {
			size_t fourthColon = keyStr.find(':', thirdColon + 1);
			if (fourthColon != std::string::npos) {
				std::string token = keyStr.substr(prefix.size(), thirdColon - prefix.size());
				std::string pk = keyStr.substr(fourthColon + 1);
				
				// Check token against all query tokens
				for (const auto& queryToken : queryTokens) {
					int distance = levenshteinDistance(queryToken, token);
					if (distance <= maxDistance) {
						tokenToDocs[token].insert(pk);
						
						// Update score: better distance = higher score
						double score = 1.0 / (1.0 + distance);
						auto it = pkScores.find(pk);
						if (it == pkScores.end()) {
							pkScores[pk] = score;
						} else {
							it->second = std::max(it->second, score);
						}
					}
				}
			}
		}
		return true;
	});
	
	// Convert to results with scores
	std::vector<FulltextResult> results = {};

	results.reserve(pkScores.size());
	
	for (const auto& [pk, score] : pkScores) {
		results.emplace_back(FulltextResult{pk, score});
	}
	
	// Sort by score descending
	std::sort(results.begin(), results.end(), [](const FulltextResult& a, const FulltextResult& b) {
		return a.score > b.score;
	});
	
	// Return top-k results
	if (results.size() > limit) {
		results.resize(limit);
	}
	
	return {Status::OK(), std::move(results)};
}

bool SecondaryIndexManager::isNullOrEmpty_(const std::optional<std::string>& value) {
	return !value.has_value() || value->empty() || *value == "null";
}

// Tokenizer: Whitespace-based, converts to lowercase
std::vector<std::string> SecondaryIndexManager::tokenize(std::string_view text) {
	std::vector<std::string> tokens = {};

	tokens.reserve(std::max<size_t>(1, text.size() / 5));
	std::string current = {};
	current.reserve(std::min<size_t>(text.size(), 32));
	
	for (char c : text) {
		if (std::isspace(static_cast<unsigned char>(c)) || std::ispunct(static_cast<unsigned char>(c))) {
			if (!current.empty()) {
				// Convert to lowercase
				std::transform(current.begin(), current.end(), current.begin(),
					[](unsigned char c) { return std::tolower(c); });
				tokens.emplace_back(std::move(current));
			}
		} else {
			current += c;
		}
	}
	
	// Don't forget last token
	if (!current.empty()) {
		std::transform(current.begin(), current.end(), current.begin(),
			[](unsigned char c) { return std::tolower(c); });
		tokens.emplace_back(std::move(current));
	}
	
	return tokens;
}

// Tokenizer with Stemming support
std::vector<std::string> SecondaryIndexManager::tokenize(std::string_view text, const FulltextConfig& config) {
	// Optional: normalize umlauts/ß for German-like content before tokenization
	std::string normalized = {};
	if (config.normalize_umlauts) {
		normalized = utils::Normalizer::normalizeUmlauts(text);
	}
	// First tokenize normally (lowercase + whitespace split)
	std::vector<std::string> tokens = tokenize(normalized.empty() ? text : std::string_view(normalized));
	
	// Remove stopwords if enabled
	if (config.stopwords_enabled) {
		auto base = utils::Stopwords::defaults(config.language);
		auto sw = utils::Stopwords::merge(base, config.stopwords);
		tokens.erase(std::remove_if(tokens.begin(), tokens.end(), [&]([[maybe_unused]] const std::string& t){
			return sw.find(t) != sw.end();
		}), tokens.end());
	}

	// Apply stemming if enabled
	if (config.stemming_enabled) {
		auto lang = utils::Stemmer::parseLanguage(config.language);
		for (auto& token : tokens) {
			token = utils::Stemmer::stem(token, lang);
		}
	}
	
	return tokens;
}

// =============================================================================
// Index Statistics & Maintenance
// =============================================================================

std::vector<SecondaryIndexManager::IndexStats> SecondaryIndexManager::getAllIndexStats(const std::string& table) {
	std::vector<SecondaryIndexManager::IndexStats> allStats;
	std::unordered_set<std::string> processedColumns;
	
	// Scan all meta-key prefixes and collect unique table:column combinations
	auto scanMetaPrefix = [&]([[maybe_unused]] const std::string& metaPrefix) {
		std::string prefix = metaPrefix + table + ":";
		db_.scanPrefix(prefix, [&](std::string_view key, std::string_view /*val*/) {
			// Extract column from key (format: "prefix:table:column")
			std::string keyStr(key);
			size_t firstColon = keyStr.find(':');
			if (firstColon != std::string::npos) {
				size_t secondColon = keyStr.find(':', firstColon + 1);
				if (secondColon != std::string::npos) {
					std::string column = keyStr.substr(secondColon + 1);
					
					// Remove any trailing parts (e.g., composite column suffixes)
					size_t thirdColon = column.find(':');
					if (thirdColon != std::string::npos) {
						column.resize(thirdColon);
					}
					
					processedColumns.insert(column);
				}
			}
			return true;
		});
	};
	
	// Scan all index meta-key types
	scanMetaPrefix("idxmeta:");
	scanMetaPrefix("ridxmeta:");
	scanMetaPrefix("sidxmeta:");
	scanMetaPrefix("gidxmeta:");
	scanMetaPrefix("ttlidxmeta:");
	scanMetaPrefix("ftidxmeta:");
	scanMetaPrefix("cidxmeta:");
	scanMetaPrefix("pidxmeta:");
	
	// Get stats for each unique column
	allStats.reserve(processedColumns.size());
	for (const auto& column : processedColumns) {
		allStats.emplace_back(getIndexStats(table, column));
	}
	
	return allStats;
}

void SecondaryIndexManager::rebuildIndex(const std::string& table, const std::string& column) {
    // Delegiert auf Overload mit optionalem Progress-Callback
    rebuildIndex(table, column, nullptr);
}

void SecondaryIndexManager::rebuildIndex(const std::string& table, const std::string& column,
										 std::function<bool(size_t,size_t)> progress) {
	// Start metrics tracking
	auto start_time = std::chrono::steady_clock::now();
	
	// Helper to write entries
	auto writeIndexEntry = [&](const std::string& key, const std::string& pk) {
		std::vector<uint8_t> pkBytes(pk.begin(), pk.end());
		db_.put(key, pkBytes);
	};

	// Determine index type
	std::string indexType = {};
	std::string indexPrefix = {};

	if (db_.get(makeTTLIndexMetaKey(table, column)).has_value()) {
		indexType = "ttl";
		indexPrefix = std::string("ttlidx:") + table + ":" + column + ":";
	} else if (db_.get(makeFulltextIndexMetaKey(table, column)).has_value()) {
		indexType = "fulltext";
		indexPrefix = std::string("ftidx:") + table + ":" + column + ":";
	} else if (db_.get(makeGeoIndexMetaKey(table, column)).has_value()) {
		indexType = "geo";
		indexPrefix = std::string("gidx:") + table + ":" + column + ":";
	} else if (db_.get(makeSparseIndexMetaKey(table, column)).has_value()) {
		indexType = "sparse";
		indexPrefix = std::string("sidx:") + table + ":" + column + ":";
	} else if (db_.get(makeRangeIndexMetaKey(table, column)).has_value()) {
		indexType = "range";
		indexPrefix = std::string("ridx:") + table + ":" + column + ":";
	} else if (column.find('+') != std::string::npos) {
		// Composite index
		std::vector<std::string> cols;
		size_t pos = 0;
		while (pos < column.size()) {
			size_t p = column.find('+', pos);
			if (p == std::string::npos) {
			  p = column.size();
			}
			cols.emplace_back(column.substr(pos, p - pos));
			pos = p + 1;
		}
		if (db_.get(makeCompositeIndexMetaKey(table, cols)).has_value()) {
			indexType = "composite";
			indexPrefix = std::string("idx:") + table + ":" + column + ":";
		} else {
			return; // No index
		}
	} else if (db_.get(makeIndexMetaKey(table, column)).has_value()) {
		indexType = "regular";
		indexPrefix = std::string("idx:") + table + ":" + column + ":";
	} else if (db_.get(makePartialIndexMetaKey(table, column)).has_value()) {
		indexType = "partial";
		indexPrefix = std::string("pidx:") + table + ":" + column + ":";
	} else {
		return; // No index found
	}

	// Step 2: Delete all existing index entries
	std::vector<std::string> keysToDelete;
	db_.scanPrefix(indexPrefix, [&keysToDelete](std::string_view key, std::string_view) {
		keysToDelete.emplace_back(key);
		return true;
	});

	for (const auto& key : keysToDelete) {
		db_.del(key);
	}

	// Step 3.0: Total entities under <table>:
	const std::string entityPrefix = KeySchema::makeRelationalKey(table, "");
	size_t total = 0;
	db_.scanPrefix(entityPrefix, [&total](std::string_view /*k*/, std::string_view /*v*/) {
		++total;
		return true;
	});
	size_t done = 0;
	auto advance = [&]() -> bool {
		++done;
		if (progress) {
		  return progress(done, total);
		}
		return true;
	};

	// Step 3: Scan entities and rebuild
	if (indexType == "ttl") {
		int64_t ttl_sec = getTTLSeconds_(table, column);
		auto now = std::chrono::system_clock::now();
		auto now_ts = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

		bool aborted = false;
		db_.scanPrefix(entityPrefix, [&](std::string_view key, std::string_view val) {
			size_t lastColon = key.rfind(':');
			if (lastColon == std::string::npos) {
			  return true;
			}
			std::string pk(key.substr(lastColon + 1));

			BaseEntity::Blob blob(val.begin(), val.end());
			BaseEntity entity = BaseEntity::deserialize(pk, blob);
			auto maybeVal = entity.extractField(column);
			if (!maybeVal || isNullOrEmpty_(maybeVal)) { if (!advance()) { aborted = true; return false; } return true; }

			int64_t expire_ts = now_ts + ttl_sec;
			std::string ttlKey = makeTTLIndexKey(table, column, expire_ts, pk);
			writeIndexEntry(ttlKey, pk);
			if (!advance()) { aborted = true; return false; }
			return true;
		});
		if (aborted) {
		  return;
		}
	} else if (indexType == "fulltext") {
		// Get index config for consistent tokenization
		auto config = getFulltextConfig(table, column).value_or(FulltextConfig{});
		
		bool aborted = false;
		db_.scanPrefix(entityPrefix, [&](std::string_view key, std::string_view val) {
			size_t lastColon = key.rfind(':');
			if (lastColon == std::string::npos) {
			  return true;
			}
			std::string pk(key.substr(lastColon + 1));

			BaseEntity::Blob blob(val.begin(), val.end());
			BaseEntity entity = BaseEntity::deserialize(pk, blob);
			auto maybeVal = entity.extractField(column);
			if (!maybeVal) { if (!advance()) { aborted = true; return false; } return true; }

			auto tokens = tokenize(*maybeVal, config);
			for (const auto& token : tokens) {
				std::string ftKey = makeFulltextIndexKey(table, column, token, pk);
				writeIndexEntry(ftKey, pk);
			}
			if (!advance()) { aborted = true; return false; }
			return true;
		});
		if (aborted) {
		  return;
		}
	} else if (indexType == "geo") {
		bool aborted = false;
		db_.scanPrefix(entityPrefix, [&](std::string_view key, std::string_view val) {
			size_t lastColon = key.rfind(':');
			if (lastColon == std::string::npos) {
			  return true;
			}
			std::string pk(key.substr(lastColon + 1));

			BaseEntity::Blob blob(val.begin(), val.end());
			BaseEntity entity = BaseEntity::deserialize(pk, blob);

			std::string latField = column + "_lat";
			std::string lonField = column + "_lon";
			auto maybeLat = entity.extractField(latField);
			auto maybeLon = entity.extractField(lonField);
			if (!maybeLat || !maybeLon) { if (!advance()) { aborted = true; return false; } return true; }

			try {
				double lat = std::stod(*maybeLat);
				double lon = std::stod(*maybeLon);
				std::string geohash = encodeGeohash(lat, lon, 12);
				std::string gkey = makeGeoIndexKey(table, column, geohash, pk);
				writeIndexEntry(gkey, pk);
			} catch (...) {
				// skip invalid
			}
			if (!advance()) { aborted = true; return false; }
			return true;
		});
		if (aborted) {
		  return;
		}
	} else if (indexType == "sparse") {
		bool aborted = false;
		db_.scanPrefix(entityPrefix, [&](std::string_view key, std::string_view val) {
			size_t lastColon = key.rfind(':');
			if (lastColon == std::string::npos) {
			  return true;
			}
			std::string pk(key.substr(lastColon + 1));

			BaseEntity::Blob blob(val.begin(), val.end());
			BaseEntity entity = BaseEntity::deserialize(pk, blob);
			auto maybeVal = entity.extractField(column);
			if (!maybeVal || isNullOrEmpty_(maybeVal)) { if (!advance()) { aborted = true; return false; } return true; }

			std::string encoded = encodeKeyComponent(*maybeVal);
			std::string skey = makeSparseIndexKey(table, column, encoded, pk);
			writeIndexEntry(skey, pk);
			if (!advance()) { aborted = true; return false; }
			return true;
		});
		if (aborted) {
		  return;
		}
	} else if (indexType == "range") {
		bool aborted = false;
		db_.scanPrefix(entityPrefix, [&](std::string_view key, std::string_view val) {
			size_t lastColon = key.rfind(':');
			if (lastColon == std::string::npos) {
			  return true;
			}
			std::string pk(key.substr(lastColon + 1));

			BaseEntity::Blob blob(val.begin(), val.end());
			BaseEntity entity = BaseEntity::deserialize(pk, blob);
			auto maybeVal = entity.extractField(column);
			if (!maybeVal) { if (!advance()) { aborted = true; return false; } return true; }

			std::string rkey = makeRangeIndexKey(table, column, *maybeVal, pk);
			writeIndexEntry(rkey, pk);
			if (!advance()) { aborted = true; return false; }
			return true;
		});
		if (aborted) {
		  return;
		}
	} else if (indexType == "composite") {
		// Parse columns
		std::vector<std::string> columns;
		size_t pos = 0;
		while (pos < column.size()) {
			size_t p = column.find('+', pos);
			if (p == std::string::npos) {
			  p = column.size();
			}
			columns.emplace_back(column.substr(pos, p - pos));
			pos = p + 1;
		}

		bool aborted = false;
		db_.scanPrefix(entityPrefix, [&](std::string_view key, std::string_view val) {
			size_t lastColon = key.rfind(':');
			if (lastColon == std::string::npos) {
			  return true;
			}
			std::string pk(key.substr(lastColon + 1));

			BaseEntity::Blob blob(val.begin(), val.end());
			BaseEntity entity = BaseEntity::deserialize(pk, blob);

			std::vector<std::string> values = {};

			values.reserve(columns.size());
			for (const auto& col : columns) {
				auto maybeVal = entity.extractField(col);
				if (!maybeVal) { if (!advance()) { aborted = true; return false; } return true; }
				values.emplace_back(*maybeVal);
			}

			std::string ckey = makeCompositeIndexKey(table, columns, values, pk);
			writeIndexEntry(ckey, pk);
			if (!advance()) { aborted = true; return false; }
			return true;
		});
		if (aborted) {
		  return;
		}
	} else if (indexType == "regular") {
		bool aborted = false;
		db_.scanPrefix(entityPrefix, [&](std::string_view key, std::string_view val) {
			size_t lastColon = key.rfind(':');
			if (lastColon == std::string::npos) {
			  return true;
			}
			std::string pk(key.substr(lastColon + 1));

			BaseEntity::Blob blob(val.begin(), val.end());
			BaseEntity entity = BaseEntity::deserialize(pk, blob);
			auto maybeVal = entity.extractField(column);
			if (!maybeVal) { if (!advance()) { aborted = true; return false; } return true; }

			std::string encoded = encodeKeyComponent(*maybeVal);
			std::string idxKey = KeySchema::makeSecondaryIndexKey(table, column, encoded, pk);
			writeIndexEntry(idxKey, pk);
			if (!advance()) { aborted = true; return false; }
			return true;
		});
		if (aborted) {
		  return;
		}
	} else if (indexType == "partial") {
		// Load predicate for this partial index
		auto predOpt = getPartialIndexPredicate(table, column);
		if (!predOpt) return; // shouldn't happen
		const std::string& predicate = *predOpt;

		bool aborted = false;
		db_.scanPrefix(entityPrefix, [&](std::string_view key, std::string_view val) {
			size_t lastColon = key.rfind(':');
			if (lastColon == std::string::npos) {
			  return true;
			}
			std::string pk(key.substr(lastColon + 1));

			BaseEntity::Blob blob(val.begin(), val.end());
			BaseEntity entity = BaseEntity::deserialize(pk, blob);

			// Only index entities that satisfy the predicate
			if (!evaluatePartialPredicate_(entity, predicate)) {
				if (!advance()) { aborted = true; return false; }
				return true;
			}

			auto maybeVal = entity.extractField(column);
			if (!maybeVal) { if (!advance()) { aborted = true; return false; } return true; }

			std::string encoded = encodeKeyComponent(*maybeVal);
			std::string pidxKey = makePartialIndexKey(table, column, encoded, pk);
			writeIndexEntry(pidxKey, pk);
			if (!advance()) { aborted = true; return false; }
			return true;
		});
		if (aborted) {
		  return;
		}
	}
	
	// Update metrics at the end
	auto end_time = std::chrono::steady_clock::now();
	auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
	
	rebuild_metrics_.rebuild_count.fetch_add(1, std::memory_order_relaxed);
	rebuild_metrics_.rebuild_duration_ms.fetch_add(duration_ms, std::memory_order_relaxed);
	rebuild_metrics_.rebuild_entities_processed.fetch_add(done, std::memory_order_relaxed);
}

// Online rebuild: keeps the live index available for reads throughout the scan phase.
// New entries are written under a shadow prefix ("__rb__:<livePrefix>"). After the scan
// completes, a single atomic WriteBatch deletes the stale live entries and promotes the
// shadow entries to the live prefix, minimising the window where reads are affected.
void SecondaryIndexManager::rebuildIndexOnline(const std::string& table, const std::string& column,
                                               uint32_t throttle_us,
                                               std::function<bool(size_t,size_t)> progress) {
	auto start_time = std::chrono::steady_clock::now();

	// Step 1: Determine index type and live prefix (mirrors rebuildIndex logic)
	std::string indexType = {};
	std::string livePrefix = {};

	if (db_.get(makeTTLIndexMetaKey(table, column)).has_value()) {
		indexType = "ttl";
		livePrefix = std::string("ttlidx:") + table + ":" + column + ":";
	} else if (db_.get(makeFulltextIndexMetaKey(table, column)).has_value()) {
		indexType = "fulltext";
		livePrefix = std::string("ftidx:") + table + ":" + column + ":";
	} else if (db_.get(makeGeoIndexMetaKey(table, column)).has_value()) {
		indexType = "geo";
		livePrefix = std::string("gidx:") + table + ":" + column + ":";
	} else if (db_.get(makeSparseIndexMetaKey(table, column)).has_value()) {
		indexType = "sparse";
		livePrefix = std::string("sidx:") + table + ":" + column + ":";
	} else if (db_.get(makeRangeIndexMetaKey(table, column)).has_value()) {
		indexType = "range";
		livePrefix = std::string("ridx:") + table + ":" + column + ":";
	} else if (column.find('+') != std::string::npos) {
		std::vector<std::string> cols;
		size_t pos = 0;
		while (pos < column.size()) {
			size_t p = column.find('+', pos);
			if (p == std::string::npos) {
			  p = column.size();
			}
			cols.emplace_back(column.substr(pos, p - pos));
			pos = p + 1;
		}
		if (db_.get(makeCompositeIndexMetaKey(table, cols)).has_value()) {
			indexType = "composite";
			livePrefix = std::string("idx:") + table + ":" + column + ":";
		} else {
			return;
		}
	} else if (db_.get(makeIndexMetaKey(table, column)).has_value()) {
		indexType = "regular";
		livePrefix = std::string("idx:") + table + ":" + column + ":";
	} else {
		return;
	}

	// Step 2: Shadow prefix – all new entries are written here during the scan
	const std::string shadowPrefix = "__rb__:" + livePrefix;

	// Step 3: Remove any stale shadow entries from a previously interrupted online rebuild
	{
		std::vector<std::string> stale;
		db_.scanPrefix(shadowPrefix, [&stale](std::string_view k, std::string_view) {
			stale.emplace_back(k);
			return true;
		});
		for (const auto& k : stale) {
		  db_.del(k);
		}
	}

	// Step 4: Count entities for progress reporting
	const std::string entityPrefix = KeySchema::makeRelationalKey(table, "");
	size_t total = 0;
	db_.scanPrefix(entityPrefix, [&total](std::string_view, std::string_view) { ++total; return true; });

	size_t done = 0;
	auto advance = [&]() -> bool {
		++done;
		if (throttle_us > 0 && done % 100 == 0)
			std::this_thread::sleep_for(std::chrono::microseconds(throttle_us));
		if (progress) {
		  return progress(done, total);
		}
		return true;
	};

	// Writes a new entry to the shadow prefix
	auto writeShadow = [&](const std::string& liveKey, const std::string& pk) {
		std::string shadowKey = "__rb__:" + liveKey;
		db_.put(shadowKey, toBytes(pk));
	};

	// Step 5: Scan all entities and write new entries to the shadow prefix.
	//         The live index is NOT touched – reads continue to work normally.
	if (indexType == "ttl") {
		int64_t ttl_sec = getTTLSeconds_(table, column);
		auto now_ts = std::chrono::duration_cast<std::chrono::seconds>(
		    std::chrono::system_clock::now().time_since_epoch()).count();
		bool aborted = false;
		db_.scanPrefix(entityPrefix, [&](std::string_view key, std::string_view val) {
			size_t lc = key.rfind(':');
			if (lc == std::string::npos) {
			  return true;
			}
			std::string pk(key.substr(lc + 1));
			BaseEntity entity = BaseEntity::deserialize(pk, BaseEntity::Blob(val.begin(), val.end()));
			auto maybeVal = entity.extractField(column);
			if (!maybeVal || isNullOrEmpty_(maybeVal)) { if (!advance()) { aborted = true; return false; } return true; }
			int64_t expire_ts = now_ts + ttl_sec;
			writeShadow(makeTTLIndexKey(table, column, expire_ts, pk), pk);
			if (!advance()) { aborted = true; return false; }
			return true;
		});
		if (aborted) {
		  return;
		}
	} else if (indexType == "fulltext") {
		auto config = getFulltextConfig(table, column).value_or(FulltextConfig{});
		bool aborted = false;
		db_.scanPrefix(entityPrefix, [&](std::string_view key, std::string_view val) {
			size_t lc = key.rfind(':');
			if (lc == std::string::npos) {
			  return true;
			}
			std::string pk(key.substr(lc + 1));
			BaseEntity entity = BaseEntity::deserialize(pk, BaseEntity::Blob(val.begin(), val.end()));
			auto maybeVal = entity.extractField(column);
			if (!maybeVal) { if (!advance()) { aborted = true; return false; } return true; }
			for (const auto& token : tokenize(*maybeVal, config))
				writeShadow(makeFulltextIndexKey(table, column, token, pk), pk);
			if (!advance()) { aborted = true; return false; }
			return true;
		});
		if (aborted) {
		  return;
		}
	} else if (indexType == "geo") {
		bool aborted = false;
		db_.scanPrefix(entityPrefix, [&](std::string_view key, std::string_view val) {
			size_t lc = key.rfind(':');
			if (lc == std::string::npos) {
			  return true;
			}
			std::string pk(key.substr(lc + 1));
			BaseEntity entity = BaseEntity::deserialize(pk, BaseEntity::Blob(val.begin(), val.end()));
			auto maybeLat = entity.extractField(column + "_lat");
			auto maybeLon = entity.extractField(column + "_lon");
			if (!maybeLat || !maybeLon) { if (!advance()) { aborted = true; return false; } return true; }
			try {
				std::string geohash = encodeGeohash(std::stod(*maybeLat), std::stod(*maybeLon), 12);
				writeShadow(makeGeoIndexKey(table, column, geohash, pk), pk);
			} catch (...) {}
			if (!advance()) { aborted = true; return false; }
			return true;
		});
		if (aborted) {
		  return;
		}
	} else if (indexType == "sparse") {
		bool aborted = false;
		db_.scanPrefix(entityPrefix, [&](std::string_view key, std::string_view val) {
			size_t lc = key.rfind(':');
			if (lc == std::string::npos) {
			  return true;
			}
			std::string pk(key.substr(lc + 1));
			BaseEntity entity = BaseEntity::deserialize(pk, BaseEntity::Blob(val.begin(), val.end()));
			auto maybeVal = entity.extractField(column);
			if (!maybeVal || isNullOrEmpty_(maybeVal)) { if (!advance()) { aborted = true; return false; } return true; }
			writeShadow(makeSparseIndexKey(table, column, encodeKeyComponent(*maybeVal), pk), pk);
			if (!advance()) { aborted = true; return false; }
			return true;
		});
		if (aborted) {
		  return;
		}
	} else if (indexType == "range") {
		bool aborted = false;
		db_.scanPrefix(entityPrefix, [&](std::string_view key, std::string_view val) {
			size_t lc = key.rfind(':');
			if (lc == std::string::npos) {
			  return true;
			}
			std::string pk(key.substr(lc + 1));
			BaseEntity entity = BaseEntity::deserialize(pk, BaseEntity::Blob(val.begin(), val.end()));
			auto maybeVal = entity.extractField(column);
			if (!maybeVal) { if (!advance()) { aborted = true; return false; } return true; }
			writeShadow(makeRangeIndexKey(table, column, *maybeVal, pk), pk);
			if (!advance()) { aborted = true; return false; }
			return true;
		});
		if (aborted) {
		  return;
		}
	} else if (indexType == "composite") {
		std::vector<std::string> cols;
		size_t pos = 0;
		while (pos < column.size()) {
			size_t p = column.find('+', pos);
			if (p == std::string::npos) {
			  p = column.size();
			}
			cols.emplace_back(column.substr(pos, p - pos));
			pos = p + 1;
		}
		bool aborted = false;
		db_.scanPrefix(entityPrefix, [&](std::string_view key, std::string_view val) {
			size_t lc = key.rfind(':');
			if (lc == std::string::npos) {
			  return true;
			}
			std::string pk(key.substr(lc + 1));
			BaseEntity entity = BaseEntity::deserialize(pk, BaseEntity::Blob(val.begin(), val.end()));
			std::vector<std::string> values = {};

			values.reserve(cols.size());
			for (const auto& col : cols) {
				auto mv = entity.extractField(col);
				if (!mv) { if (!advance()) { aborted = true; return false; } return true; }
				values.emplace_back(*mv);
			}
			writeShadow(makeCompositeIndexKey(table, cols, values, pk), pk);
			if (!advance()) { aborted = true; return false; }
			return true;
		});
		if (aborted) {
		  return;
		}
	} else { // regular
		bool aborted = false;
		db_.scanPrefix(entityPrefix, [&](std::string_view key, std::string_view val) {
			size_t lc = key.rfind(':');
			if (lc == std::string::npos) {
			  return true;
			}
			std::string pk(key.substr(lc + 1));
			BaseEntity entity = BaseEntity::deserialize(pk, BaseEntity::Blob(val.begin(), val.end()));
			auto maybeVal = entity.extractField(column);
			if (!maybeVal) { if (!advance()) { aborted = true; return false; } return true; }
			writeShadow(KeySchema::makeSecondaryIndexKey(table, column, encodeKeyComponent(*maybeVal), pk), pk);
			if (!advance()) { aborted = true; return false; }
			return true;
		});
		if (aborted) {
		  return;
		}
	}

	// Step 6: Atomic swap – in one WriteBatch:
	//   a) delete all stale live entries
	//   b) promote each shadow entry to the live prefix
	//   c) delete each shadow entry
	// During this brief batch write the live index briefly transitions; concurrent
	// readers may observe either old or new entries, but never an empty index.
	auto batch = db_.createWriteBatch();

	db_.scanPrefix(livePrefix, [&batch](std::string_view k, std::string_view) {
		batch->del(k);
		return true;
	});

	db_.scanPrefix(shadowPrefix, [&batch](std::string_view k, std::string_view v) {
		// live key = shadow key with the leading "__rb__:" (7 chars) stripped
		std::string liveKey(k.substr(7));
		batch->put(liveKey, std::vector<uint8_t>(v.begin(), v.end()));
		batch->del(k);
		return true;
	});

	batch->commit();

	// Step 7: Update metrics
	auto end_time = std::chrono::steady_clock::now();
	auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
	rebuild_metrics_.rebuild_duration_ms.fetch_add(duration_ms, std::memory_order_relaxed);
	rebuild_metrics_.rebuild_entities_processed.fetch_add(done, std::memory_order_relaxed);
	rebuild_metrics_.online_rebuild_count.fetch_add(1, std::memory_order_relaxed);
}

SecondaryIndexManager::IndexStats
SecondaryIndexManager::getIndexStats(std::string_view table, std::string_view column) const {
	IndexStats stats;
	const std::string tableStr(table);
	const std::string columnStr(column);
	stats.table = tableStr;
	stats.column = columnStr;
	stats.entry_count = 0;
	stats.estimated_size_bytes = 0;
	stats.unique = false;

	// Helper to read meta value using RocksDBWrapper::get()
	auto readMeta = [&]([[maybe_unused]] const std::string& key)->std::optional<std::string> {
		auto opt = db_.get(key);
		if (!opt) {
		  return std::nullopt;
		}
		return std::string(opt->begin(), opt->end());
	};

	bool found = false;

	// Composite index detection: column contains '+'
	if (columnStr.find('+') != std::string::npos && !found) {
		std::vector<std::string> cols;
		size_t pos = 0;
		while (pos < columnStr.size()) {
			size_t p = columnStr.find('+', pos);
			if (p == std::string::npos) {
			  p = columnStr.size();
			}
			cols.emplace_back(columnStr.substr(pos, p - pos));
			pos = p + 1;
		}
		std::string metaKey = makeCompositeIndexMetaKey(table, cols);
		if (auto mv = readMeta(metaKey)) {
		stats.type = "composite";
		stats.unique = (mv->find("unique") != std::string::npos);
		// additional_info ist die Spaltenliste
		std::string colList = {};
		if (!cols.empty()) {
			size_t totalLen = 0;
			for (const auto& c : cols) {
			  totalLen += c.size();
			}
			totalLen += (cols.size() - 1) * 2; // ", " separators
			colList.reserve(totalLen);
			for (size_t i = 0; i < cols.size(); ++i) {
				if (i > 0) {
				  colList += ", ";
				}
				colList += cols[i];
			}
		}
		stats.additional_info = colList;			std::string prefix = std::string("idx:") + tableStr + ":" + columnStr + ":";
			db_.scanPrefix(prefix, [&stats](std::string_view /*k*/, std::string_view /*v*/) {
				stats.entry_count++;
				return true;
			});
			found = true;
		}
	}

	// TTL
	if (!found) {
		std::string metaKey = makeTTLIndexMetaKey(table, column);
		if (auto mv = readMeta(metaKey)) {
			stats.type = "ttl";
			stats.unique = false;
			stats.additional_info = std::string("ttl_seconds=") + *mv;

			std::string prefix = std::string("ttlidx:") + tableStr + ":" + columnStr + ":";
			db_.scanPrefix(prefix, [&stats](std::string_view /*k*/, std::string_view /*v*/) {
				stats.entry_count++;
				return true;
			});
			found = true;
		}
	}

	// Fulltext
	if (!found) {
		std::string metaKey = makeFulltextIndexMetaKey(table, column);
		if (auto mv = readMeta(metaKey)) {
			stats.type = "fulltext";
			stats.unique = false;
			stats.additional_info = "inverted_index";

			std::string prefix = std::string("ftidx:") + tableStr + ":" + columnStr + ":";
			db_.scanPrefix(prefix, [&stats](std::string_view /*k*/, std::string_view /*v*/) {
				stats.entry_count++;
				return true;
			});
			found = true;
		}
	}

	// Geo
	if (!found) {
		std::string metaKey = makeGeoIndexMetaKey(table, column);
		if (auto mv = readMeta(metaKey)) {
			stats.type = "geo";
			stats.unique = false;
			stats.additional_info = "geohash";

			std::string prefix = std::string("gidx:") + tableStr + ":" + columnStr + ":";
			db_.scanPrefix(prefix, [&stats](std::string_view /*k*/, std::string_view /*v*/) {
				stats.entry_count++;
				return true;
			});
			found = true;
		}
	}

	// Sparse
	if (!found) {
		std::string metaKey = makeSparseIndexMetaKey(table, column);
		if (auto mv = readMeta(metaKey)) {
			stats.type = "sparse";
			stats.unique = (*mv == "unique");
			stats.additional_info = *mv;

			std::string prefix = std::string("sidx:") + tableStr + ":" + columnStr + ":";
			db_.scanPrefix(prefix, [&stats](std::string_view /*k*/, std::string_view /*v*/) {
				stats.entry_count++;
				return true;
			});
			found = true;
		}
	}

	// Range
	if (!found) {
		std::string metaKey = makeRangeIndexMetaKey(table, column);
		if (auto mv = readMeta(metaKey)) {
			stats.type = "range";
			stats.unique = false;
			stats.additional_info = "sorted";

			std::string prefix = std::string("ridx:") + tableStr + ":" + columnStr + ":";
			db_.scanPrefix(prefix, [&stats](std::string_view /*k*/, std::string_view /*v*/) {
				stats.entry_count++;
				return true;
			});
			found = true;
		}
	}

	// Regular
	if (!found) {
		std::string metaKey = makeIndexMetaKey(table, column);
		if (auto mv = readMeta(metaKey)) {
			stats.type = "regular";
			stats.unique = (*mv == "unique");
			stats.additional_info = *mv;

			std::string prefix = std::string("idx:") + tableStr + ":" + columnStr + ":";
			db_.scanPrefix(prefix, [&stats](std::string_view /*k*/, std::string_view /*v*/) {
				stats.entry_count++;
				return true;
			});
			found = true;
		}
	}

	// Partial (filtered)
	if (!found) {
		std::string metaKey = makePartialIndexMetaKey(table, column);
		if (auto mv = readMeta(metaKey)) {
			stats.type = "partial";
			stats.unique = (mv->find("|unique") != std::string::npos);
			// Extract predicate from meta value
			auto pipePos = mv->find('|');
			std::string predicate = (pipePos != std::string::npos) ? mv->substr(0, pipePos) : *mv;
			stats.additional_info = "predicate=" + predicate;

			std::string prefix = std::string("pidx:") + tableStr + ":" + columnStr + ":";
			db_.scanPrefix(prefix, [&stats](std::string_view /*k*/, std::string_view /*v*/) {
				stats.entry_count++;
				return true;
			});
			found = true;
		}
	}

	stats.estimated_size_bytes = stats.entry_count * 100;
	return stats;
}

void SecondaryIndexManager::reindexTable(const std::string& table) {
	std::unordered_set<std::string> columns;
	
	auto scanMetaPrefix = [&]([[maybe_unused]] const std::string& metaPrefix) {
		std::string prefix = metaPrefix + table + ":";
		db_.scanPrefix(prefix, [&](std::string_view key, std::string_view /*val*/) {
			std::string keyStr(key);
			size_t firstColon = keyStr.find(':');
			if (firstColon != std::string::npos) {
				size_t secondColon = keyStr.find(':', firstColon + 1);
				if (secondColon != std::string::npos) {
					std::string column = keyStr.substr(secondColon + 1);
					size_t thirdColon = column.find(':');
					if (thirdColon != std::string::npos) {
						column.resize(thirdColon);
					}
					columns.insert(column);
				}
			}
			return true;
		});
	};
	
	scanMetaPrefix("idxmeta:");
	scanMetaPrefix("ridxmeta:");
	scanMetaPrefix("sidxmeta:");
	scanMetaPrefix("gidxmeta:");
	scanMetaPrefix("ttlidxmeta:");
	scanMetaPrefix("ftidxmeta:");
	scanMetaPrefix("cidxmeta:");
	scanMetaPrefix("pidxmeta:");
	
	for (const auto& column : columns) {
		rebuildIndex(table, column);
	}
}

// ============================================================================
// MVCC Transaction Variants
// ============================================================================

SecondaryIndexManager::Status SecondaryIndexManager::put(
	std::string_view table, 
	const BaseEntity& entity, 
	RocksDBWrapper::TransactionWrapper& txn) {
	
	if (table.empty()) {
	  return Status::Error("put(mvcc): table darf nicht leer sein");
	}
	if (entity.getPrimaryKey().empty()) {
	  return Status::Error("put(mvcc): pk darf nicht leer sein");
	}
	if (!db_.isOpen()) {
	  return Status::Error("put(mvcc): Datenbank ist nicht geöffnet");
	}
	if (!txn.isActive()) {
	  return Status::Error("put(mvcc): Transaction ist nicht aktiv");
	}

	const std::string& pk = entity.getPrimaryKey();
	const std::string relKey = KeySchema::makeRelationalKey(table, pk);

	// Alte Entity lesen (mit MVCC Snapshot)
	std::optional<std::vector<uint8_t>> oldBlob = txn.get(relKey);
	std::unique_ptr<BaseEntity> oldEntity = {};

	if (oldBlob) {
		try { 
			oldEntity = std::make_unique<BaseEntity>(BaseEntity::deserialize(pk, *oldBlob)); 
		}
		catch (...) { 
			THEMIS_WARN("put(mvcc): alte Entity für PK={} nicht deserialisierbar", pk); 
		}
	}

	// Neue Entity schreiben
	txn.put(relKey, entity.serialize());

	// Indexes aktualisieren
	if (oldEntity) {
		auto delStatus = updateIndexesForDelete_(table, pk, oldEntity.get(), txn);
		if (!delStatus.ok) {
		  return delStatus;
		}
	}
	return updateIndexesForPut_(table, pk, entity, txn);
}

SecondaryIndexManager::Status SecondaryIndexManager::erase(
	std::string_view table, 
	std::string_view pk, 
	RocksDBWrapper::TransactionWrapper& txn) {
	
	if (table.empty()) {
	  return Status::Error("erase(mvcc): table darf nicht leer sein");
	}
	if (pk.empty()) {
	  return Status::Error("erase(mvcc): pk darf nicht leer sein");
	}
	if (!db_.isOpen()) {
	  return Status::Error("erase(mvcc): Datenbank ist nicht geöffnet");
	}
	if (!txn.isActive()) {
	  return Status::Error("erase(mvcc): Transaction ist nicht aktiv");
	}

	const std::string relKey = KeySchema::makeRelationalKey(table, pk);
	
	// Alte Entity lesen (mit MVCC Snapshot)
	std::optional<std::vector<uint8_t>> oldBlob = txn.get(relKey);
	std::unique_ptr<BaseEntity> oldEntity = {};

	if (oldBlob) {
		try { 
			oldEntity = std::make_unique<BaseEntity>(BaseEntity::deserialize(std::string(pk), *oldBlob)); 
		}
		catch (...) { 
			THEMIS_WARN("erase(mvcc): alte Entity für PK={} nicht deserialisierbar", pk); 
		}
	}

	txn.del(relKey);
	return updateIndexesForDelete_(table, pk, oldEntity.get(), txn);
}

// ============================================================================
// MVCC Helper Methods
// ============================================================================

SecondaryIndexManager::Status SecondaryIndexManager::updateIndexesForPut_(
	std::string_view table,
	std::string_view pk,
	const BaseEntity& newEntity,
	RocksDBWrapper::TransactionWrapper& txn) {
    
	// v1.3.4 OPTIMIZATION: Use metadata cache to avoid repeated DB scans
	auto& cache = SecondaryIndexMetadataCache::instance();
	auto cachedMetadata = cache.get(table);
	const bool hasCachedMetadata = cachedMetadata.has_value();

	std::unordered_map<std::string, bool> regularUniqueCache;
	std::unordered_map<std::string, bool> sparseUniqueCache;
	std::unordered_map<std::string, bool> compositeUniqueCache;
	std::unordered_map<std::string, int64_t> ttlSecondsCache;
	std::unordered_map<std::string, SecondaryIndexMetadataCache::CachedFulltextConfig> fulltextConfigsCache;
	std::unordered_map<std::string, std::string> partialPredicatesCache;
	std::unordered_map<std::string, bool> partialUniqueCache;
	std::unordered_set<std::string> sparseColsCache;
	std::unordered_set<std::string> geoColsCache;
	std::unordered_set<std::string> ttlColsCache;
	std::unordered_set<std::string> fulltextColsCache;
	std::vector<std::string> partialColsOrderCache;

	// On cache hit use the precomputed sets directly; on miss load from DB and
	// populate the cache including the precomputed sets for future hits.
	const std::unordered_set<std::string>* indexedColsPtr = nullptr;
	const std::unordered_set<std::string>* rangeColsPtr   = nullptr;
	std::unordered_set<std::string> indexedColsMiss, rangeColsMiss;
	std::unordered_set<std::string> indexedColsCache, rangeColsCache;

	if (hasCachedMetadata) {
		const auto metadata = *cachedMetadata;
		indexedColsCache = metadata.regular_indexes_set;
		rangeColsCache = metadata.range_indexes_set;
		regularUniqueCache = metadata.regular_unique;
		sparseUniqueCache = metadata.sparse_unique;
		compositeUniqueCache = metadata.composite_unique;
		ttlSecondsCache = metadata.ttl_seconds;
		fulltextConfigsCache = metadata.fulltext_configs;
		partialPredicatesCache = metadata.partial_predicates;
		partialUniqueCache = metadata.partial_unique;
		sparseColsCache = {metadata.sparse_indexes.begin(), metadata.sparse_indexes.end()};
		geoColsCache = {metadata.geo_indexes.begin(), metadata.geo_indexes.end()};
		ttlColsCache = {metadata.ttl_indexes.begin(), metadata.ttl_indexes.end()};
		fulltextColsCache = {metadata.fulltext_indexes.begin(), metadata.fulltext_indexes.end()};
		partialColsOrderCache = metadata.partial_indexes;
		indexedColsPtr = &indexedColsCache;
		rangeColsPtr   = &rangeColsCache;
	} else {
		// Cache miss - load from DB and populate cache
		indexedColsMiss = loadIndexedColumns_(table);
		rangeColsMiss   = loadRangeIndexedColumns_(table);

		SecondaryIndexMetadataCache::IndexMetadata metadata;
		metadata.regular_indexes = std::vector<std::string>(indexedColsMiss.begin(), indexedColsMiss.end());
		metadata.range_indexes   = std::vector<std::string>(rangeColsMiss.begin(), rangeColsMiss.end());
		metadata.regular_indexes_set = indexedColsMiss;
		metadata.range_indexes_set   = rangeColsMiss;
		for (const auto& col : metadata.regular_indexes) {
			metadata.regular_unique[col] = isUniqueIndex_(table, col);
		}

		// Load other index types too for future calls
		auto sparseColsLoad = loadSparseIndexedColumns_(table);
		metadata.sparse_indexes = std::vector<std::string>(sparseColsLoad.begin(), sparseColsLoad.end());
		for (const auto& col : metadata.sparse_indexes) {
			metadata.sparse_unique[col] = isSparseIndexUnique_(table, col);
		}
		auto geoColsLoad = loadGeoIndexedColumns_(table);
		metadata.geo_indexes = std::vector<std::string>(geoColsLoad.begin(), geoColsLoad.end());
		auto ttlColsLoad = loadTTLIndexedColumns_(table);
		metadata.ttl_indexes = std::vector<std::string>(ttlColsLoad.begin(), ttlColsLoad.end());
		auto ftColsLoad = loadFulltextIndexedColumns_(table);
		metadata.fulltext_indexes = std::vector<std::string>(ftColsLoad.begin(), ftColsLoad.end());
		auto partialColsLoad = loadPartialIndexedColumns_(table);
		for (const auto& [col, pred] : partialColsLoad) {
			metadata.partial_indexes.emplace_back(col);
			metadata.partial_predicates[col] = pred;
			metadata.partial_unique[col] = isPartialIndexUnique_(table, col);
		}


		// v1.3.5: cache per-column TTL seconds to avoid db.get on every insert
		for (const auto& tcol : metadata.ttl_indexes) {
			metadata.ttl_seconds[tcol] = getTTLSeconds_(table, tcol);
		}

		// v1.3.5: cache per-column fulltext config to avoid db.get + JSON parse on every insert
		for (const auto& fcol : metadata.fulltext_indexes) {
			auto cfg = getFulltextConfig(table, fcol).value_or(FulltextConfig{});
			SecondaryIndexMetadataCache::CachedFulltextConfig cached;
			cached.stemming_enabled  = cfg.stemming_enabled;
			cached.language          = cfg.language;
			cached.stopwords_enabled = cfg.stopwords_enabled;
			cached.stopwords         = cfg.stopwords;
			cached.normalize_umlauts = cfg.normalize_umlauts;
			metadata.fulltext_configs[fcol] = std::move(cached);
		}

		// v1.3.5: cache composite unique flags to avoid db.get per composite insert
		for (const auto& col : metadata.regular_indexes) {
			if (col.find('+') != std::string::npos) {
				std::vector<std::string> columns;
				columns.reserve(std::count(col.begin(), col.end(), '+') + 1);
				size_t start = 0;
				while (start < col.size()) {
					size_t pos = col.find('+', start);
					if (pos == std::string::npos) { columns.emplace_back(col.substr(start)); break; }
					columns.emplace_back(col.substr(start, pos - start));
					start = pos + 1;
				}
				metadata.composite_unique[col] = isUniqueCompositeIndex_(table, columns);
			}
		}
		cache.set(table, metadata);
		indexedColsPtr = &indexedColsMiss;
		rangeColsPtr   = &rangeColsMiss;
	}

	const auto& indexedCols = *indexedColsPtr;
	const auto& rangeCols   = *rangeColsPtr;

	// Micro-Optimization: compute PK bytes once and reuse
	std::vector<uint8_t> pkBytes = toBytes(pk);

	// Trennen: Single-Column vs. Composite (enthält '+')
	for (const auto& col : indexedCols) {
		if (col.find('+') == std::string::npos) {
			// Single-Column
			auto maybe = newEntity.extractField(col);
			if (!maybe) {
			  continue;
			}
			const std::string encodedVal = encodeKeyComponent(*maybe);
			
			// Unique-Constraint prüfen
			bool uniqueIndex = false;
			if (hasCachedMetadata) {
				auto it = regularUniqueCache.find(col);
				uniqueIndex = (it != regularUniqueCache.end()) ? it->second : isUniqueIndex_(table, col);
			} else {
				uniqueIndex = isUniqueIndex_(table, col);
			}
			if (uniqueIndex) {
				// Fix Concurrent-Unique-Lücke (ACID): acquire an exclusive lock on a
				// sentinel key that identifies this (table, col, value) triple.  The
				// lock is held until the transaction commits or rolls back, serializing
				// all concurrent transactions that try to insert the same unique value.
				// Without this, two transactions could both pass the db_.scanPrefix
				// check, both commit, and yield a duplicate unique-index entry.
				if (!txn.getForUpdate(makeUniqueSentinelKey_(table, col, encodedVal))) {
					return Status::Error("Unique constraint write conflict: " + std::string(table) + "." + col + " = " + *maybe + " (concurrent transaction holds lock)");
				}
				// Prüfe ob bereits ein anderer PK mit diesem Wert existiert
				std::string prefix = std::string("idx:") + std::string(table) + ":" + col + ":" + encodedVal + ":";
				bool conflict = false;
				db_.scanPrefix(prefix, [&pk, &conflict](std::string_view key, std::string_view /*val*/) {
					// Extrahiere PK aus key: idx:table:column:value:PK
					size_t lastColon = key.rfind(':');
					if (lastColon != std::string_view::npos) {
						std::string_view existingPK = key.substr(lastColon + 1);
						if (existingPK != pk) {
							conflict = true;
							return false; // Stop scan
						}
					}
					return true;
				});
				if (conflict) {
					return Status::Error("Unique constraint violation: " + std::string(table) + "." + col + " = " + *maybe);
				}
			}
			
			const std::string idxKey = KeySchema::makeSecondaryIndexKey(table, col, encodedVal, pk);
			txn.put(idxKey, pkBytes);
			
			// Falls Range-Index für diese Spalte existiert, ebenfalls pflegen
			if (rangeCols.find(col) != rangeCols.end()) {
				const std::string rkey = makeRangeIndexKey(table, col, *maybe, pk);
				txn.put(rkey, pkBytes);
			}
		} else {
			// Composite: col = "col1+col2+..."
			// Parse columns
			std::vector<std::string> columns;
			size_t start = 0;
			while (start < col.size()) {
				size_t pos = col.find('+', start);
				if (pos == std::string::npos) {
					columns.emplace_back(col.substr(start));
					break;
				}
				columns.emplace_back(col.substr(start, pos - start));
				start = pos + 1;
			}
			
			// Extract values
			std::vector<std::string> values = {};

			values.reserve(columns.size());
			bool allPresent = true;
			for (const auto& c : columns) {
				auto maybe = newEntity.extractField(c);
				if (!maybe) {
					allPresent = false;
					break;
				}
				values.emplace_back(*maybe);
			}
			
			if (!allPresent) continue; // Skip wenn nicht alle Felder vorhanden
			
			// Unique-Constraint prüfen für Composite Index
			// Use cache to avoid db.get per composite insert; fall back to DB on cache miss.
			bool compositeUnique = false;
			if (hasCachedMetadata) {
				auto it = compositeUniqueCache.find(col);
				compositeUnique = (it != compositeUniqueCache.end()) && it->second;
			} else {
				compositeUnique = isUniqueCompositeIndex_(table, columns);
			}
			if (compositeUnique) {
				// Fix Concurrent-Unique-Lücke (ACID): lock a composite sentinel key
				// derived from (table, columns, values) to serialize concurrent writes
				// of the same unique composite value across transactions.
				if (!txn.getForUpdate(makeCompositeUniqueSentinelKey_(table, columns, values))) {
					std::string valueStr = {};
					for (size_t i = 0; i < values.size(); ++i) {
						if (i > 0) {
						  valueStr += ", ";
						}
						valueStr += columns[i] + "=" + values[i];
					}
					return Status::Error("Unique constraint write conflict: " + std::string(table) + ".{" + valueStr + "} (concurrent transaction holds lock)");
				}
				// Prüfe ob bereits ein anderer PK mit dieser Wertekombination existiert
				std::string prefix = makeCompositeIndexPrefix(table, columns, values);
				bool conflict = false;
				db_.scanPrefix(prefix, [&pk, &conflict](std::string_view key, std::string_view /*val*/) {
					// Extrahiere PK aus key (letztes Segment nach ':')
					size_t lastColon = key.rfind(':');
					if (lastColon != std::string_view::npos) {
						std::string_view existingPK = key.substr(lastColon + 1);
						if (existingPK != pk) {
							conflict = true;
							return false; // Stop scan
						}
					}
					return true;
				});
				if (conflict) {
					std::string valueStr = {};
					for (size_t i = 0; i < values.size(); ++i) {
						if (i > 0) {
						  valueStr += ", ";
						}
						valueStr += columns[i] + "=" + values[i];
					}
					return Status::Error("Unique constraint violation: " + std::string(table) + ".{" + valueStr + "}");
				}
			}
			
			const std::string idxKey = makeCompositeIndexKey(table, columns, values, pk);
			txn.put(idxKey, pkBytes);
		}
	}

	// Zusätzlich: Range-Indizes pflegen, die keine Equality-Indizes haben
	for (const auto& rcol : rangeCols) {
		// Wenn diese Spalte bereits im obigen Loop gepflegt wurde (weil Equality-Index existiert), überspringen
		if (indexedCols.find(rcol) != indexedCols.end()) {
		  continue;
		}
		// Nur Single-Column Range-Indizes unterstützen (Composite-Range-Indizes sind nicht implementiert)
		auto maybe = newEntity.extractField(rcol);
		if (!maybe) {
		  continue;
		}
		const std::string rkey = makeRangeIndexKey(table, rcol, *maybe, pk);
		txn.put(rkey, pkBytes);
	}

	// Sparse-Indizes pflegen (v1.3.4: use cache)
	std::unordered_set<std::string> sparseCols = {};

	if (hasCachedMetadata) {
		sparseCols = sparseColsCache;
	} else {
		sparseCols = loadSparseIndexedColumns_(table);
	}
	for (const auto& scol : sparseCols) {
		auto maybe = newEntity.extractField(scol);
		if (!maybe || isNullOrEmpty_(*maybe)) continue; // Skip NULL/empty values
		
		const std::string encodedVal = encodeKeyComponent(*maybe);
		
		// Unique-Constraint prüfen für Sparse Index
		bool sparseUnique = false;
		if (hasCachedMetadata) {
			auto it = sparseUniqueCache.find(scol);
			sparseUnique = (it != sparseUniqueCache.end()) ? it->second : isSparseIndexUnique_(table, scol);
		} else {
			sparseUnique = isSparseIndexUnique_(table, scol);
		}
		if (sparseUnique) {
			std::string prefix = makeSparseIndexKey(table, scol, encodedVal, "");
			bool conflict = false;
			db_.scanPrefix(prefix, [&pk, &conflict](std::string_view key, std::string_view /*val*/) {
				size_t lastColon = key.rfind(':');
				if (lastColon != std::string_view::npos) {
					std::string_view extractedPK = key.substr(lastColon + 1);
					if (extractedPK != pk) {
						conflict = true;
						return false;
					}
				}
				return true;
			});
			if (conflict) {
				return Status::Error("Sparse unique constraint violation: " + std::string(table) + "." + scol + " = " + *maybe);
			}
		}
		
		const std::string sidxKey = makeSparseIndexKey(table, scol, encodedVal, pk);
		txn.put(sidxKey, pkBytes);
	}

	// Geo-Indizes pflegen (v1.3.4: use cache)
	std::unordered_set<std::string> geoCols = {};

	if (hasCachedMetadata) {
		geoCols = geoColsCache;
	} else {
		geoCols = loadGeoIndexedColumns_(table);
	}
	for (const auto& gcol : geoCols) {
		// Geo-Index erwartet zwei Felder: gcol_lat und gcol_lon (oder einfach lat/lon)
		// Konvention: Spaltenname ist z.B. "location", dann Felder "location_lat" und "location_lon"
		std::string latField = gcol + "_lat";
		std::string lonField = gcol + "_lon";
		
		auto maybeLat = newEntity.extractField(latField);
		auto maybeLon = newEntity.extractField(lonField);
		
		if (!maybeLat || !maybeLon) continue; // Skip wenn Koordinaten fehlen
		
		try {
			double lat = std::stod(*maybeLat);
			double lon = std::stod(*maybeLon);
			
			std::string geohash = encodeGeohash(lat, lon);
			const std::string gidxKey = makeGeoIndexKey(table, gcol, geohash, pk);
			txn.put(gidxKey, pkBytes);
		} catch (...) {
			THEMIS_WARN("updateIndexesForPut_(mvcc): Ungültige Geo-Koordinaten für {}.{}: lat={}, lon={}", 
					   table, gcol, *maybeLat, *maybeLon);
			continue;
		}
	}

	// TTL-Indizes pflegen (v1.3.4: use cache)
	std::unordered_set<std::string> ttlCols = {};

	if (hasCachedMetadata) {
		ttlCols = ttlColsCache;
	} else {
		ttlCols = loadTTLIndexedColumns_(table);
	}
	for (const auto& tcol : ttlCols) {
		auto maybeValue = newEntity.extractField(tcol);
		if (!maybeValue) {
		  continue;
		}
		
		// Calculate expire timestamp: now + TTL seconds (use cached TTL to avoid db.get, v1.3.5)
		auto now = std::chrono::system_clock::now();
		auto epoch = now.time_since_epoch();
		int64_t currentTimestamp = std::chrono::duration_cast<std::chrono::seconds>(epoch).count();
		int64_t ttlSeconds = 0;
		if (hasCachedMetadata) {
			auto it = ttlSecondsCache.find(tcol);
			if (it != ttlSecondsCache.end()) {
			  ttlSeconds = it->second;
			}
		} else {
			ttlSeconds = getTTLSeconds_(table, tcol);
		}
		if (ttlSeconds <= 0) {
		  continue;
		}
		
		int64_t expireTimestamp = currentTimestamp + ttlSeconds;
		const std::string ttlKey = makeTTLIndexKey(table, tcol, expireTimestamp, pk);
		txn.put(ttlKey, pkBytes);
	}

	// Fulltext-Indizes pflegen (v1.3.4: use cache)
	std::unordered_set<std::string> fulltextCols = {};

	if (hasCachedMetadata) {
		fulltextCols = fulltextColsCache;
	} else {
		fulltextCols = loadFulltextIndexedColumns_(table);
	}
	for (const auto& fcol : fulltextCols) {
		auto maybeText = newEntity.extractField(fcol);
		if (!maybeText || isNullOrEmpty_(maybeText)) {
		  continue;
		}
		
		// Use cached fulltext config to avoid db.get + JSON parse on every insert (v1.3.5)
		FulltextConfig config = {};
		if (hasCachedMetadata) {
			auto it = fulltextConfigsCache.find(fcol);
			if (it != fulltextConfigsCache.end()) {
				const auto& c = it->second;
				config.stemming_enabled  = c.stemming_enabled;
				config.language          = c.language;
				config.stopwords_enabled = c.stopwords_enabled;
				config.stopwords         = c.stopwords;
				config.normalize_umlauts = c.normalize_umlauts;
			}
		} else {
			config = getFulltextConfig(table, fcol).value_or(FulltextConfig{});
		}
		auto tokens = tokenize(*maybeText, config);
		
		std::unordered_map<std::string, uint32_t> tf = {};

		for (const auto& t : tokens) { if (!t.empty()) tf[t]++; }
		const std::string dkey = makeFulltextDocLenKey(table, fcol, pk);
		{
			std::string s = std::to_string(tokens.size());
			std::vector<uint8_t> val(s.begin(), s.end());
			txn.put(dkey, val);
		}
		for (const auto& [token, count] : tf) {
			const std::string ftKey = makeFulltextIndexKey(table, fcol, token, pk);
			txn.put(ftKey, pkBytes);
			const std::string tfKey = makeFulltextTFKey(table, fcol, token, pk);
			std::string s = std::to_string(count);
			std::vector<uint8_t> tfVal(s.begin(), s.end());
			txn.put(tfKey, tfVal);
		}
	}

	// Partial (filtered) indexes pflegen
	{
		std::unordered_map<std::string, std::string> partialCols = {};

		if (hasCachedMetadata) {
			for (const auto& col : partialColsOrderCache) {
				auto it = partialPredicatesCache.find(col);
				if (it != partialPredicatesCache.end())
					partialCols[col] = it->second;
			}
		} else {
			partialCols = loadPartialIndexedColumns_(table);
		}
		for (const auto& [pcol, ppred] : partialCols) {
			// Only index if entity satisfies the predicate
			if (!evaluatePartialPredicate_(newEntity, ppred)) {
			  continue;
			}
			auto maybe = newEntity.extractField(pcol);
			if (!maybe) {
			  continue;
			}
			const std::string encodedVal = encodeKeyComponent(*maybe);

			// Unique-Constraint prüfen
			bool partialUnique = false;
			if (hasCachedMetadata) {
				auto it = partialUniqueCache.find(pcol);
				partialUnique = (it != partialUniqueCache.end()) ? it->second : isPartialIndexUnique_(table, pcol);
			} else {
				partialUnique = isPartialIndexUnique_(table, pcol);
			}
			if (partialUnique) {
				const std::string checkPrefix = makePartialIndexPrefix(table, pcol, encodedVal);
				bool conflict = false;
				db_.scanPrefix(checkPrefix, [&pk, &conflict](std::string_view key, std::string_view) {
					size_t lastColon = key.rfind(':');
					if (lastColon != std::string_view::npos && key.substr(lastColon + 1) != pk) {
						conflict = true;
						return false;
					}
					return true;
				});
				if (conflict)
					return Status::Error("Partial index unique constraint violation: " +
					                     std::string(table) + "." + pcol + " = " + *maybe);
			}

			const std::string pidxKey = makePartialIndexKey(table, pcol, encodedVal, pk);
			txn.put(pidxKey, pkBytes);
		}
	}

	return Status::OK();
}

SecondaryIndexManager::Status SecondaryIndexManager::updateIndexesForDelete_(
	std::string_view table,
	std::string_view pk,
	const BaseEntity* oldEntityOpt,
	RocksDBWrapper::TransactionWrapper& txn) {

	// Use metadata cache to avoid repeated DB meta-scans on every delete/upsert.
	auto& cache = SecondaryIndexMetadataCache::instance();
	auto cachedMetadata = cache.get(table);
	const bool hasCachedMetadata = cachedMetadata.has_value();

	std::unordered_set<std::string> indexedColsCache;
	std::unordered_set<std::string> rangeColsCache;
	std::unordered_set<std::string> sparseColsCache;
	std::unordered_set<std::string> geoColsCache;
	std::unordered_set<std::string> ttlColsCache;
	std::unordered_set<std::string> fulltextColsCache;
	std::unordered_map<std::string, std::string> partialColsCache;
	std::unordered_map<std::string, SecondaryIndexMetadataCache::CachedFulltextConfig> fulltextConfigsCache = {};

	if (hasCachedMetadata) {
		const auto metadata = *cachedMetadata;
		indexedColsCache = metadata.regular_indexes_set;
		rangeColsCache = metadata.range_indexes_set;
		sparseColsCache = {metadata.sparse_indexes.begin(), metadata.sparse_indexes.end()};
		geoColsCache = {metadata.geo_indexes.begin(), metadata.geo_indexes.end()};
		ttlColsCache = {metadata.ttl_indexes.begin(), metadata.ttl_indexes.end()};
		fulltextColsCache = {metadata.fulltext_indexes.begin(), metadata.fulltext_indexes.end()};
		for (const auto& col : metadata.partial_indexes) {
			auto it = metadata.partial_predicates.find(col);
			partialColsCache[col] = (it != metadata.partial_predicates.end()) ? it->second : "";
		}
		fulltextConfigsCache = metadata.fulltext_configs;
	}

	const auto indexedCols = hasCachedMetadata ? indexedColsCache : loadIndexedColumns_(table);
	const auto rangeCols = hasCachedMetadata ? rangeColsCache : loadRangeIndexedColumns_(table);
	const auto sparseCols = hasCachedMetadata ? sparseColsCache : loadSparseIndexedColumns_(table);
	const auto geoCols = hasCachedMetadata ? geoColsCache : loadGeoIndexedColumns_(table);
	const auto ttlCols = hasCachedMetadata ? ttlColsCache : loadTTLIndexedColumns_(table);
	const auto fulltextCols = hasCachedMetadata ? fulltextColsCache : loadFulltextIndexedColumns_(table);
	const auto partialCols = hasCachedMetadata ? partialColsCache : loadPartialIndexedColumns_(table);

	if (!oldEntityOpt) {
		// Falls keine alte Entity, können wir die spezifischen Index-Keys nicht sicher bestimmen.
		// Defensive strategy: alle Index-Prefixe für diesen PK löschen via Scan.
		for (const auto& col : indexedCols) {
			std::string prefix = {};
			if (col.find('+') == std::string::npos) {
				// Single
				prefix = std::string("idx:") + std::string(table) + ":" + col + ":";
			} else {
				// Composite
				prefix = std::string("idx:") + std::string(table) + ":" + col + ":";
			}
			// W5: Snapshot pk locally; eliminate [this] capture to avoid manager state closure
			const std::string_view pk_snapshot = pk;
			db_.scanPrefix(prefix, [&pk_snapshot, &txn](std::string_view key, std::string_view /*val*/){
				// Prüfen, ob PK am Ende passt (endet mit :PK)
				std::string_view keyView(key);
				size_t lastColon = keyView.rfind(':');
				if (lastColon != std::string_view::npos) {
					std::string_view extractedPK = keyView.substr(lastColon + 1);
					if (extractedPK == pk_snapshot) {
						txn.del(std::string(key));
					}
				}
				return true;
			});
		}
		// Auch alle Range-Index-Einträge mit diesem PK für diese Tabelle entfernen
		for (const auto& rcol : rangeCols) {
			std::string rprefix = std::string("ridx:") + std::string(table) + ":" + rcol + ":";
			db_.scanPrefix(rprefix, [&pk, &txn](std::string_view key, std::string_view /*val*/){
				size_t lastColon = key.rfind(':');
				if (lastColon != std::string_view::npos) {
					std::string_view existingPK = key.substr(lastColon + 1);
					if (existingPK == pk) {
						txn.del(std::string(key));
					}
				}
				return true;
			});
		}
		// Partial index entries löschen (via Scan, da Wert unbekannt)
		for (const auto& [pcol, ppred] : partialCols) {
			std::string pprefix = makePartialIndexPrefix(table, pcol);
			db_.scanPrefix(pprefix, [&pk, &txn](std::string_view key, std::string_view) {
				size_t lastColon = key.rfind(':');
				if (lastColon != std::string_view::npos && key.substr(lastColon + 1) == pk) {
					txn.del(std::string(key));
				}
				return true;
			});
		}
		return Status::OK();
	}

	// Zielgerichtet löschen basierend auf alten Feldwerten.
	// Metadata sets were materialized once above to avoid deferred state captures.

	for (const auto& col : indexedCols) {
		if (col.find('+') == std::string::npos) {
			// Single-Column
			auto maybe = oldEntityOpt->extractField(col);
			if (!maybe) {
			  continue;
			}
			const std::string encodedVal = encodeKeyComponent(*maybe);
			const std::string idxKey = KeySchema::makeSecondaryIndexKey(table, col, encodedVal, pk);
			txn.del(idxKey);
			// Auch Range-Index-Eintrag löschen, falls vorhanden
			if (rangeCols.find(col) != rangeCols.end()) {
				const std::string rkey = makeRangeIndexKey(table, col, *maybe, pk);
				txn.del(rkey);
			}
		} else {
			// Composite
			std::vector<std::string> columns;
			size_t start = 0;
			while (start < col.size()) {
				size_t pos = col.find('+', start);
				if (pos == std::string::npos) {
					columns.emplace_back(col.substr(start));
					break;
				}
				columns.emplace_back(col.substr(start, pos - start));
				start = pos + 1;
			}
			
			std::vector<std::string> values = {};

			values.reserve(columns.size());
			bool allPresent = true;
			for (const auto& c : columns) {
				auto maybe = oldEntityOpt->extractField(c);
				if (!maybe) {
					allPresent = false;
					break;
				}
				values.emplace_back(*maybe);
			}
			
			if (!allPresent) {
			  continue;
			}
			
			const std::string idxKey = makeCompositeIndexKey(table, columns, values, pk);
			txn.del(idxKey);
		}
	}

	// Zusätzlich: Range-Indizes löschen, die keine passenden Equality-Indizes haben
	{
		for (const auto& rcol : rangeCols) {
			if (indexedCols.find(rcol) != indexedCols.end()) continue; // bereits oben behandelt
			auto maybe = oldEntityOpt->extractField(rcol);
			if (!maybe) {
			  continue;
			}
			const std::string rkey = makeRangeIndexKey(table, rcol, *maybe, pk);
			txn.del(rkey);
		}
	}

	// Sparse-Indizes löschen
	{
		for (const auto& scol : sparseCols) {
			auto maybe = oldEntityOpt->extractField(scol);
			if (!maybe || isNullOrEmpty_(*maybe)) continue; // War nicht im Index
			const std::string encodedVal = encodeKeyComponent(*maybe);
			const std::string sidxKey = makeSparseIndexKey(table, scol, encodedVal, pk);
			txn.del(sidxKey);
		}
	}

	// Geo-Indizes löschen
	{
		for (const auto& gcol : geoCols) {
			std::string latField = gcol + "_lat";
			std::string lonField = gcol + "_lon";
			
			auto maybeLat = oldEntityOpt->extractField(latField);
			auto maybeLon = oldEntityOpt->extractField(lonField);
			
			if (!maybeLat || !maybeLon) {
			  continue;
			}
			
			try {
				double lat = std::stod(*maybeLat);
				double lon = std::stod(*maybeLon);
				
				std::string geohash = encodeGeohash(lat, lon);
				const std::string gidxKey = makeGeoIndexKey(table, gcol, geohash, pk);
				txn.del(gidxKey);
			} catch (...) {
				// Koordinaten waren ungültig, wahrscheinlich war kein Index-Eintrag vorhanden
				continue;
			}
		}
	}

	// TTL-Indizes löschen
	{
		for (const auto& tcol : ttlCols) {
			auto maybeValue = oldEntityOpt->extractField(tcol);
			if (!maybeValue) {
			  continue;
			}
			
			// We need to find the TTL index entry, but we don't know the exact timestamp
			// Scan the TTL index prefix and delete matching PKs
			std::string prefix = makeTTLIndexPrefix(table, tcol);
			db_.scanPrefix(prefix, [&pk, &txn](std::string_view key, std::string_view /*val*/) {
				// Extract PK from ttlidx:table:column:timestamp:PK
				size_t lastColon = key.rfind(':');
				if (lastColon != std::string_view::npos) {
					std::string_view extractedPK = key.substr(lastColon + 1);
					if (extractedPK == pk) {
						txn.del(std::string(key));
						return false; // Stop after finding the matching entry
					}
				}
				return true;
			});
		}
	}

	// Fulltext-Indizes löschen
	{
		for (const auto& fcol : fulltextCols) {
			auto maybeText = oldEntityOpt->extractField(fcol);
			if (!maybeText || isNullOrEmpty_(maybeText)) {
			  continue;
			}
			
			// Use cached fulltext config to avoid db.get + JSON parse on every upsert/delete (v1.3.5)
			FulltextConfig config = {};
			if (hasCachedMetadata) {
				auto it = fulltextConfigsCache.find(fcol);
				if (it != fulltextConfigsCache.end()) {
					const auto& c = it->second;
					config.stemming_enabled  = c.stemming_enabled;
					config.language          = c.language;
					config.stopwords_enabled = c.stopwords_enabled;
					config.stopwords         = c.stopwords;
					config.normalize_umlauts = c.normalize_umlauts;
				}
			} else {
				config = getFulltextConfig(table, fcol).value_or(FulltextConfig{});
			}
			auto tokens = tokenize(*maybeText, config);
			
			std::unordered_set<std::string> uniqueTokens(tokens.begin(), tokens.end());
			for (const auto& token : uniqueTokens) {
				if (token.empty()) {
				  continue;
				}
				const std::string ftKey = makeFulltextIndexKey(table, fcol, token, pk);
				txn.del(ftKey);
				const std::string tfKey = makeFulltextTFKey(table, fcol, token, pk);
				txn.del(tfKey);
			}
			// DocLength löschen
			const std::string dkey = makeFulltextDocLenKey(table, fcol, pk);
			txn.del(dkey);
		}
	}

	// Partial (filtered) indexes löschen
	{
		for (const auto& [pcol, ppred] : partialCols) {
			auto maybe = oldEntityOpt->extractField(pcol);
			if (!maybe) {
			  continue;
			}
			const std::string encodedVal = encodeKeyComponent(*maybe);
			const std::string pidxKey = makePartialIndexKey(table, pcol, encodedVal, pk);
			txn.del(pidxKey);
		}
	}

	return Status::OK();
}

} // namespace themis

