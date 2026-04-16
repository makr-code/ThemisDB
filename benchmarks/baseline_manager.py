"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            baseline_manager.py                                ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     311                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Baseline Manager for ThemisDB Performance Benchmarks

Manages baseline storage, retrieval, and updates for performance regression detection.
"""

import json
import os
import argparse
import shutil
from pathlib import Path
from datetime import datetime, timezone
from typing import Dict, Optional, Any


class BaselineManager:
    """Manages benchmark baselines for regression detection"""
    
    def __init__(self, baselines_dir: str = None):
        """
        Initialize BaselineManager
        
        Args:
            baselines_dir: Root directory for baseline storage
        """
        if baselines_dir is None:
            # Default to benchmarks/baselines relative to script location
            script_dir = Path(__file__).parent
            baselines_dir = script_dir / "baselines"
        
        self.baselines_dir = Path(baselines_dir)
        self.main_dir = self.baselines_dir / "main"
        self.releases_dir = self.baselines_dir / "releases"
        self.develop_dir = self.baselines_dir / "develop"
        
        # Ensure directories exist
        self.main_dir.mkdir(parents=True, exist_ok=True)
        self.releases_dir.mkdir(parents=True, exist_ok=True)
        self.develop_dir.mkdir(parents=True, exist_ok=True)
    
    def save_baseline(
        self,
        benchmark_data: Dict[str, Any],
        branch: str,
        version: str,
        commit: str,
        is_release: bool = False
    ) -> Path:
        """
        Save a baseline for a specific branch/release
        
        Args:
            benchmark_data: Dictionary of benchmark results
            branch: Git branch name (main, develop, feature/xyz)
            version: Version string (e.g., "1.4.1")
            commit: Git commit hash
            is_release: Whether this is a release tag
            
        Returns:
            Path to saved baseline file
        """
        baseline = {
            "version": version,
            "branch": branch,
            "commit": commit,
            "timestamp": datetime.now(timezone.utc).isoformat().replace('+00:00', 'Z'),
            "benchmarks": benchmark_data
        }
        
        if is_release:
            # Save as versioned release
            filepath = self.releases_dir / f"v{version}.json"
        elif branch == "main":
            filepath = self.main_dir / "latest.json"
        elif branch == "develop":
            filepath = self.develop_dir / "latest.json"
        else:
            # For feature branches, save with timestamp
            timestamp = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")
            safe_branch = branch.replace("/", "_").replace("\\", "_")
            filepath = self.baselines_dir / f"{safe_branch}_{timestamp}.json"
        
        # Write baseline file
        with open(filepath, 'w') as f:
            json.dump(baseline, f, indent=2)
        
        print(f"✅ Baseline saved: {filepath}")
        return filepath
    
    def load_baseline(
        self,
        branch: str = None,
        version: str = None
    ) -> Optional[Dict[str, Any]]:
        """
        Load a baseline for comparison
        
        Args:
            branch: Branch name (main, develop) for latest baseline
            version: Specific version (e.g., "1.4.0") for release baseline
            
        Returns:
            Baseline dictionary or None if not found
        """
        if version:
            filepath = self.releases_dir / f"v{version}.json"
        elif branch == "main":
            filepath = self.main_dir / "latest.json"
        elif branch == "develop":
            filepath = self.develop_dir / "latest.json"
        else:
            print(f"❌ Invalid branch or version specified")
            return None
        
        if not filepath.exists():
            print(f"⚠️  Baseline not found: {filepath}")
            return None
        
        with open(filepath, 'r') as f:
            baseline = json.load(f)
        
        print(f"✅ Baseline loaded: {filepath}")
        return baseline
    
    def list_baselines(self) -> Dict[str, list]:
        """
        List all available baselines
        
        Returns:
            Dictionary with categories and their baseline files
        """
        baselines = {
            "main": [],
            "develop": [],
            "releases": []
        }
        
        # Check main
        main_latest = self.main_dir / "latest.json"
        if main_latest.exists():
            baselines["main"].append(str(main_latest))
        
        # Check develop
        develop_latest = self.develop_dir / "latest.json"
        if develop_latest.exists():
            baselines["develop"].append(str(develop_latest))
        
        # Check releases
        for release_file in sorted(self.releases_dir.glob("*.json")):
            baselines["releases"].append(str(release_file))
        
        return baselines
    
    def load_benchmark_results(self, results_path: str) -> Dict[str, Any]:
        """
        Load benchmark results from Google Benchmark JSON output
        
        Args:
            results_path: Path to benchmark JSON file or directory
            
        Returns:
            Dictionary of benchmark name -> metrics
        """
        results_path = Path(results_path)
        all_benchmarks = {}
        
        if results_path.is_file():
            # Single file
            benchmarks = self._parse_benchmark_json(results_path)
            all_benchmarks.update(benchmarks)
        elif results_path.is_dir():
            # Directory - process all JSON files
            for json_file in results_path.glob("*.json"):
                benchmarks = self._parse_benchmark_json(json_file)
                all_benchmarks.update(benchmarks)
        else:
            print(f"❌ Path not found: {results_path}")
            return {}
        
        print(f"✅ Loaded {len(all_benchmarks)} benchmark results")
        return all_benchmarks
    
    def _parse_benchmark_json(self, filepath: Path) -> Dict[str, Any]:
        """Parse a Google Benchmark JSON file"""
        try:
            with open(filepath, 'r') as f:
                data = json.load(f)
            
            benchmarks = {}
            if 'benchmarks' in data:
                for bench in data['benchmarks']:
                    name = bench.get('name', 'unknown')
                    benchmarks[name] = {
                        'real_time': bench.get('real_time'),
                        'cpu_time': bench.get('cpu_time'),
                        'iterations': bench.get('iterations'),
                        'items_per_second': bench.get('items_per_second'),
                        'bytes_per_second': bench.get('bytes_per_second'),
                    }
            
            return benchmarks
        except Exception as e:
            print(f"⚠️  Error parsing {filepath}: {e}")
            return {}


def main():
    parser = argparse.ArgumentParser(
        description="Manage ThemisDB benchmark baselines"
    )
    subparsers = parser.add_subparsers(dest='command', help='Commands')
    
    # Save baseline command
    save_parser = subparsers.add_parser('save', help='Save a new baseline')
    save_parser.add_argument('--results', required=True,
                           help='Path to benchmark results (file or directory)')
    save_parser.add_argument('--branch', required=True,
                           help='Git branch name')
    save_parser.add_argument('--version', required=True,
                           help='Version string')
    save_parser.add_argument('--commit', required=True,
                           help='Git commit hash')
    save_parser.add_argument('--release', action='store_true',
                           help='Mark as release baseline')
    
    # Load baseline command
    load_parser = subparsers.add_parser('load', help='Load a baseline')
    load_parser.add_argument('--branch', help='Branch name (main, develop)')
    load_parser.add_argument('--version', help='Release version (e.g., 1.4.0)')
    
    # List baselines command
    list_parser = subparsers.add_parser('list', help='List all baselines')
    
    args = parser.parse_args()
    
    manager = BaselineManager()
    
    if args.command == 'save':
        # Load benchmark results
        benchmark_data = manager.load_benchmark_results(args.results)
        if not benchmark_data:
            print("❌ No benchmark data found")
            return 1
        
        # Save baseline
        manager.save_baseline(
            benchmark_data=benchmark_data,
            branch=args.branch,
            version=args.version,
            commit=args.commit,
            is_release=args.release
        )
        
    elif args.command == 'load':
        baseline = manager.load_baseline(
            branch=args.branch,
            version=args.version
        )
        if baseline:
            print(f"\nBaseline info:")
            print(f"  Version: {baseline.get('version')}")
            print(f"  Branch: {baseline.get('branch')}")
            print(f"  Commit: {baseline.get('commit')}")
            print(f"  Timestamp: {baseline.get('timestamp')}")
            print(f"  Benchmarks: {len(baseline.get('benchmarks', {}))}")
        
    elif args.command == 'list':
        baselines = manager.list_baselines()
        print("\n📊 Available Baselines:")
        print("\nMain branch:")
        for b in baselines['main']:
            print(f"  • {b}")
        print("\nDevelop branch:")
        for b in baselines['develop']:
            print(f"  • {b}")
        print("\nReleases:")
        for b in baselines['releases']:
            print(f"  • {b}")
    
    else:
        parser.print_help()
        return 1
    
    return 0


if __name__ == "__main__":
    exit(main())
