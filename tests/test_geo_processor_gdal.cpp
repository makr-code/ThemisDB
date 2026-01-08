/**
 * @file test_geo_processor_gdal.cpp
 * @brief Unit tests for GDAL integration in GeoProcessor
 * 
 * Tests Shapefile and GeoTIFF processing with GDAL/OGR.
 * 
 * @author ThemisDB Team
 * @date January 2026
 */

#include <gtest/gtest.h>
#include "content/geo_processor.h"
#include <fstream>
#include <filesystem>

#ifdef THEMIS_ENABLE_GDAL
#include <gdal/gdal.h>
#include <gdal/gdal_priv.h>
#include <gdal/ogrsf_frmts.h>
#endif

using namespace themis::content;

class GeoProcessorGDALTest : public ::testing::Test {
protected:
    void SetUp() override {
        processor = std::make_unique<GeoProcessor>();
        
        PluginConfig config;
        config.data["crs.default"] = "EPSG:4326";
        config.data["limits.max_features"] = 100000;
        config.data["analysis.centroid"] = true;
        
        ASSERT_TRUE(processor->initialize(config));
        
        // Create test data directory
        test_dir = std::filesystem::temp_directory_path() / "themis_geo_test";
        std::filesystem::create_directories(test_dir);
    }
    
    void TearDown() override {
        processor->shutdown();
        
        // Clean up test files
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }
    
#ifdef THEMIS_ENABLE_GDAL
    /**
     * @brief Create a simple test Shapefile with points
     */
    std::string createTestShapefile(int num_points = 10) {
        std::string shp_path = (test_dir / "test_points.shp").string();
        
        // Create shapefile driver
        GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
        if (!driver) {
            return "";
        }
        
        // Create dataset
        GDALDataset* dataset = driver->Create(shp_path.c_str(), 0, 0, 0, GDT_Unknown, nullptr);
        if (!dataset) {
            return "";
        }
        
        // Create layer with WGS84 projection
        OGRSpatialReference srs;
        srs.importFromEPSG(4326);
        
        OGRLayer* layer = dataset->CreateLayer("test_layer", &srs, wkbPoint, nullptr);
        if (!layer) {
            GDALClose(dataset);
            return "";
        }
        
        // Add field definitions
        OGRFieldDefn field_id("id", OFTInteger);
        OGRFieldDefn field_name("name", OFTString);
        layer->CreateField(&field_id);
        layer->CreateField(&field_name);
        
        // Add features
        for (int i = 0; i < num_points; ++i) {
            OGRFeature* feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
            
            // Set attributes
            feature->SetField("id", i);
            feature->SetField("name", ("Point_" + std::to_string(i)).c_str());
            
            // Set geometry
            OGRPoint point;
            point.setX(-122.0 + i * 0.1);  // Longitude
            point.setY(37.0 + i * 0.1);     // Latitude
            feature->SetGeometry(&point);
            
            // Add to layer
            if (layer->CreateFeature(feature) != OGRERR_NONE) {
                OGRFeature::DestroyFeature(feature);
                GDALClose(dataset);
                return "";
            }
            
            OGRFeature::DestroyFeature(feature);
        }
        
        GDALClose(dataset);
        return shp_path;
    }
    
    /**
     * @brief Create a simple test GeoTIFF
     */
    std::string createTestGeoTIFF(int width = 100, int height = 100) {
        std::string tif_path = (test_dir / "test_raster.tif").string();
        
        // Create GeoTIFF driver
        GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
        if (!driver) {
            return "";
        }
        
        // Create dataset with 3 bands (RGB)
        GDALDataset* dataset = driver->Create(
            tif_path.c_str(), 
            width, 
            height, 
            3,  // 3 bands (RGB)
            GDT_Byte, 
            nullptr
        );
        
        if (!dataset) {
            return "";
        }
        
        // Set geotransform (origin at -122, 37, pixel size 0.001 degrees)
        double geotransform[6] = {
            -122.0,  // Origin X
            0.001,   // Pixel width
            0.0,     // Rotation X
            37.0,    // Origin Y
            0.0,     // Rotation Y
            -0.001   // Pixel height (negative for north-up)
        };
        dataset->SetGeoTransform(geotransform);
        
        // Set projection to WGS84
        OGRSpatialReference srs;
        srs.importFromEPSG(4326);
        char* wkt = nullptr;
        srs.exportToWkt(&wkt);
        dataset->SetProjection(wkt);
        CPLFree(wkt);
        
        // Write some test data to bands
        std::vector<uint8_t> red_data(width * height, 255);
        std::vector<uint8_t> green_data(width * height, 128);
        std::vector<uint8_t> blue_data(width * height, 64);
        
        dataset->GetRasterBand(1)->RasterIO(
            GF_Write, 0, 0, width, height,
            red_data.data(), width, height, GDT_Byte, 0, 0
        );
        dataset->GetRasterBand(2)->RasterIO(
            GF_Write, 0, 0, width, height,
            green_data.data(), width, height, GDT_Byte, 0, 0
        );
        dataset->GetRasterBand(3)->RasterIO(
            GF_Write, 0, 0, width, height,
            blue_data.data(), width, height, GDT_Byte, 0, 0
        );
        
        GDALClose(dataset);
        return tif_path;
    }
    
    /**
     * @brief Read file into vector
     */
    std::vector<uint8_t> readFile(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            return {};
        }
        
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<uint8_t> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            return {};
        }
        
        return buffer;
    }
#endif
    
    std::unique_ptr<GeoProcessor> processor;
    std::filesystem::path test_dir;
};

// ============================================================================
// Basic Tests (No GDAL Required)
// ============================================================================

TEST_F(GeoProcessorGDALTest, InitializeAndShutdown) {
    EXPECT_TRUE(processor->healthCheck());
}

TEST_F(GeoProcessorGDALTest, GetInfo) {
    auto info = processor->getInfo();
    
    EXPECT_EQ(info.name, "geo-processor");
    EXPECT_FALSE(info.mime_types.empty());
    EXPECT_FALSE(info.extensions.empty());
    
    // Check for GeoTIFF support
    EXPECT_NE(std::find(info.mime_types.begin(), info.mime_types.end(), "image/tiff"), 
              info.mime_types.end());
    EXPECT_NE(std::find(info.extensions.begin(), info.extensions.end(), "tif"), 
              info.extensions.end());
}

TEST_F(GeoProcessorGDALTest, CanProcessMimeTypes) {
    EXPECT_TRUE(processor->canProcess("application/geo+json"));
    EXPECT_TRUE(processor->canProcess("application/x-shapefile"));
    EXPECT_TRUE(processor->canProcess("image/tiff"));
    EXPECT_TRUE(processor->canProcess("image/x-tiff"));
    EXPECT_FALSE(processor->canProcess("text/plain"));
}

TEST_F(GeoProcessorGDALTest, GeoJSONExtraction) {
    // Test basic GeoJSON parsing (doesn't require GDAL)
    std::string geojson = R"({
        "type": "Feature",
        "geometry": {
            "type": "Point",
            "coordinates": [-122.0, 37.0]
        },
        "properties": {
            "name": "Test Point"
        }
    })";
    
    std::vector<uint8_t> blob(geojson.begin(), geojson.end());
    ExtractionOptions options;
    
    auto result = processor->extract(blob, "application/geo+json", options);
    
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.input_size_bytes, 0);
    EXPECT_FALSE(result.text.empty());
    EXPECT_TRUE(result.geo.has_value());
    
    if (result.geo.has_value()) {
        const auto& geo = result.geo.value();
        EXPECT_EQ(geo.geometry_type, "Point");
        EXPECT_EQ(geo.coordinates.size(), 1);
        EXPECT_DOUBLE_EQ(geo.coordinates[0].first, 37.0);   // Latitude
        EXPECT_DOUBLE_EQ(geo.coordinates[0].second, -122.0); // Longitude
    }
}

// ============================================================================
// GDAL-Specific Tests
// ============================================================================

#ifdef THEMIS_ENABLE_GDAL

TEST_F(GeoProcessorGDALTest, ShapefileExtractionBasic) {
    // Create test shapefile
    std::string shp_path = createTestShapefile(5);
    ASSERT_FALSE(shp_path.empty()) << "Failed to create test shapefile";
    
    // Read shapefile into memory
    auto blob = readFile(shp_path);
    ASSERT_FALSE(blob.empty()) << "Failed to read shapefile";
    
    // Extract data
    ExtractionOptions options;
    auto result = processor->extract(blob, "application/x-shapefile", options);
    
    EXPECT_TRUE(result.success) << "Error: " << result.error_message;
    EXPECT_GT(result.input_size_bytes, 0);
    EXPECT_TRUE(result.geo.has_value());
    
    if (result.geo.has_value()) {
        const auto& geo = result.geo.value();
        EXPECT_EQ(geo.geometry_type, "POINT");
        EXPECT_EQ(geo.coordinates.size(), 5) << "Expected 5 points";
        
        // Check CRS
        EXPECT_FALSE(geo.crs.empty());
        EXPECT_NE(geo.crs.find("4326"), std::string::npos) << "Expected EPSG:4326";
        
        // Check properties
        EXPECT_TRUE(geo.properties.contains("feature_count"));
        EXPECT_EQ(geo.properties["feature_count"], "5");
        
        // Check bounding box
        EXPECT_LT(geo.bounds[0], -120.0);  // minX
        EXPECT_GT(geo.bounds[2], -121.0);  // maxX
        EXPECT_LT(geo.bounds[1], 38.0);    // minY
        EXPECT_GT(geo.bounds[3], 36.0);    // maxY
    }
}

TEST_F(GeoProcessorGDALTest, ShapefileExtractionLarge) {
    // Test with 1000 features to verify performance
    std::string shp_path = createTestShapefile(1000);
    ASSERT_FALSE(shp_path.empty()) << "Failed to create test shapefile";
    
    auto blob = readFile(shp_path);
    ASSERT_FALSE(blob.empty());
    
    auto start = std::chrono::steady_clock::now();
    
    ExtractionOptions options;
    auto result = processor->extract(blob, "application/x-shapefile", options);
    
    auto end = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_TRUE(result.success) << "Error: " << result.error_message;
    EXPECT_TRUE(result.geo.has_value());
    
    if (result.geo.has_value()) {
        const auto& geo = result.geo.value();
        EXPECT_EQ(geo.coordinates.size(), 1000);
        EXPECT_EQ(geo.properties["feature_count"], "1000");
    }
    
    // Performance check: 1000 features should take < 1 second
    EXPECT_LT(duration_ms, 1000) << "Processing took " << duration_ms << "ms";
}

TEST_F(GeoProcessorGDALTest, ShapefileSpatialReference) {
    std::string shp_path = createTestShapefile(1);
    ASSERT_FALSE(shp_path.empty());
    
    auto blob = readFile(shp_path);
    ASSERT_FALSE(blob.empty());
    
    ExtractionOptions options;
    auto result = processor->extract(blob, "application/x-shapefile", options);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.geo.has_value());
    
    if (result.geo.has_value()) {
        const auto& geo = result.geo.value();
        
        // Verify WKT format in CRS
        EXPECT_FALSE(geo.crs.empty());
        EXPECT_NE(geo.crs.find("GEOGCS"), std::string::npos) 
            << "Expected WKT format with GEOGCS";
        
        // Should contain WGS84 reference
        EXPECT_NE(geo.crs.find("WGS"), std::string::npos)
            << "Expected WGS84 reference";
    }
}

TEST_F(GeoProcessorGDALTest, GeoTIFFExtractionBasic) {
    // Create test GeoTIFF
    std::string tif_path = createTestGeoTIFF(100, 100);
    ASSERT_FALSE(tif_path.empty()) << "Failed to create test GeoTIFF";
    
    // Read GeoTIFF into memory
    auto blob = readFile(tif_path);
    ASSERT_FALSE(blob.empty()) << "Failed to read GeoTIFF";
    
    // Extract data
    ExtractionOptions options;
    auto result = processor->extract(blob, "image/tiff", options);
    
    EXPECT_TRUE(result.success) << "Error: " << result.error_message;
    EXPECT_GT(result.input_size_bytes, 0);
    EXPECT_TRUE(result.geo.has_value());
    
    if (result.geo.has_value()) {
        const auto& geo = result.geo.value();
        EXPECT_EQ(geo.geometry_type, "Raster");
        
        // Check raster metadata
        EXPECT_TRUE(geo.properties.contains("width"));
        EXPECT_TRUE(geo.properties.contains("height"));
        EXPECT_TRUE(geo.properties.contains("bands"));
        
        EXPECT_EQ(geo.properties["width"], "100");
        EXPECT_EQ(geo.properties["height"], "100");
        EXPECT_EQ(geo.properties["bands"], "3");
        
        // Check geotransform
        EXPECT_TRUE(geo.properties.contains("geotransform_origin_x"));
        EXPECT_TRUE(geo.properties.contains("geotransform_origin_y"));
        EXPECT_TRUE(geo.properties.contains("pixel_width"));
        EXPECT_TRUE(geo.properties.contains("pixel_height"));
        
        // Check projection
        EXPECT_FALSE(geo.crs.empty());
        EXPECT_TRUE(geo.properties.contains("projection"));
        
        // Check EPSG code
        EXPECT_TRUE(geo.properties.contains("epsg_code"));
        EXPECT_EQ(geo.properties["epsg_code"], "EPSG:4326");
        
        // Check bounding box
        EXPECT_LT(geo.bounds[0], -121.0);  // minX
        EXPECT_GT(geo.bounds[2], -122.0);  // maxX
        EXPECT_LT(geo.bounds[1], 36.9);    // minY
        EXPECT_GT(geo.bounds[3], 37.0);    // maxY
    }
}

TEST_F(GeoProcessorGDALTest, GeoTIFFBandMetadata) {
    std::string tif_path = createTestGeoTIFF(50, 50);
    ASSERT_FALSE(tif_path.empty());
    
    auto blob = readFile(tif_path);
    ASSERT_FALSE(blob.empty());
    
    ExtractionOptions options;
    auto result = processor->extract(blob, "image/tiff", options);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.geo.has_value());
    
    if (result.geo.has_value()) {
        const auto& geo = result.geo.value();
        
        // Check band-specific metadata
        for (int i = 1; i <= 3; ++i) {
            std::string prefix = "band_" + std::to_string(i);
            
            EXPECT_TRUE(geo.properties.contains(prefix + "_data_type"));
            EXPECT_TRUE(geo.properties.contains(prefix + "_block_size"));
            EXPECT_TRUE(geo.properties.contains(prefix + "_color_interpretation"));
            
            // Data type should be Byte
            EXPECT_EQ(geo.properties[prefix + "_data_type"], "Byte");
        }
    }
}

TEST_F(GeoProcessorGDALTest, GeoTIFFCoordinateSystem) {
    std::string tif_path = createTestGeoTIFF(100, 100);
    ASSERT_FALSE(tif_path.empty());
    
    auto blob = readFile(tif_path);
    ASSERT_FALSE(blob.empty());
    
    ExtractionOptions options;
    auto result = processor->extract(blob, "image/tiff", options);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.geo.has_value());
    
    if (result.geo.has_value()) {
        const auto& geo = result.geo.value();
        
        // Verify coordinate system parsing
        EXPECT_FALSE(geo.crs.empty());
        
        // Should have WKT format
        EXPECT_NE(geo.crs.find("GEOGCS"), std::string::npos);
        
        // Should have EPSG code
        EXPECT_TRUE(geo.properties.contains("epsg_code"));
        EXPECT_EQ(geo.properties["epsg_code"], "EPSG:4326");
        
        // Check geotransform values are reasonable
        double origin_x = std::stod(geo.properties["geotransform_origin_x"]);
        double origin_y = std::stod(geo.properties["geotransform_origin_y"]);
        double pixel_width = std::stod(geo.properties["pixel_width"]);
        double pixel_height = std::stod(geo.properties["pixel_height"]);
        
        EXPECT_DOUBLE_EQ(origin_x, -122.0);
        EXPECT_DOUBLE_EQ(origin_y, 37.0);
        EXPECT_DOUBLE_EQ(pixel_width, 0.001);
        EXPECT_DOUBLE_EQ(pixel_height, -0.001);
    }
}

TEST_F(GeoProcessorGDALTest, EmptyBlobHandling) {
    std::vector<uint8_t> empty_blob;
    ExtractionOptions options;
    
    auto result = processor->extract(empty_blob, "application/x-shapefile", options);
    
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

#else

TEST_F(GeoProcessorGDALTest, GDALNotEnabled) {
    // When GDAL is not enabled, should get error message
    std::vector<uint8_t> dummy_blob = {0x00, 0x01, 0x02};
    ExtractionOptions options;
    
    EXPECT_THROW({
        processor->extract(dummy_blob, "application/x-shapefile", options);
    }, std::runtime_error);
}

#endif // THEMIS_ENABLE_GDAL

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(GeoProcessorGDALTest, Statistics) {
    auto stats = processor->getStatistics();
    
    EXPECT_TRUE(stats.contains("files_processed"));
    EXPECT_TRUE(stats.contains("total_features"));
    EXPECT_TRUE(stats.contains("errors"));
    
    EXPECT_GE(stats["files_processed"].get<uint64_t>(), 0);
    EXPECT_GE(stats["total_features"].get<uint64_t>(), 0);
    EXPECT_GE(stats["errors"].get<uint64_t>(), 0);
}
