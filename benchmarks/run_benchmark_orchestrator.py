"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            run_benchmark_orchestrator.py                      ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     244                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Master Benchmark Orchestrator
==============================
Manages complete benchmark suite execution:
1. Starts all databases in Docker
2. Waits for health checks
3. Runs unified benchmarks
4. Generates comparison reports
5. Cleanup

Author: ThemisDB Team
Date: 2025-12-04
"""

import subprocess
import time
import requests
import sys
import os
from datetime import datetime

class BenchmarkOrchestrator:
    def __init__(self):
        self.compose_file = "docker-compose.benchmark.yml"
        self.databases = {
            'themis': {'url': 'http://localhost:8765/health', 'name': 'ThemisDB'},
            'postgresql': {'cmd': 'docker exec benchmark_postgresql pg_isready -U postgres', 'name': 'PostgreSQL'},
            'mongodb': {'cmd': 'docker exec benchmark_mongodb mongosh --eval "db.adminCommand(\'ping\')"', 'name': 'MongoDB'},
            'redis': {'cmd': 'docker exec benchmark_redis redis-cli ping', 'name': 'Redis'},
            'elasticsearch': {'url': 'http://localhost:9200/_cluster/health', 'name': 'Elasticsearch'}
        }
    
    def print_header(self, text):
        """Print formatted header"""
        print("\n" + "="*80)
        print(f"  {text}")
        print("="*80 + "\n")
    
    def check_docker(self):
        """Verify Docker is running"""
        self.print_header("Checking Docker")
        try:
            result = subprocess.run(['docker', 'info'], capture_output=True, text=True, timeout=5)
            if result.returncode == 0:
                print("✓ Docker is running")
                return True
            else:
                print("✗ Docker is not running")
                return False
        except Exception as e:
            print(f"✗ Error checking Docker: {e}")
            return False
    
    def start_databases(self):
        """Start all databases with Docker Compose"""
        self.print_header("Starting Databases")
        
        print("Starting containers with docker-compose...")
        try:
            subprocess.run(['docker-compose', '-f', self.compose_file, 'up', '-d'], 
                          check=True, timeout=120)
            print("✓ Containers started")
            return True
        except Exception as e:
            print(f"✗ Failed to start containers: {e}")
            return False
    
    def wait_for_health(self, max_wait=120):
        """Wait for all databases to be healthy"""
        self.print_header("Waiting for Database Health Checks")
        
        start_time = time.time()
        healthy = {db: False for db in self.databases}
        
        while time.time() - start_time < max_wait:
            all_healthy = True
            
            for db_key, db_info in self.databases.items():
                if healthy[db_key]:
                    continue
                
                try:
                    if 'url' in db_info:
                        # HTTP health check
                        r = requests.get(db_info['url'], timeout=2)
                        if r.status_code in [200, 201]:
                            healthy[db_key] = True
                            print(f"✓ {db_info['name']:20s} is healthy")
                    elif 'cmd' in db_info:
                        # Command health check
                        result = subprocess.run(db_info['cmd'].split(), 
                                              capture_output=True, timeout=2)
                        if result.returncode == 0:
                            healthy[db_key] = True
                            print(f"✓ {db_info['name']:20s} is healthy")
                except:
                    all_healthy = False
            
            if all(healthy.values()):
                print(f"\n✓ All databases are healthy (took {time.time() - start_time:.1f}s)")
                return True
            
            time.sleep(2)
        
        print(f"\n⚠ Timeout waiting for databases:")
        for db_key, is_healthy in healthy.items():
            if not is_healthy:
                print(f"  ✗ {self.databases[db_key]['name']} did not become healthy")
        
        return False
    
    def run_benchmarks(self):
        """Execute unified benchmark suite"""
        self.print_header("Running Unified Benchmark Suite")
        
        try:
            result = subprocess.run(['python3', 'unified_benchmark_suite.py', 
                                   '--iterations', '5', '--warmup', '2', '--dataset-size', '10000'],
                                  timeout=1800)  # 30 min timeout
            
            if result.returncode == 0:
                print("\n✓ Benchmarks completed successfully")
                return True
            else:
                print(f"\n✗ Benchmarks failed with code {result.returncode}")
                return False
        except subprocess.TimeoutExpired:
            print("\n✗ Benchmarks timed out after 30 minutes")
            return False
        except Exception as e:
            print(f"\n✗ Error running benchmarks: {e}")
            return False
    
    def stop_databases(self):
        """Stop all database containers"""
        self.print_header("Stopping Databases")
        
        try:
            subprocess.run(['docker-compose', '-f', self.compose_file, 'down'], 
                          check=True, timeout=60)
            print("✓ Containers stopped")
            return True
        except Exception as e:
            print(f"✗ Failed to stop containers: {e}")
            return False
    
    def show_resource_usage(self):
        """Show Docker resource usage"""
        self.print_header("Container Resource Usage")
        
        try:
            result = subprocess.run(['docker', 'stats', '--no-stream', '--format', 
                                   'table {{.Container}}\t{{.CPUPerc}}\t{{.MemUsage}}'],
                                  capture_output=True, text=True, timeout=5)
            print(result.stdout)
        except Exception as e:
            print(f"Could not retrieve stats: {e}")
    
    def run(self, skip_startup=False, skip_cleanup=False):
        """Run complete benchmark orchestration"""
        print("\n╔════════════════════════════════════════════════════════════════════════════╗")
        print("║                  Benchmark Orchestrator                                    ║")
        print("║                  Unified DB Comparison Suite                               ║")
        print("╚════════════════════════════════════════════════════════════════════════════╝")
        
        print(f"\nStart Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        
        # Step 1: Check Docker
        if not self.check_docker():
            print("\n✗ Cannot proceed without Docker")
            return False
        
        # Step 2: Start databases (unless skipped)
        if not skip_startup:
            if not self.start_databases():
                print("\n✗ Cannot proceed without databases")
                return False
            
            # Step 3: Wait for health
            if not self.wait_for_health():
                print("\n⚠ Proceeding despite unhealthy databases...")
            
            time.sleep(5)  # Extra buffer
        else:
            print("\n⚠ Skipping database startup (--skip-startup)")
        
        # Step 4: Show resource usage
        self.show_resource_usage()
        
        # Step 5: Run benchmarks
        success = self.run_benchmarks()
        
        # Step 6: Cleanup (unless skipped)
        if not skip_cleanup:
            self.stop_databases()
        else:
            print("\n⚠ Skipping cleanup (--skip-cleanup)")
        
        # Summary
        self.print_header("Benchmark Orchestration Complete")
        print(f"End Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"Status: {'✓ SUCCESS' if success else '✗ FAILED'}")
        
        return success

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description='Benchmark Orchestrator')
    parser.add_argument('--skip-startup', action='store_true', 
                       help='Skip database startup (use existing containers)')
    parser.add_argument('--skip-cleanup', action='store_true',
                       help='Skip cleanup (leave containers running)')
    
    args = parser.parse_args()
    
    orchestrator = BenchmarkOrchestrator()
    success = orchestrator.run(skip_startup=args.skip_startup, 
                              skip_cleanup=args.skip_cleanup)
    
    sys.exit(0 if success else 1)
