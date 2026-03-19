# Workflow Migration Runbook

## Phase 1: Pre-Migration Checklist
1. Review all existing workflows for dependencies.
2. Update documentation to reflect the new directory structure.
3. Communicate changes to all team members involved.
4. Backup current workflows to ensure data integrity.

## Phase 2: Migration Steps
1. Create a new hierarchical directory structure within the repository.
   - Example:
     - `.github/workflows/project1/`
     - `.github/workflows/project2/`

2. Move existing workflow YAML files to their respective directories.

3. Update references in any documentation that points to the old flat structure.

## Phase 3: Testing Protocol
1. After the migration, validate the workflows:
   - Use CI tools to ensure workflows trigger correctly.
   - Perform tests to confirm that all workflows run as expected.

2. Document any issues that arise during testing for further review.

## Phase 4: Rollback Procedures
1. If critical errors occur post-migration, revert to the backup of the workflows.
2. Assess errors and determine if paths need to be adjusted or if any workflows need to be fixed before retrying the migration.

## Phase 5: Troubleshooting Guide
- Common issues include:
   - Workflows not triggering
   - Incorrect paths in workflow YAML files
   - Dependencies that were not properly addressed during migration

- For each common issue, provide potential solutions, such as:
   - Check the event triggers in the workflow are accurate.
   - Confirm that all required environment variables are set.
   - Review the logs for clear error messages to identify problems.

## Conclusion
Ensure thorough testing and backtrack procedures are in place before finalizing the migration process.