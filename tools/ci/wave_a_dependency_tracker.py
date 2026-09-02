#!/usr/bin/env python3
"""
WAVE A Execution Dependency Tracker & Auto-Blocking Manager
Monitors cross-module dependencies and prevents premature execution.
"""

import json
import sys
from datetime import datetime, timedelta
from typing import Dict, List, Set, Tuple

# Module Dependency Graph (Hard + Soft Blockers)
DEPENDENCY_GRAPH = {
    "transaction": {
        "hard_blockers": [],
        "soft_blockers": [],
        "blocking": ["sharding", "distributed_knowledge"],  # Who depends on me
        "target_start": "2026-09-02",
        "target_end": "2026-09-12",
    },
    "gpu": {
        "hard_blockers": [],
        "soft_blockers": [],
        "blocking": ["llm"],
        "target_start": "2026-09-02",
        "target_end": "2026-09-15",
    },
    "query": {
        "hard_blockers": [],
        "soft_blockers": [],
        "blocking": ["index"],
        "target_start": "2026-09-03",
        "target_end": "2026-09-22",
    },
    "server": {
        "hard_blockers": [],
        "soft_blockers": [],
        "blocking": [],
        "target_start": "2026-09-07",
        "target_end": "2026-09-22",
    },
    "storage": {
        "hard_blockers": ["query"],  # Wait for QUERY design approval (Sept 10)
        "soft_blockers": [],
        "blocking": ["sharding"],
        "target_start": "2026-09-07",
        "target_end": "2026-09-20",
    },
    "index": {
        "hard_blockers": ["query"],  # FTS executor blocks full completion
        "soft_blockers": [],
        "blocking": ["distributed_knowledge"],
        "target_start": "2026-09-15",
        "target_end": "2026-09-28",
    },
    "sharding": {
        "hard_blockers": ["transaction"],  # AC-6 crash-recovery proof required
        "soft_blockers": ["storage"],  # AccessCoordinator wiring preferred
        "blocking": ["distributed_knowledge"],
        "target_start": "2026-09-20",
        "target_end": "2026-09-30",
    },
    "distributed_knowledge": {
        "hard_blockers": ["transaction"],  # AC-5 timeout determinism required
        "soft_blockers": ["sharding"],  # Consensus protocol needed
        "blocking": [],
        "target_start": "2026-09-20",
        "target_end": "2026-10-05",
    },
    "llm": {
        "hard_blockers": [],
        "soft_blockers": ["gpu"],  # GPU wrapper adoption preferred
        "blocking": [],
        "target_start": "2026-09-15",
        "target_end": "2026-09-25",
    },
}

# AC Completion Evidence (Updated weekly by module owners)
AC_COMPLETION_STATUS = {
    "transaction": {
        "AC-6": {"status": "in-progress", "completion_pct": 0, "target_date": "2026-09-09"},
        "AC-9": {"status": "not-started", "completion_pct": 0, "target_date": "2026-09-15"},
        "AC-5": {"status": "not-started", "completion_pct": 0, "target_date": "2026-09-08"},
    },
    "gpu": {
        "AC-1-5": {"status": "in-progress", "completion_pct": 0, "target_date": "2026-09-15"},
        "AC-4": {"status": "not-started", "completion_pct": 0, "target_date": "2026-09-30"},
    },
    "query": {
        "DESIGN": {"status": "in-review", "completion_pct": 80, "target_date": "2026-09-10"},
        "PHASE-1": {"status": "waiting-approval", "completion_pct": 0, "target_date": "2026-09-22"},
    },
    "storage": {
        "COORDINATOR-WIRING": {"status": "blocked", "completion_pct": 0, "target_date": "2026-09-20"},
    },
    "index": {
        "DETERMINISM": {"status": "ready", "completion_pct": 0, "target_date": "2026-09-25"},
        "FTS-INTEGRATION": {"status": "blocked", "completion_pct": 0, "target_date": "2026-10-05"},
    },
    "sharding": {
        "CONSENSUS": {"status": "blocked", "completion_pct": 0, "target_date": "2026-09-25"},
    },
    "distributed_knowledge": {
        "VALIDATOR": {"status": "blocked", "completion_pct": 0, "target_date": "2026-09-30"},
    },
    "llm": {
        "GPU-KERNEL": {"status": "blocked", "completion_pct": 0, "target_date": "2026-09-22"},
    },
}


class DependencyTracker:
    """Tracks module dependencies and validates execution readiness."""

    def __init__(self, graph: Dict, ac_status: Dict):
        self.graph = graph
        self.ac_status = ac_status
        self.today = datetime.now().date()

    def get_blocker_status(self, module: str) -> Tuple[bool, List[str]]:
        """Check if module can start (all hard blockers completed)."""
        if module not in self.graph:
            return False, ["Unknown module"]

        hard_blockers = self.graph[module]["hard_blockers"]
        blocking_acs = []

        for blocker_module in hard_blockers:
            blocker_acs = self.ac_status.get(blocker_module, {})
            for ac_id, ac_info in blocker_acs.items():
                if ac_info["status"] != "complete":
                    blocking_acs.append(f"{blocker_module}.{ac_id}")

        return len(blocking_acs) == 0, blocking_acs

    def get_soft_blocker_warnings(self, module: str) -> List[str]:
        """Get soft-blocker warnings (can proceed, but with caution)."""
        soft_blockers = self.graph[module]["soft_blockers"]
        warnings = []

        for blocker_module in soft_blockers:
            blocker_acs = self.ac_status.get(blocker_module, {})
            for ac_id, ac_info in blocker_acs.items():
                if ac_info["status"] != "complete":
                    warnings.append(
                        f"Soft blocker: {blocker_module}.{ac_id} not complete (status: {ac_info['status']})"
                    )

        return warnings

    def get_start_date_readiness(self, module: str) -> Tuple[bool, str]:
        """Check if module should start based on target_start date."""
        target_start = datetime.strptime(self.graph[module]["target_start"], "%Y-%m-%d").date()
        if self.today >= target_start:
            return True, f"On schedule (target: {target_start})"
        else:
            days_until = (target_start - self.today).days
            return False, f"Not yet started (starts in {days_until} days)"

    def get_status_report(self) -> Dict:
        """Generate execution status report for all modules."""
        report = {
            "generated_at": self.today.isoformat(),
            "modules": {},
        }

        for module in sorted(self.graph.keys()):
            can_start, hard_blockers = self.get_blocker_status(module)
            soft_warnings = self.get_soft_blocker_warnings(module)
            schedule_ok, schedule_msg = self.get_start_date_readiness(module)

            status = "ready"
            if not schedule_ok:
                status = "not-scheduled"
            elif not can_start:
                status = "blocked"
            elif soft_warnings:
                status = "caution"

            report["modules"][module] = {
                "status": status,
                "can_start": can_start,
                "hard_blockers": hard_blockers,
                "soft_warnings": soft_warnings,
                "schedule": schedule_msg,
                "target_start": self.graph[module]["target_start"],
                "target_end": self.graph[module]["target_end"],
            }

        return report

    def validate_execution_order(self) -> List[str]:
        """Suggest execution order based on dependencies."""
        order = []
        done = set()
        max_iterations = len(self.graph)  # Prevent infinite loops

        for _ in range(max_iterations):
            for module in self.graph.keys():
                if module in done:
                    continue

                hard_blockers = self.graph[module]["hard_blockers"]
                if all(b in done for b in hard_blockers):
                    order.append(module)
                    done.add(module)

            if len(done) == len(self.graph):
                break

        return order

    def print_status_table(self, report: Dict) -> None:
        """Print formatted status table."""
        print("\n" + "=" * 120)
        print("WAVE A EXECUTION STATUS — Dependency Blocking Tracker".center(120))
        print(f"Generated: {report['generated_at']}".center(120))
        print("=" * 120)
        print(
            f"{'Module':<20} {'Status':<15} {'Hard Blockers':<40} {'Soft Warnings':<40} {'Schedule':<20}"
        )
        print("-" * 120)

        for module, info in sorted(report["modules"].items()):
            status_emoji = {
                "ready": "✅",
                "blocked": "❌",
                "caution": "⚠️",
                "not-scheduled": "⏳",
            }.get(info["status"], "?")

            hard_str = ", ".join(info["hard_blockers"]) if info["hard_blockers"] else "None"
            soft_str = ", ".join(info["soft_warnings"][:1]) + "..." if info["soft_warnings"] else "None"

            print(
                f"{module:<20} {status_emoji} {info['status']:<13} {hard_str:<40} {soft_str:<40} {info['schedule']:<20}"
            )

        print("=" * 120)

    def print_execution_order(self, order: List[str]) -> None:
        """Print recommended execution order."""
        print("\nRecommended Execution Order (respecting dependencies):")
        print("-" * 60)
        for i, module in enumerate(order, 1):
            target = self.graph[module]["target_start"]
            print(f"{i}. {module:<25} (Target start: {target})")
        print("-" * 60)

    def check_critical_path(self) -> None:
        """Identify critical path (longest dependency chain)."""
        print("\nCritical Path Analysis:")
        print("-" * 60)

        # Simplified: find longest chain
        def longest_chain(node, graph, visited=None):
            if visited is None:
                visited = set()
            if node in visited:
                return []
            visited.add(node)
            deps = graph[node]["blocking"]
            if not deps:
                return [node]
            chains = [longest_chain(d, graph, visited.copy()) for d in deps]
            return [node] + (max(chains, key=len) if chains else [])

        all_chains = []
        for module in self.graph.keys():
            chain = longest_chain(module, self.graph)
            all_chains.append(chain)

        critical = max(all_chains, key=len)
        print(f"Critical Path: {' → '.join(reversed(critical))}")
        print(f"Chain Length: {len(critical)} modules")
        print(f"Sequential Effort: ~{sum(2 for _ in critical)} weeks")
        print("-" * 60)


def main():
    """Main execution."""
    tracker = DependencyTracker(DEPENDENCY_GRAPH, AC_COMPLETION_STATUS)

    # Generate and print report
    report = tracker.get_status_report()
    tracker.print_status_table(report)

    # Print execution order
    order = tracker.validate_execution_order()
    tracker.print_execution_order(order)

    # Critical path
    tracker.check_critical_path()

    # Save JSON report
    with open("/home/runner/work/ThemisDB/ThemisDB/ai_working/WAVE_A_DEPENDENCY_STATUS.json", "w") as f:
        json.dump(report, f, indent=2)
    print(f"\n✅ Report saved to: ai_working/WAVE_A_DEPENDENCY_STATUS.json")

    # Check for unblocked modules ready to start
    print("\n" + "=" * 120)
    print("ACTIONABLE MODULES (ready to start this week)".center(120))
    print("=" * 120)
    ready = [m for m, info in report["modules"].items() if info["status"] == "ready"]
    if ready:
        for module in ready:
            print(f"✅ {module}")
    else:
        print("⏳ No modules ready yet. Check back Monday.")
    print("=" * 120)


if __name__ == "__main__":
    main()
