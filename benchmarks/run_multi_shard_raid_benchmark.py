"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            run_multi_shard_raid_benchmark.py                  ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     453                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Multi-Shard RAID Benchmark Orchestrator
Executes comprehensive performance tests across different RAID configurations
"""

import asyncio
import aiohttp
import time
import json
import os
import sys
from dataclasses import dataclass, asdict
from typing import List, Dict, Optional
from datetime import datetime
import statistics

@dataclass
class ShardConfig:
    shard_id: str
    endpoint: str
    raid_level: str
    rack_id: str
    datacenter: str

@dataclass
class BenchmarkConfig:
    scenario: str
    num_shards: int
    raid_level: str
    workload_type: str
    duration_hours: int
    target_qps: int
    concurrent_clients: int
    data_size_gb: int

@dataclass
class QueryMetrics:
    operation: str
    latency_ms: float
    success: bool
    shard_id: str
    timestamp: float

@dataclass
class BenchmarkResult:
    scenario: str
    shard_count: int
    raid_level: str
    workload_type: str
    start_time: str
    end_time: str
    duration_seconds: float
    
    total_queries: int
    successful_queries: int
    failed_queries: int
    
    throughput_qps: float
    latency_p50_ms: float
    latency_p95_ms: float
    latency_p99_ms: float
    latency_p999_ms: float
    latency_max_ms: float
    latency_avg_ms: float
    
    cpu_usage_avg_pct: float
    memory_usage_avg_mb: float
    disk_read_mbps: float
    disk_write_mbps: float
    disk_iops_avg: int
    network_rx_mbps: float
    network_tx_mbps: float
    
    cross_shard_queries: int
    cross_shard_latency_avg_ms: float

class MultiShardRaidBenchmark:
    def __init__(self, config: BenchmarkConfig, shards: List[ShardConfig]):
        self.config = config
        self.shards = shards
        self.metrics: List[QueryMetrics] = []
        self.start_time = None
        self.end_time = None
        
    def log(self, message: str, level: str = "INFO"):
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"[{timestamp}] [{level}] {message}", flush=True)
        
    async def execute_query(self, session: aiohttp.ClientSession, shard: ShardConfig, operation: str) -> QueryMetrics:
        """Execute a single query against a shard"""
        start = time.time()
        success = False
        
        try:
            endpoint = f"http://{shard.endpoint}"
            
            if operation == "point_read":
                url = f"{endpoint}/entities/test_{int(time.time() * 1000000)}"
                async with session.get(url, timeout=aiohttp.ClientTimeout(total=5)) as resp:
                    success = resp.status == 200 or resp.status == 404
                    
            elif operation == "point_write":
                url = f"{endpoint}/entities"
                data = {
                    "id": f"test_{int(time.time() * 1000000)}",
                    "data": {
                        "name": f"User {int(time.time())}",
                        "timestamp": datetime.now().isoformat(),
                        "category": "benchmark"
                    }
                }
                async with session.post(url, json=data, timeout=aiohttp.ClientTimeout(total=10)) as resp:
                    success = resp.status in [200, 201]
                    
            elif operation == "range_scan":
                url = f"{endpoint}/query"
                query = {
                    "collection": "entities",
                    "filter": {"category": "benchmark"},
                    "limit": 100
                }
                async with session.post(url, json=query, timeout=aiohttp.ClientTimeout(total=30)) as resp:
                    success = resp.status == 200
                    
            elif operation == "vector_search":
                url = f"{endpoint}/vector/search"
                search = {
                    "object": "documents",
                    "query_vector": [0.1] * 768,  # Dummy embedding
                    "k": 10
                }
                async with session.post(url, json=search, timeout=aiohttp.ClientTimeout(total=20)) as resp:
                    success = resp.status == 200
                    
        except asyncio.TimeoutError:
            self.log(f"Timeout for {operation} on {shard.shard_id}", "WARN")
        except Exception as e:
            self.log(f"Error in {operation} on {shard.shard_id}: {e}", "ERROR")
            
        latency = (time.time() - start) * 1000  # Convert to ms
        
        return QueryMetrics(
            operation=operation,
            latency_ms=latency,
            success=success,
            shard_id=shard.shard_id,
            timestamp=time.time()
        )
    
    async def workload_oltp(self, session: aiohttp.ClientSession, duration_seconds: int):
        """OLTP workload: 40% read, 30% write, 20% range scan, 10% vector search"""
        operations = [
            ("point_read", 0.40),
            ("point_write", 0.30),
            ("range_scan", 0.20),
            ("vector_search", 0.10)
        ]
        
        end_time = time.time() + duration_seconds
        query_count = 0
        
        while time.time() < end_time:
            # Select operation based on distribution
            import random
            rand = random.random()
            cumulative = 0
            selected_op = operations[0][0]
            
            for op, prob in operations:
                cumulative += prob
                if rand < cumulative:
                    selected_op = op
                    break
            
            # Select random shard
            shard = random.choice(self.shards)
            
            # Execute query
            metric = await self.execute_query(session, shard, selected_op)
            self.metrics.append(metric)
            query_count += 1
            
            # Log progress every 1000 queries
            if query_count % 1000 == 0:
                elapsed = time.time() - self.start_time
                current_qps = query_count / elapsed
                self.log(f"Progress: {query_count} queries, {current_qps:.1f} QPS, {len(self.metrics)} metrics")
            
            # Rate limiting to reach target QPS
            if self.config.target_qps > 0:
                await asyncio.sleep(1.0 / self.config.target_qps)
    
    async def workload_olap(self, session: aiohttp.ClientSession, duration_seconds: int):
        """OLAP workload: Complex queries, cross-shard joins, aggregations"""
        operations = [
            ("range_scan", 0.50),
            ("cross_shard_join", 0.30),
            ("aggregation", 0.20)
        ]
        
        end_time = time.time() + duration_seconds
        query_count = 0
        
        while time.time() < end_time:
            # For OLAP, typically fewer concurrent queries but more complex
            import random
            rand = random.random()
            
            if rand < 0.50:
                # Large range scan
                shard = random.choice(self.shards)
                metric = await self.execute_query(session, shard, "range_scan")
                self.metrics.append(metric)
                
            elif rand < 0.80:
                # Cross-shard join (scatter-gather)
                tasks = []
                for shard in self.shards:
                    tasks.append(self.execute_query(session, shard, "range_scan"))
                results = await asyncio.gather(*tasks)
                self.metrics.extend(results)
                
            else:
                # Aggregation across shards
                tasks = []
                for shard in random.sample(self.shards, min(3, len(self.shards))):
                    tasks.append(self.execute_query(session, shard, "range_scan"))
                results = await asyncio.gather(*tasks)
                self.metrics.extend(results)
            
            query_count += 1
            
            if query_count % 100 == 0:
                elapsed = time.time() - self.start_time
                current_qps = query_count / elapsed
                self.log(f"OLAP Progress: {query_count} complex queries, {current_qps:.1f} QPS")
            
            # OLAP queries are slower, less aggressive rate limiting
            await asyncio.sleep(0.5)
    
    async def run_workload(self):
        """Execute the configured workload"""
        self.log(f"Starting workload: {self.config.workload_type}")
        self.log(f"Duration: {self.config.duration_hours} hours ({self.config.duration_hours * 3600} seconds)")
        self.log(f"Target QPS: {self.config.target_qps}")
        self.log(f"Concurrent Clients: {self.config.concurrent_clients}")
        
        self.start_time = time.time()
        duration_seconds = self.config.duration_hours * 3600
        
        # Create HTTP session with connection pooling
        connector = aiohttp.TCPConnector(limit=self.config.concurrent_clients)
        async with aiohttp.ClientSession(connector=connector) as session:
            # Create concurrent client tasks
            tasks = []
            
            for i in range(self.config.concurrent_clients):
                if self.config.workload_type.upper() == "OLTP":
                    task = self.workload_oltp(session, duration_seconds)
                elif self.config.workload_type.upper() == "OLAP":
                    task = self.workload_olap(session, duration_seconds)
                else:  # MIXED
                    if i < self.config.concurrent_clients // 2:
                        task = self.workload_oltp(session, duration_seconds)
                    else:
                        task = self.workload_olap(session, duration_seconds)
                tasks.append(task)
            
            # Execute all client tasks concurrently
            await asyncio.gather(*tasks)
        
        self.end_time = time.time()
        self.log(f"Workload completed. Total metrics: {len(self.metrics)}")
    
    def calculate_results(self) -> BenchmarkResult:
        """Calculate benchmark results from collected metrics"""
        self.log("Calculating benchmark results...")
        
        duration = self.end_time - self.start_time
        successful = [m for m in self.metrics if m.success]
        failed = [m for m in self.metrics if not m.success]
        
        latencies = [m.latency_ms for m in successful]
        latencies.sort()
        
        def percentile(data, p):
            if not data:
                return 0.0
            k = (len(data) - 1) * p
            f = int(k)
            c = f + 1
            if c >= len(data):
                return data[-1]
            d0 = data[f] * (c - k)
            d1 = data[c] * (k - f)
            return d0 + d1
        
        result = BenchmarkResult(
            scenario=self.config.scenario,
            shard_count=self.config.num_shards,
            raid_level=self.config.raid_level,
            workload_type=self.config.workload_type,
            start_time=datetime.fromtimestamp(self.start_time).isoformat(),
            end_time=datetime.fromtimestamp(self.end_time).isoformat(),
            duration_seconds=duration,
            
            total_queries=len(self.metrics),
            successful_queries=len(successful),
            failed_queries=len(failed),
            
            throughput_qps=len(self.metrics) / duration if duration > 0 else 0,
            latency_p50_ms=percentile(latencies, 0.50),
            latency_p95_ms=percentile(latencies, 0.95),
            latency_p99_ms=percentile(latencies, 0.99),
            latency_p999_ms=percentile(latencies, 0.999),
            latency_max_ms=max(latencies) if latencies else 0,
            latency_avg_ms=statistics.mean(latencies) if latencies else 0,
            
            # Placeholder values (would be collected from Prometheus in real scenario)
            cpu_usage_avg_pct=0.0,
            memory_usage_avg_mb=0.0,
            disk_read_mbps=0.0,
            disk_write_mbps=0.0,
            disk_iops_avg=0,
            network_rx_mbps=0.0,
            network_tx_mbps=0.0,
            
            cross_shard_queries=0,
            cross_shard_latency_avg_ms=0.0
        )
        
        return result
    
    def save_results(self, result: BenchmarkResult, output_dir: str = "/results"):
        """Save benchmark results to JSON file"""
        os.makedirs(output_dir, exist_ok=True)
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"{output_dir}/{self.config.scenario}_{self.config.raid_level}_{timestamp}.json"
        
        with open(filename, 'w') as f:
            json.dump(asdict(result), f, indent=2)
        
        self.log(f"Results saved to: {filename}")
        
        # Also save raw metrics
        metrics_file = f"{output_dir}/{self.config.scenario}_{self.config.raid_level}_{timestamp}_metrics.jsonl"
        with open(metrics_file, 'w') as f:
            for metric in self.metrics:
                f.write(json.dumps(asdict(metric)) + '\n')
        
        self.log(f"Raw metrics saved to: {metrics_file}")
        
        # Print summary to console
        self.print_summary(result)
    
    def print_summary(self, result: BenchmarkResult):
        """Print benchmark summary"""
        print("\n" + "="*80)
        print(f"BENCHMARK RESULTS: {result.scenario}")
        print("="*80)
        print(f"Configuration:")
        print(f"  Shards: {result.shard_count}")
        print(f"  RAID Level: {result.raid_level}")
        print(f"  Workload: {result.workload_type}")
        print(f"  Duration: {result.duration_seconds:.1f}s ({result.duration_seconds/3600:.2f}h)")
        print(f"\nPerformance:")
        print(f"  Total Queries: {result.total_queries:,}")
        print(f"  Successful: {result.successful_queries:,} ({result.successful_queries/result.total_queries*100:.1f}%)")
        print(f"  Failed: {result.failed_queries:,}")
        print(f"  Throughput: {result.throughput_qps:.2f} QPS")
        print(f"\nLatency (ms):")
        print(f"  Average: {result.latency_avg_ms:.2f}")
        print(f"  P50: {result.latency_p50_ms:.2f}")
        print(f"  P95: {result.latency_p95_ms:.2f}")
        print(f"  P99: {result.latency_p99_ms:.2f}")
        print(f"  P999: {result.latency_p999_ms:.2f}")
        print(f"  Max: {result.latency_max_ms:.2f}")
        print("="*80 + "\n")

async def main():
    # Load configuration from environment
    config = BenchmarkConfig(
        scenario=os.getenv("SCENARIO", "S1"),
        num_shards=int(os.getenv("NUM_SHARDS", "6")),
        raid_level=os.getenv("RAID_LEVEL", "RAID10"),
        workload_type=os.getenv("WORKLOAD_TYPE", "OLTP"),
        duration_hours=int(os.getenv("DURATION_HOURS", "4")),
        target_qps=int(os.getenv("TARGET_QPS", "10000")),
        concurrent_clients=int(os.getenv("CONCURRENT_CLIENTS", "128")),
        data_size_gb=int(os.getenv("DATA_SIZE_GB", "100"))
    )
    
    # Parse shard endpoints - use localhost ports if running outside Docker
    env_endpoints = os.getenv("SHARD_ENDPOINTS", "")
    if env_endpoints:
        endpoints = env_endpoints.split(",")
    else:
        # Default: localhost ports 8080-8085 (mapped from Docker container ports 8765)
        endpoints = [f"localhost:{8080+i}" for i in range(config.num_shards)]
    
    shards = []
    for i, endpoint in enumerate(endpoints[:config.num_shards]):
        shard = ShardConfig(
            shard_id=f"shard_{i}",
            endpoint=endpoint.strip(),
            raid_level=config.raid_level,
            rack_id=f"rack_{i % 3 + 1}",
            datacenter="dc_1"
        )
        shards.append(shard)
    
    # Create and run benchmark
    benchmark = MultiShardRaidBenchmark(config, shards)
    
    try:
        await benchmark.run_workload()
        result = benchmark.calculate_results()
        benchmark.save_results(result)
    except KeyboardInterrupt:
        benchmark.log("Benchmark interrupted by user", "WARN")
        sys.exit(1)
    except Exception as e:
        benchmark.log(f"Benchmark failed: {e}", "ERROR")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    asyncio.run(main())
