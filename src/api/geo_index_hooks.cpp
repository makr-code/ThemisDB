#include "api/geo_index_hooks.h"
#include "index/spatial_index.h"
#include "storage/base_entity.h"
#include "utils/geo/ewkb.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <iostream>

namespace themis {
namespace api {

using json = nlohmann::json;

void GeoIndexHooks::onEntityPut(
    RocksDBWrapper& db,
    index::SpatialIndexManager* spatial_mgr,
    const std::string& table,
    const std::string& pk,
    const std::vector<uint8_t>& blob
) {
    (void)db; // May be used for direct RocksDB access in future
    // Skip if spatial index not available or table doesn't have spatial index
    if (!spatial_mgr || !spatial_mgr->hasSpatialIndex(table)) {
        return;
    }

    try {
        // Prefer direct JSON parse for raw JSON blobs
        nlohmann::json j;
        
        // Quick heuristic: MessagePack blobs start with specific markers (0x80-0x8F for fixmap, 0xDE/0xDF for map16/32)
        // JSON blobs start with '{' (0x7B) or '[' (0x5B)
        bool likely_json = false;
        if (!blob.empty()) {
            uint8_t first_byte = blob[0];
            likely_json = (first_byte == '{' || first_byte == '[');
        }
        
        if (likely_json) {
            try {
                std::string blob_str(reinterpret_cast<const char*>(blob.data()), blob.size());
                j = nlohmann::json::parse(blob_str);
            } catch (const std::exception& e) {
                throw;
            }
        } else {
            // Fallback: deserialize MessagePack via BaseEntity
            auto entity = BaseEntity::deserialize(pk, blob);
            j = nlohmann::json::parse(entity.toJson());
        }

        // Look for geometry field (common names: geometry, geom, location)
        std::vector<uint8_t> geom_blob;
        bool found_geometry = false;

        if (j.contains("geometry") && j["geometry"].is_string()) {
            // Geometry as hex-encoded EWKB string
            std::string hex_ewkb = j["geometry"].get<std::string>();
            // Simple hex decode (assumes valid hex)
            geom_blob.reserve(hex_ewkb.size() / 2);
            for (size_t i = 0; i < hex_ewkb.size(); i += 2) {
            uint8_t byte = static_cast<uint8_t>(
                std::stoi(hex_ewkb.substr(i, 2), nullptr, 16)
            );
            geom_blob.push_back(byte);
        }
        found_geometry = true;
    } else if (j.contains("geometry") && j["geometry"].is_object()) {
        // Geometry as GeoJSON - parse directly without re-serialization
        std::string geojson = j["geometry"].dump();
        auto geom_info = geo::EWKBParser::parseGeoJSON(geojson);
        geom_blob = geo::EWKBParser::serialize(geom_info);
        auto sidecar = geo::EWKBParser::computeSidecar(geom_info);
        
        // Insert into spatial index
        auto status = spatial_mgr->insert(table, pk, sidecar);
        if (!status) {
            THEMIS_WARN("Spatial index insert failed for {}:{}: {}", 
                        table, pk, status.message);
        } else {
            THEMIS_DEBUG("Spatial index updated for {}:{}", table, pk);
        }
        return;
    } else if (j.contains("geom_blob") && j["geom_blob"].is_array()) {
        // Geometry as binary array
        for (auto& byte : j["geom_blob"]) {
            geom_blob.push_back(static_cast<uint8_t>(byte.get<int>()));
        }
        found_geometry = true;
    } else if (j.contains("location")) {
        // Support for 'location' field commonly used in entities
        if (j["location"].is_string()) {
            // GeoJSON as string
            std::string geojson = j["location"].get<std::string>();
            try {
                auto geom_info = geo::EWKBParser::parseGeoJSON(geojson);
                auto sidecar = geo::EWKBParser::computeSidecar(geom_info);
                auto status = spatial_mgr->insert(table, pk, sidecar);
                if (!status) {
                    THEMIS_WARN("Spatial index insert failed for {}:{}: {}", table, pk, status.message);
                } else {
                    THEMIS_DEBUG("Spatial index updated (location) for {}:{}", table, pk);
                }
                return;
            } catch (const std::exception& e) {
                THEMIS_WARN("Geo hook parse error (location string) for {}:{}: {}", table, pk, e.what());
            }
        } else if (j["location"].is_object()) {
            // GeoJSON object
            std::string geojson = j["location"].dump();
            try {
                auto geom_info = geo::EWKBParser::parseGeoJSON(geojson);
                auto sidecar = geo::EWKBParser::computeSidecar(geom_info);
                auto status = spatial_mgr->insert(table, pk, sidecar);
                if (!status) {
                    THEMIS_WARN("Spatial index insert failed for {}:{}: {}", table, pk, status.message);
                } else {
                    THEMIS_DEBUG("Spatial index updated (location) for {}:{}", table, pk);
                }
                return;
            } catch (const std::exception& e) {
                THEMIS_WARN("Geo hook parse error (location object) for {}:{}: {}", table, pk, e.what());
            }
        }
    } else if (j.contains("geom")) {
        // Support for 'geom' alias
        if (j["geom"].is_string()) {
            std::string geojson = j["geom"].get<std::string>();
            try {
                auto geom_info = geo::EWKBParser::parseGeoJSON(geojson);
                auto sidecar = geo::EWKBParser::computeSidecar(geom_info);
                auto status = spatial_mgr->insert(table, pk, sidecar);
                if (!status) {
                    THEMIS_WARN("Spatial index insert failed for {}:{}: {}", table, pk, status.message);
                } else {
                    THEMIS_DEBUG("Spatial index updated (geom) for {}:{}", table, pk);
                }
                return;
            } catch (const std::exception& e) {
                THEMIS_WARN("Geo hook parse error (geom string) for {}:{}: {}", table, pk, e.what());
            }
        } else if (j["geom"].is_object()) {
            std::string geojson = j["geom"].dump();
            try {
                auto geom_info = geo::EWKBParser::parseGeoJSON(geojson);
                auto sidecar = geo::EWKBParser::computeSidecar(geom_info);
                auto status = spatial_mgr->insert(table, pk, sidecar);
                if (!status) {
                    THEMIS_WARN("Spatial index insert failed for {}:{}: {}", table, pk, status.message);
                } else {
                    THEMIS_DEBUG("Spatial index updated (geom) for {}:{}", table, pk);
                }
                return;
            } catch (const std::exception& e) {
                THEMIS_WARN("Geo hook parse error (geom object) for {}:{}: {}", table, pk, e.what());
            }
        }
    }

    if (!found_geometry) {
        // No geometry field, skip silently
        return;
    }

    // Parse EWKB and compute sidecar (for hex string or binary array paths)
    auto geom_info = geo::EWKBParser::parse(geom_blob);
    auto sidecar = geo::EWKBParser::computeSidecar(geom_info);

    // Insert into spatial index
    auto status = spatial_mgr->insert(table, pk, sidecar);
    if (!status) {
        THEMIS_WARN("Spatial index insert failed for {}:{}: {}", 
                    table, pk, status.message);
    } else {
        THEMIS_DEBUG("Spatial index updated for {}:{}", table, pk);
    }    } catch (const json::exception& e) {
        // JSON parse error - log but don't fail the entity write
        THEMIS_WARN("Geo hook JSON parse error for {}:{}: {}", table, pk, e.what());
    } catch (const std::exception& e) {
        // EWKB parse or other error - log but don't fail the entity write
        THEMIS_WARN("Geo hook error for {}:{}: {}", table, pk, e.what());
    }
}

// Phase 2: Atomic entity PUT with spatial index update via WriteBatch
bool GeoIndexHooks::onEntityPutAtomic(
    RocksDBWrapper::WriteBatchWrapper& batch,
    index::SpatialIndexManager* spatial_mgr,
    const std::string& table,
    const std::string& pk,
    const std::vector<uint8_t>& blob
) {
    // Skip if spatial index not available or table doesn't have spatial index
    if (!spatial_mgr || !spatial_mgr->hasSpatialIndex(table)) {
        return false;
    }

    try {
        // Parse blob as JSON to extract geometry field
        std::string blob_str(reinterpret_cast<const char*>(blob.data()), blob.size());
        auto j = json::parse(blob_str);

        // Look for geometry field
        std::vector<uint8_t> geom_blob;
        bool found_geometry = false;

        if (j.contains("geometry") && j["geometry"].is_string()) {
            std::string hex_ewkb = j["geometry"].get<std::string>();
            geom_blob.reserve(hex_ewkb.size() / 2);
            for (size_t i = 0; i < hex_ewkb.size(); i += 2) {
                uint8_t byte = static_cast<uint8_t>(
                    std::stoi(hex_ewkb.substr(i, 2), nullptr, 16)
                );
                geom_blob.push_back(byte);
            }
            found_geometry = true;
        } else if (j.contains("geometry") && j["geometry"].is_object()) {
            std::string geojson = j["geometry"].dump();
            auto geom_info = geo::EWKBParser::parseGeoJSON(geojson);
            geom_blob = geo::EWKBParser::serialize(geom_info);
            found_geometry = true;
        } else if (j.contains("geom_blob") && j["geom_blob"].is_array()) {
            for (auto& byte : j["geom_blob"]) {
                geom_blob.push_back(static_cast<uint8_t>(byte.get<int>()));
            }
            found_geometry = true;
        } else if (j.contains("location")) {
            if (j["location"].is_string()) {
                std::string geojson = j["location"].get<std::string>();
                try {
                    auto geom_info = geo::EWKBParser::parseGeoJSON(geojson);
                    geom_blob = geo::EWKBParser::serialize(geom_info);
                    found_geometry = true;
                } catch (const std::exception& e) {
                    THEMIS_WARN("Geo hook parse error (location string) for {}:{}: {}", table, pk, e.what());
                }
            } else if (j["location"].is_object()) {
                std::string geojson = j["location"].dump();
                try {
                    auto geom_info = geo::EWKBParser::parseGeoJSON(geojson);
                    geom_blob = geo::EWKBParser::serialize(geom_info);
                    found_geometry = true;
                } catch (const std::exception& e) {
                    THEMIS_WARN("Geo hook parse error (location object) for {}:{}: {}", table, pk, e.what());
                }
            }
        } else if (j.contains("geom")) {
            if (j["geom"].is_string()) {
                std::string geojson = j["geom"].get<std::string>();
                try {
                    auto geom_info = geo::EWKBParser::parseGeoJSON(geojson);
                    geom_blob = geo::EWKBParser::serialize(geom_info);
                    found_geometry = true;
                } catch (const std::exception& e) {
                    THEMIS_WARN("Geo hook parse error (geom string) for {}:{}: {}", table, pk, e.what());
                }
            } else if (j["geom"].is_object()) {
                std::string geojson = j["geom"].dump();
                try {
                    auto geom_info = geo::EWKBParser::parseGeoJSON(geojson);
                    geom_blob = geo::EWKBParser::serialize(geom_info);
                    found_geometry = true;
                } catch (const std::exception& e) {
                    THEMIS_WARN("Geo hook parse error (geom object) for {}:{}: {}", table, pk, e.what());
                }
            }
        }

        if (!found_geometry) {
            return false;
        }

        // Parse EWKB and compute sidecar
        auto geom_info = geo::EWKBParser::parse(geom_blob);
        auto sidecar = geo::EWKBParser::computeSidecar(geom_info);
        
        // Add spatial index writes to WriteBatch (atomic!)
        auto status = spatial_mgr->insertBatch(batch, table, pk, sidecar);
        if (!status) {
            THEMIS_WARN("Atomic spatial index insert failed for {}:{}: {}", 
                        table, pk, status.message);
            return false;
        }
        
        THEMIS_DEBUG("Atomic spatial index update added to batch for {}:{}", table, pk);
        return true;

    } catch (const json::exception& e) {
        THEMIS_WARN("Geo hook atomic JSON parse error for {}:{}: {}", table, pk, e.what());
        return false;
    } catch (const std::exception& e) {
        THEMIS_WARN("Geo hook atomic error for {}:{}: {}", table, pk, e.what());
        return false;
    }
}

void GeoIndexHooks::onEntityDelete(
    RocksDBWrapper& db,
    index::SpatialIndexManager* spatial_mgr,
    const std::string& table,
    const std::string& pk,
    const std::vector<uint8_t>& old_blob
) {
    (void)db; // May be used for direct RocksDB access in future
    // Skip if spatial index not available
    if (!spatial_mgr || !spatial_mgr->hasSpatialIndex(table)) {
        return;
    }

    try {
        // Parse old blob to get geometry sidecar for removal
        std::string blob_str(reinterpret_cast<const char*>(old_blob.data()), 
                           old_blob.size());
        auto j = json::parse(blob_str);

        // Extract geometry blob
        std::vector<uint8_t> geom_blob;
        bool found_geometry = false;

        if (j.contains("geometry") && j["geometry"].is_string()) {
            std::string hex_ewkb = j["geometry"].get<std::string>();
            geom_blob.reserve(hex_ewkb.size() / 2);
            for (size_t i = 0; i < hex_ewkb.size(); i += 2) {
                uint8_t byte = static_cast<uint8_t>(
                    std::stoi(hex_ewkb.substr(i, 2), nullptr, 16)
                );
                geom_blob.push_back(byte);
            }
            found_geometry = true;
        } else if (j.contains("geometry") && j["geometry"].is_object()) {
            std::string geojson = j["geometry"].dump();
            auto geom_info = geo::EWKBParser::parseGeoJSON(geojson);
            geom_blob = geo::EWKBParser::serialize(geom_info);
            found_geometry = true;
        } else if (j.contains("geom_blob") && j["geom_blob"].is_array()) {
            for (auto& byte : j["geom_blob"]) {
                geom_blob.push_back(static_cast<uint8_t>(byte.get<int>()));
            }
            found_geometry = true;
        }

        if (!found_geometry) {
            return;
        }

        // Parse and compute sidecar
        auto geom_info = geo::EWKBParser::parse(geom_blob);
        auto sidecar = geo::EWKBParser::computeSidecar(geom_info);

        // Remove from spatial index
        auto status = spatial_mgr->remove(table, pk, sidecar);
        if (!status) {
            THEMIS_WARN("Spatial index remove failed for {}:{}: {}", 
                        table, pk, status.message);
        } else {
            THEMIS_DEBUG("Spatial index entry removed for {}:{}", table, pk);
        }

    } catch (const std::exception& e) {
        // Parse error - log but don't fail the delete
        THEMIS_WARN("Geo hook delete error for {}:{}: {}", table, pk, e.what());
    }
}

} // namespace api
} // namespace themis
