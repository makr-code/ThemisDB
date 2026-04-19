> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# DocumentManager Module

## Overview

DocumentManager handles document upload, text extraction, chunking, embedding, and graph construction. It integrates with VectorIndexManager and GraphIndexManager for advanced document search capabilities.

## Location History

This module was previously located in the `document` module and has been moved to the `projects` module to better reflect its nature as a standalone component that can be developed independently of ThemisDB core.

## Key Features

- Document upload and storage
- Text extraction from various formats (plain text, PDF, etc.)
- Text chunking with configurable overlap
- Embedding generation for semantic search
- Graph construction for document relationships
- Integration with vector and graph indexes

## Usage

```cpp
#include "projects/DocumentManager/document_manager.h"

// Create DocumentManager instance
auto doc_manager = std::make_shared<themis::projects::DocumentManager>(
    storage,
    vector_index,
    graph_index
);

// Upload a document
auto result = doc_manager->uploadDocument(
    blob,
    "text/plain",
    "example.txt",
    std::nullopt,  // text (optional)
    nlohmann::json::object(),  // metadata
    true  // store_blob
);

if (result.ok) {
    std::cout << "Document uploaded with ID: " << result.doc_id << std::endl;
    std::cout << "Created " << result.chunks_created << " chunks" << std::endl;
}
```

## API Reference

See the header file for detailed API documentation: `document_manager.h`

## Migration from document module

If you're migrating from the old `document` module:

1. Update include path:
   - Old: `#include "document/document_manager.h"`
   - New: `#include "projects/DocumentManager/document_manager.h"`

2. Update namespace:
   - Old: `themis::document::DocumentManager`
   - New: `themis::projects::DocumentManager`

For temporary backward compatibility, you can use `#include "document/document_manager_deprecated.h"` which provides namespace aliases.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
