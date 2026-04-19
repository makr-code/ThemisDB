# Branch API - OpenAPI Schema Addition

## Branch Endpoints to Add to openapi.yaml

Add these endpoints after the `/api/v1/diff/cache` section:

### Endpoints:
- `POST /api/v1/branches` - Create a new branch
- `GET /api/v1/branches` - List all branches
- `GET /api/v1/branches/active` - Get currently active branch
- `GET /api/v1/branches/stats` - Get branch statistics
- `POST /api/v1/branches/merge` - Merge branches
- `GET /api/v1/branches/{name}` - Get branch metadata
- `DELETE /api/v1/branches/{name}` - Delete a branch
- `POST /api/v1/branches/{name}/switch` - Switch to a branch

### Schema to Add:

```yaml
Branch:
  type: object
  properties:
    branch_name:
      type: string
      example: 'feature/new-schema'
    parent_branch:
      type: string
      example: 'main'
    creation_sequence:
      type: integer
      format: int64
      example: 12345
    creation_timestamp_ms:
      type: integer
      format: int64
      example: 1736657231000
    description:
      type: string
      example: 'Branch for testing new user schema'
    created_by:
      type: string
      example: 'alice'
    is_active:
      type: boolean
      example: false
  required:
    - branch_name
    - parent_branch
    - creation_sequence
    - creation_timestamp_ms
    - description
    - created_by
    - is_active
```

### Tags:
All branch endpoints use tags: `[mvcc, branches]`

## Implementation Status
✅ REST API implementation complete in `src/server/branch_api_handler.cpp`
✅ All endpoints registered in HTTP server
✅ Full documentation in `docs/en/features/features_branches.md` and `docs/de/features/features_branches.md`

## Next Steps
Update `openapi/openapi.yaml` to include the complete Branch API specification with request/response schemas.
