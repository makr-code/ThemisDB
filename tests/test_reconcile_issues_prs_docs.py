import importlib.util
from pathlib import Path


_SCRIPT = Path(__file__).parent.parent / "scripts" / "reconcile-issues-prs-docs.py"


def _load_module():
    spec = importlib.util.spec_from_file_location("reconcile_issues_prs_docs", _SCRIPT)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


mod = _load_module()


def test_linked_open_issue_milestones_ignores_closed_links():
    linked_issue_numbers = [101, 202, 303]
    issue_milestone_by_number = {
        101: "v1.4.0",
        202: "v1.4.0",
        303: "v1.3.1",
    }
    issue_state_by_number = {
        101: "open",
        202: "closed",
        303: "open",
    }

    assert mod.linked_open_issue_milestones(
        linked_issue_numbers,
        issue_milestone_by_number,
        issue_state_by_number,
    ) == ["v1.4.0", "v1.3.1"]


def test_pr_alignment_ignores_closed_prs_and_closed_linked_issues():
    prs = [
        {
            "number": 500,
            "title": "open pr",
            "state": "open",
            "milestone": {"title": "v1.4.0"},
            "closingIssuesReferences": [{"number": 101}, {"number": 202}],
        },
        {
            "number": 501,
            "title": "closed pr",
            "state": "closed",
            "milestone": {"title": "v1.4.0"},
            "closingIssuesReferences": [{"number": 101}],
        },
    ]
    issue_milestone_by_number = {
        101: "v1.4.0",
        202: "v1.4.0",
    }
    issue_state_by_number = {
        101: "open",
        202: "closed",
    }

    proposals = []
    for pr in prs:
        pr_num = int(pr.get("number"))
        pr_state = str(pr.get("state") or "").lower()
        if pr_state != "open":
            continue

        current_pr_milestone = ((pr.get("milestone") or {}) or {}).get("title")
        linked_issues = [
            int(r["number"]) for r in (pr.get("closingIssuesReferences") or [])
            if isinstance(r.get("number"), int)
        ]
        linked_issue_milestones = mod.linked_open_issue_milestones(
            linked_issues,
            issue_milestone_by_number,
            issue_state_by_number,
        )
        if not linked_issue_milestones:
            continue

        counts = mod.Counter(linked_issue_milestones)
        suggested_pr_milestone, _ = counts.most_common(1)[0]
        if current_pr_milestone != suggested_pr_milestone:
            proposals.append({
                "pr_number": pr_num,
                "suggested_pr_milestone": suggested_pr_milestone,
            })

    assert proposals == []
