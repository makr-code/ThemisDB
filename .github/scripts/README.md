# GitHub Scripts

This directory contains utility scripts for managing GitHub repository configuration.

## Available Scripts

### sync-labels.py / sync-labels.sh

Synchronizes GitHub labels from `.github/labels.yml` to the repository.

**Prerequisites:**
- GitHub CLI (`gh`) installed and authenticated
- Python 3.x with PyYAML (`pip install pyyaml`) for Python script
- Write access to the repository

**Usage:**

```bash
# Dry-run mode (shows what would be done, no changes made)
python3 sync-labels.py
# or
./sync-labels.sh

# Apply changes to GitHub
python3 sync-labels.py --apply
# or
./sync-labels.sh --apply

# Delete all existing labels and recreate (dangerous!)
python3 sync-labels.py --delete-existing --apply
```

**Recommended Workflow:**

1. Edit `.github/labels.yml` with your desired labels
2. Run in dry-run mode to preview changes: `python3 sync-labels.py`
3. Review the output carefully
4. Apply changes: `python3 sync-labels.py --apply`

**Note:** The Python script is recommended as it's more portable and has better error handling.

## Label Management

For detailed information about the label system, see:
- `.github/LABELS_GUIDE.md` - Comprehensive guide to using labels
- `.github/labels.yml` - Label definitions

## GitHub CLI Authentication

If you haven't authenticated with GitHub CLI yet:

```bash
gh auth login
```

Follow the prompts to authenticate with your GitHub account.
