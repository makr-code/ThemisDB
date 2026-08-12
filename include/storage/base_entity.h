/**
 * @file base_entity.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <map>
#include <variant>
#include "utils/geo/ewkb.h"  // For GeoSidecar definition

namespace themis {

/// Value type that can represent different data types
using Value = std::variant<
    std::monostate,           // null
    bool,                      // boolean
    int64_t,                   // integer
    double,                    // floating point
    std::string,               // string
    std::vector<float>,        // float vector (for embeddings)
    std::vector<uint8_t>       // binary blob
>;

/// Base Entity: The canonical storage unit for all data models
/// 
/// Each logical entity (row, document, node, edge, vector object) is stored as one blob
/// 
/// Architecture:
/// - Storage format: Custom binary serialization (similar to VelocyPack/MessagePack)
/// - Fast field extraction: simdjson on-demand parsing for index updates
/// - Multi-model support: Flexible schema-less document model
/// 
/// Sources:
/// - Concept: Unified Multi-Model Storage with Canonical Entity Pattern
/// - Origin: ThemisDB Original Design
/// - Design Philosophy: "One canonical storage, multiple projection layers"
/// - Inspiration:
///   - ArangoDB: Multi-model architecture with unified storage
///   - CozoDB: Hybrid relational-graph-vector design
///   - Azure Cosmos DB: Multi-model APIs over single storage engine
/// - Innovation: ThemisDB extends the multi-model concept with:
///   - True unified storage (not multiple engines)
///   - ACID transactions across all models simultaneously
///   - Atomic multi-index updates (secondary, graph, vector, fulltext)
///   - Zero-overhead model projection (no data duplication)
/// - Implementation: ThemisDB Core Team
/// - First Introduced: ThemisDB v1.0.0
/// - Key Differentiators:
///   - Transactional vector indexes (unique to ThemisDB)
///   - Integrated LLM engine with zero-copy data access
///   - Field-level encryption within base entity
///   - Temporal versioning via MVCC snapshots
class BaseEntity {
public:
    using Blob = std::vector<uint8_t>;
    using Attributes = std::map<std::string, std::string>;
    using FieldMap = std::map<std::string, Value>;
    
    /// Storage format type
    enum class Format {
        BINARY,   // Custom binary format (fast, compact)
        JSON      // JSON text (human-readable, for compatibility)
    };
    
    BaseEntity() = default;
    explicit BaseEntity(std::string_view pk);
    BaseEntity(std::string_view pk, const FieldMap& fields);
    BaseEntity(std::string_view pk, Blob blob, Format format = Format::BINARY);
    
    /// Get primary key
    const std::string& getPrimaryKey() const { return primary_key_; }
    
    /// Set primary key
    void setPrimaryKey(std::string_view pk) { primary_key_ = pk; }
    
    /// Get binary blob
    const Blob& getBlob() const { return blob_; }
    
    /// Set binary blob (invalidates cache)
    void setBlob(Blob blob, Format format = Format::BINARY);
    
    /// Get storage format
    Format getFormat() const { return format_; }
    
    // ===== Field Access (lazy parsing) =====
    
    /// Check if field exists
    bool hasField(std::string_view field_name) const;
    
    /// Get field value (returns nullopt if not found)
    std::optional<Value> getField(std::string_view field_name) const;

    /// Convenience helpers returning defaults
    std::string getFieldString(std::string_view field_name) const;
    int64_t getFieldInt(std::string_view field_name) const;
    
    /// Get field as string (with type conversion)
    std::optional<std::string> getFieldAsString(std::string_view field_name) const;
    
    /// Get field as int64
    std::optional<int64_t> getFieldAsInt(std::string_view field_name) const;
    
    /// Get field as double
    std::optional<double> getFieldAsDouble(std::string_view field_name) const;
    
    /// Get field as bool
    std::optional<bool> getFieldAsBool(std::string_view field_name) const;
    
    /// Get field as float vector (for embeddings)
    std::optional<std::vector<float>> getFieldAsVector(std::string_view field_name) const;

    /// Get field as a string array.
    ///
    /// Attempts to decode the named field as an ordered list of strings.
    /// The following encodings are recognised, in priority order:
    ///   1. A JSON array stored as a plain string value, e.g. `["a","b","c"]`.
    ///   2. A comma-separated plain string (legacy format, backward-compatible read).
    ///
    /// Returns `std::nullopt` when the field is absent or cannot be decoded as
    /// any string-like type.  Returns an empty vector for an empty array or an
    /// empty string.
    std::optional<std::vector<std::string>> getFieldAsStringArray(std::string_view field_name) const;

    /// Set field value (modifies blob)
    ///
    /// Sets a field in the entity's field map, triggering a blob rebuild for serialization.
    /// Implements fail-closed validation: rejects empty field_name to prevent silent field map corruption.
    ///
    /// @param field_name Field identifier (non-empty std::string_view required)
    /// @param value Value to set for this field
    ///
    /// @note **Fail-Closed Behavior:** If field_name is empty, this method logs an error and returns
    ///       without modifying the field cache. This prevents creating corrupt field map entries with
    ///       empty keys that would propagate through getAllFields() and toJson() calls.
    ///
    /// @see getAllFields(), toJson() — downstream methods that depend on valid field keys
    void setField(std::string_view field_name, const Value& value);
    
    /// Get all fields (full parse)
    FieldMap getAllFields() const;
    
    // ===== Serialization =====
    
    /// Serialize current fields to binary blob
    Blob serialize() const;
    
    /// Serialize to JSON string
    std::string toJson() const;
    
    /// Create from JSON string (using simdjson for speed)
    static BaseEntity fromJson(std::string_view pk, std::string_view json_str);
    
    /// Create from field map
    static BaseEntity fromFields(std::string_view pk, const FieldMap& fields);
    
    /// Deserialize from binary blob
    static BaseEntity deserialize(std::string_view pk, const Blob& blob);
    
    // ===== Index Support (fast field extraction) =====
    
    /// Extract specific field without full deserialization
    /// Critical for index updates - uses simdjson on-demand API
    std::optional<std::string> extractField(std::string_view field_name) const;
    
    /// Extract vector embedding field (for ANN index)
    std::optional<std::vector<float>> extractVector(std::string_view field_name = "embedding") const;
    
    // ===== Geo Support (Cross-Cutting Capability) =====
    
    /// Check if entity has geometry
    bool hasGeometry() const { return geometry_.has_value(); }
    
    /// Get geometry blob (EWKB format)
    const std::optional<Blob>& getGeometry() const { return geometry_; }
    
    /// Set geometry blob (EWKB format)
    void setGeometry(const Blob& ewkb);
    
    /// Get geo sidecar (MBR, centroid, z-range)
    const std::optional<geo::GeoSidecar>& getGeoSidecar() const { return geo_sidecar_; }
    
    /// Set geo sidecar (computed from geometry)
    void setGeoSidecar(const geo::GeoSidecar& sidecar);
    
    /// Clear geometry (remove geo capability)
    void clearGeometry();
    
    // ===== Index Support (fast field extraction) =====
    
    /// Get all indexable fields (for secondary index maintenance)
    /// Returns field_name -> string_value pairs
    Attributes extractAllFields() const;
    
    /// Extract fields matching a prefix (e.g., "metadata.*")
    Attributes extractFieldsWithPrefix(std::string_view prefix) const;
    
    // ===== Rotary Embeddings Support =====
    
    /// Check if entity has a rotated embedding for given field
    /// Looks for field_name + "_rotation_pos" metadata
    bool hasRotatedEmbedding(std::string_view field_name) const;
    
    /// Get rotation position used for this field's embedding
    /// Returns nullopt if field is not rotated
    std::optional<size_t> getRotationPosition(std::string_view field_name) const;
    
    /// Get rotation type (relation type for relational rotation)
    /// Returns nullopt if field is not relationally rotated
    std::optional<std::string> getRotationType(std::string_view field_name) const;
    
    // ===== Metadata =====
    
    /// Get blob size in bytes
    size_t getBlobSize() const { return blob_.size(); }
    
    /// Check if entity is empty
    bool isEmpty() const { return blob_.empty(); }
    
    /// Clear all data
    void clear();
    
private:
    std::string primary_key_;
    Blob blob_;
    Format format_ = Format::BINARY;
    
    // Lazy-parsed field cache (shared_ptr für Copy-Semantik)
    mutable std::shared_ptr<FieldMap> field_cache_;
    mutable bool cache_valid_ = false;
    mutable bool parse_failed_ = false;  // Track parse failure to avoid retry spam
    
    // Geo support (optional, cross-cutting capability)
    std::optional<Blob> geometry_;                      // EWKB blob
    std::optional<geo::GeoSidecar> geo_sidecar_;       // MBR, centroid, z-range
    
    // Parse blob into field cache
    void ensureCache() const;
    
    // Invalidate cache (after blob modification)
    void invalidateCache();
    
    // Parse JSON using simdjson (fast path for JSON format)
    FieldMap parseJson() const;
    
    // Parse binary format
    FieldMap parseBinary() const;
    
    // Rebuild blob from cache
    void rebuildBlob();
};

} // namespace themis
