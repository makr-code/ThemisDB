"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            raid_endurance_test.py                             ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:48:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     424                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB RAID Cluster Endurance Test
=====================================
Testet RAID 0, RAID 1, RAID 5 über 2 Stunden mit:
- Schreib- und Leseoperationen
- Performance-Metriken
- Fehlerüberwachung
- Lastverteilung
"""

import requests
import time
import json
import random
import string
import statistics
import os
import re
from datetime import datetime, timedelta
from typing import Dict, List, Tuple
import sys

# RAID Cluster Konfiguration
RAID_CONFIG = {
    "raid0": {
        "name": "RAID 0 (Striping - Performance)",
        "shards": ["http://localhost:8080", "http://localhost:8081", "http://localhost:8082"],
        "ports": [18765, 18766, 18767],
        "metrics": ["http://localhost:9091/metrics", "http://localhost:9092/metrics", "http://localhost:9093/metrics"],
    },
    "raid1": {
        "name": "RAID 1 (Mirroring - Redundanz)",
        "shards": ["http://localhost:8083", "http://localhost:8084"],
        "ports": [18768, 18769],
        "metrics": ["http://localhost:9094/metrics", "http://localhost:9095/metrics"],
    },
    "raid5": {
        "name": "RAID 5 (Striping + Parity)",
        "shards": ["http://localhost:8085", "http://localhost:8086", "http://localhost:8087"],
        "ports": [18770, 18771, 18772],
        "metrics": ["http://localhost:9096/metrics", "http://localhost:9097/metrics", "http://localhost:9098/metrics"],
    }
}

TEST_DURATION_HOURS = float(os.getenv("RAID_TEST_HOURS", "2"))
BATCH_SIZE = int(os.getenv("RAID_TEST_BATCH_SIZE", "100"))
SLEEP_BETWEEN_BATCHES = int(os.getenv("RAID_TEST_SLEEP_SECONDS", "5"))  # Sekunden
SYNC_WAIT_SECONDS = float(os.getenv("RAID_SYNC_WAIT_SECONDS", "0.3"))

class RAIDTester:
    def __init__(self):
        self.start_time = datetime.now()
        self.end_time = self.start_time + timedelta(hours=TEST_DURATION_HOURS)
        self.stats = {
            "raid0": {"writes": 0, "reads": 0, "errors": 0, "write_times": [], "read_times": []},
            "raid1": {"writes": 0, "reads": 0, "errors": 0, "write_times": [], "read_times": []},
            "raid5": {"writes": 0, "reads": 0, "errors": 0, "write_times": [], "read_times": []}
        }
        self.written_keys = {"raid0": [], "raid1": [], "raid5": []}
        self.key_locations = {"raid0": {}, "raid1": {}, "raid5": {}}
        self.sync_stats = {
            "raid1": {"checks": 0, "failures": 0, "latencies": []},
            "raid5": {"checks": 0, "failures": 0, "latencies": []}
        }
        self.enable_plots = os.getenv("RAID_TEST_PLOT", "1") != "0"
        default_metrics = [
            "themis_replication_lag_seconds",
            "themis_raft_last_log_index",
            "themis_cluster_shards",
            "themis_storage_pending_writes"
        ]
        env_metrics = os.getenv("RAID_PROM_METRICS")
        self.prom_metric_names = [m.strip() for m in env_metrics.split(',')] if env_metrics else default_metrics
        self.prom_data = {m: [] for m in self.prom_metric_names}
        
    def log(self, message: str):
        """Schreibt Log-Nachricht mit Zeitstempel"""
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"[{timestamp}] {message}")
        sys.stdout.flush()
        
    def generate_test_data(self, size_kb: int = 10) -> Dict:
        """Generiert zufällige Testdaten"""
        return {
            "id": ''.join(random.choices(string.ascii_letters + string.digits, k=20)),
            "timestamp": datetime.now().isoformat(),
            "data": ''.join(random.choices(string.ascii_letters + string.digits, k=size_kb * 1024)),
            "metadata": {
                "test_type": "endurance",
                "iteration": random.randint(1, 1000000)
            }
        }
        
    def check_health(self, raid_mode: str) -> Tuple[bool, List[str]]:
        """Prüft Health-Status aller Shards eines RAID-Modus"""
        healthy_shards = []
        config = RAID_CONFIG[raid_mode]
        
        for shard_url in config["shards"]:
            try:
                response = requests.get(f"{shard_url}/health", timeout=5)
                if response.status_code == 200:
                    healthy_shards.append(shard_url)
            except Exception as e:
                print(f"  WARNING: Shard {shard_url} unhealthy: {e}")
                
        all_healthy = len(healthy_shards) == len(config["shards"])
        return all_healthy, healthy_shards
        
    def verify_replication(self, raid_mode: str, key: str, origin_shard: str, timeout_sec: float = 3.0):
        """Verifiziert Synchronisation mit kleinem Retry/Backoff."""
        if raid_mode not in ("raid1", "raid5"):
            return
        config = RAID_CONFIG[raid_mode]
        other_shards = [s for s in config["shards"] if s != origin_shard]
        start = time.time()
        for shard_url in other_shards:
            attempts = 0
            max_attempts = 5
            while attempts < max_attempts:
                attempts += 1
                try:
                    resp = requests.get(
                        f"{shard_url}/entities/{key}",
                        headers={"Content-Type": "application/json"},
                        timeout=timeout_sec
                    )
                    self.sync_stats[raid_mode]["checks"] += 1
                    if resp.status_code == 200:
                        latency = time.time() - start
                        self.sync_stats[raid_mode]["latencies"].append(latency)
                        break
                    else:
                        if attempts == max_attempts:
                            self.sync_stats[raid_mode]["failures"] += 1
                            self.log(f"  Sync miss ({raid_mode}) on {shard_url}: {resp.status_code}")
                        else:
                            time.sleep(0.5 * attempts)
                except Exception as e:
                    if attempts == max_attempts:
                        self.sync_stats[raid_mode]["checks"] += 1
                        self.sync_stats[raid_mode]["failures"] += 1
                        print(f"  Sync error ({raid_mode}) on {shard_url}: {e}")
                    else:
                        time.sleep(0.5 * attempts)

    def scrape_prom_metrics(self, raid_mode: str):
        """Liest Prometheus-Metriken der Shards und extrahiert definierte Kennzahlen."""
        config = RAID_CONFIG[raid_mode]
        if "metrics" not in config:
            return
        for url in config["metrics"]:
            try:
                resp = requests.get(url, timeout=3)
                if resp.status_code != 200:
                    continue
                lines = resp.text.splitlines()
                for line in lines:
                    # simple parser: metric value is last token
                    for metric in self.prom_metric_names:
                        if not line.startswith(metric):
                            continue
                        tokens = line.strip().split()
                        if len(tokens) >= 2:
                            try:
                                value = float(tokens[-1])
                                self.prom_data[metric].append(value)
                            except ValueError:
                                pass
            except Exception as e:
                print(f"  Prom scrape failed {url}: {e}")

    def write_test(self, raid_mode: str, batch_size: int) -> int:
        """Führt Schreibtest auf RAID-Modus durch"""
        config = RAID_CONFIG[raid_mode]
        success_count = 0
        
        for i in range(batch_size):
            # Round-robin über Shards
            shard_idx = i % len(config["shards"])
            shard_url = config["shards"][shard_idx]
            
            data = self.generate_test_data(size_kb=10)
            # API erwartet Key im Format table:pk und ein Feld 'blob' als JSON-String
            key = f"test:{raid_mode}_{int(time.time())}_{i}"
            
            try:
                start = time.time()
                response = requests.post(
                    f"{shard_url}/entities",
                    json={"key": key, "blob": json.dumps(data)},
                    headers={"Content-Type": "application/json"},
                    timeout=10
                )
                elapsed = time.time() - start
                
                if response.status_code in [200, 201]:
                    self.stats[raid_mode]["writes"] += 1
                    self.stats[raid_mode]["write_times"].append(elapsed)
                    self.written_keys[raid_mode].append(key)
                    self.key_locations[raid_mode][key] = shard_url
                    # Replikationsprüfung (nur für RAID1/RAID5)
                    if raid_mode in ("raid1", "raid5"):
                        self.verify_replication(raid_mode, key, shard_url)
                    success_count += 1
                else:
                    self.stats[raid_mode]["errors"] += 1
                    self.log(f"  Write failed ({raid_mode}) [{response.status_code}]: {response.text[:200]}")
                    
            except Exception as e:
                self.stats[raid_mode]["errors"] += 1
                print(f"  Write error ({raid_mode}): {e}")
                
        return success_count
        
    def read_test(self, raid_mode: str, sample_size: int) -> int:
        """Führt Lesetest auf RAID-Modus durch"""
        config = RAID_CONFIG[raid_mode]
        keys = self.written_keys[raid_mode]
        
        if not keys:
            return 0
            
        # Zufällige Auswahl von Keys zum Lesen
        sample_keys = random.sample(keys, min(sample_size, len(keys)))
        success_count = 0
        
        for key in sample_keys:
            # Lese vom ursprünglichen Shard, um 404 durch falsches Shard-Routing zu vermeiden
            shard_url = self.key_locations[raid_mode].get(key, random.choice(config["shards"]))
            
            try:
                start = time.time()
                response = requests.get(
                    f"{shard_url}/entities/{key}",
                    headers={"Content-Type": "application/json"},
                    timeout=10
                )
                elapsed = time.time() - start
                
                if response.status_code == 200:
                    self.stats[raid_mode]["reads"] += 1
                    self.stats[raid_mode]["read_times"].append(elapsed)
                    success_count += 1
                else:
                    self.stats[raid_mode]["errors"] += 1
                    self.log(f"  Read failed ({raid_mode}) [{response.status_code}]: {response.text[:200]}")
                    
            except Exception as e:
                self.stats[raid_mode]["errors"] += 1
                print(f"  Read error ({raid_mode}): {e}")
        
        return success_count
        
    def print_statistics(self):
        """Gibt aktuelle Statistiken aus"""
        self.log("\n" + "="*80)
        self.log("RAID CLUSTER STATISTICS")
        self.log("="*80)
        
        elapsed = datetime.now() - self.start_time
        remaining = self.end_time - datetime.now()
        
        self.log(f"Elapsed: {elapsed}, Remaining: {remaining}")
        self.log("")
        
        for raid_mode, config in RAID_CONFIG.items():
            stats = self.stats[raid_mode]
            self.log(f"{config['name']}:")
            self.log(f"  Writes:  {stats['writes']:>8} ops")
            self.log(f"  Reads:   {stats['reads']:>8} ops")
            self.log(f"  Errors:  {stats['errors']:>8}")
            
            if stats['write_times']:
                avg_write = statistics.mean(stats['write_times']) * 1000
                self.log(f"  Avg Write: {avg_write:>6.2f} ms")
                
            if stats['read_times']:
                avg_read = statistics.mean(stats['read_times']) * 1000
                self.log(f"  Avg Read:  {avg_read:>6.2f} ms")
                
            self.log(f"  Keys stored: {len(self.written_keys[raid_mode])}")
            self.log("")

        # Sync-Metriken für RAID1/RAID5
        for raid_mode in ("raid1", "raid5"):
            sync = self.sync_stats[raid_mode]
            if sync["checks"] > 0:
                failure_rate = sync["failures"] / sync["checks"] * 100
                avg_latency = statistics.mean(sync["latencies"]) * 1000 if sync["latencies"] else 0
                self.log(f"Sync {raid_mode}: checks={sync['checks']} failures={sync['failures']} ({failure_rate:.2f}%) avg-lag={avg_latency:.2f} ms")
        self.log("")

        # Prometheus-Metriken (Aggregat über letzte Scrapes)
        if any(self.prom_data[m] for m in self.prom_metric_names):
            self.log("Prometheus metrics (aggregated):")
            for metric in self.prom_metric_names:
                values = self.prom_data.get(metric, [])
                if not values:
                    continue
                self.log(
                    f"  {metric}: avg={statistics.mean(values):.2f} min={min(values):.2f} max={max(values):.2f} samples={len(values)}"
                )
            self.log("")
            
        self.log("="*80 + "\n")
        
    def run(self):
        """Führt 2-Stunden-Dauertest durch"""
        self.log(f"Starting RAID Endurance Test - Duration: {TEST_DURATION_HOURS} hours")
        self.log(f"Start: {self.start_time.strftime('%Y-%m-%d %H:%M:%S')}")
        self.log(f"End:   {self.end_time.strftime('%Y-%m-%d %H:%M:%S')}")
        self.log("")
        
        # Initial Health Check
        self.log("Initial Health Check:")
        for raid_mode, config in RAID_CONFIG.items():
            healthy, shards = self.check_health(raid_mode)
            status = "✓ OK" if healthy else "✗ DEGRADED"
            self.log(f"  {config['name']}: {status} ({len(shards)}/{len(config['shards'])} shards)")
        self.log("")
        
        iteration = 0
        
        try:
            while datetime.now() < self.end_time:
                iteration += 1
                self.log(f"Iteration {iteration} - Testing all RAID modes...")
                
                # Test RAID 0 (Performance-fokussiert)
                self.log(f"  RAID 0: Writing {BATCH_SIZE} entries...")
                writes = self.write_test("raid0", BATCH_SIZE)
                self.log(f"  RAID 0: Reading {BATCH_SIZE//2} entries...")
                reads = self.read_test("raid0", BATCH_SIZE // 2)
                self.log(f"  RAID 0: ✓ {writes} writes, {reads} reads")
                
                # Test RAID 1 (Redundanz-fokussiert)
                self.log(f"  RAID 1: Writing {BATCH_SIZE//2} entries...")
                writes = self.write_test("raid1", BATCH_SIZE // 2)
                if writes > 0:
                    time.sleep(SYNC_WAIT_SECONDS)
                self.log(f"  RAID 1: Reading {BATCH_SIZE//4} entries...")
                reads = self.read_test("raid1", BATCH_SIZE // 4)
                self.log(f"  RAID 1: ✓ {writes} writes, {reads} reads")
                
                # Test RAID 5 (Balanced)
                self.log(f"  RAID 5: Writing {BATCH_SIZE} entries...")
                writes = self.write_test("raid5", BATCH_SIZE)
                if writes > 0:
                    time.sleep(SYNC_WAIT_SECONDS)
                self.log(f"  RAID 5: Reading {BATCH_SIZE//2} entries...")
                reads = self.read_test("raid5", BATCH_SIZE // 2)
                self.log(f"  RAID 5: ✓ {writes} writes, {reads} reads")
                
                # Statistiken alle 10 Iterationen
                if iteration % 10 == 0:
                    # Prometheus-Metriken sammeln vor Ausgabe
                    for rm in RAID_CONFIG.keys():
                        self.scrape_prom_metrics(rm)
                    self.print_statistics()
                    
                # Health Check alle 20 Iterationen
                if iteration % 20 == 0:
                    self.log("Health Check:")
                    for raid_mode, config in RAID_CONFIG.items():
                        healthy, shards = self.check_health(raid_mode)
                        status = "✓" if healthy else "✗"
                        self.log(f"  {status} {config['name']}: {len(shards)}/{len(config['shards'])}")
                    self.log("")
                    
                # Pause zwischen Batches
                time.sleep(SLEEP_BETWEEN_BATCHES)
                
        except KeyboardInterrupt:
            self.log("\nTest interrupted by user (Ctrl+C)")
            
        # Final Statistics
        self.log("\n" + "="*80)
        self.log("FINAL RAID CLUSTER TEST RESULTS")
        self.log("="*80)
        self.print_statistics()
        
        # Zusammenfassung
        total_writes = sum(s["writes"] for s in self.stats.values())
        total_reads = sum(s["reads"] for s in self.stats.values())
        total_errors = sum(s["errors"] for s in self.stats.values())
        
        self.log("OVERALL SUMMARY:")
        self.log(f"  Total Writes: {total_writes}")
        self.log(f"  Total Reads:  {total_reads}")
        self.log(f"  Total Errors: {total_errors}")
        self.log(f"  Success Rate: {((total_writes + total_reads) / (total_writes + total_reads + total_errors) * 100):.2f}%")
        self.log(f"  Duration:     {datetime.now() - self.start_time}")
        self.log("="*80)
        
        return total_errors == 0

if __name__ == "__main__":
    tester = RAIDTester()
    success = tester.run()
    sys.exit(0 if success else 1)
