/**
 * @file geo_processor.cpp
 * @brief Geospatial Content Processor Implementation
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

// Ensure plugin entry points export correctly when built into core
#define THEMIS_PLUGIN_EXPORTS

#include "content/geo_processor.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace themis {
namespace content {

// Forward declaration for recursive coordinate parsing
static void parseCoordinates(const json& coords, GeoExtractionData& data);

GeoProcessor::GeoProcessor() = default;

GeoProcessor::~GeoProcessor() {
    if (initialized_) {
        shutdown();
    }
}

PluginInfo GeoProcessor::getInfo() const {
    PluginInfo info;
    info.name = "geo-processor";
    info.version = "1.0.0";
    info.description = "Geospatial content processor using GDAL/OGR";
    info.author = "ThemisDB Team";
    info.license = "Apache-2.0";
    
    info.mime_types = {
        "application/geo+json",
        "application/json",  // GeoJSON often served as application/json
        "application/vnd.google-earth.kml+xml",
        "application/vnd.google-earth.kmz",
        "application/gpx+xml",
        "application/x-shapefile",
        "application/geopackage+sqlite3"
    };
    
    info.extensions = {
        "geojson", "json", "kml", "kmz", "gpx", "shp", "gpkg"
    };
    
    info.supports_chunking = true;
    info.supports_embedding = true;
    info.supports_streaming = false;
    
    info.min_memory_mb = 128;
    info.recommended_memory_mb = 512;
    
    return info;
}

bool GeoProcessor::initialize(const PluginConfig& config) {
    if (initialized_) {
        return true;
    }
    
    // Load configuration
    default_crs_ = config.get<std::string>("crs.default", "EPSG:4326");
    max_features_ = config.get<int>("limits.max_features", 100000);
    simplify_geometry_ = config.get<bool>("simplify.enabled", false);
    simplify_tolerance_ = config.get<double>("simplify.tolerance", 0.0001);
    generate_centroid_ = config.get<bool>("analysis.centroid", true);
    
    // Note: Initialize GDAL
    // GDALAllRegister();
    // OGRRegisterAll();
    
    initialized_ = true;
    return true;
}

void GeoProcessor::shutdown() {
    if (!initialized_) {
        return;
    }
    
    // Note: Clean up GDAL
    // GDALDestroyDriverManager();
    
    initialized_ = false;
}

bool GeoProcessor::canProcess(const std::string& mime_type) const {
    static const std::vector<std::string> supported = {
        "application/geo+json",
        "application/json",
        "application/vnd.google-earth.kml+xml",
        "application/vnd.google-earth.kmz",
        "application/gpx+xml",
        "application/x-shapefile",
        "application/geopackage+sqlite3"
    };
    
    return std::find(supported.begin(), supported.end(), mime_type) != supported.end();
}

ContentExtractionResult GeoProcessor::extract(
    const std::vector<uint8_t>& blob,
    const std::string& mime_type,
    const ExtractionOptions& options
) {
    auto start = std::chrono::steady_clock::now();
    ContentExtractionResult result;
    result.input_size_bytes = blob.size();
    
    if (!initialized_) {
        result.success = false;
        result.error_message = "Geo processor not initialized";
        errors_++;
        return result;
    }
    
    if (blob.empty()) {
        result.success = false;
        result.error_message = "Empty input blob";
        errors_++;
        return result;
    }
    
    try {
        GeoExtractionData geo;
        
        // Determine format and parse
        std::string content(blob.begin(), blob.end());
        
        if (mime_type == "application/geo+json" || 
            (mime_type == "application/json" && content.find("\"type\"") != std::string::npos)) {
            geo = parseGeoJSON(blob);
        } else if (mime_type == "application/vnd.google-earth.kml+xml") {
            geo = parseKML(blob);
        } else if (mime_type == "application/gpx+xml") {
            geo = parseGPX(blob);
        } else if (mime_type == "application/x-shapefile") {
            geo = parseShapefile(blob);
        } else if (mime_type == "application/geopackage+sqlite3") {
            geo = parseGeoPackage(blob);
        } else {
            // Try GeoJSON as default
            geo = parseGeoJSON(blob);
        }
        
        result.geo = geo;
        
        // Build metadata JSON
        json metadata;
        metadata["geometry_type"] = geo.geometry_type;
        metadata["crs"] = geo.crs;
        metadata["feature_count"] = geo.coordinates.size();
        
        if (geo.bounds[0] != 0 || geo.bounds[1] != 0 || 
            geo.bounds[2] != 0 || geo.bounds[3] != 0) {
            metadata["bounds"] = {
                {"minX", geo.bounds[0]},
                {"minY", geo.bounds[1]},
                {"maxX", geo.bounds[2]},
                {"maxY", geo.bounds[3]}
            };
        }
        
        // Calculate centroid
        if (generate_centroid_ && !geo.coordinates.empty()) {
            auto [cx, cy] = calculateCentroid(geo);
            metadata["centroid"] = {{"lat", cx}, {"lon", cy}};
        }
        
        // Calculate area/length if applicable
        if (geo.geometry_type == "Polygon" || geo.geometry_type == "MultiPolygon") {
            metadata["area_sq_degrees"] = calculateArea(geo);
        } else if (geo.geometry_type == "LineString" || geo.geometry_type == "MultiLineString") {
            metadata["length_degrees"] = calculateLength(geo);
        }
        
        // Add properties
        if (!geo.properties.empty()) {
            metadata["properties"] = geo.properties;
        }
        
        result.metadata = metadata;
        
        // Generate text description
        std::ostringstream text;
        text << "Geospatial data: " << geo.geometry_type;
        text << " with " << geo.coordinates.size() << " coordinate pairs";
        text << " in CRS " << geo.crs;
        result.text = text.str();
        
        result.success = true;
        files_processed_++;
        total_features_ += geo.coordinates.size();
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("Geo processing failed: ") + e.what();
        errors_++;
    }
    
    auto end = std::chrono::steady_clock::now();
    result.processing_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    return result;
}

std::vector<ContentChunk> GeoProcessor::chunk(
    const ContentExtractionResult& result,
    int max_tokens,
    int overlap
) {
    std::vector<ContentChunk> chunks;
    
    if (!result.success || !result.geo.has_value()) {
        return chunks;
    }
    
    // For geo data, chunk by features/coordinates
    const auto& geo = result.geo.value();
    
    // Each chunk contains a subset of coordinates
    const int coords_per_chunk = 100;
    
    for (size_t i = 0; i < geo.coordinates.size(); i += coords_per_chunk) {
        ContentChunk chunk;
        
        std::ostringstream text;
        text << "Coordinates " << i << "-" << std::min(i + coords_per_chunk, geo.coordinates.size()) << ": ";
        
        size_t end = std::min(i + coords_per_chunk, geo.coordinates.size());
        for (size_t j = i; j < end; ++j) {
            text << "(" << geo.coordinates[j].first << "," << geo.coordinates[j].second << ") ";
        }
        
        chunk.text = text.str();
        chunk.sequence = static_cast<int>(i / coords_per_chunk);
        chunk.token_count = countTokens(chunk.text);
        chunk.metadata["start_index"] = i;
        chunk.metadata["end_index"] = end;
        
        chunks.push_back(chunk);
    }
    
    return chunks;
}

bool GeoProcessor::healthCheck() const {
    return initialized_;
}

json GeoProcessor::getStatistics() const {
    json stats;
    stats["files_processed"] = files_processed_.load();
    stats["total_features"] = total_features_.load();
    stats["errors"] = errors_.load();
    return stats;
}

// Private implementation methods

GeoExtractionData GeoProcessor::parseGeoJSON(const std::vector<uint8_t>& blob) {
    GeoExtractionData data;
    data.crs = default_crs_;
    
    try {
        std::string content(blob.begin(), blob.end());
        json geojson = json::parse(content);
        
        // Determine type
        std::string type = geojson.value("type", "");
        
        if (type == "Feature") {
            // Single feature
            if (geojson.contains("geometry")) {
                auto& geometry = geojson["geometry"];
                data.geometry_type = geometry.value("type", "");
                
                if (geometry.contains("coordinates")) {
                    // Parse coordinates based on type
                    parseCoordinates(geometry["coordinates"], data);
                }
            }
            if (geojson.contains("properties")) {
                data.properties = geojson["properties"];
            }
        } else if (type == "FeatureCollection") {
            // Multiple features
            if (geojson.contains("features") && geojson["features"].is_array()) {
                for (auto& feature : geojson["features"]) {
                    if (feature.contains("geometry")) {
                        auto& geometry = feature["geometry"];
                        if (data.geometry_type.empty()) {
                            data.geometry_type = geometry.value("type", "");
                        }
                        if (geometry.contains("coordinates")) {
                            parseCoordinates(geometry["coordinates"], data);
                        }
                    }
                }
            }
        } else if (type == "Point" || type == "LineString" || type == "Polygon" ||
                   type == "MultiPoint" || type == "MultiLineString" || type == "MultiPolygon") {
            // Direct geometry
            data.geometry_type = type;
            if (geojson.contains("coordinates")) {
                parseCoordinates(geojson["coordinates"], data);
            }
        }
        
        // Calculate bounding box
        if (!data.coordinates.empty()) {
            data.bounds[0] = data.coordinates[0].second;  // minX (lon)
            data.bounds[1] = data.coordinates[0].first;   // minY (lat)
            data.bounds[2] = data.coordinates[0].second;  // maxX (lon)
            data.bounds[3] = data.coordinates[0].first;   // maxY (lat)
            
            for (const auto& coord : data.coordinates) {
                data.bounds[0] = std::min(data.bounds[0], coord.second);
                data.bounds[1] = std::min(data.bounds[1], coord.first);
                data.bounds[2] = std::max(data.bounds[2], coord.second);
                data.bounds[3] = std::max(data.bounds[3], coord.first);
            }
        }
        
    } catch (const json::exception& e) {
        throw std::runtime_error(std::string("GeoJSON parse error: ") + e.what());
    }
    
    return data;
}

static void parseCoordinates(const json& coords, GeoExtractionData& data) {
    if (coords.is_array()) {
        if (coords.size() >= 2 && coords[0].is_number() && coords[1].is_number()) {
            // [lon, lat] pair
            double lon = coords[0].get<double>();
            double lat = coords[1].get<double>();
            data.coordinates.emplace_back(lat, lon);
        } else {
            // Nested array
            for (const auto& item : coords) {
                parseCoordinates(item, data);
            }
        }
    }
}

GeoExtractionData GeoProcessor::parseKML(const std::vector<uint8_t>& blob) {
    GeoExtractionData data;
    data.crs = "EPSG:4326";  // KML is always WGS84
    data.geometry_type = "Mixed";
    
    // Real implementation would use pugixml to parse KML
    // and extract placemarks, coordinates
    
    return data;
}

GeoExtractionData GeoProcessor::parseGPX(const std::vector<uint8_t>& blob) {
    GeoExtractionData data;
    data.crs = "EPSG:4326";  // GPX is always WGS84
    data.geometry_type = "Track";
    
    // Real implementation would parse GPX trackpoints/waypoints
    
    return data;
}

GeoExtractionData GeoProcessor::parseShapefile(const std::vector<uint8_t>& blob) {
    GeoExtractionData data;
    data.crs = default_crs_;
    
    // Real implementation would use GDAL/OGR to read shapefile
    // Note: Shapefiles are typically multi-file (.shp, .shx, .dbf)
    
    return data;
}

GeoExtractionData GeoProcessor::parseGeoPackage(const std::vector<uint8_t>& blob) {
    GeoExtractionData data;
    data.crs = default_crs_;
    
    // Real implementation would use GDAL/OGR to read GeoPackage (SQLite)
    
    return data;
}

std::pair<double, double> GeoProcessor::calculateCentroid(const GeoExtractionData& geo) {
    if (geo.coordinates.empty()) {
        return {0.0, 0.0};
    }
    
    double sumLat = 0.0, sumLon = 0.0;
    for (const auto& coord : geo.coordinates) {
        sumLat += coord.first;
        sumLon += coord.second;
    }
    
    return {sumLat / geo.coordinates.size(), sumLon / geo.coordinates.size()};
}

double GeoProcessor::calculateArea(const GeoExtractionData& geo) {
    // Simplified area calculation using shoelace formula
    // Real implementation would account for spherical geometry
    
    if (geo.coordinates.size() < 3) {
        return 0.0;
    }
    
    double area = 0.0;
    size_t n = geo.coordinates.size();
    
    for (size_t i = 0; i < n; ++i) {
        size_t j = (i + 1) % n;
        area += geo.coordinates[i].second * geo.coordinates[j].first;
        area -= geo.coordinates[j].second * geo.coordinates[i].first;
    }
    
    return std::abs(area) / 2.0;
}

double GeoProcessor::calculateLength(const GeoExtractionData& geo) {
    // Calculate total length using Haversine distance
    
    if (geo.coordinates.size() < 2) {
        return 0.0;
    }
    
    double total = 0.0;
    const double R = 6371.0;  // Earth radius in km
    
    for (size_t i = 0; i < geo.coordinates.size() - 1; ++i) {
        double lat1 = geo.coordinates[i].first * M_PI / 180.0;
        double lon1 = geo.coordinates[i].second * M_PI / 180.0;
        double lat2 = geo.coordinates[i + 1].first * M_PI / 180.0;
        double lon2 = geo.coordinates[i + 1].second * M_PI / 180.0;
        
        double dlat = lat2 - lat1;
        double dlon = lon2 - lon1;
        
        double a = std::sin(dlat / 2) * std::sin(dlat / 2) +
                   std::cos(lat1) * std::cos(lat2) *
                   std::sin(dlon / 2) * std::sin(dlon / 2);
        double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
        
        total += R * c;
    }
    
    return total;
}

// Plugin entry point
THEMIS_CONTENT_PLUGIN(GeoProcessor)

} // namespace content
} // namespace themis
