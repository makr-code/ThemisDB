// Vector ANN index implementation

#include "index/vector_index.h"
#include "index/secondary_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/key_schema.h"
#include "storage/base_entity.h"
#include "utils/logger.h"
#include "utils/simd_distance.h"

// EXPERIMENTAL: Lossless vector compression support
// NOTE: This is a scientific experiment and may be rolled back.
// See docs/performance/performance_vector_compression_lossless.md for details
#include "utils/lossless_vector_integration.h"

// Phase 1: Vector encryption support
#include "security/encryption.h"

#include <shared_mutex>
#ifdef IN
#undef IN
#endif

#if defined(_MSC_VER)
#include <immintrin.h>
#endif

#ifdef THEMIS_HNSW_ENABLED
#include <hnswlib/hnswlib.h>
#endif

#include <algorithm>
#include <queue>
#include <limits>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <unordered_set>
#include <nlohmann/json.hpp>

namespace themis {

VectorIndexManager::VectorIndexManager(RocksDBWrapper& db) : db_(db) {}

VectorIndexManager::~VectorIndexManager() {
	shutdown();
}

VectorIndexManager::Status VectorIndexManager::shutdown() {
	if (autoSave_ && !savePath_.empty() && !objectName_.empty() && useHnsw_) {
		THEMIS_INFO("VectorIndexManager::shutdown - Auto-saving index for '{}' to '{}'", objectName_, savePath_);
		auto status = saveIndex(savePath_);
		if (!status.ok) {
			THEMIS_WARN("VectorIndexManager::shutdown - Failed to save index: {}", status.message);
			return status;
		}
		THEMIS_INFO("VectorIndexManager::shutdown - Index saved successfully");
	}
	return Status::OK();
}

void VectorIndexManager::setAutoSavePath(const std::string& savePath, bool autoSave) {
	savePath_ = savePath;
	autoSave_ = autoSave;
	
	// Versuche existierenden Index zu laden, wenn Pfad gesetzt wird
	if (!savePath_.empty() && !objectName_.empty()) {
		namespace fs = std::filesystem;
		if (fs::exists(fs::path(savePath_) / "meta.txt")) {
			THEMIS_INFO("VectorIndexManager::setAutoSavePath - Found existing index at '{}', loading...", savePath_);
			auto loadStatus = loadIndex(savePath_);
			if (loadStatus.ok) {
				THEMIS_INFO("VectorIndexManager::setAutoSavePath - Index loaded successfully ({} vectors)", getVectorCount());
			} else {
				THEMIS_WARN("VectorIndexManager::setAutoSavePath - Failed to load index: {}", loadStatus.message);
			}
		}
	}
}

// Phase 1: Vector encryption configuration
bool VectorIndexManager::isVectorEncryptionEnabled() const {
	try {
		if (auto cfg = db_.get("config:vector")) {
			std::string s(cfg->begin(), cfg->end());
			nlohmann::json j = nlohmann::json::parse(s);
			return j.value("encryption_enabled", false);
		}
	} catch (...) {
		// If config doesn't exist or can't be parsed, default to disabled
	}
	return false;  // Default: encryption disabled (backward compatible)
}

void VectorIndexManager::setVectorEncryptionEnabled(bool enabled) {
	try {
		nlohmann::json j;
		// Read existing config if present
		if (auto cfg = db_.get("config:vector")) {
			std::string s(cfg->begin(), cfg->end());
			j = nlohmann::json::parse(s);
		}
		
		// Update encryption setting
		j["encryption_enabled"] = enabled;
		
		// Write back to database
		std::string json_str = j.dump();
		std::vector<uint8_t> data(json_str.begin(), json_str.end());
		db_.put("config:vector", data);
		
		THEMIS_INFO("VectorIndexManager: Vector encryption {}", enabled ? "ENABLED" : "DISABLED");
	} catch (const std::exception& ex) {
		THEMIS_ERROR("VectorIndexManager: Failed to set encryption config: {}", ex.what());
	}
}

float VectorIndexManager::l2(const std::vector<float>& a, const std::vector<float>& b) {
	if (a.size() != b.size()) return std::numeric_limits<float>::infinity();
	// Return squared L2 to match existing distance semantics (lower is better)
	return simd::l2_distance_sq(a.data(), b.data(), a.size());
}

float VectorIndexManager::cosineOneMinus(const std::vector<float>& a, const std::vector<float>& b) {
	float dot = 0.0f, na = 0.0f, nb = 0.0f;
	size_t n = a.size();
	size_t i = 0;
	const size_t simd_width = 8;
	
	// SIMD-optimized loop with OpenMP reduction
	#pragma omp simd reduction(+:dot,na,nb) collapse(1)
	for (; i + simd_width <= n; i += simd_width) {
		#if defined(__clang__) || defined(__GNUC__)
		#pragma unroll(8)
		#endif
		for (size_t j = 0; j < simd_width; ++j) {
			dot += a[i+j] * b[i+j];
			na += a[i+j] * a[i+j];
			nb += b[i+j] * b[i+j];
		}
	}
	
	// Remainder loop
	for (; i < n; ++i) {
		dot += a[i] * b[i];
		na += a[i] * a[i];
		nb += b[i] * b[i];
	}
	
	float denom = std::sqrt(std::max(na * nb, 1e-12f));
	float cosv = denom > 0 ? (dot / denom) : 0.0f;
	return 1.0f - cosv;
}

float VectorIndexManager::dotProduct(const std::vector<float>& a, const std::vector<float>& b) {
	float dot = 0.0f;
	size_t n = a.size();
	size_t i = 0;
	const size_t simd_width = 8;
	
	// SIMD-optimized loop with OpenMP
	#pragma omp simd reduction(+:dot)
	for (; i + simd_width <= n; i += simd_width) {
		for (size_t j = 0; j < simd_width; ++j) {
			dot += a[i+j] * b[i+j];
		}
	}
	
	// Remainder loop
	for (; i < n; ++i) {
		dot += a[i] * b[i];
	}
	
	// Return negative dot product so that "lower is better" ordering works
	// (higher dot product = more similar → negate for distance semantics)
	return -dot;
}

void VectorIndexManager::normalizeL2(std::vector<float>& v) {
	float n2 = 0.0f;
	for (float x : v) n2 += x * x;
	float n = std::sqrt(std::max(n2, 1e-12f));
	if (n > 0.f) {
		for (float& x : v) x /= n;
	}
}

float VectorIndexManager::distance(const std::vector<float>& a, const std::vector<float>& b) const {
	if (metric_ == Metric::L2) return l2(a, b);
	if (metric_ == Metric::DOT) return dotProduct(a, b);
	return cosineOneMinus(a, b); // COSINE
}

std::string VectorIndexManager::makeObjectKey(std::string_view pk) const {
	return KeySchema::makeVectorKey(objectName_, pk);
}

VectorIndexManager::Status VectorIndexManager::init(std::string_view objectName, int dim, Metric metric,
													int M, int efConstruction, int efSearch,
													const std::string& savePath) {
	if (objectName.empty()) return Status::Error("init: objectName darf nicht leer sein");
	if (dim <= 0) return Status::Error("init: dim muss > 0 sein");
	objectName_ = std::string(objectName);
	dim_ = dim;
	metric_ = metric;
	efSearch_ = efSearch;
    m_ = M;
    efConstruction_ = efConstruction;
	// Apply save path only if provided; otherwise keep previously configured path
	if (!savePath.empty()) {
		savePath_ = savePath;
		autoSave_ = true;
	}

	// Versuche Index zu laden, falls vorhanden (savePath kann aus Param oder vorheriger Konfiguration stammen)
	if (!savePath_.empty()) {
		namespace fs = std::filesystem;
		if (fs::exists(fs::path(savePath_) / "meta.txt")) {
			THEMIS_INFO("VectorIndexManager::init - Found existing index at '{}', loading...", savePath_);
			auto loadStatus = loadIndex(savePath_);
			if (loadStatus.ok) {
				THEMIS_INFO("VectorIndexManager::init - Index loaded successfully ({} vectors)", getVectorCount());
				return Status::OK();
			} else {
				THEMIS_WARN("VectorIndexManager::init - Failed to load index: {}, creating new index", loadStatus.message);
			}
		}
	}

#ifdef THEMIS_HNSW_ENABLED
	try {
		hnswlib::SpaceInterface<float>* space = nullptr;
		if (metric == Metric::L2) {
			space = new hnswlib::L2Space(dim);
		} else if (metric == Metric::DOT) {
			// DOT uses InnerProductSpace (same as COSINE, but without normalization)
			space = new hnswlib::InnerProductSpace(dim);
		} else { // COSINE
			space = new hnswlib::InnerProductSpace(dim);
		}
		auto* appr = new hnswlib::HierarchicalNSW<float>(space, 1000 /*initial*/, M, efConstruction);
		appr->ef_ = efSearch;
		hnswIndex_ = static_cast<void*>(appr);
		useHnsw_ = true;
	} catch (...) {
		useHnsw_ = false;
		THEMIS_WARN("init: HNSW initialisierung fehlgeschlagen, Fallback auf Brute-Force");
	}
#else
	useHnsw_ = false;
#endif
	return Status::OK();
}

	VectorIndexManager::Status VectorIndexManager::setEfSearch(int efSearch) {
		if (efSearch <= 0) return Status::Error("setEfSearch: efSearch muss > 0 sein");
		efSearch_ = efSearch;
	#ifdef THEMIS_HNSW_ENABLED
		if (useHnsw_) {
			try {
				auto* appr = static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
				appr->ef_ = efSearch_;
			} catch (...) {
				return Status::Error("setEfSearch: HNSW ef_-Update fehlgeschlagen");
			}
		}
	#endif
		return Status::OK();
	}

VectorIndexManager::Status VectorIndexManager::rebuildFromStorage() {
	if (objectName_.empty() || dim_ <= 0) return Status::Error("rebuildFromStorage: Manager nicht initialisiert");
	cache_.clear();
	pkToId_.clear();
	idToPk_.clear();

	const std::string prefix = objectName_ + ":"; // KeySchema::makeVectorKey(object, pk) = object:pk
	size_t nextId = 0;
	db_.scanPrefix(prefix, [&](std::string_view key, std::string_view value) {
		std::string pk = KeySchema::extractPrimaryKey(key);
		std::vector<uint8_t> bytes(value.begin(), value.end());
		try {
			BaseEntity e = BaseEntity::deserialize(pk, bytes);
			
			std::vector<float> v;
			
			// Phase 1: Try encrypted vector first
			auto encFieldOpt = e.getField("embedding_encrypted");
			if (encFieldOpt) {
				try {
					// Extract encrypted base64 string
					const auto* enc_str = std::get_if<std::string>(&(*encFieldOpt));
					if (enc_str && !enc_str->empty()) {
						// Decrypt using EncryptedField
						auto enc_field = EncryptedField<std::vector<float>>::fromBase64(*enc_str);
						v = enc_field.decrypt();
						THEMIS_DEBUG("rebuildFromStorage: Decrypted vector for pk={}", pk);
					}
				} catch (const std::exception& ex) {
					THEMIS_WARN("rebuildFromStorage: Failed to decrypt vector for pk={}: {}", pk, ex.what());
					return true;  // Skip this entity, continue to next
				}
			}
			// EXPERIMENTAL: Try lossless decompression
			else if (auto losslessVec = experimental::VectorCompressionHelper::decompressVector(e); losslessVec.has_value()) {
				// EXPERIMENTAL: Use lossless decompressed vector
				v = std::move(*losslessVec);
				THEMIS_DEBUG("rebuildFromStorage: Using experimental lossless decompression for pk={}", pk);
			}
			// EXISTING IMPLEMENTATION (preserved, not deleted)
			else {
				auto vecOpt = e.extractVector("embedding");
				if (vecOpt && vecOpt->size() == static_cast<size_t>(dim_)) {
					v = *vecOpt;
				} else {
					// Try SQ8-coded embedding (EXISTING)
					auto qbufOpt = e.getField("embedding_q");
					auto scaleOpt = e.getFieldAsDouble("embedding_scale");
					if (!qbufOpt || !scaleOpt) return true;
					const auto* qv = std::get_if<std::vector<uint8_t>>(&(*qbufOpt));
					if (!qv || qv->size() != static_cast<size_t>(dim_)) return true;
					v.resize(dim_);
					float s = static_cast<float>(*scaleOpt);
					for (size_t i = 0; i < qv->size(); ++i) {
						int8_t code = static_cast<int8_t>((*qv)[i]);
						v[i] = static_cast<float>(code) * s;
					}
				}
			}
			
			// Normalize only for COSINE (not for DOT or L2)
			if (metric_ == Metric::COSINE) normalizeL2(v);
			cache_[pk] = v;
			if (useHnsw_) {
#ifdef THEMIS_HNSW_ENABLED
				auto* appr = static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
				if (pkToId_.count(pk) == 0) {
					pkToId_[pk] = nextId;
					idToPk_.push_back(pk);
					appr->addPoint(v.data(), nextId);
					++nextId;
				}
#endif
			} else {
				// Fallback: nur Cache
			}
		} catch (...) {
			THEMIS_WARN("rebuildFromStorage: Deserialisierung fehlgeschlagen für PK={}", pk);
		}
		return true;
	});
	return Status::OK();
}

VectorIndexManager::Status VectorIndexManager::addEntity(const BaseEntity& e, std::string_view vectorField) {
	if (objectName_.empty()) return Status::Error("addEntity: Manager nicht initialisiert");
	const std::string& pk = e.getPrimaryKey();
	auto v = e.extractVector(vectorField);
	if (!v) return Status::Error("addEntity: Vektor-Feld fehlt oder hat falsches Format");
	
	// Phase 1: Check if encryption is enabled
	bool encryptVectors = isVectorEncryptionEnabled();
	
	// EXPERIMENTAL: Try lossless compression first (if enabled)
	// NOTE: Scientific experiment - may be rolled back
	// Priority: Encryption > Lossless > SQ8 > Raw storage
	auto losslessCompressed = experimental::VectorCompressionHelper::tryLosslessCompression(e, *v, db_);
	if (!losslessCompressed.has_value()) {
		THEMIS_DEBUG("VectorIndexManager: Lossless compression not applicable for pk={}, falling back", pk);
	}
	
	// Decide on SQ8 quantization based on config in DB
	// NOTE: This is the EXISTING implementation (preserved, not deleted)
	// If encryption is enabled, quantization is disabled (they are mutually exclusive)
	// If lossless compression succeeded, this code path is skipped
	auto shouldQuantize = [&]() -> bool {
		if (encryptVectors) return false; // Disable quantization when encryption is enabled
		if (losslessCompressed.has_value()) return false; // Lossless takes precedence
		
		std::string mode = "auto"; int64_t threshold = 1000000;
		try {
			if (auto cfg = db_.get("config:vector")) {
				std::string s(cfg->begin(), cfg->end());
				nlohmann::json j = nlohmann::json::parse(s);
				mode = j.value("quantization", std::string("auto"));
				threshold = j.value("auto_threshold", 1000000);
			}
		} catch (...) {}
		if (mode == "none") return false;
		if (mode == "sq8") return true;
		return static_cast<int64_t>(getVectorCount()) >= threshold;
	}();


	// Persistenz in RocksDB
	std::string key = makeObjectKey(pk);
	std::vector<uint8_t> serialized;
	
	if (encryptVectors) {
		// Phase 1: Encrypt vector and store
		// Note: Uses global FieldEncryption state set via EncryptedField::setFieldEncryption()
		// This pattern is consistent with existing EncryptedField usage throughout the codebase
		try {
			EncryptedField<std::vector<float>> enc_field;
			enc_field.encrypt(*v, vectorKeyId_);
			
			// Create storage entity with encrypted vector
			auto fields = e.getAllFields();
			fields.erase(std::string(vectorField));  // Remove plaintext vector
			fields["embedding_encrypted"] = enc_field.toBase64();
			
			BaseEntity encrypted_entity = BaseEntity::fromFields(pk, fields);
			serialized = encrypted_entity.serialize();
			
			THEMIS_DEBUG("VectorIndexManager: Encrypted vector for pk={}", pk);
		} catch (const std::exception& ex) {
			return Status::Error("addEntity: Vector encryption failed: " + std::string(ex.what()));
		}
	} else if (losslessCompressed.has_value()) {
		// EXPERIMENTAL: Use lossless compressed data
		serialized = std::move(*losslessCompressed);
		THEMIS_DEBUG("VectorIndexManager: Using experimental lossless compression for pk={}", pk);
	} else if (shouldQuantize) {
		// EXISTING SQ8 IMPLEMENTATION (preserved, not deleted)
		float amax = 0.0f; for (float x : *v) amax = std::max(amax, std::fabs(x));
		float scale = (amax > 0.f) ? (amax / 127.0f) : 1.0f;
		std::vector<uint8_t> codes(v->size());
		for (size_t i = 0; i < v->size(); ++i) {
			int q = static_cast<int>(std::round((*v)[i] / scale));
			q = std::max(-127, std::min(127, q));
			codes[i] = static_cast<uint8_t>(static_cast<int8_t>(q));
		}
		auto fields = e.getAllFields();
		fields.erase("embedding");
		fields["embedding_q"] = codes;
		fields["embedding_scale"] = static_cast<double>(scale);
		BaseEntity eq = BaseEntity::fromFields(pk, fields);
		serialized = eq.serialize();
	} else {
		// EXISTING RAW STORAGE (preserved, not deleted)
		serialized = e.serialize();
	}
	
	if (!db_.put(key, serialized)) {
		return Status::Error("addEntity: RocksDB put fehlgeschlagen");
	}

	// In-Memory Cache aktualisieren (nur COSINE normalisiert; DOT/L2 bleiben raw)
	std::vector<float> vv = *v;
	if (metric_ == Metric::COSINE) normalizeL2(vv);
	cache_[pk] = vv;
#ifdef THEMIS_HNSW_ENABLED
	if (useHnsw_) {
		auto* appr = static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
		size_t id;
		auto it = pkToId_.find(pk);
		if (it == pkToId_.end()) {
			id = idToPk_.size();
			pkToId_[pk] = id;
			idToPk_.push_back(pk);
		} else {
			id = it->second;
		}
		try { appr->addPoint(cache_[pk].data(), id); } catch (...) { /* evtl. schon vorhanden */ }
	}
#endif
	return Status::OK();
}

VectorIndexManager::Status VectorIndexManager::addEntity(const BaseEntity& e, RocksDBWrapper::WriteBatchWrapper& batch,
                                                          std::string_view vectorField) {
	if (objectName_.empty()) return Status::Error("addEntity: Manager nicht initialisiert");
	const std::string& pk = e.getPrimaryKey();
	auto v = e.extractVector(vectorField);
	if (!v) return Status::Error("addEntity: Vektor-Feld fehlt oder hat falsches Format");
	if (v->size() != static_cast<size_t>(dim_)) return Status::Error("addEntity: Vektordimension passt nicht");

	// Persistenz via WriteBatch (für Transaktionen)
	auto shouldQuantize = [&]() -> bool {
		std::string mode = "auto"; int64_t threshold = 1000000;
		try {
			if (auto cfg = db_.get("config:vector")) {
				std::string s(cfg->begin(), cfg->end());
				nlohmann::json j = nlohmann::json::parse(s);
				mode = j.value("quantization", std::string("auto"));
				threshold = j.value("auto_threshold", 1000000);
			}
		} catch (...) {}
		if (mode == "none") return false; if (mode == "sq8") return true;
		return static_cast<int64_t>(getVectorCount()) >= threshold;
	}();
	std::string key = makeObjectKey(pk);
	std::vector<uint8_t> serialized;
	if (shouldQuantize) {
		float amax = 0.0f; for (float x : *v) amax = std::max(amax, std::fabs(x));
		float scale = (amax > 0.f) ? (amax / 127.0f) : 1.0f;
		std::vector<uint8_t> codes(v->size());
		for (size_t i = 0; i < v->size(); ++i) {
			int q = static_cast<int>(std::round((*v)[i] / scale));
			q = std::max(-127, std::min(127, q));
			codes[i] = static_cast<uint8_t>(static_cast<int8_t>(q));
		}
		auto fields = e.getAllFields();
		fields.erase("embedding");
		fields["embedding_q"] = codes;
		fields["embedding_scale"] = static_cast<double>(scale);
		BaseEntity eq = BaseEntity::fromFields(pk, fields);
		serialized = eq.serialize();
	} else {
		serialized = e.serialize();
	}
	batch.put(key, serialized);

	// In-Memory Cache aktualisieren (nur COSINE normalisiert; DOT/L2 bleiben raw)
	std::vector<float> vv = *v;
	if (metric_ == Metric::COSINE) normalizeL2(vv);
	cache_[pk] = vv;
#ifdef THEMIS_HNSW_ENABLED
	if (useHnsw_) {
		auto* appr = static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
		size_t id;
		auto it = pkToId_.find(pk);
		if (it == pkToId_.end()) {
			id = idToPk_.size();
			pkToId_[pk] = id;
			idToPk_.push_back(pk);
		} else {
			id = it->second;
		}
		try { appr->addPoint(cache_[pk].data(), id); } catch (...) { /* evtl. schon vorhanden */ }
	}
#endif
	return Status::OK();
}

VectorIndexManager::Status VectorIndexManager::updateEntity(const BaseEntity& e, std::string_view vectorField) {
	// einfache Strategie: remove + add
	auto r = removeByPk(e.getPrimaryKey());
	if (!r.ok) THEMIS_WARN("updateEntity: remove warn: {}", r.message);
	return addEntity(e, vectorField);
}

VectorIndexManager::Status VectorIndexManager::updateEntity(const BaseEntity& e, RocksDBWrapper::WriteBatchWrapper& batch,
                                                             std::string_view vectorField) {
	// einfache Strategie: remove + add (beide via Batch)
	auto r = removeByPk(e.getPrimaryKey(), batch);
	if (!r.ok) THEMIS_WARN("updateEntity: remove warn: {}", r.message);
	return addEntity(e, batch, vectorField);
}

VectorIndexManager::Status VectorIndexManager::removeByPk(std::string_view pk) {
	// RocksDB löschen
	std::string key = makeObjectKey(pk);
	if (!db_.del(key)) {
		THEMIS_WARN("removeByPk: RocksDB delete fehlgeschlagen für key={}", key);
	}

	// In-Memory Cache löschen
	cache_.erase(std::string(pk));
#ifdef THEMIS_HNSW_ENABLED
	if (useHnsw_) {
		auto* appr = static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
		auto it = pkToId_.find(std::string(pk));
		if (it != pkToId_.end()) {
			try { appr->markDelete(it->second); } catch (...) {}
		}
	}
#endif
	return Status::OK();
}

VectorIndexManager::Status VectorIndexManager::removeByPk(std::string_view pk, RocksDBWrapper::WriteBatchWrapper& batch) {
	// RocksDB löschen via WriteBatch
	std::string key = makeObjectKey(pk);
	batch.del(key);

	// In-Memory Cache löschen
	cache_.erase(std::string(pk));
#ifdef THEMIS_HNSW_ENABLED
	if (useHnsw_) {
		auto* appr = static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
		auto it = pkToId_.find(std::string(pk));
		if (it != pkToId_.end()) {
			try { appr->markDelete(it->second); } catch (...) {}
		}
	}
#endif
	return Status::OK();
}

std::vector<VectorIndexManager::Result>
VectorIndexManager::bruteForceSearch_(const std::vector<float>& query, size_t k,
									  const std::vector<std::string>* whitelist) const {
	// Cache-aware optimized implementation with prefetching and partial sort
	std::vector<Result> heap;
	heap.reserve(k * 2);  // Reserve extra space to reduce reallocations
	float threshold = std::numeric_limits<float>::infinity();

	auto prefetch = [](const void* ptr) {
		#if defined(_MSC_VER)
			_mm_prefetch(reinterpret_cast<const char*>(ptr), _MM_HINT_T0);
		#elif defined(__GNUC__) || defined(__clang__)
			__builtin_prefetch(ptr, 0, 3);
		#else
			(void)ptr;
		#endif
	};
	
	auto consider = [&](const std::string& pk, const std::vector<float>& vec) {
		// Prefetch next vector for cache locality
		prefetch(&vec.front());
		
		float dist = distance(query, vec);
		
		// Skip vectors that can't possibly be in top-k
		if (dist > threshold && heap.size() >= k) {
			return;
		}
		
		heap.push_back({pk, dist});
		
		// Update threshold periodically using nth_element (partial sort)
		if (heap.size() >= k && heap.size() % 32 == 0) {
			std::nth_element(heap.begin(), heap.begin() + k, heap.end(),
				[](const Result& a, const Result& b) { return a.distance < b.distance; });
			threshold = heap[k-1].distance;
			heap.resize(k);
		}
	};

	if (whitelist && !whitelist->empty()) {
		for (const auto& pk : *whitelist) {
			auto it = cache_.find(pk);
			if (it != cache_.end() && it->second.size() == static_cast<size_t>(dim_)) {
				consider(pk, it->second);
			} else {
				// Lade aus Storage on-demand
				auto blob = db_.get(makeObjectKey(pk));
				if (!blob) continue;
				try {
					BaseEntity e = BaseEntity::deserialize(pk, *blob);
					auto vec = e.extractVector("embedding");
					if (vec && vec->size() == static_cast<size_t>(dim_)) {
						consider(pk, *vec);
					} else {
						auto qbufOpt = e.getField("embedding_q");
						auto scaleOpt = e.getFieldAsDouble("embedding_scale");
						if (qbufOpt && scaleOpt) {
							const auto* by = std::get_if<std::vector<uint8_t>>(&(*qbufOpt));
							if (by && by->size() == static_cast<size_t>(dim_)) {
								std::vector<float> v(dim_);
								float s = static_cast<float>(*scaleOpt);
								for (size_t i = 0; i < by->size(); ++i) {
									int8_t code = static_cast<int8_t>((*by)[i]);
									v[i] = static_cast<float>(code) * s;
								}
								consider(pk, v);
							}
						}
					}
				} catch (...) {}
			}
		}
	} else {
		for (const auto& [pk, vec] : cache_) {
			if (vec.size() == static_cast<size_t>(dim_)) consider(pk, vec);
		}
	}
	
	// Final partial sort: O(n log k) instead of O(n log n)
	if (heap.size() > k) {
		std::partial_sort(heap.begin(), heap.begin() + k, heap.end(),
			[](const Result& a, const Result& b) { return a.distance < b.distance; });
		heap.resize(k);
	} else {
		std::sort(heap.begin(), heap.end(),
			[](const Result& a, const Result& b) { return a.distance < b.distance; });
	}
	
	return heap;
}

std::pair<VectorIndexManager::Status, std::vector<VectorIndexManager::Result>>
VectorIndexManager::searchKnn(const std::vector<float>& query, size_t k, const std::vector<std::string>* whitelist) const {
	if (query.size() != static_cast<size_t>(dim_)) {
		return {Status::Error("searchKnn: Query-Dimension passt nicht"), {}};
	}

#ifdef THEMIS_HNSW_ENABLED
	// Fall 1: HNSW-Suche ohne Whitelist
	if (useHnsw_ && (!whitelist || whitelist->empty())) {
		try {
			auto* appr = static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
			std::vector<float> q = query;
			if (metric_ == Metric::COSINE) normalizeL2(q);
			auto topk = appr->searchKnn(q.data(), static_cast<size_t>(k));
			std::vector<Result> out;
			out.reserve(topk.size());
			while (!topk.empty()) {
				auto p = topk.top();
				topk.pop();
				size_t id = p.second;
				float d = p.first;
				if (id < idToPk_.size()) out.push_back({idToPk_[id], d});
			}
			std::reverse(out.begin(), out.end()); // kleinste Distanz zuerst
			return {Status::OK(), std::move(out)};
		} catch (...) {
			THEMIS_WARN("searchKnn: HNSW-Suche fehlgeschlagen, Fallback auf Brute-Force");
		}
	}

	// Fall 2: HNSW vorhanden + Whitelist → iterativ Kandidaten vergrößern und filtern
	if (useHnsw_ && whitelist && !whitelist->empty()) {
		try {
			auto* appr = static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
			std::vector<float> q = query;
			if (metric_ == Metric::COSINE) normalizeL2(q);

			std::unordered_set<std::string> wl(whitelist->begin(), whitelist->end());
			// Optimierte Prefilter-Parameter für Memory-Bandwidth-Constraints
			int initialFactor = 2;           // k * initialFactor (reduziert von 3)
			int minCandidatesFloor = 16;     // Untergrenze (reduziert von 32)
			int maxAttempts = 4;             // Iterationen
			double growthFactor = 1.5;       // Multiplikator (reduziert von 2.0 für sanfteres Wachstum)
			bool prefilterEnabled = true;    // Kann deaktiviert werden
			try {
				if (auto cfgBlob = db_.get("config:vector")) {
					std::string s(cfgBlob->begin(), cfgBlob->end());
					auto j = nlohmann::json::parse(s);
					prefilterEnabled = j.value("whitelist_prefilter_enabled", true);
					initialFactor = j.value("whitelist_initial_factor", 3);
					minCandidatesFloor = j.value("whitelist_min_candidates", 32);
					maxAttempts = j.value("whitelist_max_attempts", 4);
					growthFactor = j.value("whitelist_growth_factor", 2.0);
				}
			} catch (...) {
				// Ignoriere Parsingfehler und nutze Defaults
			}

			if (!prefilterEnabled) {
				// Prefilter deaktiviert → exakter (langsamer) Brute-Force über Whitelist
				THEMIS_INFO("searchKnn: Prefilter deaktiviert, Brute-Force über Whitelist");
				auto bf = bruteForceSearch_(query, k, whitelist);
				return {Status::OK(), std::move(bf)};
			}

			// Starte mit konfigurierter Kandidatenanzahl und wachse bis ausreichend Treffer
			// Adaptive Begrenzung: Cap candidate count to memory-efficient size
			size_t candidateCount = std::max(static_cast<size_t>(k * initialFactor), static_cast<size_t>(std::max(efSearch_ * 2, minCandidatesFloor)));
			
			// Cap to whitelist size * 2 to avoid excessive memory bandwidth
			if (whitelist && !whitelist->empty()) {
				candidateCount = std::min(candidateCount, whitelist->size() * 2);
			}

			std::vector<Result> filtered;
			filtered.reserve(k);
			std::unordered_set<std::string> seen;

			for (size_t attempt = 0; attempt < static_cast<size_t>(maxAttempts) && filtered.size() < k; ++attempt) {
				auto top = appr->searchKnn(q.data(), candidateCount);
				std::vector<Result> tmp;
				tmp.reserve(top.size());
				while (!top.empty()) {
					auto p = top.top();
					top.pop();
					size_t id = p.second;
					float d = p.first;
					if (id < idToPk_.size()) {
						const std::string& pk = idToPk_[id];
						if (wl.find(pk) != wl.end()) {
							tmp.push_back({pk, d});
						}
					}
				}
				std::reverse(tmp.begin(), tmp.end()); // kleinste Distanz zuerst
				
				// Early termination if we have enough candidates
				if (tmp.size() >= k) {
					filtered.insert(filtered.end(), tmp.begin(), tmp.begin() + k);
					break;
				}

				for (const auto& r : tmp) {
					if (seen.insert(r.pk).second) {
						filtered.push_back(r);
						if (filtered.size() >= k) break;
					}
				}

				// Nächster Versuch mit Wachstum
				candidateCount = static_cast<size_t>(candidateCount * growthFactor);
			}

			if (filtered.size() >= k) {
				if (filtered.size() > k) filtered.resize(k);
				return {Status::OK(), std::move(filtered)};
			}
			// Wenn nicht genügend Treffer: Fallback für Rest via Brute-Force über Whitelist (korrekt und vollständig)
			THEMIS_INFO("searchKnn: HNSW+Whitelist lieferte nur {} von {} – ergänze via Brute-Force", filtered.size(), k);
			auto bf = bruteForceSearch_(query, k, whitelist);
			return {Status::OK(), std::move(bf)};
		} catch (...) {
			THEMIS_WARN("searchKnn: HNSW-Whitelist-Suche fehlgeschlagen, Fallback auf Brute-Force");
			// weiter unten erfolgt Brute-Force
		}
	}
#endif
	// Fallback oder Whitelist-Fall: Brute-Force
	return {Status::OK(), bruteForceSearch_(query, k, whitelist)};
}

// =============================================================================
// Filtered KNN Search with Attribute Filtering (Post-Filtering)
// =============================================================================

std::pair<VectorIndexManager::Status, std::vector<VectorIndexManager::Result>>
VectorIndexManager::searchKnnFiltered(
	const std::vector<float>& query,
	size_t k,
	const std::vector<AttributeFilter>& filters,
	size_t candidateMultiplier
) const {
	if (query.size() != static_cast<size_t>(dim_)) {
		return {Status::Error("searchKnnFiltered: Query-Dimension passt nicht"), {}};
	}

	if (filters.empty()) {
		// No filters: fallback to standard KNN
		return searchKnn(query, k, nullptr);
	}

	// Strategy: Fetch k * candidateMultiplier candidates from HNSW, then post-filter
	size_t candidateCount = k * candidateMultiplier;

#ifdef THEMIS_HNSW_ENABLED
	if (useHnsw_) {
		try {
			auto* appr = static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
			std::vector<float> q = query;
			if (metric_ == Metric::COSINE) normalizeL2(q);
			
			auto topk = appr->searchKnn(q.data(), candidateCount);
			std::vector<Result> candidates;
			candidates.reserve(topk.size());
			
			while (!topk.empty()) {
				auto p = topk.top();
				topk.pop();
				size_t id = p.second;
				float d = p.first;
				if (id < idToPk_.size()) {
					candidates.push_back({idToPk_[id], d});
				}
			}
			std::reverse(candidates.begin(), candidates.end());
			
			// Post-filter: Load entities and check attributes
			std::vector<Result> filtered;
			for (const auto& candidate : candidates) {
				std::string objKey = makeObjectKey(candidate.pk);
				auto entity_opt = db_.get(objKey);
				if (!entity_opt) continue;
				
				BaseEntity entity = BaseEntity::deserialize(candidate.pk, entity_opt.value());
				
				// Apply all filters
				bool passes = true;
				for (const auto& filter : filters) {
					auto val_opt = entity.getFieldAsString(filter.field);
					if (!val_opt.has_value()) {
						passes = false;
						break;
					}
					
					std::string fieldValue = val_opt.value();
					
					switch (filter.op) {
						case AttributeFilter::Op::EQUALS:
							if (fieldValue != filter.value) passes = false;
							break;
						case AttributeFilter::Op::NOT_EQUALS:
							if (fieldValue == filter.value) passes = false;
							break;
						case AttributeFilter::Op::CONTAINS:
							if (fieldValue.find(filter.value) == std::string::npos) passes = false;
							break;
					}
					
					if (!passes) break;
				}
				
				if (passes) {
					filtered.push_back(candidate);
					if (filtered.size() >= k) break;
				}
			}
			
			return {Status::OK(), std::move(filtered)};
			
		} catch (const std::exception& ex) {
			THEMIS_WARN("searchKnnFiltered: HNSW-Suche fehlgeschlagen: {}", ex.what());
			return {Status::Error(std::string("HNSW exception: ") + ex.what()), {}};
		}
	}
#endif

	// Fallback: Brute-force with filtering
	std::vector<Result> allResults = bruteForceSearch_(query, candidateCount, nullptr);
	std::vector<Result> filtered;
	
	for (const auto& candidate : allResults) {
		std::string objKey = makeObjectKey(candidate.pk);
		auto entity_opt = db_.get(objKey);
		if (!entity_opt) continue;
		
		BaseEntity entity = BaseEntity::deserialize(candidate.pk, entity_opt.value());
		
		bool passes = true;
		for (const auto& filter : filters) {
			auto val_opt = entity.getFieldAsString(filter.field);
			if (!val_opt.has_value()) {
				passes = false;
				break;
			}
			
			std::string fieldValue = val_opt.value();
			
			switch (filter.op) {
				case AttributeFilter::Op::EQUALS:
					if (fieldValue != filter.value) passes = false;
					break;
				case AttributeFilter::Op::NOT_EQUALS:
					if (fieldValue == filter.value) passes = false;
					break;
				case AttributeFilter::Op::CONTAINS:
					if (fieldValue.find(filter.value) == std::string::npos) passes = false;
					break;
			}
			
			if (!passes) break;
		}
		
		if (passes) {
			filtered.push_back(candidate);
			if (filtered.size() >= k) break;
		}
	}
	
	return {Status::OK(), std::move(filtered)};
}

// =============================================================================
// Pre-Filtered Vector Search (AttributeFilterV2 + SecondaryIndexManager)
// =============================================================================

std::pair<VectorIndexManager::Status, std::vector<VectorIndexManager::Result>>
VectorIndexManager::searchKnnPreFiltered(
	const std::vector<float>& query,
	size_t k,
	const std::vector<AttributeFilterV2>& filters,
	SecondaryIndexManager* secondaryIdx
) const {
	if (query.size() != static_cast<size_t>(dim_)) {
		return {Status::Error("searchKnnPreFiltered: Query-Dimension passt nicht"), {}};
	}

	if (filters.empty()) {
		// No filters: standard KNN
		return searchKnn(query, k, nullptr);
	}

	if (!secondaryIdx) {
		// No SecondaryIndexManager: fallback to post-filtering
		THEMIS_WARN("searchKnnPreFiltered: SecondaryIndexManager nicht verf\u00fcgbar, Fallback auf Post-Filtering");
		std::vector<AttributeFilter> legacyFilters;
		for (const auto& f : filters) {
			if (f.op == AttributeFilterV2::Op::EQUALS) {
				legacyFilters.push_back({f.field, f.value, AttributeFilter::Op::EQUALS});
			}
		}
		return searchKnnFiltered(query, k, legacyFilters, 3);
	}

	// Strategy: Generate whitelist from SecondaryIndex scans, then HNSW with whitelist
	std::vector<std::string> whitelist;
	std::unordered_set<std::string> whitelistSet;

	// Read config for max filter scan size
	size_t maxFilterScanSize = 100000; // Default: 100k entities
	try {
		if (auto cfgBlob = db_.get("config:vector")) {
			std::string s(cfgBlob->begin(), cfgBlob->end());
			auto j = nlohmann::json::parse(s);
			maxFilterScanSize = j.value("max_filter_scan_size", 100000);
		}
	} catch (...) {
		// Ignore parse errors, use default
	}

	// Process filters and build whitelist
	bool isFirstFilter = true;
	for (const auto& filter : filters) {
		std::vector<std::string> filterResults;

		switch (filter.op) {
			case AttributeFilterV2::Op::EQUALS: {
				auto [st, pks] = secondaryIdx->scanKeysEqual(objectName_, filter.field, filter.value);
				if (st.ok) {
					filterResults = std::move(pks);
				} else {
					THEMIS_WARN("searchKnnPreFiltered: SecondaryIndex scan failed for {}={}: {}", 
						filter.field, filter.value, st.message);
					// Continue with empty result for this filter
				}
				break;
			}
			case AttributeFilterV2::Op::RANGE: {
				auto [st, pks] = secondaryIdx->scanKeysRange(
					objectName_, 
					filter.field,
					std::optional<std::string>(filter.value_min),
					std::optional<std::string>(filter.value_max),
					true,  // includeLower
					true,  // includeUpper
					maxFilterScanSize
				);
				if (st.ok) {
					filterResults = std::move(pks);
				} else {
					THEMIS_WARN("searchKnnPreFiltered: Range scan failed for {} [{}, {}]: {}", 
						filter.field, filter.value_min, filter.value_max, st.message);
				}
				break;
			}
			case AttributeFilterV2::Op::IN: {
				std::unordered_set<std::string> inResults;
				for (const auto& val : filter.values) {
					auto [st, pks] = secondaryIdx->scanKeysEqual(objectName_, filter.field, val);
					if (st.ok) {
						inResults.insert(pks.begin(), pks.end());
					}
				}
				filterResults.assign(inResults.begin(), inResults.end());
				break;
			}
			case AttributeFilterV2::Op::GREATER_THAN:
			case AttributeFilterV2::Op::GREATER_EQUAL:
			case AttributeFilterV2::Op::LESS_THAN:
			case AttributeFilterV2::Op::LESS_EQUAL: {
				// Use range scan with appropriate bounds
				std::optional<std::string> lower, upper;
				bool includeLower = false, includeUpper = false;

				if (filter.op == AttributeFilterV2::Op::GREATER_THAN) {
					lower = filter.value;
					includeLower = false;
				} else if (filter.op == AttributeFilterV2::Op::GREATER_EQUAL) {
					lower = filter.value;
					includeLower = true;
				} else if (filter.op == AttributeFilterV2::Op::LESS_THAN) {
					upper = filter.value;
					includeUpper = false;
				} else { // LESS_EQUAL
					upper = filter.value;
					includeUpper = true;
				}

				auto [st, pks] = secondaryIdx->scanKeysRange(
					objectName_, 
					filter.field,
					lower,
					upper,
					includeLower,
					includeUpper,
					maxFilterScanSize
				);
				if (st.ok) {
					filterResults = std::move(pks);
				}
				break;
			}
			case AttributeFilterV2::Op::NOT_EQUALS:
			case AttributeFilterV2::Op::CONTAINS: {
				// These require full scan, not supported for pre-filtering
				THEMIS_WARN("searchKnnPreFiltered: {} operator requires post-filtering, skipping", 
					filter.op == AttributeFilterV2::Op::NOT_EQUALS ? "NOT_EQUALS" : "CONTAINS");
				continue;
			}
		}

		// Intersect with existing whitelist (AND logic)
		if (isFirstFilter) {
			whitelistSet.insert(filterResults.begin(), filterResults.end());
			isFirstFilter = false;
		} else {
			std::unordered_set<std::string> intersection;
			for (const auto& pk : filterResults) {
				if (whitelistSet.count(pk)) {
					intersection.insert(pk);
				}
			}
			whitelistSet = std::move(intersection);
		}

		// Early exit if whitelist is empty
		if (whitelistSet.empty()) {
			THEMIS_INFO("searchKnnPreFiltered: Whitelist empty after filter on {}", filter.field);
			return {Status::OK(), {}};
		}
	}

	// Convert set to vector for searchKnn
	whitelist.assign(whitelistSet.begin(), whitelistSet.end());

	THEMIS_INFO("searchKnnPreFiltered: Generated whitelist with {} candidates from {} filters", 
		whitelist.size(), filters.size());

	// Check if whitelist is too large (inefficient for HNSW prefilter)
	if (whitelist.size() > maxFilterScanSize) {
		THEMIS_WARN("searchKnnPreFiltered: Whitelist size {} exceeds max {}, using post-filtering instead", 
			whitelist.size(), maxFilterScanSize);
		// Fallback to standard KNN with post-filtering
		std::vector<AttributeFilter> legacyFilters;
		for (const auto& f : filters) {
			if (f.op == AttributeFilterV2::Op::EQUALS) {
				legacyFilters.push_back({f.field, f.value, AttributeFilter::Op::EQUALS});
			}
		}
		return searchKnnFiltered(query, k, legacyFilters, 5);
	}

	// Execute HNSW with whitelist
	return searchKnn(query, k, &whitelist);
}

// =============================================================================
// Radius Search (Epsilon Neighbors)
// =============================================================================

std::pair<VectorIndexManager::Status, std::vector<VectorIndexManager::Result>>
VectorIndexManager::searchKnnRadius(
	const std::vector<float>& query,
	float epsilon,
	size_t max_results,
	const std::vector<std::string>* whitelistPks
) const {
	if (static_cast<int>(query.size()) != dim_) {
		return {Status::Error("searchKnnRadius: Query-Dimension passt nicht"), {}};
	}

	std::vector<Result> results;

#ifdef THEMIS_HNSW_ENABLED
	if (useHnsw_) {
		// HNSW unterstützt keine native radius search; nutze searchKnn mit großem k und filter
		size_t fetchK = max_results > 0 ? std::max(max_results * 2, size_t(100)) : pkToId_.size();
		auto [st, candidates] = searchKnn(query, fetchK, whitelistPks);
		if (!st.ok) return {st, {}};
		
		for (const auto& c : candidates) {
			if (c.distance <= epsilon) {
				results.push_back(c);
				if (max_results > 0 && results.size() >= max_results) break;
			}
		}
		return {Status::OK(), results};
	}
#endif

	// Fallback: Brute-Force über Cache/Storage
	const auto& searchSpace = whitelistPks ? *whitelistPks : std::vector<std::string>{};
	bool useWhitelist = (whitelistPks != nullptr);

	if (!useWhitelist) {
		// Scan über cache_
		for (const auto& [pk, vec] : cache_) {
			float dist = distance(query, vec);
			if (dist <= epsilon) {
				results.push_back({pk, dist});
				if (max_results > 0 && results.size() >= max_results) break;
			}
		}
	} else {
		// Nur Whitelist prüfen
		for (const auto& pk : searchSpace) {
			auto it = cache_.find(pk);
			if (it == cache_.end()) {
				// Lade aus Storage
				std::string key = makeObjectKey(pk);
				auto blob = db_.get(key);
				if (!blob) continue;
				try {
					BaseEntity e = BaseEntity::deserialize(pk, *blob);
					auto vecOpt = e.extractVector("embedding");
					if (!vecOpt) continue;
					cache_[pk] = *vecOpt;
					it = cache_.find(pk);
				} catch (...) { continue; }
			}
			if (it != cache_.end()) {
				float dist = distance(query, it->second);
				if (dist <= epsilon) {
					results.push_back({pk, dist});
					if (max_results > 0 && results.size() >= max_results) break;
				}
			}
		}
	}

	// Sortiere nach Distanz
	std::sort(results.begin(), results.end(), [](const Result& a, const Result& b) {
		return a.distance < b.distance;
	});

	return {Status::OK(), results};
}

std::pair<VectorIndexManager::Status, std::vector<VectorIndexManager::Result>>
VectorIndexManager::searchKnnRadiusPreFiltered(
	const std::vector<float>& query,
	float epsilon,
	size_t max_results,
	const std::vector<AttributeFilterV2>& filters,
	SecondaryIndexManager* secondaryIdx
) const {
	if (static_cast<int>(query.size()) != dim_) {
		return {Status::Error("searchKnnRadiusPreFiltered: Query-Dimension passt nicht"), {}};
	}

	if (filters.empty()) {
		return searchKnnRadius(query, epsilon, max_results, nullptr);
	}

	if (!secondaryIdx) {
		THEMIS_WARN("searchKnnRadiusPreFiltered: SecondaryIndexManager nicht verfügbar, Fallback ohne Filter");
		return searchKnnRadius(query, epsilon, max_results, nullptr);
	}

	// Reuse whitelist generation from searchKnnPreFiltered
	std::vector<std::string> whitelist;
	std::unordered_set<std::string> whitelistSet;

	size_t maxFilterScanSize = 100000;
	try {
		if (auto cfgBlob = db_.get("config:vector")) {
			std::string s(cfgBlob->begin(), cfgBlob->end());
			auto j = nlohmann::json::parse(s);
			maxFilterScanSize = j.value("max_filter_scan_size", 100000);
		}
	} catch (...) {}

	bool isFirstFilter = true;
	for (const auto& filter : filters) {
		std::vector<std::string> filterResults;

		switch (filter.op) {
			case AttributeFilterV2::Op::EQUALS: {
				auto [st, pks] = secondaryIdx->scanKeysEqual(objectName_, filter.field, filter.value);
				if (st.ok) filterResults = std::move(pks);
				break;
			}
			case AttributeFilterV2::Op::RANGE: {
				auto [st, pks] = secondaryIdx->scanKeysRange(
					objectName_, filter.field,
					std::optional<std::string>(filter.value_min),
					std::optional<std::string>(filter.value_max),
					true, true, maxFilterScanSize
				);
				if (st.ok) filterResults = std::move(pks);
				break;
			}
			case AttributeFilterV2::Op::IN: {
				std::unordered_set<std::string> inResults;
				for (const auto& val : filter.values) {
					auto [st, pks] = secondaryIdx->scanKeysEqual(objectName_, filter.field, val);
					if (st.ok) inResults.insert(pks.begin(), pks.end());
				}
				filterResults.assign(inResults.begin(), inResults.end());
				break;
			}
			case AttributeFilterV2::Op::GREATER_THAN:
			case AttributeFilterV2::Op::GREATER_EQUAL:
			case AttributeFilterV2::Op::LESS_THAN:
			case AttributeFilterV2::Op::LESS_EQUAL: {
				std::optional<std::string> lower, upper;
				bool includeLower = false, includeUpper = false;
				if (filter.op == AttributeFilterV2::Op::GREATER_THAN) {
					lower = filter.value; includeLower = false;
				} else if (filter.op == AttributeFilterV2::Op::GREATER_EQUAL) {
					lower = filter.value; includeLower = true;
				} else if (filter.op == AttributeFilterV2::Op::LESS_THAN) {
					upper = filter.value; includeUpper = false;
				} else {
					upper = filter.value; includeUpper = true;
				}
				auto [st, pks] = secondaryIdx->scanKeysRange(
					objectName_, filter.field, lower, upper,
					includeLower, includeUpper, maxFilterScanSize
				);
				if (st.ok) filterResults = std::move(pks);
				break;
			}
			default:
				THEMIS_WARN("searchKnnRadiusPreFiltered: Unsupported op, skipping");
				continue;
		}

		if (isFirstFilter) {
			whitelistSet.insert(filterResults.begin(), filterResults.end());
			isFirstFilter = false;
		} else {
			std::unordered_set<std::string> intersection;
			for (const auto& pk : filterResults) {
				if (whitelistSet.count(pk)) intersection.insert(pk);
			}
			whitelistSet = std::move(intersection);
		}

		if (whitelistSet.empty()) {
			THEMIS_INFO("searchKnnRadiusPreFiltered: Whitelist empty after filter on {}", filter.field);
			return {Status::OK(), {}};
		}
	}

	whitelist.assign(whitelistSet.begin(), whitelistSet.end());
	THEMIS_INFO("searchKnnRadiusPreFiltered: Generated whitelist with {} candidates", whitelist.size());

	return searchKnnRadius(query, epsilon, max_results, &whitelist);
}

	// =============================================================================
	// Persistenz: saveIndex / loadIndex (HNSW + Mapping + Meta)
	// =============================================================================

	VectorIndexManager::Status VectorIndexManager::saveIndex(const std::string& directory) const {
		namespace fs = std::filesystem;
		try {
			fs::create_directories(directory);
			// Speichere Mapping (id -> pk)
			{
				std::ofstream mapFile(fs::path(directory) / "labels.txt", std::ios::binary | std::ios::trunc);
				if (!mapFile) return Status::Error("saveIndex: labels.txt nicht schreibbar");
				for (const auto& pk : idToPk_) {
					mapFile << pk << "\n";
				}
			}
			// Speichere Meta
			{
				std::ofstream metaFile(fs::path(directory) / "meta.txt", std::ios::binary | std::ios::trunc);
				if (!metaFile) return Status::Error("saveIndex: meta.txt nicht schreibbar");
				metaFile << objectName_ << "\n" << dim_ << "\n" << (metric_ == Metric::L2 ? "L2" : "COSINE")
						 << "\n" << efSearch_ << "\n" << m_ << "\n" << efConstruction_ << "\n";
			}
	#ifdef THEMIS_HNSW_ENABLED
			if (useHnsw_) {
				auto* appr = static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
				std::string indexPath = (fs::path(directory) / "index.bin").string();
				appr->saveIndex(indexPath);
			}
	#endif
		} catch (const std::exception& ex) {
			return Status::Error(std::string("saveIndex: ") + ex.what());
		} catch (...) {
			return Status::Error("saveIndex: unbekannter Fehler");
		}
		return Status::OK();
	}

	VectorIndexManager::Status VectorIndexManager::loadIndex(const std::string& directory) {
		namespace fs = std::filesystem;
		try {
			// Lade Meta
			std::ifstream metaFile(fs::path(directory) / "meta.txt", std::ios::binary);
			if (!metaFile) return Status::Error("loadIndex: meta.txt nicht lesbar");
			std::string obj; std::string metricStr; int dim; int ef, m, efc;
			std::getline(metaFile, obj);
			metaFile >> dim; metaFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			std::getline(metaFile, metricStr);
			metaFile >> ef; metaFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			metaFile >> m; metaFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			metaFile >> efc; metaFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

			if (obj != objectName_) return Status::Error("loadIndex: objectName passt nicht zum Manager");
			if (dim_ != 0 && dim_ != dim) return Status::Error("loadIndex: Dimension passt nicht zum Manager");
			dim_ = dim;
			metric_ = (metricStr == "L2") ? Metric::L2 : Metric::COSINE;
			efSearch_ = ef; m_ = m; efConstruction_ = efc;

	#ifdef THEMIS_HNSW_ENABLED
			// Initialisiere Space und Index und lade
			hnswlib::SpaceInterface<float>* space = nullptr;
			if (metric_ == Metric::L2) space = new hnswlib::L2Space(dim_);
			else space = new hnswlib::InnerProductSpace(dim_);

			auto* appr = new hnswlib::HierarchicalNSW<float>(space, (fs::path(directory) / "index.bin").string(), false);
			appr->ef_ = efSearch_;
			hnswIndex_ = static_cast<void*>(appr);
			useHnsw_ = true;
	#else
			useHnsw_ = false;
	#endif
			// Lade Mapping
			pkToId_.clear(); idToPk_.clear();
			{
				std::ifstream mapFile(fs::path(directory) / "labels.txt", std::ios::binary);
				if (!mapFile) return Status::Error("loadIndex: labels.txt nicht lesbar");
				std::string line; size_t id = 0;
				while (std::getline(mapFile, line)) {
					if (line.empty()) { ++id; continue; }
					pkToId_[line] = id;
					idToPk_.push_back(line);
					++id;
				}
			}

			// Cache ggf. leer lassen; rebuildFromStorage() kann separat genutzt werden
		} catch (const std::exception& ex) {
			return Status::Error(std::string("loadIndex: ") + ex.what());
		} catch (...) {
			return Status::Error("loadIndex: unbekannter Fehler");
		}
		return Status::OK();
	}

// ============================================================================
// MVCC Transaction Variants
// ============================================================================

VectorIndexManager::Status VectorIndexManager::addEntity(const BaseEntity& e, RocksDBWrapper::TransactionWrapper& txn,
                                                          std::string_view vectorField) {
	if (objectName_.empty()) return Status::Error("addEntity(mvcc): Manager nicht initialisiert");
	if (!txn.isActive()) return Status::Error("addEntity(mvcc): Transaction ist nicht aktiv");
	
	const std::string& pk = e.getPrimaryKey();
	auto v = e.extractVector(vectorField);
	if (!v) return Status::Error("addEntity(mvcc): Vektor-Feld fehlt oder hat falsches Format");
	if (v->size() != static_cast<size_t>(dim_)) return Status::Error("addEntity(mvcc): Vektordimension passt nicht");

	// Persistenz via MVCC Transaction (optional: SQ8-Quantisierung)
	std::string key = makeObjectKey(pk);
	// Decide on SQ8
	auto shouldQuantize = [&]() -> bool {
		std::string mode = "auto"; int64_t threshold = 1000000;
		try {
			if (auto cfg = db_.get("config:vector")) {
				std::string s(cfg->begin(), cfg->end());
				nlohmann::json j = nlohmann::json::parse(s);
				mode = j.value("quantization", std::string("auto"));
				threshold = j.value("auto_threshold", 1000000);
			}
		} catch (...) {}
		if (mode == "none") return false; if (mode == "sq8") return true;
		return static_cast<int64_t>(getVectorCount()) >= threshold;
	}();
	std::vector<uint8_t> serialized;
	if (shouldQuantize) {
		float amax = 0.0f; for (float x : *v) amax = std::max(amax, std::fabs(x));
		float scale = (amax > 0.f) ? (amax / 127.0f) : 1.0f;
		std::vector<uint8_t> codes(v->size());
		for (size_t i = 0; i < v->size(); ++i) {
			int q = static_cast<int>(std::round((*v)[i] / scale));
			q = std::max(-127, std::min(127, q));
			codes[i] = static_cast<uint8_t>(static_cast<int8_t>(q));
		}
		auto fields = e.getAllFields();
		fields.erase("embedding");
		fields["embedding_q"] = codes;
		fields["embedding_scale"] = static_cast<double>(scale);
		BaseEntity eq = BaseEntity::fromFields(pk, fields);
		serialized = eq.serialize();
	} else {
		serialized = e.serialize();
	}
	txn.put(key, serialized);

	// In-Memory Cache aktualisieren (nur COSINE normalisiert; DOT/L2 bleiben raw)
	std::vector<float> vv = *v;
	if (metric_ == Metric::COSINE) normalizeL2(vv);
	cache_[pk] = vv;
#ifdef THEMIS_HNSW_ENABLED
	if (useHnsw_) {
		auto* appr = static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
		size_t id;
		auto it = pkToId_.find(pk);
		if (it == pkToId_.end()) {
			id = idToPk_.size();
			pkToId_[pk] = id;
			idToPk_.push_back(pk);
		} else {
			id = it->second;
		}
		try { appr->addPoint(cache_[pk].data(), id); } catch (...) { /* evtl. schon vorhanden */ }
	}
#endif
	return Status::OK();
}

VectorIndexManager::Status VectorIndexManager::updateEntity(const BaseEntity& e, RocksDBWrapper::TransactionWrapper& txn,
                                                             std::string_view vectorField) {
	// einfache Strategie: remove + add (beide via Transaction)
	auto r = removeByPk(e.getPrimaryKey(), txn);
	if (!r.ok) THEMIS_WARN("updateEntity(mvcc): remove warn: {}", r.message);
	return addEntity(e, txn, vectorField);
}

VectorIndexManager::Status VectorIndexManager::removeByPk(std::string_view pk, RocksDBWrapper::TransactionWrapper& txn) {
	if (!txn.isActive()) return Status::Error("removeByPk(mvcc): Transaction ist nicht aktiv");
	
	// RocksDB löschen via Transaction
	std::string key = makeObjectKey(pk);
	txn.del(key);

	// In-Memory Cache löschen
	cache_.erase(std::string(pk));
#ifdef THEMIS_HNSW_ENABLED
	if (useHnsw_) {
		auto* appr = static_cast<hnswlib::HierarchicalNSW<float>*>(hnswIndex_);
		auto it = pkToId_.find(std::string(pk));
		if (it != pkToId_.end()) {
			try { appr->markDelete(it->second); } catch (...) {}
		}
	}
#endif
	return Status::OK();
}

// =============================================================================
// Batch Operations
// =============================================================================

VectorIndexManager::Status VectorIndexManager::addBatch(
	const std::vector<BaseEntity>& entities,
	std::string_view vectorField
) {
	if (entities.empty()) {
		return Status::OK();
	}

	// Batch Write Optimization: Pre-compute quantization before WriteBatch
	// This avoids inline computation during Put operations
	std::vector<std::vector<uint8_t>> batch_serialized;
	std::vector<std::string> batch_keys;
	batch_serialized.reserve(entities.size());
	batch_keys.reserve(entities.size());
	
	// Determine if quantization is needed (once for whole batch)
	bool shouldQuantize = false;
	std::string quantMode = "auto";
	int64_t quantThreshold = 1000000;
	try {
		if (auto cfg = db_.get("config:vector")) {
			std::string s(cfg->begin(), cfg->end());
			nlohmann::json j = nlohmann::json::parse(s);
			quantMode = j.value("quantization", std::string("auto"));
			quantThreshold = j.value("auto_threshold", 1000000);
		}
	} catch (...) {}
	
	if (quantMode == "sq8") {
		shouldQuantize = true;
	} else if (quantMode != "none") {
		shouldQuantize = static_cast<int64_t>(getVectorCount()) >= quantThreshold;
	}
	
	// Pre-compute all serialization and quantization
	#pragma omp simd
	for (size_t idx = 0; idx < entities.size(); ++idx) {
		const auto& entity = entities[idx];
		const std::string& pk = entity.getPrimaryKey();
		auto v = entity.extractVector(vectorField);
		
		if (!v) {
			THEMIS_WARN("addBatch: Entity {} missing vector field", pk);
			continue;
		}
		
		std::vector<uint8_t> serialized;
		if (shouldQuantize) {
			float amax = 0.0f;
			for (float x : *v) amax = std::max(amax, std::fabs(x));
			float scale = (amax > 0.f) ? (amax / 127.0f) : 1.0f;
			std::vector<uint8_t> codes(v->size());
			for (size_t i = 0; i < v->size(); ++i) {
				int q = static_cast<int>(std::round((*v)[i] / scale));
				q = std::max(-127, std::min(127, q));
				codes[i] = static_cast<uint8_t>(static_cast<int8_t>(q));
			}
			auto fields = entity.getAllFields();
			fields.erase("embedding");
			fields["embedding_q"] = codes;
			fields["embedding_scale"] = static_cast<double>(scale);
			BaseEntity eq = BaseEntity::fromFields(pk, fields);
			serialized = eq.serialize();
		} else {
			serialized = entity.serialize();
		}
		
		batch_serialized.push_back(std::move(serialized));
		batch_keys.push_back(makeObjectKey(pk));
	}

	// Now create WriteBatch with pre-computed data (fast path)
	auto batch = db_.createWriteBatch();
	
	for (size_t i = 0; i < batch_keys.size(); ++i) {
		batch->put(batch_keys[i], batch_serialized[i]);
		
		// Also update cache (extract vector again for cache)
		const auto& entity = entities[i];
		auto v = entity.extractVector(vectorField);
		if (v) {
			std::vector<float> vv = *v;
			if (metric_ == Metric::COSINE) normalizeL2(vv);
			cache_[entity.getPrimaryKey()] = vv;
		}
	}
	
	// Commit batch atomically
	if (!batch->commit()) {
		return Status::Error("addBatch: Failed to commit WriteBatch");
	}
	
	return Status::OK();
}

VectorIndexManager::Status VectorIndexManager::updateBatch(
	const std::vector<BaseEntity>& entities,
	std::string_view vectorField
) {
	if (entities.empty()) {
		return Status::OK();
	}

	auto batch = db_.createWriteBatch();
	
	for (const auto& entity : entities) {
		auto result = updateEntity(entity, *batch, vectorField);
		if (!result.ok) {
			THEMIS_WARN("updateBatch: Failed to update entity {}: {}", entity.getPrimaryKey(), result.message);
		}
	}
	
	if (!batch->commit()) {
		return Status::Error("updateBatch: Failed to commit WriteBatch");
	}
	
	return Status::OK();
}

VectorIndexManager::Status VectorIndexManager::removeBatch(
	const std::vector<std::string>& pks
) {
	if (pks.empty()) {
		return Status::OK();
	}

	auto batch = db_.createWriteBatch();
	
	for (const auto& pk : pks) {
		auto result = removeByPk(pk, *batch);
		if (!result.ok) {
			THEMIS_WARN("removeBatch: Failed to remove entity {}: {}", pk, result.message);
		}
	}
	
	if (!batch->commit()) {
		return Status::Error("removeBatch: Failed to commit WriteBatch");
	}
	
	return Status::OK();
}

// =============================================================================
// Vector Statistics & Aggregation
// =============================================================================

std::pair<VectorIndexManager::Status, VectorIndexManager::Statistics>
VectorIndexManager::getStatistics() const {
	Statistics stats;
	stats.vector_count = cache_.size();
	stats.dimension = dim_;
	stats.metric_name = (metric_ == Metric::L2) ? "L2" : 
	                    (metric_ == Metric::COSINE) ? "COSINE" : "DOT";

	if (cache_.empty()) {
		return {Status::OK(), stats};
	}

	// Compute pairwise distance statistics (sample-based for large datasets)
	std::vector<float> distances;
	const size_t MAX_SAMPLES = 1000;
	size_t sample_count = std::min(cache_.size(), MAX_SAMPLES);
	
	std::vector<std::string> pks;
	pks.reserve(cache_.size());
	for (const auto& [pk, vec] : cache_) {
		pks.push_back(pk);
	}

	// Sample random pairs
	for (size_t i = 0; i < sample_count && i < pks.size(); ++i) {
		for (size_t j = i + 1; j < std::min(i + 10, pks.size()); ++j) {
			float dist = distance(cache_.at(pks[i]), cache_.at(pks[j]));
			distances.push_back(dist);
		}
	}

	if (distances.empty()) {
		return {Status::OK(), stats};
	}

	// Calculate statistics
	stats.min_distance = *std::min_element(distances.begin(), distances.end());
	stats.max_distance = *std::max_element(distances.begin(), distances.end());
	
	float sum = 0.0f;
	for (float d : distances) {
		sum += d;
	}
	stats.mean_distance = sum / distances.size();
	
	// Standard deviation
	float sq_sum = 0.0f;
	for (float d : distances) {
		float diff = d - stats.mean_distance;
		sq_sum += diff * diff;
	}
	stats.std_dev_distance = std::sqrt(sq_sum / distances.size());

	return {Status::OK(), stats};
}

std::pair<VectorIndexManager::Status, std::vector<float>>
VectorIndexManager::computeCentroid() const {
	if (cache_.empty()) {
		return {Status::Error("computeCentroid: No vectors in index"), {}};
	}

	std::vector<float> centroid(dim_, 0.0f);
	
	for (const auto& [pk, vec] : cache_) {
		if (vec.size() != static_cast<size_t>(dim_)) {
			continue;
		}
		for (int i = 0; i < dim_; ++i) {
			centroid[i] += vec[i];
		}
	}
	
	// Average
	for (int i = 0; i < dim_; ++i) {
		centroid[i] /= cache_.size();
	}
	
	return {Status::OK(), centroid};
}

std::pair<VectorIndexManager::Status, std::vector<float>>
VectorIndexManager::computeVariance() const {
	if (cache_.empty()) {
		return {Status::Error("computeVariance: No vectors in index"), {}};
	}

	auto [st, centroid] = computeCentroid();
	if (!st.ok) {
		return {st, {}};
	}

	std::vector<float> variance(dim_, 0.0f);
	
	for (const auto& [pk, vec] : cache_) {
		if (vec.size() != static_cast<size_t>(dim_)) {
			continue;
		}
		for (int i = 0; i < dim_; ++i) {
			float diff = vec[i] - centroid[i];
			variance[i] += diff * diff;
		}
	}
	
	// Divide by count
	for (int i = 0; i < dim_; ++i) {
		variance[i] /= cache_.size();
	}
	
	return {Status::OK(), variance};
}

std::pair<VectorIndexManager::Status, std::vector<std::string>>
VectorIndexManager::findOutliers(float threshold) const {
	if (cache_.empty()) {
		return {Status::OK(), {}};
	}

	auto [st_cent, centroid] = computeCentroid();
	if (!st_cent.ok) {
		return {st_cent, {}};
	}

	auto [st_stats, stats] = getStatistics();
	if (!st_stats.ok) {
		return {st_stats, {}};
	}

	std::vector<std::string> outliers;
	float outlier_threshold = stats.mean_distance + threshold * stats.std_dev_distance;

	for (const auto& [pk, vec] : cache_) {
		if (vec.size() != static_cast<size_t>(dim_)) {
			continue;
		}
		
		float dist = distance(vec, centroid);
		if (dist > outlier_threshold) {
			outliers.push_back(pk);
		}
	}

	return {Status::OK(), outliers};
}

} // namespace themis
