# AQL Proximity Search Patterns

## Overview

AQL (ArangoDB Query Language) proximity search in ThemisDB combines full-text search with geospatial filtering and distance-based sorting. This guide covers patterns, best practices, and optimization techniques for proximity queries.

## Basic Syntax

```aql
FOR doc IN collection
  FILTER FULLTEXT(doc.field, "search terms")
  FILTER ST_Distance(doc.location, [longitude, latitude]) < threshold
  SORT PROXIMITY(doc.location, [longitude, latitude]) ASC
  LIMIT n
  RETURN doc
```

## Core Functions

### 1. FULLTEXT(field, query, [min_score])

Full-text search on indexed fields.

```aql
-- Basic full-text search
FILTER FULLTEXT(doc.description, "coffee shop")

-- With minimum relevance score
FILTER FULLTEXT(doc.description, "restaurant", 50)

-- Multiple terms
FILTER FULLTEXT(doc.tags, "museum art gallery")
```

### 2. ST_Distance(point1, point2)

Calculate distance between two geographic points.

```aql
-- Point to point distance (meters)
FILTER ST_Distance(doc.location, [13.45, 52.55]) < 1000

-- Bidirectional syntax (same result)
FILTER ST_Distance([13.45, 52.55], doc.location) < 1000
```

### 3. ST_Within(location, bbox)

Filter points within a bounding box.

```aql
-- Bounding box: [west, south, east, north]
FILTER ST_Within(doc.location, [13.0, 52.0, 14.0, 53.0])
```

### 4. PROXIMITY(location, reference_point)

Sort by distance from a reference point.

```aql
-- Sort nearest first
SORT PROXIMITY(doc.location, [13.45, 52.55]) ASC

-- Sort farthest first
SORT PROXIMITY(doc.location, [13.45, 52.55]) DESC
```

## Common Patterns

### Pattern 1: "Near Me" Search

Find businesses near a location with text search:

```aql
FOR doc IN places
  FILTER FULLTEXT(doc.name, "coffee")
  FILTER ST_Distance(doc.location, [13.45, 52.55]) < 500
  SORT PROXIMITY(doc.location, [13.45, 52.55]) ASC
  LIMIT 10
  RETURN {
    name: doc.name,
    address: doc.address,
    distance: ST_Distance(doc.location, [13.45, 52.55])
  }
```

**Use Case:** Mobile apps showing nearby results.

### Pattern 2: Bounded Region Search

Search within a geographic area:

```aql
FOR doc IN venues
  FILTER FULLTEXT(doc.type, "concert hall")
  FILTER ST_Within(doc.location, [10.0, 50.0, 15.0, 55.0])
  SORT PROXIMITY(doc.location, [12.5, 52.5]) ASC
  LIMIT 20
  RETURN doc
```

**Use Case:** City-wide or regional searches.

### Pattern 3: K-Nearest Neighbors

Find the K closest points:

```aql
FOR doc IN stores
  FILTER FULLTEXT(doc.category, "grocery")
  SORT PROXIMITY(doc.location, [13.45, 52.55]) ASC
  LIMIT 5
  RETURN {
    name: doc.name,
    distance: ST_Distance(doc.location, [13.45, 52.55])
  }
```

**Use Case:** Recommendation systems, route planning.

### Pattern 4: Distance Buckets

Group results by distance ranges:

```aql
FOR doc IN restaurants
  FILTER FULLTEXT(doc.cuisine, "italian")
  LET dist = ST_Distance(doc.location, [13.45, 52.55])
  FILTER dist < 5000
  COLLECT bucket = FLOOR(dist / 1000) * 1000
  AGGREGATE count = COUNT(1), avg_rating = AVG(doc.rating)
  RETURN {
    distance_range: bucket,
    count: count,
    avg_rating: avg_rating
  }
```

**Use Case:** Analytics, heatmaps.

### Pattern 5: Multi-Criteria Proximity

Combine distance with other factors:

```aql
FOR doc IN hotels
  FILTER FULLTEXT(doc.amenities, "wifi parking")
  FILTER doc.rating >= 4.0
  FILTER doc.price <= 150
  FILTER ST_Distance(doc.location, [13.45, 52.55]) < 3000
  LET score = doc.rating / (ST_Distance(doc.location, [13.45, 52.55]) / 1000 + 1)
  SORT score DESC
  LIMIT 15
  RETURN {
    name: doc.name,
    rating: doc.rating,
    price: doc.price,
    distance: ST_Distance(doc.location, [13.45, 52.55]),
    score: score
  }
```

**Use Case:** Personalized recommendations, weighted scoring.

## Distance Functions

### Euclidean Distance (2D Points)

For non-geographic coordinates:

```aql
FOR doc IN points
  FILTER FULLTEXT(doc.label, "data point")
  LET dist = SQRT(POW(doc.x - 10, 2) + POW(doc.y - 20, 2))
  FILTER dist < 5
  RETURN {doc: doc, distance: dist}
```

### Manhattan Distance (Grid Systems)

For city-block distances:

```aql
FOR doc IN grid_nodes
  FILTER FULLTEXT(doc.type, "waypoint")
  LET dist = ABS(doc.x - 5) + ABS(doc.y - 10)
  FILTER dist <= 3
  SORT dist ASC
  RETURN {doc: doc, manhattan_dist: dist}
```

### Haversine Distance (Sphere)

For accurate long-distance geographic calculations:

```aql
FOR doc IN cities
  FILTER FULLTEXT(doc.description, "capital")
  FILTER ST_Distance_Sphere(doc.location, [13.45, 52.55]) < 100000
  SORT ST_Distance_Sphere(doc.location, [13.45, 52.55]) ASC
  LIMIT 5
  RETURN doc
```

**Note:** `ST_Distance_Sphere` uses Haversine formula for precise earth-surface distances.

### Custom Distance Metrics

Create domain-specific distance functions:

```aql
FOR doc IN products
  FILTER FULLTEXT(doc.description, "laptop")
  LET price_dist = ABS(doc.price - 1000) / 100
  LET rating_dist = ABS(doc.rating - 4.5)
  LET custom_score = price_dist + rating_dist * 2
  FILTER custom_score < 10
  SORT custom_score ASC
  RETURN {
    product: doc,
    price_dist: price_dist,
    rating_dist: rating_dist,
    score: custom_score
  }
```

## Advanced Patterns

### Pattern 6: Proximity in Subqueries

Find cities with nearby attractions:

```aql
FOR city IN cities
  FILTER FULLTEXT(city.name, "Berlin")
  LET nearby_museums = (
    FOR venue IN museums
      FILTER FULLTEXT(venue.type, "art museum")
      FILTER ST_Distance(venue.location, city.location) < 5000
      SORT PROXIMITY(venue.location, city.location) ASC
      LIMIT 5
      RETURN {
        name: venue.name,
        distance: ST_Distance(venue.location, city.location)
      }
  )
  RETURN {
    city: city.name,
    museums: nearby_museums
  }
```

### Pattern 7: Multi-Point Proximity

Find locations near multiple reference points:

```aql
LET points = [[13.45, 52.55], [13.40, 52.50], [13.50, 52.52]]

FOR doc IN locations
  FILTER FULLTEXT(doc.category, "parking")
  LET distances = (
    FOR p IN points
      RETURN ST_Distance(doc.location, p)
  )
  LET min_dist = MIN(distances)
  FILTER min_dist < 1000
  SORT min_dist ASC
  RETURN {
    location: doc,
    nearest_distance: min_dist
  }
```

### Pattern 8: Temporal Proximity

Combine time and space:

```aql
FOR doc IN events
  FILTER FULLTEXT(doc.description, "conference")
  LET spatial_dist = ST_Distance(doc.location, [13.45, 52.55])
  LET temporal_dist = ABS(DATE_DIFF(doc.date, "2024-06-15", "day"))
  LET combined_score = spatial_dist / 1000 + temporal_dist
  FILTER spatial_dist < 10000
  FILTER temporal_dist < 30
  SORT combined_score ASC
  RETURN {
    event: doc,
    spatial_dist: spatial_dist,
    temporal_dist: temporal_dist,
    score: combined_score
  }
```

### Pattern 9: Vector Similarity (Embeddings)

Semantic similarity using cosine distance:

```aql
FOR doc IN documents
  FILTER FULLTEXT(doc.text, "machine learning")
  LET target = [0.5, 0.3, 0.7, 0.2]  -- Query vector
  LET dot_product = (
    FOR i IN 0..LENGTH(doc.embedding)-1
      RETURN doc.embedding[i] * target[i]
  )
  LET similarity = SUM(dot_product)
  FILTER similarity > 0.8
  SORT similarity DESC
  RETURN {
    document: doc,
    similarity: similarity
  }
```

## Performance Optimization

### 1. Index Strategy

**Create appropriate indexes:**
```cpp
// Full-text index on searchable fields
idx_mgr->createIndex("places", "description", IndexType::FULLTEXT);

// Spatial index on location fields
idx_mgr->createIndex("places", "location", IndexType::GEO);
```

### 2. Limit Early

Always use LIMIT to reduce result sets:

```aql
-- Good: Limit early
FOR doc IN places
  FILTER FULLTEXT(doc.name, "restaurant")
  SORT PROXIMITY(doc.location, [13.45, 52.55]) ASC
  LIMIT 20  -- Stops processing after 20 results
  RETURN doc

-- Less optimal: No limit
FOR doc IN places
  FILTER FULLTEXT(doc.name, "restaurant")
  SORT PROXIMITY(doc.location, [13.45, 52.55]) ASC
  RETURN doc  -- Processes all results
```

### 3. Filter Before Sort

Apply filters before expensive sorting:

```aql
-- Good: Filter first
FOR doc IN venues
  FILTER FULLTEXT(doc.type, "museum")
  FILTER doc.rating >= 4.0                    -- Cheap filter
  FILTER ST_Distance(doc.location, [13.45, 52.55]) < 5000  -- Spatial filter
  SORT PROXIMITY(doc.location, [13.45, 52.55]) ASC         -- Expensive sort
  LIMIT 10
  RETURN doc
```

### 4. Avoid Redundant Calculations

Use LET to calculate once:

```aql
-- Good: Calculate once
FOR doc IN places
  FILTER FULLTEXT(doc.name, "hotel")
  LET dist = ST_Distance(doc.location, [13.45, 52.55])
  FILTER dist < 2000
  SORT dist ASC
  RETURN {place: doc, distance: dist}

-- Less optimal: Calculate multiple times
FOR doc IN places
  FILTER FULLTEXT(doc.name, "hotel")
  FILTER ST_Distance(doc.location, [13.45, 52.55]) < 2000
  SORT ST_Distance(doc.location, [13.45, 52.55]) ASC
  RETURN {
    place: doc,
    distance: ST_Distance(doc.location, [13.45, 52.55])
  }
```

## Common Pitfalls

### 1. Proximity Without Full-Text

```aql
-- ❌ Error: PROXIMITY requires FULLTEXT
FOR doc IN places
  SORT PROXIMITY(doc.location, [13.45, 52.55]) ASC
  RETURN doc

-- ✅ Correct: Include FULLTEXT filter
FOR doc IN places
  FILTER FULLTEXT(doc.description, "place")  -- Required
  SORT PROXIMITY(doc.location, [13.45, 52.55]) ASC
  RETURN doc
```

### 2. Missing Distance Threshold

```aql
-- ⚠️ Inefficient: No distance limit
FOR doc IN venues
  FILTER FULLTEXT(doc.type, "restaurant")
  SORT PROXIMITY(doc.location, [13.45, 52.55]) ASC
  LIMIT 10
  RETURN doc

-- ✅ Better: Add distance threshold
FOR doc IN venues
  FILTER FULLTEXT(doc.type, "restaurant")
  FILTER ST_Distance(doc.location, [13.45, 52.55]) < 5000
  SORT PROXIMITY(doc.location, [13.45, 52.55]) ASC
  LIMIT 10
  RETURN doc
```

### 3. Coordinate Order

```aql
-- ❌ Wrong: [latitude, longitude] is incorrect
FILTER ST_Distance(doc.location, [52.55, 13.45]) < 1000

-- ✅ Correct: [longitude, latitude] (GeoJSON standard)
FILTER ST_Distance(doc.location, [13.45, 52.55]) < 1000
```

## Testing

Comprehensive tests in `tests/test_aql_proximity.cpp` (17 tests):

- Basic operators (3 tests)
- Distance functions (4 tests)
- Different data types (3 tests)
- Complex queries (3 tests)
- Edge cases (4 tests)

## Further Reading

- [AQL Parser Implementation](../include/query/aql_parser.h)
- [AQL Translator](../include/query/aql_translator.h)
- [Test Examples](../tests/test_aql_proximity.cpp)
- [Spatial Indexing Guide](../docs/spatial-indexing.md)
