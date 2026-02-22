#!/usr/bin/env python3
"""
Validate all issues have correct label configuration
"""
import os
import json
from urllib.request import Request, urlopen

GITHUB_API = "https://api.github.com"
GITHUB_TOKEN = os.environ.get("GITHUB_TOKEN")
GITHUB_REPOSITORY = os.environ.get("GITHUB_REPOSITORY")

REQUIRED_TYPES = {"type:feature", "type:bug", "type:enhancement", "type:test", "type:documentation", "type:refactor", "type:chore"}
REQUIRED_PRIORITIES = {"priority:critical", "priority:high", "priority:medium", "priority:low"}
REQUIRED_STATUS = {"status:open", "status:in_progress", "status:blocked", "status:review", "status:ready"}
REQUIRED_AREAS = {"area:core", "area:aql", "area:query", "area:acceleration", "area:storage", "area:vector", "area:graph", "area:rag", "area:infrastructure"}

def get_all_issues():
    """Fetch all open issues/PRs"""
    url = f"{GITHUB_API}/repos/{GITHUB_REPOSITORY}/issues?state=open&per_page=100"
    headers = {
        "Accept": "application/vnd.github+json",
        "Authorization": f"Bearer {GITHUB_TOKEN}",
        "X-GitHub-Api-Version": "2022-11-28"
    }
    
    try:
        request = Request(url, headers=headers)
        response = urlopen(request)
        return json.loads(response.read().decode())
    except Exception as e:
        print(f"❌ Error fetching issues: {e}")
        return []

def validate_issue(issue):
    """Validate a single issue's labels"""
    labels = set([label["name"] for label in issue.get("labels", [])])
    violations = []
    
    # Check type
    has_type = labels & REQUIRED_TYPES
    if not has_type:
        violations.append("Missing type:*")
    elif len(has_type) > 1:
        violations.append("Multiple type:* labels")
    
    # Check priority
    has_priority = labels & REQUIRED_PRIORITIES
    if not has_priority:
        violations.append("Missing priority:*")
    elif len(has_priority) > 1:
        violations.append("Multiple priority:* labels")
    
    # Check status
    has_status = labels & REQUIRED_STATUS
    if not has_status:
        violations.append("Missing status:*")
    elif len(has_status) > 1:
        violations.append("Multiple status:* labels")
    
    # Check area
    has_area = labels & REQUIRED_AREAS
    if not has_area:
        violations.append("Missing area:*")
    
    return violations

def main():
    print("\n" + "="*70)
    print("VALIDATING LABEL CONFIGURATION")
    print("="*70 + "\n")
    
    issues = get_all_issues()
    violations_found = 0
    
    for issue in issues:
        violations = validate_issue(issue)
        if violations:
            violations_found += 1
            print(f"❌ Issue #{issue['number']}: {', '.join(violations)}")
    
    print(f"\n📊 Result: {len(issues) - violations_found}/{len(issues)} issues compliant")
    
    if violations_found > 0:
        print(f"⚠️  {violations_found} issues need remediation")
    else:
        print("✅ All issues are compliant!")
    
    print("="*70 + "\n")

if __name__ == "__main__":
    main()