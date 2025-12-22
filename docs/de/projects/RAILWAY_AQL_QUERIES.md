# Railway Monitoring System - AQL Query Examples

Complete collection of AQL (ArangoDB Query Language) queries for ThemisDB integration in the Railway Monitoring System.

## Table of Contents

1. [Train Queries](#train-queries)
2. [Station Queries](#station-queries)
3. [Time-Series Queries](#time-series-queries)
4. [Analytics Queries](#analytics-queries)
5. [Graph Queries](#graph-queries)

---

## Train Queries

### Get All Active Trains

```aql
FOR train IN entities
  FILTER train._key LIKE "trains:%"
  RETURN {
    train_number: train.train_number,
    type: train.type,
    position: train.position,
    speed: train.speed,
    delay: train.delay,
    route: train.route,
    updated_at: train.updated_at
  }
```

**HTTP Request**:
```bash
curl -X POST http://localhost:8765/query/aql \
  -H "Content-Type: application/json" \
  -d '{
    "query": "FOR train IN entities FILTER train._key LIKE \"trains:%\" RETURN train"
  }'
```

### Get Delayed Trains (>5 minutes)

```aql
FOR train IN entities
  FILTER train._key LIKE "trains:%"
  FILTER train.delay > 5
  SORT train.delay DESC
  LIMIT 10
  RETURN {
    train_number: train.train_number,
    type: train.type,
    delay: train.delay,
    origin: train.origin,
    destination: train.destination,
    current_position: train.position
  }
```

### Top 10 Fastest Trains

```aql
FOR train IN entities
  FILTER train._key LIKE "trains:%"
  FILTER train.speed > 0
  SORT train.speed DESC
  LIMIT 10
  RETURN {
    train_number: train.train_number,
    type: train.type,
    speed: train.speed,
    max_speed: train.max_speed,
    utilization: ROUND(train.speed / train.max_speed * 100, 1)
  }
```

### Trains by Type

```aql
FOR train IN entities
  FILTER train._key LIKE "trains:%"
  COLLECT type = train.type WITH COUNT INTO count
  RETURN {
    type: type,
    count: count
  }
```

### Trains on Specific Route

```aql
FOR train IN entities
  FILTER train._key LIKE "trains:%"
  FILTER train.route LIKE "%FRANKFURT_HBF%"
  RETURN {
    train_number: train.train_number,
    type: train.type,
    route: train.route,
    delay: train.delay
  }
```

---

## Station Queries

### Get All Stations

```aql
FOR station IN entities
  FILTER station._key LIKE "stations:%"
  RETURN {
    id: station.id,
    name: station.name,
    coordinates: station.coordinates,
    facilities: station.facilities
  }
```

### Stations with Most Train Traffic

```aql
FOR train IN entities
  FILTER train._key LIKE "trains:%"
  FOR station_name IN [train.origin, train.destination]
    FILTER station_name != null
    COLLECT station = station_name WITH COUNT INTO traffic
    SORT traffic DESC
    LIMIT 10
    RETURN {
      station: station,
      trains_count: traffic
    }
```

### Stations by Region

```aql
FOR station IN entities
  FILTER station._key LIKE "stations:%"
  FILTER station.coordinates.lat >= 50.0 AND station.coordinates.lat <= 52.0
  FILTER station.coordinates.lon >= 8.0 AND station.coordinates.lon <= 10.0
  RETURN {
    name: station.name,
    coordinates: station.coordinates
  }
```

---

## Time-Series Queries

### Latest GPS Position for Train

```aql
FOR gps IN entities
  FILTER gps._key LIKE "train_gps:ICE508:%"
  SORT gps._key DESC
  LIMIT 1
  RETURN {
    train_id: gps.train_id,
    latitude: gps.latitude,
    longitude: gps.longitude,
    speed: gps.speed,
    heading: gps.heading,
    timestamp: gps.timestamp
  }
```

### GPS Track for Last Hour

```aql
LET now = DATE_NOW()
LET one_hour_ago = DATE_SUBTRACT(now, 1, "hour")

FOR gps IN entities
  FILTER gps._key LIKE "train_gps:ICE508:%"
  FILTER gps.timestamp >= one_hour_ago
  SORT gps.timestamp ASC
  RETURN {
    latitude: gps.latitude,
    longitude: gps.longitude,
    speed: gps.speed,
    timestamp: gps.timestamp
  }
```

### Axle Counter Events (Last 15 Minutes)

```aql
LET now = DATE_NOW()
LET fifteen_min_ago = DATE_SUBTRACT(now, 15, "minute")

FOR event IN entities
  FILTER event._key LIKE "axle_counter:%"
  FILTER event.timestamp >= fifteen_min_ago
  SORT event.timestamp DESC
  LIMIT 50
  RETURN {
    train_id: event.train_id,
    block_id: event.block_id,
    direction: event.direction,
    axle_count: event.axle_count,
    timestamp: event.timestamp
  }
```

### Hotbox Detector Alerts

```aql
FOR event IN entities
  FILTER event._key LIKE "hotbox_detector:%"
  FILTER event.temperature > 80
  SORT event.timestamp DESC
  LIMIT 20
  RETURN {
    train_id: event.train_id,
    detector_id: event.detector_id,
    axle_position: event.axle_position,
    temperature: event.temperature,
    threshold: event.threshold,
    timestamp: event.timestamp
  }
```

### Signal State Changes

```aql
LET now = DATE_NOW()
LET last_30_min = DATE_SUBTRACT(now, 30, "minute")

FOR signal IN entities
  FILTER signal._key LIKE "signal_telemetry:%"
  FILTER signal.timestamp >= last_30_min
  SORT signal.timestamp DESC
  RETURN {
    signal_id: signal.signal_id,
    old_aspect: signal.old_aspect,
    new_aspect: signal.new_aspect,
    lamp_status: signal.lamp_status,
    timestamp: signal.timestamp
  }
```

---

## Analytics Queries

### Train Punctuality Statistics

```aql
FOR train IN entities
  FILTER train._key LIKE "trains:%"
  COLLECT type = train.type AGGREGATE
    total = COUNT(1),
    on_time = SUM(train.delay <= 5 ? 1 : 0),
    avg_delay = AVG(train.delay),
    max_delay = MAX(train.delay)
  RETURN {
    train_type: type,
    total_trains: total,
    punctuality_rate: ROUND(on_time / total * 100, 1),
    avg_delay_minutes: ROUND(avg_delay, 1),
    max_delay_minutes: max_delay
  }
```

### Speed Distribution

```aql
FOR train IN entities
  FILTER train._key LIKE "trains:%"
  FILTER train.speed > 0
  COLLECT bucket = FLOOR(train.speed / 50) * 50 WITH COUNT INTO count
  SORT bucket ASC
  RETURN {
    speed_range: CONCAT(bucket, "-", bucket + 50, " km/h"),
    train_count: count
  }
```

### Delay Reasons (from infrastructure events)

```aql
LET signal_delays = (
  FOR event IN entities
    FILTER event._key LIKE "signal_telemetry:%"
    FILTER event.new_aspect == "STOP"
    RETURN 1
)

LET hotbox_alerts = (
  FOR event IN entities
    FILTER event._key LIKE "hotbox_detector:%"
    FILTER event.temperature > event.threshold
    RETURN 1
)

RETURN {
  signal_stops: LENGTH(signal_delays),
  hotbox_alerts: LENGTH(hotbox_alerts),
  total_incidents: LENGTH(signal_delays) + LENGTH(hotbox_alerts)
}
```

### Passenger Load Analysis

```aql
FOR train IN entities
  FILTER train._key LIKE "trains:%"
  FILTER train.passengers != null
  COLLECT type = train.type AGGREGATE
    avg_passengers = AVG(train.passengers),
    avg_capacity = AVG(train.passenger_capacity),
    max_passengers = MAX(train.passengers)
  RETURN {
    train_type: type,
    avg_passengers: ROUND(avg_passengers),
    avg_occupancy: ROUND(avg_passengers / avg_capacity * 100, 1),
    max_passengers: max_passengers
  }
```

### Hourly Train Count

```aql
FOR train IN entities
  FILTER train._key LIKE "trains:%"
  LET hour = DATE_HOUR(train.updated_at)
  COLLECT h = hour WITH COUNT INTO count
  SORT h ASC
  RETURN {
    hour: h,
    train_count: count
  }
```

---

## Graph Queries

### Shortest Route Between Stations

```aql
FOR station IN entities
  FILTER station._key == "stations:FRANKFURT_HBF"
  FOR target IN entities
    FILTER target._key == "stations:KOELN_HBF"
    LET path = (
      FOR v, e IN OUTBOUND SHORTEST_PATH
        station TO target
        GRAPH "railway_network"
        RETURN {
          station: v.name,
          distance: e.distance,
          max_speed: e.max_speed
        }
    )
    RETURN {
      from: station.name,
      to: target.name,
      path: path,
      total_distance: SUM(path[*].distance),
      estimated_time: SUM(path[*].distance / path[*].max_speed * 60)
    }
```

### Adjacent Stations

```aql
FOR station IN entities
  FILTER station._key == "stations:FRANKFURT_HBF"
  FOR neighbor IN OUTBOUND station
    GRAPH "railway_network"
    OPTIONS {bfs: true, uniqueVertices: 'global'}
    FILTER IS_SAME_COLLECTION("stations", neighbor)
    LIMIT 5
    RETURN {
      station: neighbor.name,
      coordinates: neighbor.coordinates
    }
```

### Track Segments in Region

```aql
FOR segment IN entities
  FILTER segment._key LIKE "track_segments:%"
  FILTER segment.from_station LIKE "%FRANKFURT%"
     OR segment.to_station LIKE "%FRANKFURT%"
  RETURN {
    id: segment.id,
    from: segment.from_station,
    to: segment.to_station,
    distance: segment.distance,
    max_speed: segment.max_speed,
    current_speed_limit: segment.current_speed_limit
  }
```

---

## Combined Queries

### Live Dashboard Summary

```aql
LET active_trains = (
  FOR train IN entities
    FILTER train._key LIKE "trains:%"
    RETURN train
)

LET delayed_trains = (
  FOR train IN active_trains
    FILTER train.delay > 5
    RETURN train
)

LET recent_alerts = (
  FOR event IN entities
    FILTER event._key LIKE "hotbox_detector:%"
    FILTER event.temperature > event.threshold
    FILTER event.timestamp >= DATE_SUBTRACT(DATE_NOW(), 1, "hour")
    RETURN event
)

RETURN {
  total_trains: LENGTH(active_trains),
  delayed_trains: LENGTH(delayed_trains),
  punctuality_rate: ROUND((LENGTH(active_trains) - LENGTH(delayed_trains)) / LENGTH(active_trains) * 100, 1),
  avg_delay: ROUND(AVG(active_trains[*].delay), 1),
  max_delay: MAX(active_trains[*].delay),
  recent_alerts: LENGTH(recent_alerts),
  avg_speed: ROUND(AVG(active_trains[*].speed), 1),
  timestamp: DATE_NOW()
}
```

### Train Position with Route Info

```aql
FOR train IN entities
  FILTER train._key == "trains:ICE508"
  LET origin = (
    FOR station IN entities
      FILTER station._key == CONCAT("stations:", train.origin)
      RETURN station
  )[0]
  
  LET destination = (
    FOR station IN entities
      FILTER station._key == CONCAT("stations:", train.destination)
      RETURN station
  )[0]
  
  RETURN {
    train_number: train.train_number,
    type: train.type,
    current_position: train.position,
    origin: {
      name: origin.name,
      coordinates: origin.coordinates
    },
    destination: {
      name: destination.name,
      coordinates: destination.coordinates
    },
    delay: train.delay,
    speed: train.speed,
    passengers: train.passengers,
    updated_at: train.updated_at
  }
```

---

## Usage in Applications

### Python Example

```python
import requests
import json

def query_themisdb(query):
    response = requests.post(
        "http://localhost:8765/query/aql",
        json={"query": query},
        headers={"Content-Type": "application/json"}
    )
    return response.json()

# Get delayed trains
delayed_query = """
FOR train IN entities
  FILTER train._key LIKE "trains:%"
  FILTER train.delay > 5
  SORT train.delay DESC
  LIMIT 10
  RETURN train
"""

result = query_themisdb(delayed_query)
print(json.dumps(result, indent=2))
```

### JavaScript/Node Example

```javascript
const fetch = require('node-fetch');

async function queryThemisDB(query) {
  const response = await fetch('http://localhost:8765/query/aql', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ query })
  });
  return await response.json();
}

// Get active trains
const activeTrainsQuery = `
  FOR train IN entities
    FILTER train._key LIKE "trains:%"
    RETURN train
`;

queryThemisDB(activeTrainsQuery)
  .then(result => console.log(JSON.stringify(result, null, 2)));
```

### C# (.NET) Example

```csharp
using System.Net.Http;
using System.Text;
using System.Text.Json;

public async Task<JsonDocument> QueryThemisDB(string query)
{
    using var client = new HttpClient();
    var content = new StringContent(
        JsonSerializer.Serialize(new { query }),
        Encoding.UTF8,
        "application/json"
    );
    
    var response = await client.PostAsync(
        "http://localhost:8765/query/aql",
        content
    );
    
    var json = await response.Content.ReadAsStringAsync();
    return JsonDocument.Parse(json);
}

// Get punctuality statistics
var query = @"
FOR train IN entities
  FILTER train._key LIKE ""trains:%""
  COLLECT type = train.type AGGREGATE
    total = COUNT(1),
    on_time = SUM(train.delay <= 5 ? 1 : 0)
  RETURN {
    train_type: type,
    punctuality_rate: on_time / total * 100
  }
";

var result = await QueryThemisDB(query);
```

---

## Performance Tips

1. **Use FILTER early**: Apply filters before collecting or aggregating
2. **LIMIT results**: Always use LIMIT for large result sets
3. **Index usage**: ThemisDB automatically uses `_key` indexes for LIKE queries
4. **Avoid SELECT ***: Specify only needed fields in RETURN
5. **Time-Series optimization**: Use timestamp ranges with FILTER

## Next Steps

- Implement these queries in WPF ThemisDbService (Phase 1.3)
- Add query caching for frequently accessed data
- Create parameterized query templates
- Build dashboard components using these queries
