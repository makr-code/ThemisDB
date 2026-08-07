#!/usr/bin/env python3
"""
FP Detection Integration — Connect FP Engine to Gap Scanner Pipeline
===================================================================

Integrates the FPDetectionEngine with existing gap scanner output.

Usage:
    python tools/fp_detection_integration.py ai_working/gap_scan_v3_aggregate.json
"""

import json
import sys
from pathlib import Path
from typing import Dict, List, Any
import logging

# Import the FP detection engine
sys.path.insert(0, str(Path(__file__).parent))
from fp_detection_engine import FPDetectionEngine, GapAnalysis, GapClassification

logging.basicConfig(level=logging.INFO, format='[%(levelname)-8s] %(message)s')
logger = logging.getLogger(__name__)


class FPDetectionIntegration:
    """Integrates FP detection into gap scanner pipeline"""
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.engine = FPDetectionEngine(repo_root)
    
    def process_gap_scan_results(self, input_file: str, output_file: str = None) -> Dict[str, Any]:
        """
        Process gap scanner results through FP detection.
        
        Args:
            input_file: Path to gap_scan_v3_aggregate.json or similar
            output_file: Path for output (auto-generated if None)
        
        Returns:
            Summary statistics
        """
        if output_file is None:
            input_path = Path(input_file)
            output_file = str(input_path.parent / f"{input_path.stem}_fp_analyzed.json")
        
        logger.info(f"Loading gaps from: {input_file}")
        
        # Load input
        try:
            with open(input_file) as f:
                data = json.load(f)
        except FileNotFoundError:
            logger.error(f"File not found: {input_file}")
            return {'error': 'File not found', 'input_file': input_file}
        
        # Extract gaps depending on structure
        if isinstance(data, dict):
            if 'findings' in data:
                gaps = data['findings']
            elif 'gaps' in data:
                gaps = data['gaps']
            else:
                # Assume it's a list of gaps
                gaps = list(data.values()) if isinstance(data, dict) else data
        else:
            gaps = data if isinstance(data, list) else []
        
        logger.info(f"Processing {len(gaps)} gaps...")
        
        # Analyze through FP detection engine
        analyses, stats = self.engine.filter_gaps(gaps)
        
        # Export results
        self.engine.export_analysis(analyses, output_file)
        
        # Generate summary report
        summary = self._generate_summary(data, analyses, stats, output_file)
        
        logger.info(f"FP detection complete. Summary:")
        self._print_summary(summary)
        
        return summary
    
    def _generate_summary(self, original_data: Dict, analyses: List[GapAnalysis], 
                         stats: Dict[str, Any], output_file: str) -> Dict[str, Any]:
        """Generate detailed summary report"""
        
        # Count by classification
        classifications = {}
        for analysis in analyses:
            key = analysis.classification.value
            classifications[key] = classifications.get(key, 0) + 1
        
        # Count by verified severity
        verified_severities = {}
        for analysis in analyses:
            key = analysis.verified_severity
            verified_severities[key] = verified_severities.get(key, 0) + 1
        
        # Find biggest FP sources
        fp_files = {}
        for analysis in analyses:
            if analysis.is_false_positive:
                file_path = analysis.gap.get('file', 'UNKNOWN')
                if file_path not in fp_files:
                    fp_files[file_path] = {'count': 0, 'reasons': []}
                fp_files[file_path]['count'] += 1
                if analysis.reason:
                    fp_files[file_path]['reasons'].append(analysis.reason[:50])
        
        top_fp_files = sorted(fp_files.items(), key=lambda x: x[1]['count'], reverse=True)[:10]
        
        # Calculate severity reduction
        original_severities = {}
        for analysis in analyses:
            key = analysis.original_severity
            original_severities[key] = original_severities.get(key, 0) + 1
        
        summary = {
            'status': 'complete',
            'input_file': original_data.get('timestamp', 'unknown'),
            'output_file': output_file,
            'total_gaps_analyzed': stats['total_gaps'],
            'false_positives_identified': stats['gaps_filtered'],
            'fp_reduction_percentage': f"{stats['gaps_filtered']/max(1, stats['total_gaps'])*100:.1f}%",
            'remaining_gaps': stats['total_gaps'] - stats['gaps_filtered'],
            'classifications': classifications,
            'original_severity_distribution': original_severities,
            'verified_severity_distribution': verified_severities,
            'detector_effectiveness': stats['detector_hits'],
            'top_fp_sources': [
                {
                    'file': file_path,
                    'fp_count': info['count'],
                    'reasons': list(set(info['reasons']))[:3]
                }
                for file_path, info in top_fp_files
            ],
            'recommendations': self._generate_recommendations(classifications, stats)
        }
        
        return summary
    
    def _generate_recommendations(self, classifications: Dict[str, int], stats: Dict) -> List[str]:
        """Generate actionable recommendations"""
        recommendations = []
        
        fp_count = classifications.get('FALSE_POSITIVE', 0)
        total = sum(classifications.values())
        fp_rate = fp_count / max(1, total)
        
        if fp_rate > 0.3:
            recommendations.append(
                f"HIGH false positive rate ({fp_rate*100:.0f}%). "
                "Review detector tuning and confidence thresholds."
            )
        
        if stats['detector_hits'].get('TestCode', 0) > total * 0.1:
            recommendations.append(
                "Many gaps found in test code. Consider stricter file filtering."
            )
        
        if stats['detector_hits'].get('Placeholder', 0) > total * 0.2:
            recommendations.append(
                "Many intentional placeholders found. Review Phase N+1 planning."
            )
        
        audit_hits = stats['detector_hits'].get('AuditLogging', 0)
        if audit_hits > 100:
            recommendations.append(
                f"Audit logging detector identified {audit_hits} FPs. "
                "Consider security operation context filtering."
            )
        
        if classifications.get('REAL_GAP', 0) < total * 0.3:
            recommendations.append(
                "Low true positive rate. Review scanner sensitivity or "
                "update approved patterns whitelist."
            )
        
        return recommendations
    
    def _print_summary(self, summary: Dict[str, Any]) -> None:
        """Pretty-print summary"""
        print("\n" + "="*80)
        print("FALSE POSITIVE DETECTION SUMMARY")
        print("="*80)
        
        print(f"\nTotal gaps analyzed:     {summary['total_gaps_analyzed']:>10}")
        print(f"False positives found:   {summary['false_positives_identified']:>10}")
        print(f"Remaining real gaps:     {summary['remaining_gaps']:>10}")
        print(f"FP reduction rate:       {summary['fp_reduction_percentage']:>10}")
        
        print("\nClassifications:")
        for cls, count in sorted(summary['classifications'].items()):
            pct = count / summary['total_gaps_analyzed'] * 100 if summary['total_gaps_analyzed'] > 0 else 0
            print(f"  {cls:20s}: {count:6d} ({pct:5.1f}%)")
        
        print("\nSeverity Changes:")
        print("  Original  →  Verified")
        for severity in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW', 'INFO']:
            orig_count = summary['original_severity_distribution'].get(severity, 0)
            ver_count = summary['verified_severity_distribution'].get(severity, 0)
            if orig_count > 0:
                print(f"  {severity:8s}  →  {ver_count:6d} (from {orig_count:6d})")
        
        print("\nTop FP Sources:")
        for i, fp_file in enumerate(summary['top_fp_sources'][:5], 1):
            print(f"  {i}. {fp_file['file']}")
            print(f"     FPs: {fp_file['fp_count']}")
            for reason in fp_file['reasons']:
                print(f"     • {reason}")
        
        if summary['recommendations']:
            print("\nRecommendations:")
            for i, rec in enumerate(summary['recommendations'], 1):
                print(f"  {i}. {rec}")
        
        print(f"\nDetailed output: {summary['output_file']}")
        print("="*80 + "\n")


def main():
    """Command-line interface"""
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <gap_scan_results.json> [output_file.json]")
        print(f"\nExample:")
        print(f"  {sys.argv[0]} ai_working/gap_scan_v3_aggregate.json")
        print(f"  {sys.argv[0]} ai_working/gap_scan_v3_aggregate.json ai_working/gap_fp_analyzed.json")
        return 1
    
    repo_root = '.'  # Use current directory as repo root
    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else None
    
    try:
        integration = FPDetectionIntegration(repo_root)
        summary = integration.process_gap_scan_results(input_file, output_file)
        
        if 'error' in summary:
            return 1
        
        return 0
    except Exception as e:
        logger.error(f"Error: {e}", exc_info=True)
        return 1


if __name__ == '__main__':
    exit(main())
