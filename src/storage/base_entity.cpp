/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            base_entity.cpp                                    ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:10:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     655                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • 5bfa861df6  2026-03-23  Add runtime DLL copying functionality and error handling ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "storage/base_entity.h"
#include "utils/serialization.h"
#include "utils/logger.h"
#include "utils/geo/ewkb.h"
#include <simdjson.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <sstream>
#include <limits>

namespace themis {

// Global simdjson parser (thread-local for thread safety)
thread_local simdjson::ondemand::parser g_parser;

// ===== Constructors =====

BaseEntity::BaseEntity(std::string_view pk) : primary_key_(pk) {}

BaseEntity::BaseEntity(std::string_view pk, const FieldMap& fields) 
    : primary_key_(pk) {
    field_cache_ = std::make_shared<FieldMap>(fields);
    cache_valid_ = true;
    rebuildBlob();
}

BaseEntity::BaseEntity(std::string_view pk, Blob blob, Format format) 
    : primary_key_(pk), blob_(std::move(blob)), format_(format) {}

// ===== Blob Management =====

void BaseEntity::setBlob(Blob blob, Format format) {
    blob_ = std::move(blob);
    format_ = format;
    invalidateCache();
}

void BaseEntity::clear() {
    primary_key_.clear();
    blob_.clear();
    invalidateCache();
}

// ===== Field Access =====

void BaseEntity::ensureCache() const {
    if (cache_valid_ && field_cache_) {
        return;
    }
    
    // If parse previously failed, don't retry to avoid log spam and CPU churn
    if (parse_failed_) {
        return;
    }
    
    if (blob_.empty()) {
        field_cache_ = std::make_shared<FieldMap>();
        cache_valid_ = true;
        return;
    }
    
    try {
        if (format_ == Format::JSON) {
            field_cache_ = std::make_shared<FieldMap>(parseJson());
        } else {
            field_cache_ = std::make_shared<FieldMap>(parseBinary());
        }
        cache_valid_ = true;
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to parse entity blob (format={}): {}", 
                     format_ == Format::JSON ? "JSON" : "BINARY", e.what());
        // Mark parse as failed to prevent repeated retries and log spam
        parse_failed_ = true;
        field_cache_ = std::make_shared<FieldMap>();
        cache_valid_ = true;  // Cache is valid (empty) to prevent retries
    }
}

void BaseEntity::invalidateCache() {
    cache_valid_ = false;
    parse_failed_ = false;  // Reset parse failure flag on explicit invalidation
    field_cache_.reset();
}

bool BaseEntity::hasField(std::string_view field_name) const {
    ensureCache();
    return field_cache_->find(std::string(field_name)) != field_cache_->end();
}

std::optional<Value> BaseEntity::getField(std::string_view field_name) const {
    ensureCache();
    auto it = field_cache_->find(std::string(field_name));
    if (it != field_cache_->end()) {
        return it->second;
    }
    return std::nullopt;
}

std::string BaseEntity::getFieldString(std::string_view field_name) const {
    auto val = getFieldAsString(field_name);
    return val.value_or(std::string());
}

int64_t BaseEntity::getFieldInt(std::string_view field_name) const {
    auto val = getFieldAsInt(field_name);
    return val.value_or(0);
}

std::optional<std::string> BaseEntity::getFieldAsString(std::string_view field_name) const {
    auto value = getField(field_name);
    if (!value) return std::nullopt;
    
    return std::visit([](auto&& arg) -> std::optional<std::string> {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return arg;
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, bool>) {
            return arg ? "true" : "false";
        }
        return std::nullopt;
    }, *value);
}

std::optional<int64_t> BaseEntity::getFieldAsInt(std::string_view field_name) const {
    auto value = getField(field_name);
    if (!value) return std::nullopt;
    
    return std::visit([](auto&& arg) -> std::optional<int64_t> {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int64_t>) {
            return arg;
        } else if constexpr (std::is_same_v<T, double>) {
            return static_cast<int64_t>(arg);
        } else if constexpr (std::is_same_v<T, bool>) {
            return arg ? 1 : 0;
        } else if constexpr (std::is_same_v<T, std::string>) {
            try {
                size_t pos = 0;
                int64_t parsed = std::stoll(arg, &pos, 10);
                if (pos == arg.size()) {
                    return parsed;
                }
                return std::nullopt;
            } catch (...) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }, *value);
}

std::optional<double> BaseEntity::getFieldAsDouble(std::string_view field_name) const {
    auto value = getField(field_name);
    if (!value) return std::nullopt;
    
    return std::visit([](auto&& arg) -> std::optional<double> {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, double>) {
            return arg;
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return static_cast<double>(arg);
        }
        return std::nullopt;
    }, *value);
}

std::optional<bool> BaseEntity::getFieldAsBool(std::string_view field_name) const {
    auto value = getField(field_name);
    if (!value) return std::nullopt;
    
    return std::visit([](auto&& arg) -> std::optional<bool> {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, bool>) {
            return arg;
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return arg != 0;
        }
        return std::nullopt;
    }, *value);
}

std::optional<std::vector<float>> BaseEntity::getFieldAsVector(std::string_view field_name) const {
    auto value = getField(field_name);
    if (!value) return std::nullopt;
    
    return std::visit([](auto&& arg) -> std::optional<std::vector<float>> {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::vector<float>>) {
            return arg;
        }
        return std::nullopt;
    }, *value);
}

void BaseEntity::setField(std::string_view field_name, const Value& value) {
    ensureCache();
    if (field_cache_ && field_cache_.use_count() > 1) {
        field_cache_ = std::make_shared<FieldMap>(*field_cache_);
    }
    (*field_cache_)[std::string(field_name)] = value;
    rebuildBlob();
}

BaseEntity::FieldMap BaseEntity::getAllFields() const {
    ensureCache();
    return *field_cache_;
}

// ===== JSON Parsing (simdjson) =====

BaseEntity::FieldMap BaseEntity::parseJson() const {
    FieldMap fields;
    
    try {
        // Use simdjson on-demand API for maximum speed
        simdjson::padded_string padded(reinterpret_cast<const char*>(blob_.data()), blob_.size());

        // Obtain a document and object from the parser (store intermediate values as named variables
        // to satisfy the ondemand API requirements that some getters expect lvalue receivers).
        auto doc = g_parser.iterate(padded);
        auto obj = doc.get_object();

        for (auto field : obj) {
            auto key_res = field.unescaped_key();
            if (key_res.error()) {
                THEMIS_WARN("Failed to parse JSON field key: {}", simdjson::error_message(key_res.error()));
                continue;
            }
            std::string key_str(key_res.value_unsafe());

            auto val_res = field.value();
            if (val_res.error()) {
                THEMIS_WARN("Failed to parse JSON field '{}' value: {}", key_str, simdjson::error_message(val_res.error()));
                continue;
            }
            
            // Determine type and convert
            auto type_res = val_res.type();
            if (type_res.error()) {
                THEMIS_WARN("Failed to determine type for JSON field '{}': {}", key_str, simdjson::error_message(type_res.error()));
                continue;
            }
            auto type = type_res.value_unsafe();

            switch (type) {
                case simdjson::ondemand::json_type::null:
                    fields[key_str] = std::monostate{};
                    break;

                case simdjson::ondemand::json_type::boolean: {
                    auto bres = val_res.get_bool();
                    if (!bres.error()) fields[key_str] = bres.value_unsafe();
                    break;
                }

                case simdjson::ondemand::json_type::number: {
                    // Try int first, then double
                    auto ires = val_res.get_int64();
                    if (!ires.error()) {
                        fields[key_str] = ires.value_unsafe();
                    } else {
                        auto dres = val_res.get_double();
                        if (!dres.error()) fields[key_str] = dres.value_unsafe();
                    }
                    break;
                }

                case simdjson::ondemand::json_type::string: {
                    auto sv_res = val_res.get_string();
                    if (!sv_res.error()) {
                        fields[key_str] = std::string(sv_res.value_unsafe());
                    }
                    break;
                }

                case simdjson::ondemand::json_type::array: {
                    // Check if it's a float vector (for embeddings)
                    auto arr_res = val_res.get_array();
                    if (arr_res.error()) break;
                    std::vector<float> vec;
                    bool is_float_vec = true;

                    for (auto elem_res : arr_res.value_unsafe()) {
                        auto dres = elem_res.get_double();
                        if (!dres.error()) {
                            vec.push_back(static_cast<float>(dres.value_unsafe()));
                        } else {
                            is_float_vec = false;
                            break;
                        }
                    }

                    if (is_float_vec && !vec.empty()) {
                        fields[key_str] = vec;
                    }
                    // Note: nested objects/arrays not fully supported yet
                    break;
                }

                default:
                    // Skip objects and other complex types for now
                    break;
            }
        }
    } catch (const simdjson::simdjson_error& e) {
        THEMIS_ERROR("simdjson parse error: {}", e.what());
        throw std::runtime_error("JSON parse failed");
    }
    
    return fields;
}

// ===== Binary Parsing =====

BaseEntity::FieldMap BaseEntity::parseBinary() const {
    FieldMap fields;
    
    try {
        utils::Serialization::Decoder decoder(blob_);
        
        // Binary format: <num_fields> <field1> <field2> ...
        // Each field: <name_len> <name> <type_tag> <value>
        
        size_t num_fields = decoder.beginObject();
        
        for (size_t i = 0; i < num_fields; ++i) {
            std::string field_name = decoder.decodeString();
            auto type = decoder.peekType();
            
            switch (type) {
                case utils::Serialization::TypeTag::NULL_VALUE:
                    decoder.readTag();
                    fields[field_name] = std::monostate{};
                    break;
                    
                case utils::Serialization::TypeTag::BOOL_FALSE:
                case utils::Serialization::TypeTag::BOOL_TRUE:
                    fields[field_name] = decoder.decodeBool();
                    break;
                    
                case utils::Serialization::TypeTag::INT32:
                case utils::Serialization::TypeTag::INT64:
                    fields[field_name] = decoder.decodeInt64();
                    break;
                    
                case utils::Serialization::TypeTag::UINT32: {
                    // UINT32 uses 4 bytes - must use decodeUInt32() not decodeUInt64()
                    uint32_t uint_val = decoder.decodeUInt32();
                    // Safe to convert UINT32 to INT64 - UINT32_MAX < INT64_MAX
                    fields[field_name] = static_cast<int64_t>(uint_val);
                    break;
                }
                    
                case utils::Serialization::TypeTag::UINT64: {
                    uint64_t uint_val = decoder.decodeUInt64();
                    // DESIGN LIMITATION: BaseEntity::Value only supports int64_t, not uint64_t
                    // This is a schema design constraint - if you need full uint64 range,
                    // consider using binary blob or string representation
                    if (uint_val > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
                        THEMIS_ERROR("UINT64 value {} exceeds INT64_MAX for field '{}'. "
                                    "Value will be clamped. Consider using binary blob for full uint64 range.",
                                    uint_val, field_name);
                        fields[field_name] = std::numeric_limits<int64_t>::max();
                    } else {
                        fields[field_name] = static_cast<int64_t>(uint_val);
                    }
                    break;
                }
                    
                case utils::Serialization::TypeTag::FLOAT:
                    // Note: Float to double conversion is safe (widening)
                    fields[field_name] = static_cast<double>(decoder.decodeFloat());
                    break;
                    
                case utils::Serialization::TypeTag::DOUBLE:
                    fields[field_name] = decoder.decodeDouble();
                    break;
                    
                case utils::Serialization::TypeTag::STRING:
                    fields[field_name] = decoder.decodeString();
                    break;
                    
                case utils::Serialization::TypeTag::VECTOR_FLOAT:
                    fields[field_name] = decoder.decodeFloatVector();
                    break;
                    
                default:
                    // Fail fast on unknown or unsupported type tags to avoid decoder desynchronization
                    // Continuing after unknown tag can cause subsequent reads to go out of bounds
                    THEMIS_ERROR("Unknown or unsupported type tag {} for field '{}'. Cannot safely continue parsing.",
                                static_cast<int>(type), field_name);
                    throw std::runtime_error("Unknown type tag encountered while parsing BaseEntity binary blob");
            }
        }
        
        decoder.endObject();
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Binary parse error: {}", e.what());
        throw std::runtime_error("Binary parse failed");
    }
    
    return fields;
}

// ===== Serialization =====

void BaseEntity::rebuildBlob() {
    if (!field_cache_ || field_cache_->empty()) {
        blob_.clear();
        return;
    }
    
    // Always use binary format for storage (more efficient)
    utils::Serialization::Encoder encoder;
    
    encoder.beginObject(field_cache_->size());
    
    for (const auto& [name, value] : *field_cache_) {
        encoder.encodeString(name);
        
        std::visit([&encoder](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                encoder.encodeNull();
            } else if constexpr (std::is_same_v<T, bool>) {
                encoder.encodeBool(arg);
            } else if constexpr (std::is_same_v<T, int64_t>) {
                encoder.encodeInt64(arg);
            } else if constexpr (std::is_same_v<T, double>) {
                encoder.encodeDouble(arg);
            } else if constexpr (std::is_same_v<T, std::string>) {
                encoder.encodeString(arg);
            } else if constexpr (std::is_same_v<T, std::vector<float>>) {
                encoder.encodeFloatVector(arg);
            } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                encoder.encodeBinary(arg);
            }
        }, value);
    }
    
    encoder.endObject();
    blob_ = encoder.finish();
    format_ = Format::BINARY;
}

BaseEntity::Blob BaseEntity::serialize() const {
    if (!cache_valid_ || !field_cache_) {
        return blob_; // Return as-is
    }
    
    // Rebuild blob from cache if modified
    const_cast<BaseEntity*>(this)->rebuildBlob();
    return blob_;
}

std::string BaseEntity::toJson() const {
    ensureCache();
    
    std::ostringstream oss;
    oss << "{";
    
    bool first = true;
    for (const auto& [name, value] : *field_cache_) {
        if (!first) oss << ",";
        first = false;
        
        oss << "\"" << name << "\":";
        
        std::visit([&oss](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                oss << "null";
            } else if constexpr (std::is_same_v<T, bool>) {
                oss << (arg ? "true" : "false");
            } else if constexpr (std::is_same_v<T, int64_t>) {
                oss << arg;
            } else if constexpr (std::is_same_v<T, double>) {
                oss << arg;
            } else if constexpr (std::is_same_v<T, std::string>) {
                // Use nlohmann::json for proper string escaping (prevents injection)
                oss << nlohmann::json(arg).dump();
            } else if constexpr (std::is_same_v<T, std::vector<float>>) {
                oss << "[";
                for (size_t i = 0; i < arg.size(); ++i) {
                    if (i > 0) oss << ",";
                    oss << arg[i];
                }
                oss << "]";
            }
        }, value);
    }
    
    oss << "}";
    return oss.str();
}

// ===== Factory Methods =====

BaseEntity BaseEntity::fromJson(std::string_view pk, std::string_view json_str) {
    BaseEntity entity(pk);
    entity.blob_ = Blob(json_str.begin(), json_str.end());
    entity.format_ = Format::JSON;
    return entity;
}

BaseEntity BaseEntity::fromFields(std::string_view pk, const FieldMap& fields) {
    return BaseEntity(pk, fields);
}

BaseEntity BaseEntity::deserialize(std::string_view pk, const Blob& blob) {
    // Heuristik: Erkennen, ob das Blob JSON oder binär ist
    Format fmt = Format::BINARY;
    if (!blob.empty()) {
        unsigned char c = blob[0];
        // Sehr einfache Erkennung: JSON-Objekt/Array beginnt mit '{' oder '['
        if (c == '{' || c == '[') {
            fmt = Format::JSON;
        }
    }
    return BaseEntity(pk, blob, fmt);
}

// ===== Index Support (Fast Extraction) =====

std::optional<std::string> BaseEntity::extractField(std::string_view field_name) const {
    // Always use cache for reliable field extraction
    // (simdjson on-demand can only be iterated once)
    return getFieldAsString(field_name);
}

std::optional<std::vector<float>> BaseEntity::extractVector(std::string_view field_name) const {
    return getFieldAsVector(field_name);
}

BaseEntity::Attributes BaseEntity::extractAllFields() const {
    ensureCache();
    
    Attributes attrs;
    for (const auto& [name, value] : *field_cache_) {
        // Convert value to string
        std::visit([&attrs, &name](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::string>) {
                attrs[name] = arg;
            } else if constexpr (std::is_same_v<T, int64_t>) {
                attrs[name] = std::to_string(arg);
            } else if constexpr (std::is_same_v<T, double>) {
                attrs[name] = std::to_string(arg);
            } else if constexpr (std::is_same_v<T, bool>) {
                attrs[name] = arg ? "true" : "false";
            }
        }, value);
    }
    
    return attrs;
}

BaseEntity::Attributes BaseEntity::extractFieldsWithPrefix(std::string_view prefix) const {
    ensureCache();
    
    Attributes attrs;
    for (const auto& [name, value] : *field_cache_) {
        if (name.starts_with(prefix)) {
            std::visit([&attrs, &name](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    attrs[name] = arg;
                } else if constexpr (std::is_same_v<T, int64_t>) {
                    attrs[name] = std::to_string(arg);
                } else if constexpr (std::is_same_v<T, double>) {
                    attrs[name] = std::to_string(arg);
                }
            }, value);
        }
    }
    
    return attrs;
}

// ===== Geo Support (Cross-Cutting Capability) =====

void BaseEntity::setGeometry(const Blob& ewkb) {
    geometry_ = ewkb;
    
    // Compute sidecar automatically
    try {
        auto geom_info = geo::EWKBParser::parse(ewkb);
        geo_sidecar_ = geo::EWKBParser::computeSidecar(geom_info);
    } catch ([[maybe_unused]] const std::exception& e) {
        // Log warning but don't fail
        geo_sidecar_.reset();
    }
}

void BaseEntity::setGeoSidecar(const geo::GeoSidecar& sidecar) {
    geo_sidecar_ = sidecar;
}

void BaseEntity::clearGeometry() {
    geometry_.reset();
    geo_sidecar_.reset();
}

// ===== Rotary Embeddings Support =====

bool BaseEntity::hasRotatedEmbedding(std::string_view field_name) const {
    std::string rotation_pos_field = std::string(field_name) + "_rotation_pos";
    return hasField(rotation_pos_field);
}

std::optional<size_t> BaseEntity::getRotationPosition(std::string_view field_name) const {
    std::string rotation_pos_field = std::string(field_name) + "_rotation_pos";
    auto pos_value = getFieldAsInt(rotation_pos_field);
    if (pos_value && *pos_value >= 0) {
        // Safe conversion: already verified non-negative
        return static_cast<size_t>(*pos_value);
    }
    return std::nullopt;
}

std::optional<std::string> BaseEntity::getRotationType(std::string_view field_name) const {
    std::string rotation_type_field = std::string(field_name) + "_rotation_type";
    return getFieldAsString(rotation_type_field);
}

} // namespace themis
