#!/usr/bin/env python3
"""
Phase 9-2: Distributed Consistency & Consensus Scanner

CWE-366 (Race Condition), CWE-696 (Incorrect Behavior Order)

Detects:
- Missing consensus on write
- Replication lag not bounded
- Read-after-write consistency gaps
- Stale read not documented
- Causal ordering violations
- Split-brain scenarios
- Leader election without majority
- Version vector missing
- Tombstone not handled
- Lost update anomaly
- Phantom read in distributed context
"""

import re
from pathlib import Path
from typing import List, Dict


class DistributedConsistencyScan:
    """Scan for distributed consistency and consensus issues"""
    
    def __init__(self, repo_root: str = '.'):
        self.repo_root = Path(repo_root)
        self.gaps = []
    
    def scan_files(self, file_list: List[Path]) -> List[Dict]:
        """Scan files for distributed consistency issues"""
        
        for file_path in file_list:
            if not file_path.suffix in ['.cpp', '.cc', '.h', '.hpp']:
                continue
            
            # Check if file is distributed/replication-related
            if not self._is_distributed_file(str(file_path)):
                continue
            
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    lines = content.split('\n')
            except Exception:
                continue
            
            # Scan patterns
            self._check_consensus_on_write(file_path, lines)
            self._check_replication_lag(file_path, lines)
            self._check_consistency_levels(file_path, lines)
            self._check_version_tracking(file_path, lines)
            self._check_conflict_resolution(file_path, lines)
        
        return self.gaps
    
    def _is_distributed_file(self, file_path: str) -> bool:
        """Check if file is distributed/replication-related"""
        keywords = ['replication', 'distributed', 'consensus', 'raft', 'paxos', 'quorum',
                   'replica', 'sync', 'shard', 'failover']
        return any(kw in file_path.lower() for kw in keywords)

    @staticmethod
    def _strip_string_literals(text: str) -> str:
        """Remove quoted string/char literals so text constants do not trigger semantic rules."""
        return re.sub(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', '""', text)

    def _iter_code_lines(self, lines: List[str]):
        """Yield non-comment code lines with 1-based line numbers."""
        in_block_comment = False

        for idx, raw in enumerate(lines, 1):
            stripped = raw.strip()

            if in_block_comment:
                if '*/' in stripped:
                    in_block_comment = False
                continue

            if not stripped:
                continue

            if stripped.startswith('//'):
                continue

            if stripped.startswith('/*'):
                if '*/' not in stripped:
                    in_block_comment = True
                continue

            # Doxygen/multi-line comment continuation
            if stripped.startswith('*'):
                continue

            scan_line = self._strip_string_literals(raw)
            scan_stripped = scan_line.strip()

            # Prometheus/help text and metric export lines are non-behavioral text paths.
            lower = scan_stripped.lower()
            if '# help' in lower or 'themisdb_' in lower:
                continue
            if 'oss << "' in raw.lower():
                continue

            yield idx, scan_line

    @staticmethod
    def _looks_like_merge_or_conflict_call(line: str) -> bool:
        """Match executable merge/reconcile/conflict-resolution call sites, not variable names."""
        return bool(re.search(
            r'\b(?:merge\w*|reconcile\w*|resolve\w*conflict\w*|resolveConflict\w*)\s*\(',
            line,
            re.IGNORECASE,
        ))

    @staticmethod
    def _looks_like_function_signature_line(line: str) -> bool:
        """Best-effort filter for function declarations/definitions."""
        s = line.strip()
        if not s:
            return False
        if s.startswith(('if ', 'for ', 'while ', 'switch ', 'return ')):
            return False
        # Typical C++ member/free function signature
        if re.search(
            r'^[A-Za-z_][\w:<>&*\s~]*\s+[A-Za-z_][\w:~]*\s*\([^;]*\)\s*(?:const)?\s*(?:noexcept(?:\([^)]*\))?)?\s*(?:\{|$)',
            s,
        ):
            return True

        # Multiline signature start, e.g. "bool Foo::resolveConflict("
        if '::' in s and s.endswith('(') and ';' not in s:
            return True

        return False
    
    def _check_consensus_on_write(self, file_path: Path, lines: List[str]):
        """Find writes without consensus verification"""

        for idx, line in self._iter_code_lines(lines):
            # Look for write operations
            if re.search(r'(write|put|insert|update)\s*\(', line, re.IGNORECASE):
                # Check for replication/consensus
                next_lines = '\n'.join(lines[idx:min(idx+20, len(lines))])
                
                if not re.search(r'(replicate|consensus|quorum|ack|wait|sync)', next_lines, re.IGNORECASE):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'distributed_consistency',
                        'severity': 'CRITICAL',
                        'pattern': 'missing_consensus',
                        'description': 'Write without consensus/replication acknowledgment',
                        'context': line.strip()
                    })
    
    def _check_replication_lag(self, file_path: Path, lines: List[str]):
        """Find replication lag not bounded"""

        for idx, line in self._iter_code_lines(lines):
            # Look for read operations
            if re.search(r'(read|get|execute.*query)\s*\(', line, re.IGNORECASE):
                # Check if consistency level is specified
                prev_lines = '\n'.join(lines[max(0, idx-5):idx])
                
                if not re.search(r'(strong|linearizable|serializable|eventual|stale)', prev_lines, re.IGNORECASE):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'distributed_consistency',
                        'severity': 'HIGH',
                        'pattern': 'unspecified_consistency',
                        'description': 'Read without explicit consistency level (replication lag unknown)',
                        'context': line.strip()
                    })
    
    def _check_consistency_levels(self, file_path: Path, lines: List[str]):
        """Find incorrect consistency level usage"""

        for idx, line in self._iter_code_lines(lines):
            # Look for stale read patterns
            if re.search(r'\b(?:eventual|stale)\b', line, re.IGNORECASE):
                # Only evaluate behavior-relevant read/consistency contexts.
                if not re.search(r'(read|consisten|level|stale[_ ]read|eventual[_ ]read|ConsistencyLevel)', line, re.IGNORECASE):
                    continue

                # Check if it's in a safe context
                next_lines = '\n'.join(lines[idx:min(idx+10, len(lines))])
                
                # Stale reads should be documented
                if not re.search(r'(comment|//.*stale|STALE_READ_OK)', next_lines):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'distributed_consistency',
                        'severity': 'MEDIUM',
                        'pattern': 'stale_read_undocumented',
                        'description': 'Eventual/stale read without documentation of correctness',
                        'context': line.strip()
                    })
    
    def _check_version_tracking(self, file_path: Path, lines: List[str]):
        """Find missing version vectors/clocks"""

        for idx, line in self._iter_code_lines(lines):
            # Look for concurrent update patterns
            if self._looks_like_merge_or_conflict_call(line) or re.search(r'(concurrent.*write|concurrent_update)', line, re.IGNORECASE):
                if self._looks_like_function_signature_line(line):
                    continue

                # Analytics partial-result merge is query aggregation, not multi-master write conflict.
                if 'mergeResults(partials' in line or 'mergeResults (partials' in line:
                    continue

                # CRDT dispatch tables are explicit merge strategy definitions.
                if 'case CRDTType::' in line:
                    continue
                if re.search(r'\bmerge(?:GSet|ORSet|LWWMap|TwoPSet|RGA|FlagEW|FlagDW)\s*\(', line):
                    continue

                # Check for version tracking
                window_ctx = '\n'.join(
                    self._strip_string_literals(x)
                    for x in lines[max(0, idx-10):min(idx+15, len(lines))]
                )
                
                if not re.search(r'(version|timestamp|vector|clock|causality)', window_ctx, re.IGNORECASE):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'distributed_consistency',
                        'severity': 'CRITICAL',
                        'pattern': 'missing_version_tracking',
                        'description': 'Concurrent update without version vector or causal ordering',
                        'context': line.strip()
                    })
    
    def _check_conflict_resolution(self, file_path: Path, lines: List[str]):
        """Find missing conflict resolution"""

        for idx, line in self._iter_code_lines(lines):
            # Look for merge operations
            if self._looks_like_merge_or_conflict_call(line):
                if self._looks_like_function_signature_line(line):
                    continue

                # Require conflict intent in local context to avoid generic merge utility noise.
                local_ctx = '\n'.join(
                    self._strip_string_literals(x)
                    for x in lines[max(0, idx-3):min(idx+6, len(lines))]
                ).lower()
                if 'conflict' not in local_ctx and 'reconcil' not in local_ctx:
                    continue

                # Explicit strategy indicators: switch/case dispatch, CRDT enum strategy,
                # or vector-clock merge logic are already deterministic conflict policies.
                strategy_ctx = '\n'.join(
                    self._strip_string_literals(x)
                    for x in lines[max(0, idx-8):min(idx+14, len(lines))]
                ).lower()
                if ('switch' in strategy_ctx and 'case' in strategy_ctx) or 'crdttype::' in strategy_ctx:
                    continue
                if 'vector_clock' in strategy_ctx or 'merged_clock' in strategy_ctx:
                    continue
                if 'threewaymergeresolver' in strategy_ctx or 'fieldlevelmergeresolver' in strategy_ctx:
                    continue
                if 'mergestrategy::' in strategy_ctx:
                    continue
                if re.search(r'\bmerge(?:gset|orset|lwwmap|twopset|rga|flagew|flagdw)\s*\(', strategy_ctx):
                    continue

                # Check if strategy is defined
                next_lines = '\n'.join(lines[idx:min(idx+10, len(lines))])
                
                if not re.search(r'(last.*write|custom.*resolve|application.*logic)',
                                next_lines, re.IGNORECASE):
                    self.gaps.append({
                        'file': str(file_path.relative_to(self.repo_root)),
                        'line': idx,
                        'category': 'distributed_consistency',
                        'severity': 'HIGH',
                        'pattern': 'undefined_conflict_resolution',
                        'description': 'Merge without explicit conflict resolution strategy',
                        'context': line.strip()
                    })
