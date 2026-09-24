# MongoDB Adapter Phase 2 Implementation Plan

## Overview
Convert all 29 TODOs in `src/chimera/mongodb_adapter.cpp` to production-grade C++20 implementations.

## TODO Categories and Implementation Strategy

### Category 1: Connection Management (Line 69)
**TODO:** Actual `mongocxx::client` / `mongocxx::uri` creation
- ✅ **Status:** Already implemented in `connect()` method
- Uses `mongocxx::uri` with connection pooling
- Configures pool with max_pool_size=100, min_pool_size=10
- Validates connection by selecting database

### Category 2: Query Translation (Line 114)
**TODO:** Translate AQL → MongoDB aggregation pipeline and execute
- **Status:** NOT_IMPLEMENTED - Returns error
- **Strategy:** MongoDB doesn't natively support AQL; return NOT_IMPLEMENTED with guidance
- **Alternative:** User should use `find_documents()` for simple queries

### Category 3: Relational Operations
**TODOs:** Lines 139, 163, 237, 251

#### 139: Convert `RelationalRow` → BSON document and insert into collection
- **Method:** `insert_row(table_name, row)`
- **Implementation:**
  - Get collection from database
  - Convert RelationalRow to BSON document
  - Insert using collection.insert_one()
  - Return count of inserted rows (1)

#### 163: Batch insert documents via `bulk_write`
- **Method:** `batch_insert(table_name, rows)`
- **Implementation:**
  - Create bulk_write operation
  - Queue insert_one for each row
  - Execute bulk write
  - Return count of inserted rows

#### 237: Store node as document in nodes collection
- **Method:** `insert_node(node)`
- **Implementation:**
  - Get or create "nodes" collection
  - Create BSON document from GraphNode
  - Structure: { _id: node.id, label: node.label, properties: {...} }
  - Insert document
  - Return node.id

#### 251: Store edge as document with source/target node references
- **Method:** `insert_edge(edge)`
- **Implementation:**
  - Get or create "edges" collection
  - Create BSON document from GraphEdge
  - Structure: { _id: edge.id, source_id: edge.source_id, target_id: edge.target_id, label: edge.label, properties: {...}, weight: weight }
  - Insert document
  - Return edge.id

### Category 4: Document Operations
**TODOs:** Lines 311, 336, 361, 387

#### 311: Serialize doc to BSON and insert into named collection
- **Method:** `insert_document(collection, doc)`
- **Implementation:**
  - Get or create specified collection
  - Build BSON document from Document.fields
  - Add _id, timestamp fields
  - Insert using collection.insert_one()
  - Return document ID

#### 336: Batch insert BSON documents via `insert_many`
- **Method:** `batch_insert_documents(collection, docs)`
- **Implementation:**
  - Get specified collection
  - Convert all Document objects to BSON
  - Use collection.insert_many()
  - Return count of inserted documents

#### 361: Execute `find()` with BSON filter and limit, map to `Documents`
- **Method:** `find_documents(collection, filter, limit)`
- **Implementation:**
  - Get specified collection
  - Build BSON filter from filter map
  - Configure find options with limit
  - Execute find()
  - Convert each BSON document to Document
  - Return vector of Documents

#### 387: Execute `update_many()` with BSON filter and update document
- **Method:** `update_documents(collection, filter, updates)`
- **Implementation:**
  - Get specified collection
  - Build BSON filter and update document
  - Execute collection.update_many()
  - Return count of updated documents

### Category 5: Transaction Operations (Line 619)
**TODO:** Rollback-to-savepoint logic via `mongocxx` session

- **Method:** `rollback_to_savepoint(handle, savepoint_name)`
- **Implementation:**
  - MongoDB doesn't natively support savepoints
  - Use mongocxx session with snapshot isolation
  - Track savepoints in TransactionContext
  - On rollback, clear operations since savepoint
  - Return success/error

### Category 6: Helper Methods
**Additional TODOs in private helper methods:**

- `scalar_to_bson_value()` - NEW: Convert Scalar variant to BSON
- `bson_value_to_scalar()` - NEW: Convert BSON to Scalar variant
- `scalar_map_to_bson_document()` - NEW: Convert map to BSON document
- `parse_query_to_mongo()` - Return NOT_IMPLEMENTED with helpful message

## BSON Serialization Strategy

### Scalar to BSON Conversion
```
std::monostate       → bsoncxx::types::b_null{}
bool                 → bsoncxx::types::b_bool{value}
int64_t              → bsoncxx::types::b_int64{value}
double               → bsoncxx::types::b_double{value}
std::string          → bsoncxx::types::b_string{value}
std::vector<uint8_t> → bsoncxx::types::b_binary{value}
```

### Error Handling
- All operations return `Result<T>` with appropriate ErrorCode
- Use Result::ok(value) for success
- Use Result::err(ErrorCode, message) for errors
- No exceptions to propagate - catch and convert to Result

## Testing Strategy

### Unit Tests
1. Test each BSON conversion function independently
2. Test each database operation with mock data
3. Test error handling paths

### Integration Tests
1. Test full CRUD cycle
2. Test batch operations
3. Test transaction with savepoints
4. Test concurrent operations

## Documentation Requirements
- All new methods must have Doxygen documentation
- Include @param, @return, @details, @note sections
- Document thread-safety where applicable
- Document error codes that can be returned

## Build and Verification
1. Ensure code compiles with THEMIS_CHIMERA_MONGO=ON
2. Ensure graceful degradation with THEMIS_CHIMERA_MONGO=OFF
3. No compiler warnings
4. All existing tests pass
5. New tests pass

## Success Criteria
- ✅ All 29 TODOs implemented with production-grade code
- ✅ No stubs or mock implementations
- ✅ Comprehensive error handling with Result<T>
- ✅ Full Doxygen documentation
- ✅ Unit and integration tests
- ✅ Clean build with no warnings
- ✅ Backward compatible API
