#!/usr/bin/env python3
"""
ThemisDB Gap Scanner V3 - Unified CLI Interface

Single entry point for the complete gap scanning pipeline.
Orchestrates all 50+ scanners with impact classification.

Usage:
    python -m tools.gs3 scan <directory> [options]
    python -m tools.gs3 report <scan-file> [options]
    python -m tools.gs3 list-scanners [--step N]
    python -m tools.gs3 config [--show|--edit]
"""

import sys
import argparse
import json
from pathlib import Path
from typing import Optional
import logging

# Add tools/ to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from tools.gs3_base_scanner import BaseGapScanner, Gap
from tools.gs3_orchestrator import main as run_orchestrator
from tools.scanners.gs3_impact_classifier import ImpactClassifier


# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


class GS3CLI:
    """Unified CLI interface for Gap Scanner V3"""
    
    def __init__(self):
        pass
    
    def scan(self, directories: list, scan_mode: str = 'fast', output: Optional[str] = None,
             md_report: Optional[str] = None, verbose: bool = False) -> dict:
        """Execute full scanning pipeline on given directories"""
        logger.info(f"Starting scan on {len(directories)} directories: {directories}")
        logger.info(f"Scan mode: {scan_mode}")
        
        # Call orchestrator
        sys.argv = ['gs3_orchestrator.py'] + directories + [
            '--scan-mode', scan_mode,
            '--output', output or 'ai_working/scan_results.json',
            '--md-report', md_report or 'ai_working/scan_results.md'
        ]
        
        result = run_orchestrator()
        logger.info(f"Scan complete. Results saved to {output}")
        return result
    
    def report(self, scan_file: str, format: str = 'md', output: Optional[str] = None) -> str:
        """Generate report from scan results"""
        logger.info(f"Generating {format} report from {scan_file}")
        
        with open(scan_file) as f:
            data = json.load(f)
        
        gaps = [Gap(**g) for g in data.get('gaps', [])]
        
        if format == 'json':
            report = self._format_json_report(gaps)
        elif format == 'md':
            report = self._format_md_report(gaps)
        else:
            raise ValueError(f"Unknown format: {format}")
        
        if output:
            Path(output).write_text(report)
            logger.info(f"Report saved to {output}")
        
        return report
    
    def list_scanners(self, step: Optional[int] = None) -> None:
        """List all registered scanners"""
        
        print("\n" + "=" * 100)
        print("GAP SCANNER V3 - REGISTERED SCANNERS")
        print("=" * 100 + "\n")
        
        # Load scanner modules and list
        import importlib
        from pathlib import Path
        
        scanners_dir = Path(__file__).parent / 'scanners'
        scanner_files = sorted(scanners_dir.glob('gs3_step*.py'))
        
        current_step = None
        count = 0
        
        for scanner_file in scanner_files:
            if scanner_file.name.startswith('gs3_step00'):
                continue  # Skip meta-orchestrator
                
            parts = scanner_file.stem.split('_')
            if len(parts) >= 2:
                step_num = int(parts[1][4:]) if parts[1].startswith('step') else 0
                
                if step is not None and step_num != step:
                    continue
                
                if step_num != current_step:
                    current_step = step_num
                    print(f"\n{'Step ' + str(step_num):=^100}\n")
                
                name = scanner_file.stem.replace('gs3_', '').replace('_', ' ').title()
                print(f"  • {name:60s}")
                count += 1
        
        print(f"\n{count} scanners found\n" + "=" * 100 + "\n")
    
    def config(self, show: bool = False, edit: bool = False) -> None:
        """Manage GS3 configuration"""
        config_file = Path('tools/gs3_config.json')
        
        if show:
            if config_file.exists():
                config = json.loads(config_file.read_text())
                print(json.dumps(config, indent=2))
            else:
                print("No config file found")
        
        if edit:
            import subprocess
            subprocess.run(['code', str(config_file)])
    
    @staticmethod
    def _format_json_report(gaps: list) -> str:
        """Format gaps as JSON report"""
        return json.dumps({
            'total': len(gaps),
            'by_severity': {
                'CRITICAL': sum(1 for g in gaps if g.severity == 'CRITICAL'),
                'HIGH': sum(1 for g in gaps if g.severity == 'HIGH'),
                'MEDIUM': sum(1 for g in gaps if g.severity == 'MEDIUM'),
                'LOW': sum(1 for g in gaps if g.severity == 'LOW'),
            },
            'by_impact': {
                'CRITICAL': sum(1 for g in gaps if g.impact_level == 'CRITICAL'),
                'HIGH': sum(1 for g in gaps if g.impact_level == 'HIGH'),
            },
            'gaps': [vars(g) for g in gaps]
        }, indent=2)
    
    @staticmethod
    def _format_md_report(gaps: list) -> str:
        """Format gaps as Markdown report"""
        lines = [
            "# Gap Scanner V3 Report",
            "",
            f"**Total Findings**: {len(gaps)}",
            "",
            "## Summary by Severity",
            "",
            "| Severity | Count |",
            "|----------|-------|",
        ]
        
        for severity in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']:
            count = sum(1 for g in gaps if g.severity == severity)
            lines.append(f"| {severity} | {count} |")
        
        lines.extend([
            "",
            "## Summary by Impact",
            "",
            "| Impact | Count |",
            "|--------|-------|",
        ])
        
        for impact in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']:
            count = sum(1 for g in gaps if g.impact_level == impact)
            lines.append(f"| {impact} | {count} |")
        
        return "\n".join(lines)


def main():
    """Main CLI entry point"""
    parser = argparse.ArgumentParser(
        prog='gs3',
        description='ThemisDB Gap Scanner V3 - Unified gap detection and remediation',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    python -m tools.gs3 scan src include tests
    python -m tools.gs3 scan src --scan-mode thorough --output results.json
    python -m tools.gs3 report results.json --format md --output report.md
    python -m tools.gs3 list-scanners --step 1
    python -m tools.gs3 config --show
        """
    )
    
    subparsers = parser.add_subparsers(dest='command', help='Command to execute')
    
    # SCAN command
    scan_parser = subparsers.add_parser('scan', help='Run full gap scanning pipeline')
    scan_parser.add_argument('directories', nargs='+', help='Directories to scan')
    scan_parser.add_argument('--scan-mode', choices=['fast', 'thorough'], default='fast',
                            help='Scan thoroughness level')
    scan_parser.add_argument('--output', default='ai_working/scan_results.json',
                            help='Output JSON file')
    scan_parser.add_argument('--md-report', help='Generate markdown report at path')
    scan_parser.add_argument('-v', '--verbose', action='store_true', help='Verbose output')
    
    # REPORT command
    report_parser = subparsers.add_parser('report', help='Generate report from scan results')
    report_parser.add_argument('scan_file', help='Scan JSON file')
    report_parser.add_argument('--format', choices=['json', 'md'], default='md',
                              help='Report format')
    report_parser.add_argument('--output', help='Output file (default: stdout)')
    
    # LIST-SCANNERS command
    list_parser = subparsers.add_parser('list-scanners', help='List all registered scanners')
    list_parser.add_argument('--step', type=int, help='Filter by step number')
    
    # CONFIG command
    config_parser = subparsers.add_parser('config', help='Manage GS3 configuration')
    config_parser.add_argument('--show', action='store_true', help='Show current config')
    config_parser.add_argument('--edit', action='store_true', help='Edit config in VS Code')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    cli = GS3CLI()
    
    try:
        if args.command == 'scan':
            cli.scan(
                directories=args.directories,
                scan_mode=args.scan_mode,
                output=args.output,
                md_report=args.md_report,
                verbose=args.verbose
            )
        elif args.command == 'report':
            report = cli.report(
                scan_file=args.scan_file,
                format=args.format,
                output=args.output
            )
            if not args.output:
                print(report)
        elif args.command == 'list-scanners':
            cli.list_scanners(step=args.step)
        elif args.command == 'config':
            cli.config(show=args.show, edit=args.edit)
        
        return 0
    except Exception as e:
        logger.error(f"Error: {e}", exc_info=True)
        return 1


if __name__ == '__main__':
    sys.exit(main())
