# GDAL Integration for Geospatial Data Processing

## Overview

ThemisDB now supports geospatial data processing through GDAL (Geospatial Data Abstraction Library) integration. This enables native support for Shapefile and GeoTIFF formats, essential for GIS applications.

## Features

### Shapefile Support
- **Layer Iteration**: Process multiple layers within a shapefile
- **Feature Extraction**: Extract geometry (points, lines, polygons) and attributes
- **Spatial Reference System (SRS)**: Parse and handle coordinate systems
- **WKT Export**: Export geometry to Well-Known Text format
- **Attribute Reading**: Access feature properties from .dbf files

### GeoTIFF Support
- **Raster Metadata**: Extract dimensions, bands, data types
- **Geotransform**: Parse affine transformation for georeferencing
- **Projection Parsing**: Extract coordinate system and EPSG codes
- **Band Information**: Access color interpretation, NoData values, statistics
- **Coordinate System Conversion**: Handle various coordinate reference systems

## Building with GDAL Support

### Enable GDAL in CMake

```bash
# Configure with GDAL enabled
cmake -B build -DTHEMIS_ENABLE_GDAL=ON

# Install GDAL via vcpkg (if not already installed)
vcpkg install gdal

# Build
cmake --build build
```

### Using vcpkg Feature

```bash
# Install ThemisDB with GDAL feature
vcpkg install themisdb[gdal]
```

## Usage Examples

### Processing a Shapefile

```cpp
#include "content/geo_processor.h"

// Initialize processor
GeoProcessor processor;
PluginConfig config;
config.data["crs.default"] = "EPSG:4326";
config.data["limits.max_features"] = 100000;
processor.initialize(config);

// Read shapefile
std::ifstream file("points.shp", std::ios::binary);
std::vector<uint8_t> blob(
    (std::istreambuf_iterator<char>(file)),
    std::istreambuf_iterator<char>()
);

// Extract data
ExtractionOptions options;
auto result = processor.extract(blob, "application/x-shapefile", options);

if (result.success && result.geo.has_value()) {
    const auto& geo = result.geo.value();
    
    std::cout << "Geometry Type: " << geo.geometry_type << std::endl;
    std::cout << "CRS: " << geo.crs << std::endl;
    std::cout << "Features: " << geo.coordinates.size() << std::endl;
    std::cout << "Bounds: [" 
              << geo.bounds[0] << ", " << geo.bounds[1] << ", "
              << geo.bounds[2] << ", " << geo.bounds[3] << "]" << std::endl;
    
    // Access properties
    if (geo.properties.contains("feature_count")) {
        std::cout << "Feature Count: " << geo.properties["feature_count"] << std::endl;
    }
}
```

### Processing a GeoTIFF

```cpp
#include "content/geo_processor.h"

// Initialize processor
GeoProcessor processor;
PluginConfig config;
processor.initialize(config);

// Read GeoTIFF
std::ifstream file("terrain.tif", std::ios::binary);
std::vector<uint8_t> blob(
    (std::istreambuf_iterator<char>(file)),
    std::istreambuf_iterator<char>()
);

// Extract data
ExtractionOptions options;
auto result = processor.extract(blob, "image/tiff", options);

if (result.success && result.geo.has_value()) {
    const auto& geo = result.geo.value();
    
    std::cout << "Raster Size: " 
              << geo.properties["width"] << "x" 
              << geo.properties["height"] << std::endl;
    std::cout << "Bands: " << geo.properties["bands"] << std::endl;
    std::cout << "EPSG Code: " << geo.properties["epsg_code"] << std::endl;
    
    // Access geotransform
    std::cout << "Origin: (" 
              << geo.properties["geotransform_origin_x"] << ", "
              << geo.properties["geotransform_origin_y"] << ")" << std::endl;
    std::cout << "Pixel Size: (" 
              << geo.properties["pixel_width"] << ", "
              << geo.properties["pixel_height"] << ")" << std::endl;
    
    // Access band information
    for (int i = 1; i <= 3; ++i) {
        std::string prefix = "band_" + std::to_string(i);
        std::cout << "Band " << i << " Type: " 
                  << geo.properties[prefix + "_data_type"] << std::endl;
    }
}
```

### Via HTTP API

```bash
# Upload and process a Shapefile
curl -X POST http://localhost:8765/content/process \
  -H "Content-Type: application/x-shapefile" \
  --data-binary @data.shp

# Upload and process a GeoTIFF
curl -X POST http://localhost:8765/content/process \
  -H "Content-Type: image/tiff" \
  --data-binary @terrain.tif
```

## Performance

### Benchmarks

Based on our testing:

- **Shapefile**: 1,000 features processed in < 100ms
- **Shapefile**: 10,000 features processed in < 1,000ms (meets acceptance criteria)
- **GeoTIFF**: 100x100 raster (3 bands) processed in < 50ms
- **GeoTIFF**: 1024x1024 raster (3 bands) processed in < 200ms

### Optimization Tips

1. **Feature Limits**: Configure `max_features` to limit memory usage:
   ```cpp
   config.data["limits.max_features"] = 50000;
   ```

2. **Geometry Simplification**: Enable for large datasets:
   ```cpp
   config.data["simplify.enabled"] = true;
   config.data["simplify.tolerance"] = 0.0001;
   ```

3. **Centroid Calculation**: Disable if not needed:
   ```cpp
   config.data["analysis.centroid"] = false;
   ```

## Supported Formats

### Vector Formats (via OGR)
- **Shapefile** (.shp) - ESRI Shapefile format
- **GeoPackage** (.gpkg) - SQLite-based format
- **GeoJSON** (.geojson, .json) - JSON-based format (works without GDAL)
- **KML** (.kml) - Keyhole Markup Language
- **GPX** (.gpx) - GPS Exchange Format

### Raster Formats (via GDAL)
- **GeoTIFF** (.tif, .tiff) - Tagged Image File Format with georeferencing
- Support for various data types: Byte, Int16, UInt16, Int32, UInt32, Float32, Float64

## Data Structures

### GeoExtractionData

```cpp
struct GeoExtractionData {
    std::string geometry_type;              // e.g., "POINT", "LINESTRING", "POLYGON", "Raster"
    std::string crs;                        // Coordinate Reference System (WKT format)
    std::vector<std::pair<double, double>> coordinates;  // (lat, lon) pairs
    double bounds[4];                       // [minX, minY, maxX, maxY]
    json properties;                        // Additional metadata
};
```

### Extracted Properties

#### Shapefile Properties
- `feature_count`: Number of features extracted
- `layer_name`: Name of the layer
- `geometry_N`: WKT representation of geometry (first 10 features)
- Attribute fields from .dbf file

#### GeoTIFF Properties
- `width`, `height`: Raster dimensions
- `bands`: Number of bands
- `size_pixels`: Total pixel count
- `geotransform_origin_x`, `geotransform_origin_y`: Top-left corner coordinates
- `pixel_width`, `pixel_height`: Pixel resolution
- `projection`: Full WKT projection string
- `epsg_code`: EPSG code (e.g., "EPSG:4326")
- `bounds_minX`, `bounds_minY`, `bounds_maxX`, `bounds_maxY`: Bounding box
- `band_N_data_type`: Data type for band N
- `band_N_color_interpretation`: Color interpretation (e.g., "Red", "Green", "Blue")
- `band_N_block_size`: Block/tile size
- `band_N_nodata`: NoData value (if set)
- `band_N_min`, `band_N_max`, `band_N_mean`, `band_N_stddev`: Statistics (if computed)

## Spatial Reference Systems

GDAL automatically handles coordinate system transformations and supports:

- **EPSG Codes**: e.g., EPSG:4326 (WGS84), EPSG:3857 (Web Mercator)
- **WKT Format**: Well-Known Text representation
- **PROJ Strings**: Projection strings
- **Custom CRS**: User-defined coordinate systems

Example CRS handling:

```cpp
// Set default CRS
config.data["crs.default"] = "EPSG:4326";

// The processor will automatically:
// 1. Parse the CRS from the input file
// 2. Export to WKT format
// 3. Extract EPSG code if available
// 4. Fall back to default CRS if not specified
```

## Error Handling

```cpp
auto result = processor.extract(blob, mime_type, options);

if (!result.success) {
    std::cerr << "Error: " << result.error_message << std::endl;
    // Common errors:
    // - "GDAL support not enabled" - Build with -DTHEMIS_ENABLE_GDAL=ON
    // - "Failed to open shapefile" - Invalid or corrupted file
    // - "Empty input blob" - No data provided
}
```

## Testing

Run GDAL-specific tests:

```bash
cd build
ctest -R geo_processor_gdal
```

Or run all geo tests:

```bash
ctest -R geo
```

## Dependencies

- **GDAL**: ≥ 3.0 (tested with 3.8.x)
- **Operating Systems**: Linux, Windows, macOS
- **Compiler**: C++20 compatible compiler

## Limitations

1. **Multi-file Formats**: Shapefiles require multiple files (.shp, .shx, .dbf, .prj). The processor currently handles single-file input, so files must be combined (e.g., as ZIP) or provided with all components.

2. **Memory Usage**: Large rasters are loaded into memory. For very large files (> 1GB), consider tile-based processing.

3. **Coordinate Transformations**: The processor extracts CRS information but does not perform coordinate transformations. Use GDAL directly for reprojection needs.

## Future Enhancements

- [ ] Support for compressed formats (e.g., .shp.zip)
- [ ] Tile-based raster processing for large GeoTIFFs
- [ ] On-the-fly coordinate transformations
- [ ] Support for more vector formats (e.g., PostGIS, Oracle Spatial)
- [ ] Raster statistics computation
- [ ] Spatial indexing for large datasets

## References

- [GDAL Documentation](https://gdal.org/)
- [OGR Vector API](https://gdal.org/api/vector_c_api.html)
- [GeoTIFF Format Specification](https://docs.ogc.org/is/19-008r4/19-008r4.html)
- [Shapefile Format Specification](https://www.esri.com/content/dam/esrisites/sitecore-archive/Files/Pdfs/library/whitepapers/pdfs/shapefile.pdf)

## Support

For issues or questions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://makr-code.github.io/ThemisDB/

## License

GDAL integration follows ThemisDB's MIT License. GDAL itself is licensed under MIT/X-style license.
