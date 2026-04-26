"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            extended_models_benchmark.py                       ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     627                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Extended Multi-Model Benchmark Suite
Includes: Geospatial, Time-Series, BPMN Process benchmarks

Fair comparison: All databases use native client libraries
"""

import time
import statistics
import json
import random
from typing import List, Dict, Any
from dataclasses import dataclass, field
from datetime import datetime, timedelta

import psycopg2
import psycopg2.extras
from pymongo import MongoClient
from pymongo import GEOSPHERE

@dataclass
class BenchmarkResult:
    scenario: str
    database: str
    operation: str
    latencies_ms: List[float] = field(default_factory=list)

    @property
    def mean(self) -> float:
        return round(statistics.mean(self.latencies_ms), 2) if self.latencies_ms else 0.0

    @property
    def median(self) -> float:
        return round(statistics.median(self.latencies_ms), 2) if self.latencies_ms else 0.0

    @property
    def p95(self) -> float:
        if not self.latencies_ms:
            return 0.0
        sorted_vals = sorted(self.latencies_ms)
        idx = int(len(sorted_vals) * 0.95)
        return round(sorted_vals[idx], 2)

    @property
    def p99(self) -> float:
        if not self.latencies_ms:
            return 0.0
        sorted_vals = sorted(self.latencies_ms)
        idx = int(len(sorted_vals) * 0.99)
        return round(sorted_vals[idx], 2)

    def to_dict(self):
        return {
            "scenario": self.scenario,
            "database": self.database,
            "operation": self.operation,
            "mean_ms": self.mean,
            "median_ms": self.median,
            "p95_ms": self.p95,
            "p99_ms": self.p99,
            "iterations": len(self.latencies_ms)
        }


# =============================================================================
# Scenario 1: Geospatial Queries (BBOX & Radius Search)
# =============================================================================

def benchmark_geo_postgresql() -> BenchmarkResult:
    """PostgreSQL with PostGIS extension"""
    result = BenchmarkResult("Geospatial", "PostgreSQL+PostGIS", "Radius Search")
    
    try:
        conn = psycopg2.connect("postgresql://benchmark:benchmark123@localhost:5432/benchmark")
        cur = conn.cursor()
        
        # Setup: Create table with geospatial data
        cur.execute("CREATE EXTENSION IF NOT EXISTS postgis")
        cur.execute("DROP TABLE IF EXISTS locations")
        cur.execute("""
            CREATE TABLE locations (
                id SERIAL PRIMARY KEY,
                name TEXT,
                location GEOGRAPHY(POINT, 4326)
            )
        """)
        
        # Insert 1000 random locations (Berlin area)
        for i in range(1000):
            lat = 52.5 + random.uniform(-0.1, 0.1)
            lon = 13.4 + random.uniform(-0.1, 0.1)
            cur.execute(
                "INSERT INTO locations (name, location) VALUES (%s, ST_MakePoint(%s, %s)::geography)",
                (f"Location {i}", lon, lat)
            )
        conn.commit()
        
        # Create spatial index
        cur.execute("CREATE INDEX idx_locations_geog ON locations USING GIST(location)")
        conn.commit()
        
        # Warmup
        for _ in range(5):
            cur.execute("""
                SELECT id, name, ST_Distance(location, ST_MakePoint(13.4, 52.5)::geography) AS distance
                FROM locations
                WHERE ST_DWithin(location, ST_MakePoint(13.4, 52.5)::geography, 5000)
                ORDER BY distance
                LIMIT 10
            """)
            cur.fetchall()
        
        # Benchmark: Radius search (5km radius)
        for _ in range(50):
            start = time.perf_counter()
            cur.execute("""
                SELECT id, name, ST_Distance(location, ST_MakePoint(13.4, 52.5)::geography) AS distance
                FROM locations
                WHERE ST_DWithin(location, ST_MakePoint(13.4, 52.5)::geography, 5000)
                ORDER BY distance
                LIMIT 10
            """)
            cur.fetchall()
            result.latencies_ms.append((time.perf_counter() - start) * 1000)
        
        cur.close()
        conn.close()
    except Exception as e:
        print(f"[PostgreSQL PostGIS] Error: {e}")
    
    return result


def benchmark_geo_mongodb() -> BenchmarkResult:
    """MongoDB with 2dsphere index"""
    result = BenchmarkResult("Geospatial", "MongoDB", "Radius Search")
    
    try:
        client = MongoClient("mongodb://benchmark:benchmark123@localhost:27017/")
        db = client["benchmark"]
        coll = db["geo_locations"]
        coll.drop()
        
        # Insert 1000 random locations (Berlin area)
        locations = []
        for i in range(1000):
            lat = 52.5 + random.uniform(-0.1, 0.1)
            lon = 13.4 + random.uniform(-0.1, 0.1)
            locations.append({
                "name": f"Location {i}",
                "location": {
                    "type": "Point",
                    "coordinates": [lon, lat]  # GeoJSON: [longitude, latitude]
                }
            })
        coll.insert_many(locations)
        
        # Create 2dsphere index
        coll.create_index([("location", GEOSPHERE)])
        
        # Warmup
        for _ in range(5):
            list(coll.find({
                "location": {
                    "$near": {
                        "$geometry": {
                            "type": "Point",
                            "coordinates": [13.4, 52.5]
                        },
                        "$maxDistance": 5000  # 5km radius
                    }
                }
            }).limit(10))
        
        # Benchmark: Radius search
        for _ in range(50):
            start = time.perf_counter()
            list(coll.find({
                "location": {
                    "$near": {
                        "$geometry": {
                            "type": "Point",
                            "coordinates": [13.4, 52.5]
                        },
                        "$maxDistance": 5000
                    }
                }
            }).limit(10))
            result.latencies_ms.append((time.perf_counter() - start) * 1000)
        
        coll.drop()
        client.close()
    except Exception as e:
        print(f"[MongoDB Geo] Error: {e}")
    
    return result


# =============================================================================
# Scenario 2: Time-Series Aggregations (Bucketed Queries)
# =============================================================================

def benchmark_timeseries_postgresql() -> BenchmarkResult:
    """PostgreSQL with TimescaleDB extension (if available) or native time buckets"""
    result = BenchmarkResult("Time-Series", "PostgreSQL", "Hourly Aggregation")
    
    try:
        conn = psycopg2.connect("postgresql://benchmark:benchmark123@localhost:5432/benchmark")
        cur = conn.cursor()
        
        # Setup: Create metrics table
        cur.execute("DROP TABLE IF EXISTS metrics")
        cur.execute("""
            CREATE TABLE metrics (
                id SERIAL PRIMARY KEY,
                timestamp TIMESTAMPTZ NOT NULL,
                sensor_id INTEGER,
                value DOUBLE PRECISION
            )
        """)
        
        # Insert 10,000 metric data points (last 7 days, every 1 minute)
        base_time = datetime.now() - timedelta(days=7)
        for i in range(10000):
            ts = base_time + timedelta(minutes=i)
            sensor_id = random.randint(1, 10)
            value = random.uniform(20.0, 30.0)
            cur.execute(
                "INSERT INTO metrics (timestamp, sensor_id, value) VALUES (%s, %s, %s)",
                (ts, sensor_id, value)
            )
        conn.commit()
        
        # Create time index
        cur.execute("CREATE INDEX idx_metrics_time ON metrics(timestamp)")
        conn.commit()
        
        # Warmup
        for _ in range(5):
            cur.execute("""
                SELECT 
                    date_trunc('hour', timestamp) AS bucket,
                    AVG(value) AS avg_value,
                    COUNT(*) AS count
                FROM metrics
                WHERE timestamp >= NOW() - INTERVAL '24 hours'
                GROUP BY bucket
                ORDER BY bucket
            """)
            cur.fetchall()
        
        # Benchmark: Hourly aggregation (last 24 hours)
        for _ in range(50):
            start = time.perf_counter()
            cur.execute("""
                SELECT 
                    date_trunc('hour', timestamp) AS bucket,
                    AVG(value) AS avg_value,
                    COUNT(*) AS count
                FROM metrics
                WHERE timestamp >= NOW() - INTERVAL '24 hours'
                GROUP BY bucket
                ORDER BY bucket
            """)
            cur.fetchall()
            result.latencies_ms.append((time.perf_counter() - start) * 1000)
        
        cur.close()
        conn.close()
    except Exception as e:
        print(f"[PostgreSQL TimeSeries] Error: {e}")
    
    return result


def benchmark_timeseries_mongodb() -> BenchmarkResult:
    """MongoDB with aggregation pipeline"""
    result = BenchmarkResult("Time-Series", "MongoDB", "Hourly Aggregation")
    
    try:
        client = MongoClient("mongodb://benchmark:benchmark123@localhost:27017/")
        db = client["benchmark"]
        coll = db["ts_metrics"]
        coll.drop()
        
        # Insert 10,000 metric data points
        base_time = datetime.now() - timedelta(days=7)
        metrics = []
        for i in range(10000):
            ts = base_time + timedelta(minutes=i)
            sensor_id = random.randint(1, 10)
            value = random.uniform(20.0, 30.0)
            metrics.append({
                "timestamp": ts,
                "sensor_id": sensor_id,
                "value": value
            })
        coll.insert_many(metrics)
        
        # Create time index
        coll.create_index("timestamp")
        
        # Warmup
        cutoff = datetime.now() - timedelta(hours=24)
        for _ in range(5):
            list(coll.aggregate([
                {"$match": {"timestamp": {"$gte": cutoff}}},
                {"$group": {
                    "_id": {
                        "$dateTrunc": {
                            "date": "$timestamp",
                            "unit": "hour"
                        }
                    },
                    "avg_value": {"$avg": "$value"},
                    "count": {"$sum": 1}
                }},
                {"$sort": {"_id": 1}}
            ]))
        
        # Benchmark: Hourly aggregation
        for _ in range(50):
            start = time.perf_counter()
            list(coll.aggregate([
                {"$match": {"timestamp": {"$gte": cutoff}}},
                {"$group": {
                    "_id": {
                        "$dateTrunc": {
                            "date": "$timestamp",
                            "unit": "hour"
                        }
                    },
                    "avg_value": {"$avg": "$value"},
                    "count": {"$sum": 1}
                }},
                {"$sort": {"_id": 1}}
            ]))
            result.latencies_ms.append((time.perf_counter() - start) * 1000)
        
        coll.drop()
        client.close()
    except Exception as e:
        print(f"[MongoDB TimeSeries] Error: {e}")
    
    return result


# =============================================================================
# Scenario 3: BPMN Process Execution (Workflow State Management)
# =============================================================================

def benchmark_bpmn_postgresql() -> BenchmarkResult:
    """PostgreSQL simulating BPMN process instance state"""
    result = BenchmarkResult("BPMN Process", "PostgreSQL", "Process Query")
    
    try:
        conn = psycopg2.connect("postgresql://benchmark:benchmark123@localhost:5432/benchmark")
        cur = conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
        
        # Setup: BPMN process tables
        cur.execute("DROP TABLE IF EXISTS process_instances CASCADE")
        cur.execute("DROP TABLE IF EXISTS process_tasks")
        cur.execute("""
            CREATE TABLE process_instances (
                id SERIAL PRIMARY KEY,
                process_def_key TEXT,
                business_key TEXT,
                status TEXT,
                variables JSONB,
                start_time TIMESTAMPTZ,
                end_time TIMESTAMPTZ
            )
        """)
        cur.execute("""
            CREATE TABLE process_tasks (
                id SERIAL PRIMARY KEY,
                process_instance_id INTEGER REFERENCES process_instances(id),
                task_name TEXT,
                assignee TEXT,
                status TEXT,
                created_at TIMESTAMPTZ
            )
        """)
        
        # Insert 100 process instances with tasks
        for i in range(100):
            cur.execute("""
                INSERT INTO process_instances (process_def_key, business_key, status, variables, start_time)
                VALUES (%s, %s, %s, %s, NOW())
                RETURNING id
            """, (
                "order_fulfillment",
                f"order_{i}",
                "running" if i % 3 != 0 else "completed",
                json.dumps({"order_amount": random.uniform(100, 1000), "customer_id": i})
            ))
            instance_id = cur.fetchone()['id']
            
            # Create tasks
            for j in range(3):
                cur.execute("""
                    INSERT INTO process_tasks (process_instance_id, task_name, assignee, status, created_at)
                    VALUES (%s, %s, %s, %s, NOW())
                """, (
                    instance_id,
                    f"task_{j}",
                    f"user_{random.randint(1, 10)}",
                    "completed" if j < 2 else "active"
                ))
        conn.commit()
        
        # Create indexes
        cur.execute("CREATE INDEX idx_process_status ON process_instances(status)")
        cur.execute("CREATE INDEX idx_task_process ON process_tasks(process_instance_id)")
        conn.commit()
        
        # Warmup
        for _ in range(5):
            cur.execute("""
                SELECT 
                    pi.id, pi.process_def_key, pi.status, pi.variables,
                    json_agg(json_build_object('task_name', pt.task_name, 'status', pt.status)) AS tasks
                FROM process_instances pi
                LEFT JOIN process_tasks pt ON pi.id = pt.process_instance_id
                WHERE pi.status = 'running'
                GROUP BY pi.id
                LIMIT 10
            """)
            cur.fetchall()
        
        # Benchmark: Query running processes with tasks
        for _ in range(50):
            start = time.perf_counter()
            cur.execute("""
                SELECT 
                    pi.id, pi.process_def_key, pi.status, pi.variables,
                    json_agg(json_build_object('task_name', pt.task_name, 'status', pt.status)) AS tasks
                FROM process_instances pi
                LEFT JOIN process_tasks pt ON pi.id = pt.process_instance_id
                WHERE pi.status = 'running'
                GROUP BY pi.id
                LIMIT 10
            """)
            cur.fetchall()
            result.latencies_ms.append((time.perf_counter() - start) * 1000)
        
        cur.close()
        conn.close()
    except Exception as e:
        print(f"[PostgreSQL BPMN] Error: {e}")
    
    return result


def benchmark_bpmn_mongodb() -> BenchmarkResult:
    """MongoDB storing BPMN process instances"""
    result = BenchmarkResult("BPMN Process", "MongoDB", "Process Query")
    
    try:
        client = MongoClient("mongodb://benchmark:benchmark123@localhost:27017/")
        db = client["benchmark"]
        coll = db["bpmn_processes"]
        coll.drop()
        
        # Insert 100 process instances
        processes = []
        for i in range(100):
            status = "running" if i % 3 != 0 else "completed"
            processes.append({
                "process_def_key": "order_fulfillment",
                "business_key": f"order_{i}",
                "status": status,
                "variables": {
                    "order_amount": random.uniform(100, 1000),
                    "customer_id": i
                },
                "start_time": datetime.now(),
                "tasks": [
                    {"task_name": f"task_{j}", "assignee": f"user_{random.randint(1, 10)}", "status": "completed" if j < 2 else "active"}
                    for j in range(3)
                ]
            })
        coll.insert_many(processes)
        
        # Create index
        coll.create_index("status")
        
        # Warmup
        for _ in range(5):
            list(coll.find({"status": "running"}).limit(10))
        
        # Benchmark: Query running processes
        for _ in range(50):
            start = time.perf_counter()
            list(coll.find({"status": "running"}).limit(10))
            result.latencies_ms.append((time.perf_counter() - start) * 1000)
        
        coll.drop()
        client.close()
    except Exception as e:
        print(f"[MongoDB BPMN] Error: {e}")
    
    return result


# =============================================================================
# Reporting
# =============================================================================

def print_results(results: List[BenchmarkResult]):
    """Print results in ASCII table format"""
    print("\n" + "="*80)
    print("EXTENDED MULTI-MODEL BENCHMARK RESULTS")
    print("="*80 + "\n")
    
    # Group by scenario
    scenarios = {}
    for r in results:
        if r.scenario not in scenarios:
            scenarios[r.scenario] = []
        scenarios[r.scenario].append(r)
    
    for scenario, scenario_results in scenarios.items():
        print(f"\n{scenario.upper()} SCENARIO")
        print("-" * 80)
        print(f"{'Database':<30} {'Mean (ms)':<15} {'Median (ms)':<15} {'P95 (ms)':<15} {'P99 (ms)':<15}")
        print("-" * 80)
        
        for result in scenario_results:
            print(f"{result.database:<30} {result.mean:<15.2f} {result.median:<15.2f} {result.p95:<15.2f} {result.p99:<15.2f}")
        
        # Calculate advantage
        if len(scenario_results) == 2:
            db1 = scenario_results[0]
            db2 = scenario_results[1]
            advantage = ((db1.mean - db2.mean) / db1.mean) * 100
            print("-" * 80)
            if advantage > 0:
                print(f"{db2.database} Advantage: {advantage:.1f}% faster\n")
            else:
                print(f"{db1.database} Advantage: {-advantage:.1f}% faster\n")


def save_results(results: List[BenchmarkResult]):
    """Save results to JSON"""
    data = {
        "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
        "methodology": "Extended benchmarks - Geospatial, Time-Series, BPMN",
        "results": [r.to_dict() for r in results]
    }
    
    filename = "benchmark_results_extended_models.json"
    with open(filename, 'w') as f:
        json.dump(data, f, indent=2)
    
    print(f"[SAVED] Results saved to {filename}")


# =============================================================================
# Main
# =============================================================================

def main():
    print("\nExtended Multi-Model Benchmark Suite")
    print("Scenarios: Geospatial, Time-Series, BPMN Process")
    print("Iterations: 50 (warmup: 5 each)\n")
    
    results = []
    
    # Scenario 1: Geospatial
    print("[1/6] Geospatial (PostgreSQL+PostGIS)... ", end="", flush=True)
    results.append(benchmark_geo_postgresql())
    print(f"Mean: {results[-1].mean:.2f}ms")
    
    print("[2/6] Geospatial (MongoDB)... ", end="", flush=True)
    results.append(benchmark_geo_mongodb())
    print(f"Mean: {results[-1].mean:.2f}ms")
    
    # Scenario 2: Time-Series
    print("[3/6] Time-Series (PostgreSQL)... ", end="", flush=True)
    results.append(benchmark_timeseries_postgresql())
    print(f"Mean: {results[-1].mean:.2f}ms")
    
    print("[4/6] Time-Series (MongoDB)... ", end="", flush=True)
    results.append(benchmark_timeseries_mongodb())
    print(f"Mean: {results[-1].mean:.2f}ms")
    
    # Scenario 3: BPMN Process
    print("[5/6] BPMN Process (PostgreSQL)... ", end="", flush=True)
    results.append(benchmark_bpmn_postgresql())
    print(f"Mean: {results[-1].mean:.2f}ms")
    
    print("[6/6] BPMN Process (MongoDB)... ", end="", flush=True)
    results.append(benchmark_bpmn_mongodb())
    print(f"Mean: {results[-1].mean:.2f}ms")
    
    # Report
    print_results(results)
    save_results(results)
    print("[DONE] Extended Benchmark Complete!\n")


if __name__ == "__main__":
    main()
