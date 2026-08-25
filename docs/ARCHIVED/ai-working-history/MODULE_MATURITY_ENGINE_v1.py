#!/usr/bin/env python3
"""
MODULE_MATURITY_ENGINE_v1.py
ThemisDB Maturity Assessment Engine

Scans all module ROADMAP.md files to extract:
- Phase completion status (Phase 1-6)
- Gate pass/fail indicators
- Known issues and blockers
- Module maturity scoring

Output: JSON + markdown report for dashboard integration
"""

import os
import re
import json
from pathlib import Path
from collections import defaultdict
from datetime import datetime
from typing import Dict, List, Tuple

class MaturityEngine:
    def __init__(self, repo_root: str):
        self.repo_root = repo_root
        self.modules = {}
        self.timestamp = datetime.now().isoformat()
        
    def scan_module_roadmaps(self) -> Dict:
        """Scan all src/*/ROADMAP.md files and extract phase completion data."""
        src_dir = os.path.join(self.repo_root, 'src')
        
        for root, dirs, files in os.walk(src_dir):
            if 'ROADMAP.md' in files:
                module_name = os.path.basename(root)
                roadmap_path = os.path.join(root, 'ROADMAP.md')
                self.modules[module_name] = self._parse_roadmap(roadmap_path)
        
        return self.modules
    
    def _parse_roadmap(self, filepath: str) -> Dict:
        """Parse individual ROADMAP.md for phase markers and gate status."""
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                content = f.read()
        except Exception as e:
            return {'error': str(e), 'phases': {}, 'gates': []}
        
        result = {
            'path': filepath,
            'phases': self._extract_phase_completion(content),
            'gates': self._extract_gates(content),
            'known_issues': self._extract_known_issues(content),
            'blockers': self._extract_blockers(content),
        }
        
        return result
    
    def _extract_phase_completion(self, content: str) -> Dict[int, str]:
        """Extract Phase 1-6 completion status (complete, in-progress, or blocked)."""
        phases = {}
        
        for phase_num in range(1, 7):
            # Look for phase marker patterns: [x], [~], [ ]
            pattern = rf'(Phase {phase_num}[^\n]*?)\s*(?:\[([x~\s])\])?'
            
            complete_count = len(re.findall(rf'\[x\].*?Phase {phase_num}', content, re.IGNORECASE))
            in_progress_count = len(re.findall(rf'\[~\].*?Phase {phase_num}', content, re.IGNORECASE))
            blocked_count = len(re.findall(rf'\[\?\].*?Phase {phase_num}', content, re.IGNORECASE))
            open_count = len(re.findall(rf'\[\s\].*?Phase {phase_num}', content, re.IGNORECASE))
            
            # Calculate completion percentage
            total = complete_count + in_progress_count + blocked_count + open_count
            if total > 0:
                pct = (complete_count * 100) // total
            else:
                pct = 0
            
            status = 'complete' if complete_count > 0 else 'in_progress' if in_progress_count > 0 else 'open'
            
            phases[phase_num] = {
                'percentage': pct,
                'status': status,
                'complete': complete_count,
                'in_progress': in_progress_count,
                'blocked': blocked_count,
                'open': open_count,
            }
        
        return phases
    
    def _extract_gates(self, content: str) -> List[Dict]:
        """Extract gate markers (GATE-*, PASS, FAIL)."""
        gates = []
        
        # Find all GATE markers
        for match in re.finditer(r'(GATE-[A-Z0-9-]+)\s*(?:\[([xI~?])\])?.*?(PASS|FAIL)?', content):
            gate_id, status_char, result = match.groups()
            
            # Map checkbox to status
            status = 'complete' if status_char == 'x' else 'in_progress' if status_char == '~' else 'open'
            result = result or 'unknown'
            
            gates.append({
                'id': gate_id,
                'status': status,
                'result': result,
            })
        
        return gates
    
    def _extract_known_issues(self, content: str) -> List[str]:
        """Extract known issues section."""
        issues = []
        
        # Look for "Known Issues & Limitations" section
        match = re.search(r'## Known Issues[^\n]*\n(.*?)(?=\n##|\Z)', content, re.DOTALL)
        if match:
            issue_text = match.group(1)
            # Extract bullet points
            for bullet in re.findall(r'^[-*]\s*(.+?)$', issue_text, re.MULTILINE):
                issues.append(bullet.strip())
        
        return issues
    
    def _extract_blockers(self, content: str) -> List[str]:
        """Extract blockers and blocking issues."""
        blockers = []
        
        # Look for blocked/blocker patterns
        patterns = [
            r'(?:blocked|blocker)[^.]*?(?:Target|due)[^\n]*',
            r'\[\?\].*?(?:blocked|unclear)[^\n]*',
        ]
        
        for pattern in patterns:
            for match in re.finditer(pattern, content, re.IGNORECASE):
                blockers.append(match.group(0).strip())
        
        return blockers[:5]  # Limit to top 5
    
    def calculate_maturity_score(self) -> Dict:
        """Calculate overall maturity scores per module."""
        scores = {}
        
        for module_name, data in self.modules.items():
            if 'error' in data:
                scores[module_name] = {'error': data['error']}
                continue
            
            # Calculate weighted phase completion
            phase_scores = []
            for phase_num in range(1, 7):
                if phase_num in data['phases']:
                    phase_scores.append(data['phases'][phase_num]['percentage'])
            
            overall_phase_pct = sum(phase_scores) / len(phase_scores) if phase_scores else 0
            
            # Gate pass rate
            gates = data['gates']
            passed_gates = sum(1 for g in gates if g['result'] == 'PASS')
            gate_pass_rate = (passed_gates * 100) // len(gates) if gates else 0
            
            # Blocker penalty
            blocker_penalty = min(len(data['blockers']) * 5, 20)
            
            # Final score (0-100)
            final_score = int((overall_phase_pct * 0.6 + gate_pass_rate * 0.3 - blocker_penalty))
            final_score = max(0, min(100, final_score))
            
            scores[module_name] = {
                'phase_completion_pct': int(overall_phase_pct),
                'gate_pass_rate': gate_pass_rate,
                'blockers': len(data['blockers']),
                'known_issues': len(data['known_issues']),
                'maturity_score': final_score,
                'recommendation': self._score_to_recommendation(final_score),
            }
        
        return scores
    
    def _score_to_recommendation(self, score: int) -> str:
        """Convert maturity score to recommendation."""
        if score >= 90:
            return 'PRODUCTION_READY'
        elif score >= 75:
            return 'NEARLY_READY'
        elif score >= 50:
            return 'IN_PROGRESS'
        elif score >= 25:
            return 'EARLY_STAGE'
        else:
            return 'PLANNING'
    
    def generate_markdown_report(self, scores: Dict) -> str:
        """Generate markdown report of module maturity."""
        report = []
        report.append('# ThemisDB Module Maturity Assessment')
        report.append(f'\n**Generated:** {self.timestamp}')
        report.append(f'**Total Modules Scanned:** {len(self.modules)}')
        report.append('\n## Module Maturity Table\n')
        
        # Create sorted table by score
        sorted_scores = sorted(scores.items(), key=lambda x: x[1].get('maturity_score', 0), reverse=True)
        
        report.append('| Module | Phase % | Gates % | Issues | Blockers | Score | Status |')
        report.append('|--------|---------|---------|--------|----------|-------|--------|')
        
        for module_name, score_data in sorted_scores:
            if 'error' in score_data:
                continue
            
            phase_pct = score_data.get('phase_completion_pct', 0)
            gate_pct = score_data.get('gate_pass_rate', 0)
            issues = score_data.get('known_issues', 0)
            blockers = score_data.get('blockers', 0)
            score = score_data.get('maturity_score', 0)
            recommendation = score_data.get('recommendation', 'UNKNOWN')
            
            report.append(
                f'| {module_name} | {phase_pct}% | {gate_pct}% | {issues} | {blockers} | {score}/100 | {recommendation} |'
            )
        
        # Summary by status
        report.append('\n## Maturity Distribution\n')
        status_counts = defaultdict(int)
        for module_name, score_data in scores.items():
            if 'error' not in score_data:
                status = score_data.get('recommendation', 'UNKNOWN')
                status_counts[status] += 1
        
        for status in ['PRODUCTION_READY', 'NEARLY_READY', 'IN_PROGRESS', 'EARLY_STAGE', 'PLANNING']:
            count = status_counts.get(status, 0)
            report.append(f'- **{status}**: {count} modules')
        
        report.append('\n## Top Risks\n')
        # Find modules with most blockers
        risky_modules = sorted(scores.items(), key=lambda x: x[1].get('blockers', 0), reverse=True)[:5]
        for module_name, score_data in risky_modules:
            if score_data.get('blockers', 0) > 0:
                report.append(f'- **{module_name}**: {score_data.get("blockers")} blockers')
        
        return '\n'.join(report)
    
    def export_json(self, filepath: str = None):
        """Export maturity data as JSON."""
        scores = self.calculate_maturity_score()
        
        export_data = {
            'timestamp': self.timestamp,
            'scan_type': 'module_maturity_v1',
            'total_modules': len(self.modules),
            'modules': {}
        }
        
        for module_name, data in self.modules.items():
            export_data['modules'][module_name] = {
                'roadmap_path': data.get('path', ''),
                'phases': data.get('phases', {}),
                'gates': data.get('gates', []),
                'known_issues': data.get('known_issues', []),
                'blockers': data.get('blockers', []),
                'scores': scores.get(module_name, {}),
            }
        
        export_data['summary'] = {
            'scores': scores,
        }
        
        if filepath:
            with open(filepath, 'w') as f:
                json.dump(export_data, f, indent=2)
        
        return export_data

def main():
    repo_root = '/home/runner/work/ThemisDB/ThemisDB'
    
    engine = MaturityEngine(repo_root)
    
    print('[*] Scanning module ROADMAP.md files...')
    modules = engine.scan_module_roadmaps()
    print(f'[+] Scanned {len(modules)} modules')
    
    print('[*] Calculating maturity scores...')
    scores = engine.calculate_maturity_score()
    
    print('[*] Generating markdown report...')
    report = engine.generate_markdown_report(scores)
    
    # Save markdown report
    report_path = os.path.join(repo_root, 'ai_working/MODULE_MATURITY_REPORT_v1.md')
    with open(report_path, 'w') as f:
        f.write(report)
    print(f'[+] Report saved to {report_path}')
    
    # Save JSON export
    json_path = os.path.join(repo_root, 'ai_working/MODULE_MATURITY_v1.json')
    engine.export_json(json_path)
    print(f'[+] JSON export saved to {json_path}')
    
    print('\n' + report)

if __name__ == '__main__':
    main()
