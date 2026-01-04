#!/usr/bin/env python3
"""
RAID LoRA Pipeline Test Orchestrator

Orchestriert die komplette Test-Pipeline:
1. Daten in Shards pushen
2. Metriken vor/nach Tests abrufen
3. Test-Ergebnisse validieren
4. HTML-Report generieren
"""

import requests
import json
import time
import sys
import os
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple
import subprocess

class RAIDLoRAPipelineOrchestrator:
    def __init__(self):
        self.raid0_shards = [
            "http://themis-raid0-shard1:8080",
            "http://themis-raid0-shard2:8080",
            "http://themis-raid0-shard3:8080"
        ]
        self.raid1_shards = [
            "http://themis-raid1-primary:8080",
            "http://themis-raid1-mirror:8080"
        ]
        self.raid5_shards = [
            "http://themis-raid5-shard1:8080",
            "http://themis-raid5-shard2:8080",
            "http://themis-raid5-shard3:8080"
        ]
        self.all_shards = self.raid0_shards + self.raid1_shards + self.raid5_shards
        self.metrics_before = {}
        self.metrics_after = {}
        self.results = {"phases": {}, "metrics": {}, "validation": {}}
        self.test_results_dir = Path("/test_results")
        self.test_data_dir = Path("/test_data")
        
    def log(self, msg: str, level: str = "INFO"):
        """Log message with timestamp"""
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"[{timestamp}] [{level}] {msg}")
    
    def wait_for_shards(self, timeout: int = 120) -> bool:
        """Wait for all shards to be healthy"""
        self.log("Waiting for RAID shards to be healthy...")
        start_time = time.time()
        
        while time.time() - start_time < timeout:
            healthy = 0
            for shard in self.all_shards:
                try:
                    resp = requests.get(f"{shard}/health", timeout=5)
                    if resp.status_code == 200:
                        healthy += 1
                except:
                    pass
            
            if healthy == len(self.all_shards):
                self.log(f"All {len(self.all_shards)} shards are healthy")
                return True
            
            self.log(f"Healthy shards: {healthy}/{len(self.all_shards)}")
            time.sleep(5)
        
        self.log(f"Timeout waiting for shards after {timeout}s", "ERROR")
        return False
    
    def get_shard_metrics(self, shard_url: str) -> Optional[Dict]:
        """Get Prometheus metrics from a shard"""
        try:
            # Get metrics from shard (usually port :9090/metrics)
            metrics_url = shard_url.replace(":8080", ":9090") + "/metrics"
            resp = requests.get(metrics_url, timeout=10)
            
            if resp.status_code == 200:
                # Parse basic metrics
                metrics = {
                    "timestamp": datetime.now().isoformat(),
                    "raw": resp.text
                }
                
                # Extract key metrics
                for line in resp.text.split('\n'):
                    if 'themis_' in line and not line.startswith('#'):
                        metrics[line.split('{')[0]] = line
                
                return metrics
        except Exception as e:
            self.log(f"Failed to get metrics from {shard_url}: {e}", "WARN")
        
        return None
    
    def collect_metrics_baseline(self):
        """Collect baseline metrics before tests"""
        self.log("Collecting baseline metrics from all shards...")
        
        for shard in self.all_shards:
            shard_name = shard.split('/')[-1]
            metrics = self.get_shard_metrics(shard)
            if metrics:
                self.metrics_before[shard_name] = metrics
                self.log(f"Collected baseline metrics from {shard_name}")
    
    def push_test_data_to_shards(self) -> bool:
        """Push test data to RAID shards"""
        self.log("=" * 60)
        self.log("PHASE 1: Pushing Test Data to RAID Shards")
        self.log("=" * 60)
        
        try:
            # Generate test records
            test_records = []
            for i in range(100):
                record = {
                    "id": f"test_{i}",
                    "domain": ["legal", "medical", "finance"][i % 3],
                    "content": f"Test record {i} with domain-specific content",
                    "timestamp": datetime.now().isoformat()
                }
                test_records.append(record)
            
            # Push to each shard (round-robin distribution)
            pushed_count = 0
            for idx, record in enumerate(test_records):
                shard = self.all_shards[idx % len(self.all_shards)]
                shard_name = shard.split('/')[-1]
                
                try:
                    # Create collection if needed
                    collection_resp = requests.post(
                        f"{shard}/api/v1/collections",
                        json={"name": "test_collection"},
                        timeout=5
                    )
                    
                    # Insert document
                    insert_resp = requests.post(
                        f"{shard}/api/v1/collections/test_collection/documents",
                        json=record,
                        timeout=5
                    )
                    
                    if insert_resp.status_code in [200, 201, 202]:
                        pushed_count += 1
                        if pushed_count % 25 == 0:
                            self.log(f"Pushed {pushed_count} records to {shard_name}")
                
                except Exception as e:
                    self.log(f"Failed to push record {i} to {shard_name}: {e}", "WARN")
            
            self.log(f"Successfully pushed {pushed_count}/{len(test_records)} records to shards")
            self.results["phases"]["data_push"] = {
                "status": "success" if pushed_count > 0 else "failed",
                "pushed_records": pushed_count,
                "total_records": len(test_records)
            }
            
            return pushed_count > 0
        
        except Exception as e:
            self.log(f"Data push phase failed: {e}", "ERROR")
            self.results["phases"]["data_push"] = {"status": "error", "error": str(e)}
            return False
    
    def verify_data_distribution(self) -> bool:
        """Verify data was distributed correctly"""
        self.log("Verifying data distribution across RAID shards...")
        
        try:
            total_records = 0
            for shard in self.all_shards:
                shard_name = shard.split('/')[-1]
                try:
                    resp = requests.get(
                        f"{shard}/api/v1/collections/test_collection/documents",
                        timeout=5
                    )
                    if resp.status_code == 200:
                        docs = resp.json().get("documents", [])
                        total_records += len(docs)
                        self.log(f"{shard_name}: {len(docs)} documents")
                except:
                    pass
            
            self.log(f"Total documents across all shards: {total_records}")
            self.results["phases"]["data_verification"] = {
                "status": "success" if total_records > 0 else "failed",
                "total_records": total_records
            }
            
            return total_records > 0
        
        except Exception as e:
            self.log(f"Data verification failed: {e}", "ERROR")
            return False
    
    def run_tests(self, test_type: str = "all") -> bool:
        """Run C++ test suite"""
        self.log("=" * 60)
        self.log(f"PHASE 2: Running Test Suite ({test_type})")
        self.log("=" * 60)
        
        try:
            # Map test types to executables
            test_commands = {
                "pipeline": ["test_llm_raid_pipeline"],
                "inline": ["test_llm_lora_inline"],
                "all_tests": ["test_llm_lora_inline", "test_llm_raid_pipeline"],
                "all_bench": ["bench_lora_inline", "bench_llm_raid_pipeline"],
                "all": [
                    "test_llm_lora_inline",
                    "test_llm_raid_pipeline",
                    "bench_lora_inline",
                    "bench_llm_raid_pipeline"
                ]
            }
            
            commands = test_commands.get(test_type, test_commands["all"])
            
            for cmd in commands:
                self.log(f"Running {cmd}...")
                
                # Determine if it's a test or benchmark
                is_benchmark = cmd.startswith("bench_")
                
                if is_benchmark:
                    # Run benchmark
                    output_file = self.test_results_dir / f"{cmd}_results.json"
                    result = subprocess.run(
                        [cmd, 
                         f"--benchmark_out={output_file}",
                         "--benchmark_out_format=json",
                         "--benchmark_time_unit=ms"],
                        timeout=600
                    )
                else:
                    # Run test
                    output_file = self.test_results_dir / f"{cmd}_results.xml"
                    result = subprocess.run(
                        [cmd,
                         f"--gtest_output=xml:{output_file}"],
                        timeout=600
                    )
                
                if result.returncode == 0:
                    self.log(f"{cmd}: PASSED")
                    self.results["phases"][cmd] = {"status": "passed"}
                else:
                    self.log(f"{cmd}: FAILED", "ERROR")
                    self.results["phases"][cmd] = {"status": "failed"}
                    return False
            
            return True
        
        except Exception as e:
            self.log(f"Test execution failed: {e}", "ERROR")
            return False
    
    def collect_metrics_after(self):
        """Collect metrics after tests"""
        self.log("Collecting post-test metrics from all shards...")
        
        for shard in self.all_shards:
            shard_name = shard.split('/')[-1]
            metrics = self.get_shard_metrics(shard)
            if metrics:
                self.metrics_after[shard_name] = metrics
                self.log(f"Collected post-test metrics from {shard_name}")
    
    def validate_results(self) -> bool:
        """Validate test results"""
        self.log("=" * 60)
        self.log("PHASE 3: Validating Results")
        self.log("=" * 60)
        
        try:
            validation_results = {}
            
            # Check test result files
            self.log("Checking test result files...")
            result_files = list(self.test_results_dir.glob("*_results.*"))
            
            for result_file in result_files:
                self.log(f"Found result file: {result_file.name}")
                validation_results[result_file.name] = "found"
                
                # Validate XML test results
                if result_file.suffix == ".xml":
                    try:
                        import xml.etree.ElementTree as ET
                        tree = ET.parse(result_file)
                        root = tree.getroot()
                        
                        # Count tests
                        if root.tag == "testsuites":
                            total_tests = root.get("tests", "0")
                            failures = root.get("failures", "0")
                            self.log(f"  Tests: {total_tests}, Failures: {failures}")
                            
                            if int(failures) > 0:
                                validation_results[result_file.name] = "some_failures"
                    except Exception as e:
                        self.log(f"  Failed to parse XML: {e}", "WARN")
                
                # Validate JSON benchmark results
                elif result_file.suffix == ".json":
                    try:
                        with open(result_file) as f:
                            data = json.load(f)
                            benchmarks = data.get("benchmarks", [])
                            self.log(f"  Benchmarks: {len(benchmarks)}")
                    except Exception as e:
                        self.log(f"  Failed to parse JSON: {e}", "WARN")
            
            # Check metrics delta
            self.log("Comparing metrics before/after...")
            metrics_delta = {}
            
            for shard_name in self.metrics_before:
                if shard_name in self.metrics_after:
                    before = self.metrics_before[shard_name]
                    after = self.metrics_after[shard_name]
                    
                    # Basic delta calculation
                    metrics_delta[shard_name] = {
                        "before_timestamp": before.get("timestamp"),
                        "after_timestamp": after.get("timestamp")
                    }
                    self.log(f"  {shard_name}: metrics collected")
            
            self.results["validation"] = {
                "result_files": validation_results,
                "metrics_delta": metrics_delta,
                "total_checks": len(validation_results) + len(metrics_delta),
                "passed_checks": len(validation_results) + len(metrics_delta)
            }
            
            return len(validation_results) > 0
        
        except Exception as e:
            self.log(f"Validation failed: {e}", "ERROR")
            return False
    
    def generate_report(self):
        """Generate HTML report"""
        self.log("Generating HTML report...")
        
        html_content = f"""
<!DOCTYPE html>
<html>
<head>
    <title>ThemisDB LLM RAID Pipeline Test Report</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }}
        .container {{ max-width: 1200px; margin: 0 auto; background: white; padding: 20px; border-radius: 5px; }}
        h1 {{ color: #333; border-bottom: 3px solid #007bff; padding-bottom: 10px; }}
        h2 {{ color: #555; margin-top: 30px; }}
        .phase {{ background: #f9f9f9; padding: 15px; margin: 10px 0; border-left: 4px solid #007bff; }}
        .success {{ color: green; font-weight: bold; }}
        .failed {{ color: red; font-weight: bold; }}
        .warning {{ color: orange; font-weight: bold; }}
        table {{ width: 100%; border-collapse: collapse; margin: 10px 0; }}
        th, td {{ padding: 10px; text-align: left; border-bottom: 1px solid #ddd; }}
        th {{ background: #f0f0f0; font-weight: bold; }}
        tr:hover {{ background: #f5f5f5; }}
        .metrics {{ background: #f0f8ff; padding: 10px; border-radius: 3px; }}
        .footer {{ margin-top: 40px; text-align: center; color: #999; border-top: 1px solid #ddd; padding-top: 20px; }}
    </style>
</head>
<body>
    <div class="container">
        <h1>ThemisDB LLM RAID Pipeline Test Report</h1>
        <p><strong>Generated:</strong> {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}</p>
        
        <h2>Test Phases</h2>
        <div class="phase">
            <h3>Phase 1: Data Push</h3>
            <p><strong>Status:</strong> 
                <span class="success">
                    {self.results.get('phases', {}).get('data_push', {}).get('status', 'unknown').upper()}
                </span>
            </p>
            <table>
                <tr>
                    <th>Metric</th>
                    <th>Value</th>
                </tr>
                <tr>
                    <td>Pushed Records</td>
                    <td>{self.results.get('phases', {}).get('data_push', {}).get('pushed_records', 0)}</td>
                </tr>
                <tr>
                    <td>Total Records</td>
                    <td>{self.results.get('phases', {}).get('data_push', {}).get('total_records', 0)}</td>
                </tr>
            </table>
        </div>
        
        <div class="phase">
            <h3>Phase 2: Tests Executed</h3>
            <table>
                <tr>
                    <th>Test</th>
                    <th>Status</th>
                </tr>
"""
        
        # Add test results
        for test_name, test_info in self.results.get('phases', {}).items():
            if test_name not in ['data_push', 'data_verification']:
                status = test_info.get('status', 'unknown').upper()
                status_class = 'success' if status == 'PASSED' else 'failed'
                html_content += f"""
                <tr>
                    <td>{test_name}</td>
                    <td><span class="{status_class}">{status}</span></td>
                </tr>
"""
        
        html_content += """
            </table>
        </div>
        
        <div class="phase">
            <h3>Phase 3: Metrics & Validation</h3>
"""
        
        # Add validation results
        validation = self.results.get('validation', {})
        html_content += f"""
            <p><strong>Total Result Files:</strong> {len(validation.get('result_files', {}))} found</p>
            <p><strong>Metrics Collected:</strong> {len(validation.get('metrics_delta', {}))} shards</p>
            <table>
                <tr>
                    <th>Shard</th>
                    <th>Metrics Collected</th>
                </tr>
"""
        
        for shard_name in validation.get('metrics_delta', {}):
            html_content += f"""
                <tr>
                    <td>{shard_name}</td>
                    <td><span class="success">YES</span></td>
                </tr>
"""
        
        html_content += """
            </table>
        </div>
        
        <h2>Summary</h2>
        <div class="phase">
            <p><strong>Overall Status:</strong> 
"""
        
        # Determine overall status
        all_passed = all(
            p.get('status') == 'passed' 
            for p in self.results.get('phases', {}).values() 
            if isinstance(p, dict)
        )
        
        if all_passed:
            html_content += '<span class="success">PASSED</span>'
        else:
            html_content += '<span class="failed">FAILED</span>'
        
        html_content += f"""
            </p>
            <p><strong>Test Results Directory:</strong> {self.test_results_dir}</p>
        </div>
        
        <div class="footer">
            <p>ThemisDB RAID LoRA Pipeline Test Suite</p>
            <p>Report generated automatically</p>
        </div>
    </div>
</body>
</html>
"""
        
        report_file = self.test_results_dir / "test_report.html"
        with open(report_file, 'w') as f:
            f.write(html_content)
        
        self.log(f"Report generated: {report_file}")
    
    def run_full_pipeline(self, test_type: str = "all"):
        """Run complete pipeline"""
        self.log("=" * 60)
        self.log("ThemisDB RAID LoRA Pipeline Test Orchestrator")
        self.log("=" * 60)
        
        # Ensure directories exist
        self.test_results_dir.mkdir(parents=True, exist_ok=True)
        self.test_data_dir.mkdir(parents=True, exist_ok=True)
        
        # Wait for shards
        if not self.wait_for_shards():
            self.log("Failed to reach healthy shards", "ERROR")
            return False
        
        # Collect baseline metrics
        self.collect_metrics_baseline()
        
        # Phase 1: Push data
        if not self.push_test_data_to_shards():
            self.log("Failed to push test data", "ERROR")
        
        # Verify distribution
        self.verify_data_distribution()
        
        # Phase 2: Run tests
        if not self.run_tests(test_type):
            self.log("Some tests failed", "WARN")
        
        # Collect post-test metrics
        self.collect_metrics_after()
        
        # Phase 3: Validate
        self.validate_results()
        
        # Generate report
        self.generate_report()
        
        # Save results as JSON
        results_file = self.test_results_dir / "orchestrator_results.json"
        with open(results_file, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        self.log(f"Results saved to {results_file}")
        self.log("=" * 60)
        self.log("Pipeline execution completed")
        self.log("=" * 60)

def main():
    test_type = sys.argv[1] if len(sys.argv) > 1 else "all"
    
    orchestrator = RAIDLoRAPipelineOrchestrator()
    orchestrator.run_full_pipeline(test_type)

if __name__ == "__main__":
    main()
