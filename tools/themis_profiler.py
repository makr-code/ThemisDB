"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_profiler.py                                 ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:49:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     450                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Performance Profiling Tool

Command-line tool for profiling and analyzing ThemisDB performance.
"""

import argparse
import json
import sys
import time
from typing import Dict, List, Optional
from datetime import datetime

# Check for required dependencies
try:
    import requests
except ImportError:
    print("Error: 'requests' library is required but not installed.", file=sys.stderr)
    print("Install it with: pip3 install requests", file=sys.stderr)
    sys.exit(1)

class ThemisDBProfiler:
    """Client for ThemisDB profiling API"""
    
    def __init__(self, host: str = "localhost", port: int = 8080):
        self.base_url = f"http://{host}:{port}"
        
    def enable_profiling(self) -> bool:
        """Enable profiling"""
        try:
            response = requests.post(f"{self.base_url}/api/profiling/enable")
            return response.status_code == 200
        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)
            return False
    
    def disable_profiling(self) -> bool:
        """Disable profiling"""
        try:
            response = requests.post(f"{self.base_url}/api/profiling/disable")
            return response.status_code == 200
        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)
            return False
    
    def get_query_profiles(self, limit: Optional[int] = None) -> List[Dict]:
        """Get query profiles"""
        try:
            params = {}
            if limit:
                params['limit'] = limit
            response = requests.get(f"{self.base_url}/api/profiling/queries", params=params)
            if response.status_code == 200:
                return response.json()
            return []
        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)
            return []
    
    def get_slow_queries(self, threshold_ms: int = 1000) -> List[Dict]:
        """Get slow queries"""
        try:
            params = {'threshold_ms': threshold_ms}
            response = requests.get(f"{self.base_url}/api/profiling/slow-queries", params=params)
            if response.status_code == 200:
                return response.json()
            return []
        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)
            return []
    
    def get_storage_stats(self) -> Dict:
        """Get storage statistics"""
        try:
            response = requests.get(f"{self.base_url}/api/profiling/storage")
            if response.status_code == 200:
                return response.json()
            return {}
        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)
            return {}
    
    def analyze_performance(self) -> Dict:
        """Run performance analysis"""
        try:
            response = requests.post(f"{self.base_url}/api/profiling/analyze")
            if response.status_code == 200:
                return response.json()
            return {}
        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)
            return {}
    
    def export_profiles(self, filename: str) -> bool:
        """Export profiles to file"""
        try:
            response = requests.get(f"{self.base_url}/api/profiling/export")
            if response.status_code == 200:
                with open(filename, 'w') as f:
                    json.dump(response.json(), f, indent=2)
                return True
            return False
        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)
            return False

def format_duration(duration_us: float) -> str:
    """Format duration in microseconds to human-readable string"""
    if duration_us < 1000:
        return f"{duration_us:.2f} µs"
    elif duration_us < 1000000:
        return f"{duration_us / 1000:.2f} ms"
    else:
        return f"{duration_us / 1000000:.2f} s"

def cmd_enable(args):
    """Enable profiling"""
    profiler = ThemisDBProfiler(args.host, args.port)
    if profiler.enable_profiling():
        print("✓ Profiling enabled")
        return 0
    else:
        print("✗ Failed to enable profiling", file=sys.stderr)
        return 1

def cmd_disable(args):
    """Disable profiling"""
    profiler = ThemisDBProfiler(args.host, args.port)
    if profiler.disable_profiling():
        print("✓ Profiling disabled")
        return 0
    else:
        print("✗ Failed to disable profiling", file=sys.stderr)
        return 1

def cmd_queries(args):
    """Show query profiles"""
    profiler = ThemisDBProfiler(args.host, args.port)
    queries = profiler.get_query_profiles(args.limit)
    
    if not queries:
        print("No query profiles found")
        return 0
    
    print(f"\n{'='*80}")
    print(f"Query Profiles ({len(queries)} queries)")
    print(f"{'='*80}\n")
    
    for i, query in enumerate(queries, 1):
        duration = query.get('total_duration_us', 0)
        rows = query.get('results', {}).get('rows', 0)
        used_index = query.get('optimization', {}).get('used_index', False)
        
        print(f"{i}. Query ID: {query.get('query_id', 'N/A')}")
        print(f"   Duration: {format_duration(duration)}")
        print(f"   Rows: {rows}")
        print(f"   Index Used: {'Yes' if used_index else 'No'}")
        
        if args.verbose:
            print(f"   Query: {query.get('query_text', 'N/A')}")
            
            warnings = query.get('optimization', {}).get('warnings', [])
            if warnings:
                print("   Warnings:")
                for warning in warnings:
                    print(f"     - {warning}")
            
            hints = query.get('optimization', {}).get('hints', [])
            if hints:
                print("   Hints:")
                for hint in hints:
                    print(f"     - {hint}")
        
        print()
    
    return 0

def cmd_slow_queries(args):
    """Show slow queries"""
    profiler = ThemisDBProfiler(args.host, args.port)
    queries = profiler.get_slow_queries(args.threshold)
    
    if not queries:
        print(f"No slow queries found (threshold: {args.threshold} ms)")
        return 0
    
    print(f"\n{'='*80}")
    print(f"Slow Queries (>{args.threshold} ms)")
    print(f"{'='*80}\n")
    
    for i, query in enumerate(queries, 1):
        duration = query.get('total_duration_us', 0)
        rows = query.get('results', {}).get('rows', 0)
        
        print(f"{i}. Query ID: {query.get('query_id', 'N/A')}")
        print(f"   Duration: {format_duration(duration)}")
        print(f"   Rows: {rows}")
        print(f"   Query: {query.get('query_text', 'N/A')[:100]}...")
        print()
    
    return 0

def cmd_storage(args):
    """Show storage statistics"""
    profiler = ThemisDBProfiler(args.host, args.port)
    stats = profiler.get_storage_stats()
    
    if not stats:
        print("No storage statistics available")
        return 0
    
    print(f"\n{'='*80}")
    print("Storage Statistics")
    print(f"{'='*80}\n")
    
    # Operation summary
    if 'operation_summary' in stats:
        print("Operation Summary:")
        for op_type, op_stats in stats['operation_summary'].items():
            count = op_stats.get('count', 0)
            avg_duration = op_stats.get('avg_duration_us', 0)
            print(f"  {op_type}:")
            print(f"    Count: {count}")
            print(f"    Avg Duration: {format_duration(avg_duration)}")
        print()
    
    # Cache metrics
    if 'cache_metrics' in stats:
        print("Cache Metrics:")
        cache = stats['cache_metrics']
        if 'block_cache' in cache:
            block = cache['block_cache']
            print(f"  Block Cache:")
            print(f"    Hit Rate: {block.get('hit_rate_pct', 0):.2f}%")
            print(f"    Size: {block.get('size_bytes', 0) / (1024**2):.2f} MB")
            print(f"    Capacity: {block.get('capacity_bytes', 0) / (1024**2):.2f} MB")
        print()
    
    # Amplification metrics
    if 'amplification_metrics' in stats:
        print("Amplification Metrics:")
        amp = stats['amplification_metrics']
        print(f"  Write Amplification: {amp.get('write_amplification', 0):.2f}")
        print(f"  Read Amplification: {amp.get('read_amplification', 0):.2f}")
        print(f"  Space Amplification: {amp.get('space_amplification', 0):.2f}")
        print()
    
    return 0

def cmd_analyze(args):
    """Run performance analysis"""
    profiler = ThemisDBProfiler(args.host, args.port)
    
    print("Running performance analysis...")
    analysis = profiler.analyze_performance()
    
    if not analysis:
        print("✗ Analysis failed", file=sys.stderr)
        return 1
    
    print(f"\n{'='*80}")
    print("Performance Analysis Report")
    print(f"{'='*80}\n")
    
    summary = analysis.get('summary_metrics', {})
    print("Summary:")
    print(f"  Total Issues: {summary.get('total_issues', 0)}")
    print(f"  Critical: {summary.get('critical_issues', 0)}")
    print(f"  Warning: {summary.get('warning_issues', 0)}")
    print(f"  Info: {summary.get('info_issues', 0)}")
    print()
    
    issues = analysis.get('issues', [])
    if issues:
        print("Issues:")
        for i, issue in enumerate(issues, 1):
            severity = issue.get('severity', 'UNKNOWN')
            title = issue.get('title', 'N/A')
            description = issue.get('description', 'N/A')
            
            print(f"\n{i}. [{severity}] {title}")
            print(f"   {description}")
            
            recommendations = issue.get('recommendations', [])
            if recommendations:
                print("   Recommendations:")
                for rec in recommendations:
                    print(f"     - {rec}")
    
    recommendations = analysis.get('recommendations', [])
    if recommendations:
        print(f"\n{'='*80}")
        print("High-Level Recommendations:")
        for rec in recommendations:
            print(f"  • {rec}")
    
    if args.output:
        with open(args.output, 'w') as f:
            json.dump(analysis, f, indent=2)
        print(f"\n✓ Analysis exported to {args.output}")
    
    print()
    return 0

def cmd_export(args):
    """Export profiles"""
    profiler = ThemisDBProfiler(args.host, args.port)
    
    print(f"Exporting profiles to {args.output}...")
    if profiler.export_profiles(args.output):
        print(f"✓ Profiles exported to {args.output}")
        return 0
    else:
        print(f"✗ Failed to export profiles", file=sys.stderr)
        return 1

def cmd_monitor(args):
    """Monitor performance in real-time"""
    profiler = ThemisDBProfiler(args.host, args.port)
    
    print("Starting performance monitor (Ctrl+C to stop)...\n")
    
    try:
        while True:
            # Get current statistics
            stats = profiler.get_storage_stats()
            queries = profiler.get_query_profiles(limit=5)
            
            # Clear screen
            print("\033[2J\033[H", end='')
            
            print(f"{'='*80}")
            print(f"ThemisDB Performance Monitor - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
            print(f"{'='*80}\n")
            
            # Recent queries
            if queries:
                print("Recent Queries:")
                for query in queries[:5]:
                    duration = query.get('total_duration_us', 0)
                    print(f"  {query.get('query_id', 'N/A')}: {format_duration(duration)}")
                print()
            
            # Cache performance
            if stats and 'cache_metrics' in stats:
                cache = stats['cache_metrics']
                if 'block_cache' in cache:
                    block = cache['block_cache']
                    print(f"Cache Hit Rate: {block.get('hit_rate_pct', 0):.2f}%")
                    print()
            
            time.sleep(args.interval)
            
    except KeyboardInterrupt:
        print("\n\nMonitoring stopped.")
        return 0

def main():
    parser = argparse.ArgumentParser(
        description='ThemisDB Performance Profiling Tool',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    parser.add_argument('--host', default='localhost', help='ThemisDB host')
    parser.add_argument('--port', type=int, default=8080, help='ThemisDB port')
    
    subparsers = parser.add_subparsers(dest='command', help='Commands')
    
    # Enable command
    subparsers.add_parser('enable', help='Enable profiling')
    
    # Disable command
    subparsers.add_parser('disable', help='Disable profiling')
    
    # Queries command
    queries_parser = subparsers.add_parser('queries', help='Show query profiles')
    queries_parser.add_argument('-l', '--limit', type=int, help='Limit number of results')
    queries_parser.add_argument('-v', '--verbose', action='store_true', help='Verbose output')
    
    # Slow queries command
    slow_parser = subparsers.add_parser('slow-queries', help='Show slow queries')
    slow_parser.add_argument('-t', '--threshold', type=int, default=1000,
                           help='Threshold in milliseconds (default: 1000)')
    
    # Storage command
    subparsers.add_parser('storage', help='Show storage statistics')
    
    # Analyze command
    analyze_parser = subparsers.add_parser('analyze', help='Run performance analysis')
    analyze_parser.add_argument('-o', '--output', help='Output file for analysis results')
    
    # Export command
    export_parser = subparsers.add_parser('export', help='Export profiles')
    export_parser.add_argument('output', help='Output file')
    
    # Monitor command
    monitor_parser = subparsers.add_parser('monitor', help='Monitor performance in real-time')
    monitor_parser.add_argument('-i', '--interval', type=int, default=5,
                              help='Update interval in seconds (default: 5)')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    # Dispatch to command handler
    commands = {
        'enable': cmd_enable,
        'disable': cmd_disable,
        'queries': cmd_queries,
        'slow-queries': cmd_slow_queries,
        'storage': cmd_storage,
        'analyze': cmd_analyze,
        'export': cmd_export,
        'monitor': cmd_monitor,
    }
    
    handler = commands.get(args.command)
    if handler:
        return handler(args)
    else:
        print(f"Unknown command: {args.command}", file=sys.stderr)
        return 1

if __name__ == '__main__':
    sys.exit(main())
