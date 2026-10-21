import importlib.util
from pathlib import Path


_SCRIPT = Path(__file__).parent.parent / "scripts" / "reconcile-issues-prs-docs.py"


def _load_module():
    spec = importlib.util.spec_from_file_location("reconcile_issues_prs_docs", _SCRIPT)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod



def test_linked_open_issue_milestones_ignores_closed_links():
    mod = _load_module()
    linked_issue_numbers = [101, 202, 303, 404]
    issue_milestone_by_number = {
        101: "v1.4.0",
        202: "v1.9.9",
        303: "v1.3.1",
    }
    issue_state_by_number = {
        101: "open",
        202: "closed",
        303: "open",
        404: "open",
    }

    assert mod.linked_open_issue_milestones(
        linked_issue_numbers,
        issue_milestone_by_number,
        issue_state_by_number,
    ) == ["v1.4.0", "v1.3.1"]

    assert mod.linked_open_issue_milestones(
        [404],
        {},
        {404: "open"},
    ) == []


def test_pr_alignment_ignores_closed_prs_and_closed_linked_issues():
    mod = _load_module()
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
        {
            "number": 502,
            "title": "open pr with active mismatch",
            "state": "open",
            "milestone": {"title": "v1.3.0"},
            "closingIssuesReferences": [{"number": 203}],
        },
    ]
    issue_milestone_by_number = {
        101: "v1.4.0",
        202: "v1.4.0",
        203: "v1.4.0",
    }
    issue_state_by_number = {
        101: "open",
        202: "closed",
        203: "open",
    }

    proposals = mod.build_pr_alignment_proposals(
        prs,
        issue_milestone_by_number,
        issue_state_by_number,
    )

    assert proposals == [{
        "pr_number": 502,
        "pr_title": "open pr with active mismatch",
        "pr_state": "open",
        "current_pr_milestone": "v1.3.0",
        "suggested_pr_milestone": "v1.4.0",
        "confidence": "high",
        "reason": "all linked open issues share the same milestone",
        "linked_issues": [203],
        "linked_issue_milestone_counts": {"v1.4.0": 1},
    }]
