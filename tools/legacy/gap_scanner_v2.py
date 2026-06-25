#!/usr/bin/env python3
"""
ThemisDB Gap Scanner v2 — Enhanced Implementation Gap Detection

Improvements over v1:
- Contextual analysis (intentional vs. unintentional gaps)
- Conditional compilation handling (#ifdef/#endif)
- Mock/simulation framework detection
- Dead code patterns (#if 0, deprecated)
- Platform-specific fallbacks
- False-positive reduction
- Severity scoring based on context
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass, field, asdict
from typing import List, Dict, Set, Optional
from enum import Enum

class GapCategory(Enum):
    """Gap classification with context awareness"""
    # Unimplemented paths
    UNIMPLEMENTED = "unimplemented"  # throw/return empty
    INCOMPLETE = "incomplete"  # partial impl + TODO
    
    # Intentional gaps
    STUB_DOCUMENTED = "stub_documented"  # Has 4-line template
    STUB_UNDOCUMENTED = "stub_undocumented"  # No documentation
    MOCK_FRAMEWORK = "mock_framework"  # GTest/Gmock
    TEST_ONLY = "test_only"  # Test fixture
    DISABLED_CODE = "disabled_code"  # #if 0, deprecated
    
    # Platform/conditional gaps
    CONDITIONAL = "conditional"  # #ifdef THEMIS_ENABLE_*
    PLATFORM_FALLBACK = "platform_fallback"  # Platform-specific impl
    
    # Code debt
    TODO_ITEM = "todo_item"
    FIXME_ITEM = "fixme_item"
    XXX_ITEM = "xxx_item"
    TECHNICAL_DEBT = "technical_debt"  # DEBT, OPTIMIZE, HACK

class GapSeverity(Enum):
    CRITICAL = "critical"  # Production blocker
    HIGH = "high"  # Should fix soon
    MEDIUM = "medium"  # Nice to have
    LOW = "low"  # Technical debt
    INTENTIONAL = "intentional"  # By design

@dataclass
class ContextInfo:
    """Context surrounding a gap"""
    is_test_code: bool = False
    is_mock_code: bool = False
    is_platform_specific: bool = False
    has_documentation: bool = False
    documentation: str = ""
    conditional_define: Optional[str] = None  # e.g., "THEMIS_ENABLE_VULKAN"
    is_fallback: bool = False  # Intentional fallback implementation
    parent_function: Optional[str] = None
    in_disabled_block: bool = False  # #if 0 ... #endif

@dataclass
class CodeGap:
    """Represents a single implementation gap"""
    file_path: str
    line_num: int
    category: GapCategory
    severity: GapSeverity
    snippet: str
    context_info: ContextInfo
    is_false_positive: bool = False
    notes: str = ""
    
    def to_dict(self):
        return {
            'file': self.file_path,
            'line': self.line_num,
            'category': self.category.value,
            'severity': self.severity.value,
            'snippet': self.snippet,
            'is_test': self.context_info.is_test_code,
            'is_platform_specific': self.context_info.is_platform_specific,
            'has_documentation': self.context_info.has_documentation,
            'notes': self.notes
        }

class EnhancedGapScanner:
    """Advanced gap detection with context analysis"""
    
    # Enhanced regex patterns
    PATTERNS = {
        # Unimplemented
        'unimplemented_error': re.compile(
            r'throw\s+std::(runtime_error|logic_error|not_implemented_error)\s*\(\s*["\'].*?(?:not\s+implemented|unimplemented)'
        ),
        'unimplemented_return': re.compile(
            r'return\s+(?:std::make_optional\s*\(\s*\)|std::nullopt|std::optional<[^>]+>\(\)|\{\s*\}|nullptr)'
        ),
        
        # Stubs & Documentation
        'stub_marker': re.compile(r'//\s*STUB(?:\s|:|NOTE)?'),
        'mock_marker': re.compile(r'//\s*MOCK(?:\s|:|NOTE)?'),
        'simulation_marker': re.compile(r'//\s*SIMULATION(?:\s|:|NOTE)?'),
        'placeholder': re.compile(r'//\s*PLACEHOLDER'),
        'not_implemented': re.compile(r'//\s*NOT_IMPLEMENTED'),
        
        # Code debt
        'todo': re.compile(r'//\s*TODO[:\s]'),
        'fixme': re.compile(r'//\s*FIXME[:\s]'),
        'xxx': re.compile(r'//\s*XXX[:\s]'),
        'hack': re.compile(r'//\s*HACK[:\s]'),
        'debt': re.compile(r'//\s*DEBT[:\s]'),
        'optimize': re.compile(r'//\s*OPTIMIZE[:\s]'),
        
        # Advanced patterns
        'empty_function': re.compile(r'(\w+)\s*\([^)]*\)\s*\{\s*(?://.*)?(?:\n\s*)*\}'),  # Empty body
        'only_logging': re.compile(r'{\s*(?:LOG|VLOG|DLOG|std::cerr|printf?)\s*\('),  # Only logs
        'not_yet_implemented': re.compile(r'(?:not\s+yet|NYI|WIP)\s+(?:implemented|ready)'),
        
        # Conditional compilation
        'ifdef_block': re.compile(r'#ifdef\s+(\w+)'),
        'if_0_block': re.compile(r'#if\s+0\b'),
        'disabled_block': re.compile(r'/\*\s*(?:DISABLED|DEPRECATED|NOT_USED)\s*\*/', re.IGNORECASE),
        
        # Mock/Test frameworks
        'gmock_expect': re.compile(r'EXPECT_CALL|ON_CALL'),
        'gtest_mock': re.compile(r'testing::Mock'),
        'test_fixture': re.compile(r'class\s+\w+\s*:\s*(?:public\s+)?testing::Test'),
        
        # Platform-specific
        'platform_specific': re.compile(r'#ifdef\s+(?:_WIN32|_WIN64|__APPLE__|__linux__|THEMIS_ENABLE_\w+)'),
        'fallback_impl': re.compile(r'(?://.*)?fallback|(?://.*)?unsupported\s+on'),
    }
    
    # 4-line STUB documentation template (from COPILOT_INSTRUCTIONS.md § 8)
    STUB_TEMPLATE = re.compile(
        r'//\s*STUB/SIMULATION NOTE:\s*\n'
        r'//\s*Purpose:.*\n'
        r'//\s*Activation:.*\n'
        r'(?://\s*Production Delta:.*\n)?'
        r'//\s*Removal Plan:'
    )
    
    def __init__(self, repo_root: str):
        self.repo_root = Path(repo_root)
        self.gaps: List[CodeGap] = []
        self.scan_stats = {}
        
    def scan_file(self, file_path: Path) -> List[CodeGap]:
        """Scan a single file for gaps with context analysis"""
        gaps = []
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except:
            return []
        
        # Analyze file context
        is_test_file = 'test' in file_path.parts or file_path.name.endswith('_test.cpp')
        is_mock_file = 'mock' in file_path.name.lower()
        
        # Track #ifdef blocks
        ifdef_stack = []
        disabled_blocks = set()
        
        for line_num, line in enumerate(lines, 1):
            # Track ifdef blocks
            if self.PATTERNS['ifdef_block'].search(line):
                ifdef_stack.append(line_num)
            elif self.PATTERNS['if_0_block'].search(line):
                disabled_blocks.add(line_num)
            elif line.strip() == '#endif':
                if ifdef_stack:
                    ifdef_stack.pop()
            
            # Check for gaps
            for pattern_name, pattern in self.PATTERNS.items():
                if not pattern.search(line):
                    continue
                
                # Build context
                context_info = self._analyze_context(
                    lines, line_num, file_path,
                    is_test_file, is_mock_file, ifdef_stack
                )
                
                # Determine category and severity
                category, severity = self._classify_gap(
                    pattern_name, line, context_info
                )
                
                # Check for false positives
                if self._is_false_positive(category, context_info):
                    continue
                
                # Create gap record
                gap = CodeGap(
                    file_path=str(file_path.relative_to(self.repo_root)),
                    line_num=line_num,
                    category=category,
                    severity=severity,
                    snippet=line.strip()[:100],
                    context_info=context_info
                )
                
                gaps.append(gap)
        
        return gaps
    
    def _analyze_context(self, lines: List[str], line_num: int, 
                        file_path: Path, is_test: bool, is_mock: bool,
                        ifdef_stack: List[int]) -> ContextInfo:
        """Extract context around a gap"""
        context = ContextInfo(is_test_code=is_test, is_mock_code=is_mock)
        
        # Look at surrounding lines (±5 lines)
        start = max(0, line_num - 6)
        end = min(len(lines), line_num + 5)
        window = ''.join(lines[start:end])
        
        # Check for documentation
        if line_num > 1:
            prev_lines = '\n'.join(lines[max(0, line_num-5):line_num])
            if self.STUB_TEMPLATE.search(prev_lines):
                context.has_documentation = True
                context.documentation = "4-line STUB template found"
            else:
                # Look for ad-hoc documentation
                doc_match = re.search(r'//.*(?:Purpose|why|reason|note).*', prev_lines, re.IGNORECASE)
                if doc_match:
                    context.has_documentation = True
                    context.documentation = doc_match.group(0)
        
        # Check for platform-specific patterns
        if re.search(r'#ifdef.*THEMIS_ENABLE|#ifdef.*_WIN|#ifdef.*__APPLE__|#ifdef.*__linux__', window):
            context.is_platform_specific = True
            match = re.search(r'#ifdef\s+(\w+)', window)
            if match:
                context.conditional_define = match.group(1)
        
        # Check for fallback pattern
        if re.search(r'fallback|unsupported|not\s+available', window, re.IGNORECASE):
            context.is_fallback = True
        
        # Detect parent function
        func_match = re.search(r'(?:void|bool|int|auto|\w+(?:<[^>]+>)?)\s+(\w+)\s*\([^)]*\)\s*{', window)
        if func_match:
            context.parent_function = func_match.group(1)
        
        # Check for mock/test patterns
        if re.search(r'EXPECT_CALL|ON_CALL|testing::Mock|MOCK_METHOD', window):
            context.is_mock_code = True
        
        return context
    
    def _classify_gap(self, pattern_name: str, line: str, 
                     context: ContextInfo) -> tuple[GapCategory, GapSeverity]:
        """Determine gap category and severity"""
        
        # Test/Mock code is lower priority
        if context.is_test_code or context.is_mock_code:
            if 'mock' in pattern_name:
                return GapCategory.MOCK_FRAMEWORK, GapSeverity.LOW
            if 'stub' in pattern_name:
                return GapCategory.STUB_UNDOCUMENTED, GapSeverity.LOW
        
        # Documented stubs are intentional
        if 'stub' in pattern_name or 'mock' in pattern_name:
            if context.has_documentation:
                return GapCategory.STUB_DOCUMENTED, GapSeverity.INTENTIONAL
            else:
                return GapCategory.STUB_UNDOCUMENTED, GapSeverity.HIGH
        
        # Platform-specific fallbacks are intentional
        if context.is_platform_specific or context.is_fallback:
            return GapCategory.PLATFORM_FALLBACK, GapSeverity.INTENTIONAL
        
        # Code debt items
        if 'todo' in pattern_name:
            return GapCategory.TODO_ITEM, GapSeverity.MEDIUM
        if 'fixme' in pattern_name:
            return GapCategory.FIXME_ITEM, GapSeverity.MEDIUM
        if 'xxx' in pattern_name:
            return GapCategory.XXX_ITEM, GapSeverity.LOW
        if 'hack' in pattern_name or 'debt' in pattern_name:
            return GapCategory.TECHNICAL_DEBT, GapSeverity.LOW
        
        # Disabled code
        if 'disabled' in pattern_name or 'if_0' in pattern_name:
            return GapCategory.DISABLED_CODE, GapSeverity.LOW
        
        # Unimplemented is critical
        if 'unimplemented' in pattern_name:
            return GapCategory.UNIMPLEMENTED, GapSeverity.CRITICAL
        
        # Default
        return GapCategory.INCOMPLETE, GapSeverity.HIGH
    
    def _is_false_positive(self, category: GapCategory, 
                          context: ContextInfo) -> bool:
        """Detect false positives to reduce noise"""
        
        # Documented stubs are by-design
        if category == GapCategory.STUB_DOCUMENTED:
            return False  # Include for tracking compliance
        
        # Platform fallbacks are intentional
        if category == GapCategory.PLATFORM_FALLBACK and context.is_fallback:
            return False  # Include but mark as intentional
        
        # Test mocks are not production gaps
        if context.is_mock_code and context.is_test_code:
            return True
        
        # Disabled code is not a production gap (unless in src/)
        if category == GapCategory.DISABLED_CODE:
            return True
        
        return False
    
    def scan_module(self, module_name: str) -> Dict[str, List[CodeGap]]:
        """Scan an entire module directory"""
        results = {}
        
        # Scan src/<module>, include/<module>, tests/<module>, benchmarks/
        scan_paths = [
            self.repo_root / 'src' / module_name,
            self.repo_root / 'include' / module_name,
            self.repo_root / 'tests' / module_name,
            self.repo_root / 'benchmarks' / module_name,
        ]
        
        for base_path in scan_paths:
            if not base_path.exists():
                continue
            
            for cpp_file in base_path.glob('**/*.cpp'):
                gaps = self.scan_file(cpp_file)
                if gaps:
                    results[str(cpp_file.relative_to(self.repo_root))] = gaps
            
            for h_file in base_path.glob('**/*.hpp'):
                gaps = self.scan_file(h_file)
                if gaps:
                    results[str(h_file.relative_to(self.repo_root))] = gaps
        
        return results
    
    def run_full_scan(self, output_dir: str = 'ai_working'):
        """Scan all modules and generate reports"""
        output_path = Path(output_dir)
        output_path.mkdir(exist_ok=True)
        
        modules = set()
        
        # Discover all modules
        for src_item in (self.repo_root / 'src').iterdir():
            if src_item.is_dir() and not src_item.name.startswith('_'):
                modules.add(src_item.name)
        
        all_gaps = {}
        aggregate = {}
        
        for module in sorted(modules):
            print(f"Scanning {module}...", end=' ')
            gaps_by_file = self.scan_module(module)
            
            if not gaps_by_file:
                print("No gaps")
                continue
            
            all_gaps[module] = gaps_by_file
            
            # Count by category and severity
            stats = {'total': 0}
            for category in GapCategory:
                stats[category.value] = 0
            for severity in GapSeverity:
                stats[f'severity_{severity.value}'] = 0
            
            for gaps in gaps_by_file.values():
                for gap in gaps:
                    stats['total'] += 1
                    stats[gap.category.value] = stats.get(gap.category.value, 0) + 1
                    stats[f'severity_{gap.severity.value}'] = stats.get(
                        f'severity_{gap.severity.value}', 0) + 1
            
            aggregate[module] = stats
            print(f"Found {stats['total']} gaps")
            
            # Save per-module report
            module_report = {
                'module': module,
                'stats': stats,
                'gaps_by_file': {}
            }
            for file_path, gaps in gaps_by_file.items():
                module_report['gaps_by_file'][file_path] = [g.to_dict() for g in gaps]
            
            with open(output_path / f'gap_scan_v2_{module}.json', 'w') as f:
                json.dump(module_report, f, indent=2)
        
        # Save aggregate
        with open(output_path / 'gap_scan_v2_aggregate.json', 'w') as f:
            json.dump(aggregate, f, indent=2)
        
        print(f"\n[OK] Scan complete. Reports in {output_dir}/")
        return aggregate

if __name__ == '__main__':
    scanner = EnhancedGapScanner('.')
    scanner.run_full_scan()
