> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# ThemisDB Third-Party Documentation Database Builder

## Overview

The **ThemisDB Documentation Database Builder** is a standalone C++ tool that allows administrators to create compiled documentation databases from custom sources. These databases are fully compatible with ThemisDB's LLM-based documentation assistant and can be used alongside the official ThemisDB documentation.

## Features

- ✅ **Standalone Compilation**: No ThemisDB dependencies required
- ✅ **Multiple Input Formats**: Markdown, HTML, plain text, JSON
- ✅ **Full Model Support**: Generates all 7 Column Families (relational, graph, vector, metadata, :document)
- ✅ **Batch Processing**: Efficiently process thousands of documents
- ✅ **Incremental Updates**: Update existing databases without full rebuild
- ✅ **Validation**: Built-in checks for database integrity
- ✅ **Security**: Read-only mode by default, path validation, size limits
- ✅ **Configuration**: YAML config files for repeatable builds
- ✅ **Namespace Isolation**: Multi-tenant support for multiple documentation sources

## Use Cases

1. **Internal Documentation**: Company-specific docs alongside ThemisDB docs
2. **API Reference**: Custom API documentation for integrations
3. **Runbooks**: Operational procedures and troubleshooting guides
4. **Training Materials**: Educational content for users
5. **Change Logs**: Version history and release notes
6. **Compliance Documents**: Policy and regulatory documentation

## Installation

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.15+
- RocksDB 6.0+
- yaml-cpp (optional, for YAML config support)

### Build from Source

```bash
cd tools/themis_docs_builder
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install  # Optional: install to /usr/local/bin
```

### Verify Installation

```bash
./themis_docs_builder --version
# Output: ThemisDB Documentation Builder v1.0.0
```

## Quick Start

### Basic Usage

```bash
# Build database from Markdown files
./themis_docs_builder \
  --input /path/to/docs \
  --output /var/lib/themisdb/custom_docs.db \
  --format markdown \
  --namespace "mycompany" \
  --read-only
```

### With Configuration File

```bash
# Create configuration file
cat > my_docs.yaml <<EOF
input:
  paths:
    - /path/to/docs
    - /path/to/api_reference
  formats:
    - markdown
    - html
  recursive: true

output:
  path: /var/lib/themisdb/custom_docs.db
  read_only: true
  namespace: "mycompany"

metadata:
  title: "MyCompany Documentation"
  version: "1.0.0"
EOF

# Build with config
./themis_docs_builder --config my_docs.yaml
```

### Incremental Update

```bash
# Add new documents to existing database
./themis_docs_builder \
  --input /path/to/new_docs \
  --output /var/lib/themisdb/custom_docs.db \
  --mode incremental \
  --namespace "mycompany"
```

## Configuration Reference

### YAML Configuration File

```yaml
# Input configuration
input:
  paths:
    - /path/to/docs            # Input directory or file
    - /path/to/api_reference   # Multiple paths supported
  formats:
    - markdown                 # Supported: markdown, html, text, json
    - html
  recursive: true              # Scan subdirectories
  exclude_patterns:
    - "*.tmp"
    - "draft_*"
    - ".git"

# Output configuration
output:
  path: /var/lib/themisdb/custom_docs.db
  read_only: true              # Open in read-only mode (recommended)
  namespace: "mycompany"       # Namespace for document isolation

# Processing options
processing:
  batch_size: 100              # Documents per batch
  max_content_size: 10485760   # Max content size (10 MB)
  generate_embeddings: false   # Requires vector model (future)
  validate: true               # Validate output database
  deduplication: true          # Remove duplicate documents

# Metadata
metadata:
  title: "MyCompany Documentation"
  version: "1.0.0"
  author: "MyCompany IT Team"
  description: "Internal documentation database"
  contact: "it@mycompany.com"
  license: "Internal Use Only"
```

## CLI Reference

### Command Line Options

```
themis_docs_builder - ThemisDB Third-Party Documentation Database Builder

USAGE:
  themis_docs_builder [OPTIONS]

OPTIONS:
  --input PATH              Input directory or file
  --output PATH             Output database path
  --config PATH             Configuration file (YAML)
  --format FORMAT           Input format: markdown|html|text|json
  --namespace NAME          Namespace for document isolation
  --mode MODE               Build mode: full|incremental (default: full)
  --read-only              Enable read-only mode (default: true)
  --validate               Validate output database
  --batch-size N           Documents per batch (default: 100)
  --max-size N             Max content size in bytes (default: 10MB)
  --recursive              Scan subdirectories
  --exclude PATTERN        Exclude files matching pattern (can be repeated)
  --help                   Show this help message
  --version                Show version information
  --verbose                Enable verbose logging
  --quiet                  Suppress non-error output
```

### Examples

**1. Simple Markdown Build**
```bash
./themis_docs_builder \
  --input ./docs \
  --output custom_docs.db \
  --format markdown \
  --namespace "myapp"
```

**2. Multiple Formats**
```bash
./themis_docs_builder \
  --input ./docs \
  --output custom_docs.db \
  --format markdown \
  --format html \
  --namespace "myapp"
```

**3. With Exclusions**
```bash
./themis_docs_builder \
  --input ./docs \
  --output custom_docs.db \
  --format markdown \
  --recursive \
  --exclude "*.draft" \
  --exclude "temp_*" \
  --namespace "myapp"
```

**4. Incremental Update**
```bash
./themis_docs_builder \
  --input ./new_docs \
  --output custom_docs.db \
  --mode incremental \
  --namespace "myapp"
```

**5. Validation Only**
```bash
./themis_docs_builder \
  --output custom_docs.db \
  --validate
```

## Input Formats

### Markdown (.md, .markdown)

Automatically extracts:
- Title from first `#` heading or filename
- Metadata from YAML frontmatter
- Content from body
- Links for graph relationships

**Example:**
```markdown
---
title: "API Authentication"
category: "Security"
tags: ["auth", "api", "security"]
---

# API Authentication

This document describes...
```

### HTML (.html, .htm)

Extracts:
- Title from `<title>` or `<h1>` tag
- Metadata from `<meta>` tags
- Content from `<body>` (stripped of tags)
- Links from `<a href>` for graph relationships

### Plain Text (.txt)

Simple extraction:
- Title from filename
- Content as-is
- No automatic link detection

### JSON (.json)

Structured format for maximum control:

```json
{
  "documents": [
    {
      "id": "doc001",
      "title": "Getting Started",
      "content": "Full document content...",
      "metadata": {
        "author": "Admin",
        "category": "Tutorial",
        "tags": ["intro", "setup"]
      },
      "links": ["doc002", "doc003"]
    }
  ]
}
```

## Database Structure

The tool generates a standard ThemisDB-compatible RocksDB database with 7 Column Families:

- **CF 0 (default)**: Standard RocksDB default CF
- **CF 1 (relational)**: Document records with metadata (truncated to 5000 chars)
- **CF 2 (graph_nodes)**: Document nodes for graph traversal
- **CF 3 (graph_edges)**: Relationships between documents
- **CF 4 (vector)**: Embeddings for semantic search (placeholder)
- **CF 5 (metadata)**: Database version and statistics
- **CF 6 (document)**: Native `:document` collection with full content

### Key Format

All keys are prefixed with the namespace to enable multi-database support:

```
Format: "namespace:collection:id"
Example: "mycompany:document:abc123"
```

This allows ThemisDB to:
- Query multiple databases simultaneously
- Filter by source/namespace
- Prioritize official vs. third-party documentation
- Maintain security boundaries

## Integration with ThemisDB

### Configuration

Add your custom database to ThemisDB's configuration:

```yaml
# config/docs_assistant.yaml
docs_assistant:
  enabled: true
  databases:
    # Official ThemisDB documentation
    - name: "themisdb_official"
      path: "/var/lib/themisdb/docs.db"
      type: "rocksdb"
      read_only: true
      namespace: "themisdb"
      priority: 1
    
    # Your custom documentation
    - name: "mycompany_internal"
      path: "/var/lib/themisdb/custom_docs.db"
      type: "rocksdb"
      read_only: true
      namespace: "mycompany"
      priority: 2
```

### Query Examples

```aql
-- Query both databases
RETURN DOCS_QUERY('How to configure authentication?')
-- Returns results from both themisdb and mycompany namespaces

-- Filter by namespace
RETURN DOCS_SEARCH('authentication', 10, 'mycompany')
-- Returns only mycompany documentation

-- Query specific :document collection
FOR doc IN :document
  FILTER doc.namespace == 'mycompany'
  FILTER doc.type == 'documentation'
  FILTER CONTAINS(doc.title, 'API')
  RETURN doc
```

## Security Considerations

### Default Security Features

- ✅ **Read-Only Mode**: Output databases open in read-only mode by default
- ✅ **Path Validation**: Prevents directory traversal attacks
- ✅ **Size Limits**: Configurable max file size to prevent DoS
- ✅ **Input Sanitization**: Metadata fields are sanitized
- ✅ **Namespace Isolation**: Multi-tenant support with strict boundaries

### Best Practices

1. **Use Explicit Paths**: Avoid relative paths in production
2. **Set Size Limits**: Configure `max_content_size` appropriately
3. **Enable Validation**: Always validate generated databases
4. **Namespace Convention**: Use reverse domain notation (e.g., `com.mycompany`)
5. **Read-Only Mode**: Keep databases read-only for security
6. **Regular Updates**: Rebuild databases periodically with fresh content
7. **Access Control**: Restrict write access to database directories

### Security Review Required

Before deploying third-party databases in production:

- ⚠️ Review input sources for sensitive information
- ⚠️ Validate namespace isolation is enforced
- ⚠️ Test with security scanning tools
- ⚠️ Implement audit logging for database access
- ⚠️ Follow principle of least privilege for file permissions

See `docs/en/security/DOCS_ASSISTANT_SECURITY_ASSESSMENT.md` for detailed security guidance.

## Troubleshooting

### Common Issues

**1. "RocksDB error: Corruption"**
- **Cause**: Database file corrupted or incomplete
- **Solution**: Delete and rebuild database

**2. "Permission denied"**
- **Cause**: Insufficient permissions for output path
- **Solution**: Check file/directory permissions

**3. "Namespace collision"**
- **Cause**: Documents from different sources use same namespace
- **Solution**: Use unique namespaces per source

**4. "Max size exceeded"**
- **Cause**: Document exceeds `max_content_size`
- **Solution**: Increase limit or split large documents

**5. "Invalid input format"**
- **Cause**: Unsupported file format
- **Solution**: Check `--format` matches input files

### Validation Errors

Run validation to diagnose issues:

```bash
./themis_docs_builder --output custom_docs.db --validate
```

Checks performed:
- Column Family integrity
- Document count consistency
- Metadata validation
- Key format correctness
- Namespace isolation

### Verbose Logging

Enable detailed logging for debugging:

```bash
./themis_docs_builder --config my_docs.yaml --verbose
```

## Performance Tips

1. **Batch Size**: Adjust `--batch-size` based on available RAM
   - Small RAM: `--batch-size 50`
   - Large RAM: `--batch-size 500`

2. **Incremental Builds**: Use `--mode incremental` for updates
   - Faster than full rebuild
   - Only processes new/modified files

3. **Exclude Patterns**: Filter out unnecessary files
   - `--exclude "*.tmp"` `--exclude ".git"`
   - Reduces processing time

4. **Parallel Processing**: Process multiple sources separately
   ```bash
   ./themis_docs_builder --input source1 --output db1.db --namespace ns1 &
   ./themis_docs_builder --input source2 --output db2.db --namespace ns2 &
   wait
   ```

## Examples

### Example 1: Internal Company Documentation

```bash
./themis_docs_builder \
  --input /opt/company/docs \
  --output /var/lib/themisdb/company_docs.db \
  --format markdown \
  --format html \
  --recursive \
  --exclude "draft_*" \
  --exclude "*.tmp" \
  --namespace "com.mycompany" \
  --read-only \
  --validate
```

### Example 2: API Reference Documentation

```yaml
# api_docs.yaml
input:
  paths:
    - /opt/api/reference
  formats:
    - markdown
    - json
  recursive: true

output:
  path: /var/lib/themisdb/api_docs.db
  namespace: "com.mycompany.api"

metadata:
  title: "MyCompany API Reference"
  version: "2.0.0"
```

```bash
./themis_docs_builder --config api_docs.yaml
```

### Example 3: Runbook Database

```bash
./themis_docs_builder \
  --input /opt/runbooks \
  --output /var/lib/themisdb/runbooks.db \
  --format markdown \
  --namespace "com.mycompany.ops" \
  --batch-size 50
```

## License

This tool is part of the ThemisDB project and follows the same license terms.

## Support

For issues, questions, or contributions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://themisdb.org/docs
- Security: security@themisdb.org

## Version History

- **v1.0.0** (2026-01-11): Initial release
  - Markdown, HTML, text, JSON support
  - Full multi-model database generation
  - Namespace isolation
  - Security hardening
