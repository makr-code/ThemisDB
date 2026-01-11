# GitHub Labels Quick Reference

> Quick guide for using labels in ThemisDB issues and pull requests.
> 
> **Full Guide:** See [LABELS_GUIDE.md](LABELS_GUIDE.md) for complete documentation.

## 🎯 Most Common Labels

### Priority (Choose ONE)
- `priority:P0` 🔴 - Critical, blocks release
- `priority:P1` 🟠 - High, important for next release
- `priority:P2` 🟡 - Medium, future release
- `priority:P3` 🟢 - Low, backlog

### Type (Choose ONE)
- `type:bug` - Something broken
- `type:feature` - New functionality
- `type:enhancement` - Improve existing feature
- `type:documentation` - Docs changes
- `type:security` - Security issue
- `type:performance` - Performance improvement

### Area (Choose MULTIPLE if needed)
- `area:llm` - LLM/AI features
- `area:storage` - Storage layer
- `area:aql` - Query language
- `area:api` - REST API
- `area:sharding` - Distributed systems
- `area:security` - Auth/encryption
- `area:docker` - Containers
- `area:ci-cd` - Workflows
- `area:docs` - Documentation

### Status
- `status:ready` - Ready to work on
- `status:in-progress` - Being worked on
- `status:needs-review` - Needs review
- `status:needs-info` - Needs more info
- `status:blocked` - Blocked by dependency

### Effort
- `effort:small` - < 1 day
- `effort:medium` - 1-3 days
- `effort:large` - 1-2 weeks
- `effort:x-large` - > 2 weeks

### For Contributors
- `good first issue` - Good for newcomers
- `help wanted` - Looking for contributors
- `mentor available` - Maintainer will help

## 📝 Label Examples

### Critical Bug
```
priority:P0
type:bug
area:storage
status:in-progress
```

### Feature Request
```
priority:P2
type:feature
area:llm
area:api
effort:large
help wanted
```

### Documentation Issue
```
priority:P3
type:documentation
area:docs
effort:small
good first issue
```

### Security Vulnerability
```
priority:P0
type:security
area:api
area:auth
status:ready
```

## 🔧 Managing Labels

### For Issue Reporters
- Don't worry about labels - maintainers will add them
- Focus on clear description and reproduction steps

### For Contributors
- Check labels to understand priority and scope
- Look for `good first issue` if you're new
- Add `status:in-progress` when you start work

### For Maintainers
- Add priority, type, and area labels when triaging
- Add effort estimate if possible
- Add `good first issue` for beginner-friendly issues
- Keep status labels up to date

## 📚 Resources

- **Full Guide:** [LABELS_GUIDE.md](LABELS_GUIDE.md)
- **Label Definitions:** [labels.yml](labels.yml)
- **Sync Script:** [scripts/sync-labels.py](scripts/sync-labels.py)
- **Contributing:** [../CONTRIBUTING.md](../CONTRIBUTING.md)

---

**Last Updated:** 2026-01-11
