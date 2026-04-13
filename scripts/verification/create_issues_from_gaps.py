"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            create_issues_from_gaps.py                         ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:29:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     365                                            ║
    • Open Issues:     TODOs: 5, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 00f73b2e14  2026-02-25  fix: standardize priority labels in all issue creator scr... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Create GitHub Issues from Verified Gaps

This script analyzes verification reports and generates GitHub issue templates
for verified implementation gaps.

Usage:
    python3 create_issues_from_gaps.py --input=verification.json [--output=issues/]
    python3 create_issues_from_gaps.py --input=verification.json --create-issues
"""

import json
import argparse
from pathlib import Path
from typing import List, Dict
from datetime import datetime
from collections import defaultdict


class IssueGenerator:
    """Generates GitHub issues from verification gaps"""
    
    def __init__(self):
        self.issue_templates = []
    
    def load_verification_report(self, json_file: str) -> List[Dict]:
        """Load verification report and filter gaps"""
        try:
            with open(json_file, 'r', encoding='utf-8') as f:
                data = json.load(f)
            
            # Filter for actual gaps (not doc-only or likely implemented)
            gaps = [
                todo for todo in data.get('todos', [])
                if todo.get('status') in ['possible_gap', 'partial']
            ]
            
            return gaps
        
        except Exception as e:
            print(f"Error loading {json_file}: {e}")
            return []
    
    def categorize_gaps(self, gaps: List[Dict]) -> Dict[str, List[Dict]]:
        """Group gaps by category for batch issue creation"""
        categorized = defaultdict(list)
        
        for gap in gaps:
            category = gap.get('category', 'general')
            categorized[category].append(gap)
        
        return dict(categorized)
    
    def generate_issue_title(self, gap: Dict) -> str:
        """Generate a concise issue title from gap"""
        content = gap.get('content', '')
        
        # Clean up content
        content = content.replace('- [ ]', '').replace('TODO:', '').replace('TBD:', '').strip()
        
        # Truncate if too long
        if len(content) > 80:
            content = content[:77] + '...'
        
        # Add category prefix
        category = gap.get('category', 'general')
        prefix_map = {
            'security': '🔒 Security:',
            'performance': '⚡ Performance:',
            'llm-ai': '🤖 LLM/AI:',
            'analytics': '📊 Analytics:',
            'enterprise': '🏢 Enterprise:',
            'testing': '✅ Testing:',
            'documentation': '📝 Documentation:',
            'general': '🔧'
        }
        
        prefix = prefix_map.get(category, '🔧')
        
        return f"{prefix} {content}"
    
    def generate_issue_body(self, gap: Dict) -> str:
        """Generate detailed issue body"""
        file_path = gap.get('file_path', '').replace('/home/runner/work/ThemisDB/ThemisDB/', '')
        line_number = gap.get('line_number', 0)
        content = gap.get('content', '')
        status = gap.get('status', 'unknown')
        category = gap.get('category', 'general')
        evidence = gap.get('evidence', [])
        confidence = gap.get('confidence', 'low')
        notes = gap.get('notes', '')
        
        body = []
        
        # Description
        body.append("## 📋 Description\n")
        body.append(f"{content}\n\n")
        
        # Source
        body.append("## 📍 Source\n")
        body.append(f"- **Documentation**: `{file_path}` (Line {line_number})\n")
        body.append(f"- **Category**: {category}\n")
        body.append(f"- **Status**: {status}\n\n")
        
        # Verification Details
        body.append("## 🔍 Verification Details\n")
        body.append(f"- **Automated Assessment**: {status}\n")
        body.append(f"- **Confidence**: {confidence}\n")
        if notes:
            body.append(f"- **Notes**: {notes}\n")
        body.append("\n")
        
        # Evidence
        if evidence:
            body.append("## 📊 Evidence from Codebase\n")
            for ev in evidence[:5]:
                body.append(f"- {ev}\n")
            if len(evidence) > 5:
                body.append(f"- ... and {len(evidence) - 5} more references\n")
            body.append("\n")
        
        # Implementation Checklist
        body.append("## ✅ Implementation Checklist\n")
        body.append("- [ ] Verify this is an actual gap (not already implemented)\n")
        body.append("- [ ] Design implementation approach\n")
        body.append("- [ ] Implement feature/fix\n")
        body.append("- [ ] Add tests\n")
        body.append("- [ ] Update documentation\n")
        body.append("- [ ] Update original TODO in documentation\n\n")
        
        # Labels
        body.append("## 🏷️ Suggested Labels\n")
        
        label_map = {
            'security': ['security', 'high-priority', 'priority:high'],
            'performance': ['performance', 'optimization', 'priority:medium'],
            'llm-ai': ['llm', 'ai', 'enhancement', 'priority:medium'],
            'analytics': ['analytics', 'feature', 'priority:medium'],
            'enterprise': ['enterprise', 'feature', 'priority:medium'],
            'testing': ['testing', 'quality', 'priority:medium'],
            'documentation': ['documentation', 'priority:low'],
            'general': ['enhancement', 'priority:medium']
        }
        
        labels = label_map.get(category, ['enhancement'])
        
        if status == 'partial':
            labels.append('incomplete-implementation')
        else:
            labels.append('verified-gap')
        
        for label in labels:
            body.append(f"- `{label}`\n")
        
        body.append("\n")
        
        # Related Documentation
        body.append("## 📚 Related Documentation\n")
        body.append(f"- Original TODO: [{file_path}:{line_number}]({file_path}#L{line_number})\n")
        body.append("- Issue #8: Verify Documentation TODOs (Meta-Issue)\n\n")
        
        # Additional Context
        body.append("## 💡 Additional Context\n")
        body.append("This issue was generated from the documentation TODO verification process.\n")
        body.append("Please verify that this is indeed an implementation gap before starting work.\n\n")
        
        body.append("**Note**: If this feature is already implemented, please:\n")
        body.append("1. Close this issue\n")
        body.append("2. Update the documentation to mark the TODO as complete\n")
        body.append("3. Add a comment with the file paths where it's implemented\n")
        
        return ''.join(body)
    
    def generate_issue_template(self, gap: Dict) -> Dict:
        """Generate a complete issue template"""
        return {
            'title': self.generate_issue_title(gap),
            'body': self.generate_issue_body(gap),
            'labels': self.get_labels_for_gap(gap),
            'source': {
                'file': gap.get('file_path', ''),
                'line': gap.get('line_number', 0),
                'category': gap.get('category', 'general')
            }
        }
    
    def get_labels_for_gap(self, gap: Dict) -> List[str]:
        """Get appropriate labels for a gap"""
        category = gap.get('category', 'general')
        status = gap.get('status', 'unknown')
        
        label_map = {
            'security': ['security', 'verified-gap', 'priority:high'],
            'performance': ['performance', 'optimization', 'verified-gap', 'priority:medium'],
            'llm-ai': ['llm', 'ai', 'enhancement', 'verified-gap', 'priority:medium'],
            'analytics': ['analytics', 'feature', 'verified-gap', 'priority:medium'],
            'enterprise': ['enterprise', 'feature', 'verified-gap', 'priority:medium'],
            'testing': ['testing', 'quality', 'verified-gap', 'priority:medium'],
            'documentation': ['documentation', 'verified-gap', 'priority:low'],
            'general': ['enhancement', 'verified-gap', 'priority:medium']
        }
        
        labels = label_map.get(category, ['enhancement', 'verified-gap'])
        
        if status == 'partial':
            labels.append('incomplete-implementation')
        
        return labels
    
    def save_issue_templates(self, output_dir: str):
        """Save issue templates to markdown files"""
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        for i, template in enumerate(self.issue_templates, 1):
            # Generate filename
            category = template['source']['category']
            filename = f"issue_{i:03d}_{category}.md"
            file_path = output_path / filename
            
            # Write template
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(f"# {template['title']}\n\n")
                f.write(template['body'])
                f.write(f"\n\n---\n\n")
                f.write(f"**Labels**: {', '.join(template['labels'])}\n")
            
            print(f"  Created: {filename}")
    
    def generate_batch_summary(self, output_file: str):
        """Generate a summary of all issues to be created"""
        summary = []
        
        summary.append("# GitHub Issues Creation Summary\n")
        summary.append(f"**Generated**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        summary.append(f"**Total Issues**: {len(self.issue_templates)}\n\n")
        
        summary.append("---\n\n")
        
        # Group by category
        by_category = defaultdict(list)
        for template in self.issue_templates:
            category = template['source']['category']
            by_category[category].append(template)
        
        summary.append("## Issues by Category\n\n")
        
        for category, templates in sorted(by_category.items(), key=lambda x: len(x[1]), reverse=True):
            summary.append(f"### {category.title()} ({len(templates)} issues)\n\n")
            
            for template in templates:
                summary.append(f"- {template['title']}\n")
                summary.append(f"  - Source: `{template['source']['file']}:{template['source']['line']}`\n")
                summary.append(f"  - Labels: {', '.join(template['labels'])}\n")
            
            summary.append("\n")
        
        summary.append("## Next Steps\n\n")
        summary.append("1. Review each issue template in the `issues/` directory\n")
        summary.append("2. Manually verify that each is an actual implementation gap\n")
        summary.append("3. Create GitHub issues using the template content\n")
        summary.append("4. Link issues to original documentation TODOs\n")
        summary.append("5. Update project roadmap with new issues\n")
        
        with open(output_file, 'w', encoding='utf-8') as f:
            f.writelines(summary)
        
        print(f"✅ Summary generated: {output_file}")


def main():
    parser = argparse.ArgumentParser(
        description='Create GitHub issues from verified implementation gaps'
    )
    parser.add_argument(
        '--input',
        required=True,
        help='Input verification JSON file'
    )
    parser.add_argument(
        '--output',
        default='issues',
        help='Output directory for issue templates'
    )
    parser.add_argument(
        '--min-confidence',
        choices=['low', 'medium', 'high'],
        default='low',
        help='Minimum confidence level to generate issues'
    )
    parser.add_argument(
        '--category',
        help='Only generate issues for specific category'
    )
    
    args = parser.parse_args()
    
    # Initialize generator
    generator = IssueGenerator()
    
    print(f"🔍 Loading verification report: {args.input}")
    gaps = generator.load_verification_report(args.input)
    
    print(f"✅ Found {len(gaps)} potential gaps")
    
    # Filter by confidence if specified
    confidence_levels = {'low': 0, 'medium': 1, 'high': 2}
    min_conf = confidence_levels[args.min_confidence]
    
    filtered_gaps = [
        g for g in gaps
        if confidence_levels.get(g.get('confidence', 'low'), 0) >= min_conf
    ]
    
    # Filter by category if specified
    if args.category:
        filtered_gaps = [g for g in filtered_gaps if g.get('category') == args.category]
    
    print(f"📋 Generating issues for {len(filtered_gaps)} gaps...")
    
    # Generate issue templates
    for gap in filtered_gaps:
        template = generator.generate_issue_template(gap)
        generator.issue_templates.append(template)
    
    # Save templates
    generator.save_issue_templates(args.output)
    
    # Generate summary
    summary_file = f"{args.output}/SUMMARY.md"
    generator.generate_batch_summary(summary_file)
    
    print(f"\n✅ Generated {len(generator.issue_templates)} issue templates")
    print(f"📁 Output directory: {args.output}/")
    print(f"📊 Summary: {summary_file}")
    
    return 0


if __name__ == '__main__':
    exit(main())
