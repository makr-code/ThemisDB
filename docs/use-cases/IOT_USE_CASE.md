# IoT & Sensor Networks with ThemisDB

## Overview

This guide demonstrates building a production-grade IoT platform using ThemisDB's time-series capabilities, complex event processing (CEP), and graph topology management. We'll cover high-throughput sensor data ingestion, real-time anomaly detection, device management, and predictive analytics.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Schema Design](#schema-design)
3. [Time-Series Data Ingestion](#time-series-data-ingestion)
4. [Real-Time Aggregation](#real-time-aggregation)
5. [Anomaly Detection](#anomaly-detection)
6. [Device Management](#device-management)
7. [Historical Analysis](#historical-analysis)
8. [Edge-to-Cloud Architecture](#edge-to-cloud-architecture)
9. [Performance Optimization](#performance-optimization)
10. [Monitoring & Operations](#monitoring--operations)

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    IoT Device Layer                         │
├──────────┬──────────┬──────────┬──────────┬─────────────────┤
│ Temp     │ Humidity │ Pressure │ Motion   │ Industrial      │
│ Sensors  │ Sensors  │ Sensors  │ Sensors  │ Equipment       │
└────┬─────┴────┬─────┴────┬─────┴────┬─────┴────┬────────────┘
     │          │          │          │          │
     └──────────┴──────────┴──────────┴──────────┘
                         │
         ┌───────────────▼────────────────┐
         │      Edge Gateway Layer        │
         │  - Data Aggregation            │
         │  - Protocol Translation        │
         │  - Local Processing            │
         │  - Buffering & Retry           │
         └───────────────┬────────────────┘
                         │
         ┌───────────────▼────────────────┐
         │     Message Broker/Queue       │
         │  - MQTT Broker                 │
         │  - Kafka/Pulsar                │
         │  - Load Balancing              │
         └───────────────┬────────────────┘
                         │
         ┌───────────────▼────────────────┐
         │      Ingestion Service         │
         │  - Data Validation             │
         │  - Batch Processing            │
         │  - Schema Evolution            │
         └───────────────┬────────────────┘
                         │
         ┌───────────────▼────────────────┐
         │       ThemisDB Cluster         │
         │  ┌──────────────────────────┐  │
         │  │ Time-Series (Sensor Data)│  │
         │  │ - High-speed writes      │  │
         │  │ - Downsampling           │  │
         │  │ - Retention policies     │  │
         │  ├──────────────────────────┤  │
         │  │ Device Registry (Doc)    │  │
         │  │ - Device metadata        │  │
         │  │ - Configuration          │  │
         │  ├──────────────────────────┤  │
         │  │ Topology (Graph)         │  │
         │  │ - Device relationships   │  │
         │  │ - Network hierarchy      │  │
         │  ├──────────────────────────┤  │
         │  │ Alerts & Events          │  │
         │  │ - CEP rules              │  │
         │  │ - Alert history          │  │
         │  ├──────────────────────────┤  │
         │  │ ML Models & Predictions  │  │
         │  │ - Anomaly detection      │  │
         │  │ - Forecasting            │  │
         │  └──────────────────────────┘  │
         └────────────────────────────────┘
                         │
         ┌───────────────▼────────────────┐
         │     Application Layer          │
         │  - Real-time Dashboard         │
         │  - Analytics Platform          │
         │  - Alert Management            │
         │  - Device Provisioning         │
         └────────────────────────────────┘
```

## Schema Design

### Sensor Data Collection (Time-Series)

```aql
// Create time-series collection for sensor readings
CREATE COLLECTION sensor_readings {
    type: "timeseries",
    timeseries: {
        time_field: "timestamp",
        meta_field: "metadata",
        granularity: "seconds"
    },
    sharding: {
        strategy: "time_range",
        time_field: "timestamp",
        chunk_interval: "1 day",
        shards: 32
    },
    retention: {
        policy: "tiered",
        tiers: [
            {
                name: "hot",
                duration: "7 days",
                storage: "nvme",
                compression: "lz4"
            },
            {
                name: "warm",
                duration: "30 days",
                storage: "ssd",
                compression: "zstd",
                downsample: {
                    interval: "1 minute",
                    aggregates: ["avg", "min", "max", "count"]
                }
            },
            {
                name: "cold",
                duration: "365 days",
                storage: "hdd",
                compression: "zstd",
                downsample: {
                    interval: "1 hour",
                    aggregates: ["avg", "min", "max"]
                }
            }
        ]
    },
    indexes: {
        composite: [
            ["metadata.device_id", "timestamp"],
            ["metadata.sensor_type", "timestamp"],
            ["metadata.location", "timestamp"]
        ]
    }
}

// Sensor reading document
{
    "timestamp": "2024-01-20T15:45:23.123Z",
    "metadata": {
        "device_id": "sensor-temp-001",
        "sensor_type": "temperature",
        "location": "warehouse-A-zone-3",
        "unit": "celsius",
        "gateway_id": "gateway-001",
        "firmware_version": "2.1.0"
    },
    "value": 23.5,
    "quality": 1.0,  // Quality indicator 0-1
    "raw_value": 23.487,
    "calibration_offset": 0.013,
    "battery_level": 87,
    "signal_strength": -45  // dBm
}

// Multi-sensor reading (for compound devices)
{
    "timestamp": "2024-01-20T15:45:23.123Z",
    "metadata": {
        "device_id": "weather-station-042",
        "sensor_type": "environmental",
        "location": "rooftop-building-7",
        "gateway_id": "gateway-003"
    },
    "readings": {
        "temperature": {
            "value": 18.2,
            "unit": "celsius",
            "quality": 1.0
        },
        "humidity": {
            "value": 65.5,
            "unit": "percent",
            "quality": 1.0
        },
        "pressure": {
            "value": 1013.25,
            "unit": "hPa",
            "quality": 0.95
        },
        "wind_speed": {
            "value": 5.3,
            "unit": "m/s",
            "quality": 1.0
        },
        "wind_direction": {
            "value": 245,
            "unit": "degrees",
            "quality": 1.0
        }
    },
    "battery_level": 92,
    "signal_strength": -52
}
```

### Device Registry

```aql
// Device collection for metadata and configuration
CREATE COLLECTION devices {
    type: "document",
    sharding: {
        strategy: "hash",
        key: "device_id",
        shards: 8
    },
    indexes: {
        unique: ["device_id"],
        composite: [
            ["type", "status"],
            ["location", "type"]
        ]
    }
}

// Device document
{
    "device_id": "sensor-temp-001",
    "device_name": "Temperature Sensor Zone 3A",
    "type": "temperature_sensor",
    "manufacturer": "SensorTech Inc",
    "model": "TH-2000",
    "firmware_version": "2.1.0",
    "hardware_revision": "1.3",
    "serial_number": "ST2000-2024-001234",
    "location": {
        "site": "warehouse-A",
        "zone": "zone-3",
        "position": "shelf-12-top",
        "coordinates": {
            "latitude": 40.7128,
            "longitude": -74.0060,
            "altitude": 10.5
        }
    },
    "network": {
        "protocol": "mqtt",
        "gateway_id": "gateway-001",
        "ip_address": "192.168.1.105",
        "mac_address": "00:1B:44:11:3A:B7",
        "encryption": "tls_1.3"
    },
    "configuration": {
        "sampling_interval": 30,  // seconds
        "reporting_interval": 60,  // seconds
        "threshold_min": -10,
        "threshold_max": 50,
        "calibration_offset": 0.013,
        "power_mode": "balanced"  // low_power, balanced, high_performance
    },
    "specifications": {
        "measurement_range": {"min": -40, "max": 85},
        "accuracy": 0.5,
        "resolution": 0.1,
        "response_time": 2,  // seconds
        "power_consumption": "0.5mW"
    },
    "status": "active",  // active, inactive, maintenance, error
    "health": {
        "battery_level": 87,
        "signal_strength": -45,
        "last_seen": "2024-01-20T15:45:23Z",
        "uptime_hours": 2456,
        "error_count_24h": 0,
        "data_quality_score": 0.98
    },
    "provisioned_at": "2023-11-15T10:00:00Z",
    "last_maintenance": "2024-01-10T14:00:00Z",
    "next_maintenance": "2024-04-10T14:00:00Z",
    "tags": ["temperature", "warehouse", "critical"],
    "metadata": {
        "owner": "operations-team",
        "cost_center": "warehouse-ops",
        "warranty_expiry": "2026-11-15"
    }
}
```

### Device Topology Graph

```aql
// Create graph for device relationships and network hierarchy
CREATE GRAPH device_topology {
    vertices: ["devices", "gateways", "locations"],
    edges: ["connected_to", "located_at", "powered_by", "manages"]
}

// Gateway vertex
{
    "_id": "gateways/gateway-001",
    "gateway_id": "gateway-001",
    "name": "Edge Gateway - Warehouse A",
    "type": "industrial_gateway",
    "model": "EdgeBox-5000",
    "network": {
        "ip_address": "192.168.1.1",
        "wan_ip": "203.0.113.10",
        "protocols": ["mqtt", "modbus", "opcua"]
    },
    "capacity": {
        "max_devices": 500,
        "current_devices": 142
    },
    "status": "online",
    "last_seen": "2024-01-20T15:45:25Z"
}

// Connection edge
{
    "_from": "devices/sensor-temp-001",
    "_to": "gateways/gateway-001",
    "edge_type": "connected_to",
    "protocol": "mqtt",
    "connection_quality": 0.95,
    "latency_ms": 15,
    "connected_since": "2023-11-15T10:30:00Z"
}

// Location hierarchy edge
{
    "_from": "devices/sensor-temp-001",
    "_to": "locations/warehouse-A-zone-3",
    "edge_type": "located_at",
    "installed_at": "2023-11-15T10:00:00Z",
    "installation_notes": "Mounted on shelf 12, top position"
}
```

## Time-Series Data Ingestion

### High-Throughput Batch Insertion

```aql
// Batch insert sensor readings
LET readings = @batchData  // Array of sensor readings

FOR reading IN readings
    // Validate data quality
    FILTER reading.timestamp != null
    FILTER reading.metadata.device_id != null
    FILTER reading.value != null
    
    // Enrich with calculated fields
    LET enriched = MERGE(reading, {
        ingested_at: DATE_ISO8601(DATE_NOW()),
        partition_key: DATE_FORMAT(reading.timestamp, "%Y-%m-%d"),
        quality_flags: {
            in_range: reading.value >= @minValue AND reading.value <= @maxValue,
            no_spike: ABS(reading.value - @previousValue) < @spikeThreshold,
            fresh: DATE_DIFF(reading.timestamp, DATE_NOW(), "minutes") < 5
        }
    })
    
    INSERT enriched INTO sensor_readings
    OPTIONS {ignoreErrors: true}

RETURN {
    inserted: LENGTH(readings),
    timestamp: DATE_ISO8601(DATE_NOW())
}

// Using prepared bulk insert for maximum performance
PREPARE INSERT @reading INTO sensor_readings

// In application code, execute prepared statement in batches
for batch in chunk_data(readings, batch_size=1000):
    execute_batch_prepared(batch)
```

### Real-Time Streaming Ingestion

```cpp
// C++ example for high-performance streaming ingestion
#include <themis/client.hpp>
#include <themis/timeseries.hpp>

class SensorDataIngester {
private:
    themis::Client client;
    themis::BatchWriter writer;
    const size_t batch_size = 1000;
    const size_t flush_interval_ms = 100;
    
public:
    SensorDataIngester(const std::string& connection_str) 
        : client(connection_str),
          writer(client, "sensor_readings", batch_size, flush_interval_ms) {
        
        // Configure write options
        writer.set_compression(themis::Compression::LZ4);
        writer.set_write_concern(themis::WriteConcern::ACKNOWLEDGED);
    }
    
    void ingest_reading(const SensorReading& reading) {
        // Create time-series document
        themis::TimeSeriesDoc doc;
        doc.timestamp = reading.timestamp;
        doc.metadata = {
            {"device_id", reading.device_id},
            {"sensor_type", reading.sensor_type},
            {"location", reading.location}
        };
        doc.value = reading.value;
        doc.fields = {
            {"quality", reading.quality},
            {"battery_level", reading.battery_level},
            {"signal_strength", reading.signal_strength}
        };
        
        // Add to batch (auto-flushes when full)
        writer.write(doc);
    }
    
    void flush() {
        writer.flush();
    }
};

// Usage in MQTT callback
void on_mqtt_message(const MqttMessage& msg) {
    auto reading = parse_sensor_reading(msg.payload);
    ingester.ingest_reading(reading);
}
```

### Data Validation and Enrichment

```aql
// Validation pipeline with CEP rules
FOR reading IN sensor_readings_stream
    // Real-time validation
    LET device = FIRST(
        FOR d IN devices
            FILTER d.device_id == reading.metadata.device_id
            RETURN d
    )
    
    // Check if device exists and is active
    FILTER device != null AND device.status == "active"
    
    // Validate value is in specification range
    LET in_range = (
        reading.value >= device.specifications.measurement_range.min AND
        reading.value <= device.specifications.measurement_range.max
    )
    
    // Check for data staleness
    LET is_fresh = DATE_DIFF(reading.timestamp, DATE_NOW(), "minutes") < 10
    
    // Detect anomalies using statistical bounds
    LET recent_stats = FIRST(
        FOR r IN sensor_readings
            FILTER r.metadata.device_id == reading.metadata.device_id
            FILTER r.timestamp > DATE_SUBTRACT(reading.timestamp, 1, "hour")
            COLLECT AGGREGATE 
                avg = AVG(r.value),
                stddev = STDDEV(r.value)
            RETURN {avg: avg, stddev: stddev}
    )
    
    LET is_anomaly = recent_stats ? (
        ABS(reading.value - recent_stats.avg) > (3 * recent_stats.stddev)
    ) : false
    
    // Enrich reading with validation results
    LET enriched = MERGE(reading, {
        validation: {
            in_range: in_range,
            is_fresh: is_fresh,
            is_anomaly: is_anomaly
        },
        device_info: {
            location: device.location,
            type: device.type
        }
    })
    
    // Store enriched reading
    INSERT enriched INTO sensor_readings
    
    // Generate alert if needed
    FILTER !in_range OR is_anomaly
    INSERT {
        alert_type: !in_range ? "out_of_range" : "anomaly",
        device_id: reading.metadata.device_id,
        timestamp: reading.timestamp,
        value: reading.value,
        severity: !in_range ? "high" : "medium",
        message: !in_range ? 
            CONCAT("Value ", reading.value, " outside range") :
            CONCAT("Anomaly detected: ", reading.value)
    } INTO alerts
```

## Real-Time Aggregation

### Windowed Aggregations

```aql
// Tumbling window: 1-minute aggregates
FOR reading IN sensor_readings
    FILTER reading.timestamp >= DATE_SUBTRACT(DATE_NOW(), 1, "hour")
    LET window = DATE_TRUNC(reading.timestamp, "minute")
    
    COLLECT 
        device_id = reading.metadata.device_id,
        window_start = window
    AGGREGATE
        avg_value = AVG(reading.value),
        min_value = MIN(reading.value),
        max_value = MAX(reading.value),
        count = COUNT(),
        stddev = STDDEV(reading.value)
    
    RETURN {
        device_id: device_id,
        window_start: window_start,
        window_end: DATE_ADD(window_start, 1, "minute"),
        statistics: {
            avg: avg_value,
            min: min_value,
            max: max_value,
            stddev: stddev,
            sample_count: count
        }
    }

// Sliding window: 5-minute average updated every minute
FOR device IN devices
    FILTER device.status == "active"
    
    LET recent_readings = (
        FOR reading IN sensor_readings
            FILTER reading.metadata.device_id == device.device_id
            FILTER reading.timestamp >= DATE_SUBTRACT(DATE_NOW(), 5, "minutes")
            SORT reading.timestamp DESC
            RETURN reading
    )
    
    LET stats = (
        FOR r IN recent_readings
            COLLECT AGGREGATE
                current_value = FIRST(r.value),
                avg_5min = AVG(r.value),
                min_5min = MIN(r.value),
                max_5min = MAX(r.value),
                trend = (FIRST(r.value) - LAST(r.value)) / 5
            RETURN {
                current: current_value,
                avg: avg_5min,
                min: min_5min,
                max: max_5min,
                trend: trend
            }
    )
    
    RETURN {
        device_id: device.device_id,
        device_name: device.device_name,
        location: device.location.site,
        statistics: FIRST(stats),
        reading_count: LENGTH(recent_readings)
    }
```

### Multi-Device Correlation

```aql
// Correlate readings from multiple sensors in same location
LET location = "warehouse-A-zone-3"

// Get all devices in location
LET location_devices = (
    FOR device IN devices
        FILTER device.location.zone == location
        FILTER device.status == "active"
        RETURN device.device_id
)

// Get recent readings for all devices
LET recent_readings = (
    FOR reading IN sensor_readings
        FILTER reading.metadata.device_id IN location_devices
        FILTER reading.timestamp >= DATE_SUBTRACT(DATE_NOW(), 10, "minutes")
        RETURN reading
)

// Aggregate by time bucket and sensor type
FOR reading IN recent_readings
    LET time_bucket = DATE_TRUNC(reading.timestamp, "minute")
    COLLECT 
        bucket = time_bucket,
        sensor_type = reading.metadata.sensor_type
    AGGREGATE
        avg_value = AVG(reading.value),
        device_count = COUNT_DISTINCT(reading.metadata.device_id)
    
    RETURN {
        timestamp: bucket,
        location: location,
        sensor_type: sensor_type,
        avg_value: avg_value,
        device_count: device_count
    }

// Environmental correlation analysis
LET temp_readings = (
    FOR r IN sensor_readings
        FILTER r.metadata.sensor_type == "temperature"
        FILTER r.metadata.location == @location
        FILTER r.timestamp >= DATE_SUBTRACT(DATE_NOW(), 1, "hour")
        SORT r.timestamp
        RETURN {timestamp: r.timestamp, value: r.value}
)

LET humidity_readings = (
    FOR r IN sensor_readings
        FILTER r.metadata.sensor_type == "humidity"
        FILTER r.metadata.location == @location
        FILTER r.timestamp >= DATE_SUBTRACT(DATE_NOW(), 1, "hour")
        SORT r.timestamp
        RETURN {timestamp: r.timestamp, value: r.value}
)

// Calculate correlation coefficient
LET correlation = CORRELATION(
    temp_readings[*].value,
    humidity_readings[*].value
)

RETURN {
    location: @location,
    correlation_coefficient: correlation,
    interpretation: (
        ABS(correlation) > 0.7 ? "strong correlation" :
        ABS(correlation) > 0.3 ? "moderate correlation" :
        "weak correlation"
    )
}
```

## Anomaly Detection

### Statistical Anomaly Detection

```aql
// Detect anomalies using Z-score method
FOR device IN devices
    FILTER device.type == "temperature_sensor"
    
    // Get historical baseline
    LET baseline = FIRST(
        FOR r IN sensor_readings
            FILTER r.metadata.device_id == device.device_id
            FILTER r.timestamp >= DATE_SUBTRACT(DATE_NOW(), 24, "hours")
            FILTER r.timestamp < DATE_SUBTRACT(DATE_NOW(), 1, "hour")
            COLLECT AGGREGATE
                mean = AVG(r.value),
                stddev = STDDEV(r.value)
            RETURN {mean: mean, stddev: stddev}
    )
    
    // Get recent readings
    LET recent = (
        FOR r IN sensor_readings
            FILTER r.metadata.device_id == device.device_id
            FILTER r.timestamp >= DATE_SUBTRACT(DATE_NOW(), 5, "minutes")
            RETURN r
    )
    
    // Calculate Z-scores
    FOR reading IN recent
        LET z_score = (reading.value - baseline.mean) / baseline.stddev
        FILTER ABS(z_score) > 3  // 3-sigma threshold
        
        INSERT {
            anomaly_type: "statistical",
            device_id: device.device_id,
            timestamp: reading.timestamp,
            value: reading.value,
            expected_range: {
                mean: baseline.mean,
                lower: baseline.mean - (3 * baseline.stddev),
                upper: baseline.mean + (3 * baseline.stddev)
            },
            z_score: z_score,
            severity: ABS(z_score) > 5 ? "critical" : "warning",
            detected_at: DATE_ISO8601(DATE_NOW())
        } INTO anomalies

// Moving average anomaly detection
FOR device IN devices
    LET window_size = 20
    
    LET readings = (
        FOR r IN sensor_readings
            FILTER r.metadata.device_id == device.device_id
            FILTER r.timestamp >= DATE_SUBTRACT(DATE_NOW(), 30, "minutes")
            SORT r.timestamp ASC
            RETURN r
    )
    
    // Calculate moving average and detect deviations
    FOR i IN (window_size..LENGTH(readings)-1)
        LET window = SLICE(readings, i - window_size, window_size)
        LET ma = AVG(window[*].value)
        LET current = readings[i]
        LET deviation = ABS(current.value - ma) / ma
        
        FILTER deviation > 0.15  // 15% deviation threshold
        
        INSERT {
            anomaly_type: "moving_average_deviation",
            device_id: device.device_id,
            timestamp: current.timestamp,
            value: current.value,
            moving_average: ma,
            deviation_percent: deviation * 100,
            severity: deviation > 0.3 ? "critical" : "warning"
        } INTO anomalies
```

### Pattern-Based Anomaly Detection

```aql
// Detect sudden spikes or drops
FOR device IN devices
    LET readings = (
        FOR r IN sensor_readings
            FILTER r.metadata.device_id == device.device_id
            FILTER r.timestamp >= DATE_SUBTRACT(DATE_NOW(), 1, "hour")
            SORT r.timestamp ASC
            RETURN r
    )
    
    // Calculate rate of change
    FOR i IN (1..LENGTH(readings)-1)
        LET prev = readings[i-1]
        LET curr = readings[i]
        LET rate_of_change = (curr.value - prev.value) / 
                            DATE_DIFF(prev.timestamp, curr.timestamp, "seconds")
        
        // Detect rapid changes
        FILTER ABS(rate_of_change) > @rateThreshold
        
        INSERT {
            anomaly_type: "rapid_change",
            device_id: device.device_id,
            timestamp: curr.timestamp,
            value: curr.value,
            previous_value: prev.value,
            rate_of_change: rate_of_change,
            severity: ABS(rate_of_change) > (@rateThreshold * 2) ? "critical" : "warning"
        } INTO anomalies

// Detect missing data / sensor failures
FOR device IN devices
    FILTER device.status == "active"
    
    LET last_reading = FIRST(
        FOR r IN sensor_readings
            FILTER r.metadata.device_id == device.device_id
            SORT r.timestamp DESC
            LIMIT 1
            RETURN r
    )
    
    LET silence_duration = DATE_DIFF(last_reading.timestamp, DATE_NOW(), "minutes")
    LET expected_interval = device.configuration.reporting_interval / 60
    
    FILTER silence_duration > (expected_interval * 3)  // 3x expected interval
    
    INSERT {
        anomaly_type: "sensor_silence",
        device_id: device.device_id,
        last_seen: last_reading.timestamp,
        silence_duration_minutes: silence_duration,
        expected_interval_minutes: expected_interval,
        severity: silence_duration > (expected_interval * 10) ? "critical" : "warning",
        detected_at: DATE_ISO8601(DATE_NOW())
    } INTO anomalies
```

### ML-Based Anomaly Detection

```aql
// Use LLM for anomaly detection
LET device_id = @deviceId

// Get recent readings
LET readings = (
    FOR r IN sensor_readings
        FILTER r.metadata.device_id == device_id
        FILTER r.timestamp >= DATE_SUBTRACT(DATE_NOW(), 24, "hours")
        SORT r.timestamp ASC
        RETURN {
            timestamp: r.timestamp,
            value: r.value
        }
)

// Get historical baseline
LET baseline = (
    FOR r IN sensor_readings
        FILTER r.metadata.device_id == device_id
        FILTER r.timestamp >= DATE_SUBTRACT(DATE_NOW(), 30, "days")
        FILTER r.timestamp < DATE_SUBTRACT(DATE_NOW(), 1, "days")
        COLLECT AGGREGATE
            mean = AVG(r.value),
            min = MIN(r.value),
            max = MAX(r.value),
            stddev = STDDEV(r.value)
        RETURN {mean: mean, min: min, max: max, stddev: stddev}
)

// Use LLM to analyze pattern
LET analysis = LLM_QUERY(
    CONCAT(
        "Analyze this sensor data for anomalies. ",
        "Recent readings: ", TO_STRING(readings), ". ",
        "Historical baseline: mean=", baseline.mean, 
        ", stddev=", baseline.stddev, ". ",
        "Identify any anomalous patterns and explain."
    ),
    {
        model: "llama-3-8b",
        temperature: 0.3,
        max_tokens: 500
    }
)

RETURN {
    device_id: device_id,
    analysis: analysis,
    readings_analyzed: LENGTH(readings),
    baseline: baseline
}
```

## Device Management

### Device Provisioning

```aql
// Register new device
BEGIN TRANSACTION

// Insert device document
LET device = INSERT {
    device_id: @deviceId,
    device_name: @deviceName,
    type: @deviceType,
    manufacturer: @manufacturer,
    model: @model,
    serial_number: @serialNumber,
    location: @location,
    network: @networkConfig,
    configuration: @configuration,
    status: "provisioning",
    provisioned_at: DATE_ISO8601(DATE_NOW())
} INTO devices RETURN NEW

// Create gateway connection
INSERT {
    _from: CONCAT("devices/", device.device_id),
    _to: CONCAT("gateways/", @gatewayId),
    edge_type: "connected_to",
    protocol: @protocol,
    connected_since: DATE_ISO8601(DATE_NOW())
} INTO connected_to

// Create location relationship
INSERT {
    _from: CONCAT("devices/", device.device_id),
    _to: CONCAT("locations/", @locationId),
    edge_type: "located_at",
    installed_at: DATE_ISO8601(DATE_NOW())
} INTO located_at

// Initialize device state
INSERT {
    device_id: device.device_id,
    state: "initialized",
    configuration_sent: false,
    first_reading_received: false,
    health_check_passed: false,
    updated_at: DATE_ISO8601(DATE_NOW())
} INTO device_states

COMMIT TRANSACTION

RETURN device
```

### Device Health Monitoring

```aql
// Monitor device health status
FOR device IN devices
    FILTER device.status == "active"
    
    // Get latest reading
    LET latest = FIRST(
        FOR r IN sensor_readings
            FILTER r.metadata.device_id == device.device_id
            SORT r.timestamp DESC
            LIMIT 1
            RETURN r
    )
    
    // Calculate health metrics
    LET last_seen_minutes = DATE_DIFF(latest.timestamp, DATE_NOW(), "minutes")
    LET is_online = last_seen_minutes < 10
    LET battery_status = latest.battery_level > 20 ? "good" : 
                        latest.battery_level > 10 ? "low" : "critical"
    LET signal_status = latest.signal_strength > -70 ? "good" :
                       latest.signal_strength > -85 ? "fair" : "poor"
    
    // Get error count
    LET error_count = LENGTH(
        FOR a IN anomalies
            FILTER a.device_id == device.device_id
            FILTER a.detected_at >= DATE_SUBTRACT(DATE_NOW(), 24, "hours")
            RETURN 1
    )
    
    // Calculate data quality
    LET quality_score = (
        FOR r IN sensor_readings
            FILTER r.metadata.device_id == device.device_id
            FILTER r.timestamp >= DATE_SUBTRACT(DATE_NOW(), 1, "hour")
            COLLECT AGGREGATE avg_quality = AVG(r.quality)
            RETURN avg_quality
    )
    
    // Overall health score
    LET health_score = (
        (is_online ? 25 : 0) +
        (battery_status == "good" ? 25 : battery_status == "low" ? 15 : 0) +
        (signal_status == "good" ? 25 : signal_status == "fair" ? 15 : 0) +
        (FIRST(quality_score) * 25)
    )
    
    // Update device health
    UPDATE device WITH {
        health: {
            score: health_score,
            status: health_score > 75 ? "healthy" : 
                   health_score > 50 ? "degraded" : "unhealthy",
            last_seen: latest.timestamp,
            is_online: is_online,
            battery_level: latest.battery_level,
            battery_status: battery_status,
            signal_strength: latest.signal_strength,
            signal_status: signal_status,
            error_count_24h: error_count,
            data_quality_score: FIRST(quality_score) || 0
        },
        updated_at: DATE_ISO8601(DATE_NOW())
    } IN devices
    
    RETURN {
        device_id: device.device_id,
        health_score: health_score,
        issues: (
            !is_online ? ["offline"] :
            battery_status == "critical" ? ["battery_critical"] :
            battery_status == "low" ? ["battery_low"] :
            signal_status == "poor" ? ["poor_signal"] :
            []
        )
    }
```

### Network Topology Analysis

```aql
// Analyze device network topology
FOR gateway IN gateways
    // Get all connected devices
    LET devices = (
        FOR v, e IN 1..1 INBOUND gateway connected_to
            RETURN v
    )
    
    // Calculate gateway metrics
    LET active_devices = LENGTH(
        FOR d IN devices
            FILTER d.status == "active"
            FILTER d.health.is_online == true
            RETURN 1
    )
    
    LET avg_signal = AVG(devices[* FILTER CURRENT.health.signal_strength != null].health.signal_strength)
    LET low_battery = LENGTH(
        FOR d IN devices
            FILTER d.health.battery_level < 20
            RETURN 1
    )
    
    // Identify bottlenecks
    LET capacity_usage = active_devices / gateway.capacity.max_devices
    
    RETURN {
        gateway_id: gateway.gateway_id,
        name: gateway.name,
        status: gateway.status,
        devices: {
            total: LENGTH(devices),
            active: active_devices,
            offline: LENGTH(devices) - active_devices
        },
        metrics: {
            capacity_usage: capacity_usage,
            capacity_status: capacity_usage > 0.9 ? "critical" :
                           capacity_usage > 0.7 ? "warning" : "normal",
            avg_signal_strength: avg_signal,
            low_battery_count: low_battery
        },
        alerts: (
            capacity_usage > 0.9 ? ["capacity_critical"] :
            capacity_usage > 0.7 ? ["capacity_warning"] :
            []
        )
    }

// Find optimal gateway for new device
FOR gateway IN gateways
    FILTER gateway.status == "online"
    
    LET current_load = gateway.capacity.current_devices / gateway.capacity.max_devices
    LET distance = DISTANCE(
        @deviceLocation.latitude,
        @deviceLocation.longitude,
        gateway.location.latitude,
        gateway.location.longitude
    )
    
    // Calculate suitability score
    LET score = (
        (1 - current_load) * 50 +  // Load factor
        (1 / (distance + 1)) * 30 +  // Distance factor
        (gateway.health_score / 100) * 20  // Health factor
    )
    
    SORT score DESC
    LIMIT 1
    RETURN {
        gateway_id: gateway.gateway_id,
        name: gateway.name,
        current_load: current_load,
        distance_km: distance,
        score: score
    }
```

## Historical Analysis

### Time-Series Queries

```aql
// Query hourly averages for last 30 days
FOR reading IN sensor_readings
    FILTER reading.metadata.device_id == @deviceId
    FILTER reading.timestamp >= DATE_SUBTRACT(DATE_NOW(), 30, "days")
    LET hour = DATE_TRUNC(reading.timestamp, "hour")
    
    COLLECT time_bucket = hour
    AGGREGATE
        avg_value = AVG(reading.value),
        min_value = MIN(reading.value),
        max_value = MAX(reading.value),
        sample_count = COUNT()
    
    SORT time_bucket ASC
    RETURN {
        timestamp: time_bucket,
        avg: avg_value,
        min: min_value,
        max: max_value,
        samples: sample_count
    }

// Compare current period vs historical baseline
LET current_period = (
    FOR r IN sensor_readings
        FILTER r.metadata.device_id == @deviceId
        FILTER r.timestamp >= DATE_SUBTRACT(DATE_NOW(), 7, "days")
        COLLECT AGGREGATE
            avg = AVG(r.value),
            min = MIN(r.value),
            max = MAX(r.value)
        RETURN {avg: avg, min: min, max: max}
)

LET historical_period = (
    FOR r IN sensor_readings
        FILTER r.metadata.device_id == @deviceId
        FILTER r.timestamp >= DATE_SUBTRACT(DATE_NOW(), 37, "days")
        FILTER r.timestamp < DATE_SUBTRACT(DATE_NOW(), 7, "days")
        COLLECT AGGREGATE
            avg = AVG(r.value),
            min = MIN(r.value),
            max = MAX(r.value)
        RETURN {avg: avg, min: min, max: max}
)

RETURN {
    current: FIRST(current_period),
    historical: FIRST(historical_period),
    change: {
        avg_pct: ((FIRST(current_period).avg - FIRST(historical_period).avg) / 
                 FIRST(historical_period).avg) * 100,
        min_pct: ((FIRST(current_period).min - FIRST(historical_period).min) / 
                 FIRST(historical_period).min) * 100,
        max_pct: ((FIRST(current_period).max - FIRST(historical_period).max) / 
                 FIRST(historical_period).max) * 100
    }
}
```

### Forecasting

```aql
// Simple linear regression forecast
LET historical_data = (
    FOR r IN sensor_readings
        FILTER r.metadata.device_id == @deviceId
        FILTER r.timestamp >= DATE_SUBTRACT(DATE_NOW(), 30, "days")
        LET hour_offset = DATE_DIFF(r.timestamp, DATE_SUBTRACT(DATE_NOW(), 30, "days"), "hours")
        RETURN {x: hour_offset, y: r.value}
)

LET n = LENGTH(historical_data)
LET sum_x = SUM(historical_data[*].x)
LET sum_y = SUM(historical_data[*].y)
LET sum_xy = SUM(
    FOR d IN historical_data
        RETURN d.x * d.y
)
LET sum_x2 = SUM(
    FOR d IN historical_data
        RETURN d.x * d.x
)

// Calculate slope and intercept
LET slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x)
LET intercept = (sum_y - slope * sum_x) / n

// Generate forecast for next 7 days
FOR hour_offset IN 720..888  // Hours 720-888 (next 7 days)
    LET forecast_value = slope * hour_offset + intercept
    LET timestamp = DATE_ADD(DATE_SUBTRACT(DATE_NOW(), 30, "days"), hour_offset, "hours")
    
    RETURN {
        timestamp: timestamp,
        forecasted_value: forecast_value,
        confidence: "medium"  // Could calculate confidence intervals
    }
```

## Edge-to-Cloud Architecture

### Edge Processing Configuration

```yaml
# Edge gateway configuration
edge:
  gateway_id: gateway-001
  processing:
    # Local aggregation before sending to cloud
    aggregation:
      enabled: true
      window: 5m
      functions: [avg, min, max, count]
    
    # Filter out-of-range values
    filtering:
      enabled: true
      rules:
        - type: range_check
          min: -40
          max: 85
        - type: spike_detection
          threshold: 5.0
    
    # Local alerting for critical conditions
    local_alerts:
      enabled: true
      rules:
        - condition: value > 50
          severity: critical
          action: notify_local
        - condition: value < -10
          severity: warning
          action: log
  
  # Buffering for offline resilience
  buffer:
    max_size: 1000000  # 1M readings
    persistence: disk
    retry_interval: 60s
  
  # Data compression
  compression:
    enabled: true
    algorithm: lz4
  
  # Uplink configuration
  uplink:
    protocol: mqtt
    broker: cloud.themisdb.com:8883
    qos: 1
    batch_size: 100
    batch_interval: 10s
```

### Edge-to-Cloud Data Flow

```aql
// Cloud-side: Process incoming edge-aggregated data
FOR batch IN edge_data_stream
    // Batch contains pre-aggregated data from edge
    FOR reading IN batch.readings
        // Store aggregated reading
        INSERT reading INTO sensor_readings
        
        // Check for cloud-level alerts
        LET device = FIRST(FOR d IN devices FILTER d.device_id == reading.metadata.device_id RETURN d)
        
        // Cloud-specific alerting (cross-device, multi-site)
        FILTER reading.value > device.configuration.threshold_max
        
        // Check if multiple devices in same location are affected
        LET affected_devices = LENGTH(
            FOR r IN sensor_readings
                FILTER r.metadata.location == reading.metadata.location
                FILTER r.timestamp >= DATE_SUBTRACT(DATE_NOW(), 5, "minutes")
                FILTER r.value > device.configuration.threshold_max
                RETURN DISTINCT r.metadata.device_id
        )
        
        FILTER affected_devices >= 3  // Multiple device threshold
        
        INSERT {
            alert_type: "multi_device_threshold_breach",
            location: reading.metadata.location,
            affected_devices: affected_devices,
            timestamp: DATE_ISO8601(DATE_NOW()),
            severity: "critical"
        } INTO alerts
```

## Performance Optimization

### Sharding Strategy

```yaml
# Optimal sharding for IoT workloads
collections:
  sensor_readings:
    sharding:
      strategy: time_range
      time_field: timestamp
      chunk_interval: 1 day
      shard_count: 32
      # Distribute by device ID within time chunks
      secondary_key: metadata.device_id
    
    partitioning:
      # Partition by device type for better query performance
      partition_key: metadata.sensor_type
      partition_count: 8
    
    replication:
      factor: 3
      # Asynchronous replication for high write throughput
      mode: async
      lag_tolerance: 5s
```

### Write Optimization

```aql
// Batch writes with prepared statements
PREPARE batch_insert AS
    FOR reading IN @batch
        INSERT reading INTO sensor_readings
        OPTIONS {
            ignoreErrors: true,
            waitForSync: false  // Async writes for performance
        }

// Use connection pooling
// Configure in application
connection_pool:
  min_size: 10
  max_size: 100
  max_wait_time: 5s
  
write_options:
  batch_size: 1000
  compression: lz4
  write_concern: acknowledged  // Don't wait for replication
  timeout: 10s
```

### Query Optimization

```aql
// Use materialized views for common aggregations
CREATE MATERIALIZED VIEW hourly_device_stats AS
    FOR reading IN sensor_readings
        LET hour = DATE_TRUNC(reading.timestamp, "hour")
        COLLECT 
            device_id = reading.metadata.device_id,
            hour_bucket = hour
        AGGREGATE
            avg_value = AVG(reading.value),
            min_value = MIN(reading.value),
            max_value = MAX(reading.value),
            count = COUNT()
        RETURN {
            device_id: device_id,
            hour: hour_bucket,
            stats: {
                avg: avg_value,
                min: min_value,
                max: max_value,
                count: count
            }
        }
    
    OPTIONS {
        refresh_interval: "5 minutes",
        store: true
    }

// Query the materialized view instead of raw data
FOR stats IN hourly_device_stats
    FILTER stats.device_id == @deviceId
    FILTER stats.hour >= DATE_SUBTRACT(DATE_NOW(), 7, "days")
    SORT stats.hour ASC
    RETURN stats
```

### Data Retention and Archival

```aql
// Automated data lifecycle management
// Delete raw data older than 90 days
FOR reading IN sensor_readings
    FILTER reading.timestamp < DATE_SUBTRACT(DATE_NOW(), 90, "days")
    REMOVE reading IN sensor_readings
    OPTIONS {batchSize: 10000}

// Archive to cold storage
FOR reading IN sensor_readings
    FILTER reading.timestamp < DATE_SUBTRACT(DATE_NOW(), 30, "days")
    FILTER reading.timestamp >= DATE_SUBTRACT(DATE_NOW(), 90, "days")
    
    // Export to Parquet format
    INSERT reading INTO cold_storage
    OPTIONS {
        format: "parquet",
        compression: "snappy",
        path: "s3://iot-archive/{{year}}/{{month}}/{{day}}/"
    }
```

## Monitoring & Operations

### System Metrics

```aql
// Ingestion rate monitoring
FOR reading IN sensor_readings
    FILTER reading.ingested_at >= DATE_SUBTRACT(DATE_NOW(), 5, "minutes")
    LET minute = DATE_TRUNC(reading.ingested_at, "minute")
    COLLECT minute_bucket = minute
    AGGREGATE count = COUNT()
    SORT minute_bucket DESC
    RETURN {
        minute: minute_bucket,
        readings_per_minute: count,
        readings_per_second: count / 60
    }

// Storage metrics by device type
FOR reading IN sensor_readings
    FILTER reading.timestamp >= DATE_SUBTRACT(DATE_NOW(), 24, "hours")
    COLLECT sensor_type = reading.metadata.sensor_type
    AGGREGATE 
        count = COUNT(),
        avg_size = AVG(LENGTH(TO_STRING(reading)))
    RETURN {
        sensor_type: sensor_type,
        reading_count: count,
        avg_doc_size_bytes: avg_size,
        estimated_daily_storage_mb: (count * avg_size) / (1024 * 1024)
    }
```

### Alerting Dashboard

```aql
// Real-time alerting overview
RETURN {
    active_alerts: LENGTH(FOR a IN alerts FILTER a.resolved == false RETURN 1),
    by_severity: (
        FOR a IN alerts
            FILTER a.resolved == false
            COLLECT severity = a.severity WITH COUNT INTO count
            RETURN {severity: severity, count: count}
    ),
    by_type: (
        FOR a IN alerts
            FILTER a.resolved == false
            COLLECT type = a.alert_type WITH COUNT INTO count
            SORT count DESC
            LIMIT 10
            RETURN {type: type, count: count}
    ),
    recent_critical: (
        FOR a IN alerts
            FILTER a.severity == "critical"
            FILTER a.resolved == false
            SORT a.timestamp DESC
            LIMIT 10
            RETURN a
    )
}
```

## Best Practices

1. **Time-Series Optimization**
   - Use time-based sharding for even data distribution
   - Implement retention policies and downsampling
   - Query downsampled data for historical analysis
   - Use materialized views for common aggregations

2. **Write Performance**
   - Batch insertions (1000-10000 records)
   - Use asynchronous writes for non-critical data
   - Enable compression (LZ4 for speed, Zstd for ratio)
   - Implement edge aggregation to reduce cloud writes

3. **Anomaly Detection**
   - Combine statistical and ML-based methods
   - Implement tiered alerting (edge, gateway, cloud)
   - Use CEP for real-time pattern matching
   - Maintain historical baselines for comparison

4. **Device Management**
   - Implement health scoring system
   - Monitor device lifecycle (battery, connectivity)
   - Use graph queries for topology analysis
   - Automate device provisioning and decommissioning

5. **Edge Processing**
   - Aggregate at edge to reduce bandwidth
   - Implement local buffering for resilience
   - Filter invalid data before transmission
   - Enable edge-based alerting for critical conditions

## Related Documentation

- [Time-Series Guide](../features/timeseries.md)
- [Complex Event Processing](../features/cep.md)
- [Graph Queries](../aql/graph-traversal.md)
- [Sharding Configuration](../architecture/sharding.md)

## Example Projects

- [IoT Sensor Network](../../examples/09_iot_sensor_network/)
- [Time-Series Monitor](../../examples/05_time_series_monitor/)
- [Smart Home](../../examples/20_smart_home/)

## Conclusion

ThemisDB's multi-model architecture makes it ideal for IoT workloads, combining time-series efficiency with document flexibility and graph-based device topology. The platform handles millions of sensor readings per second while providing real-time analytics and anomaly detection capabilities.
