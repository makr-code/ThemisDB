# PostgreSQL Wire Protocol Implementation Guide

## Overview

ThemisDB implements the PostgreSQL wire protocol for BI tool compatibility, allowing connections from tools like Tableau, Metabase, psql, and JDBC/ODBC clients.

## Implementation Status

### ✅ Implemented Features

#### Part 1: Prepared Statements
- **Parse Message Handler**: Validates queries and stores prepared statements with parameter types
- **Bind Message Handler**: Binds parameters to prepared statements with validation
- **Execute Message Handler**: Executes portals with bound parameters and parameter substitution
- **Describe Message Handler**: Returns metadata for statements (ParameterDescription + RowDescription) and portals (RowDescription)
- **Close Message Handler**: Deallocates prepared statements and portals with proper cleanup

#### Part 2: Extended Query Protocol
- **Parameter Format Support**: Parses parameter format codes (text/binary) in Bind message
- **Transaction State Tracking**: Tracks IDLE, IN_TRANSACTION, and FAILED states
- **Transaction Control**: BEGIN, COMMIT, ROLLBACK with proper state transitions
- **Error Recovery**: Failed transactions properly tracked and recovered with ROLLBACK
- **ReadyForQuery Messages**: Sends correct transaction status ('I', 'T', 'E')

#### Part 3: Protocol Messages
- **ParameterDescription ('t')**: Describes parameter types for prepared statements
- **NoData ('n')**: Indicates queries that don't return result sets
- **CloseComplete ('3')**: Confirms statement/portal closure
- **Improved RowDescription ('T')**: Properly encodes all field metadata (OIDs, sizes, format codes)

#### Part 4: BI Tool Compatibility
- **Schema Introspection**: Support for pg_catalog and information_schema queries
- **PostgreSQL Functions**: version(), current_database()
- **Type OID Mappings**: Common PostgreSQL type OIDs (int4=23, text=25, etc.)

### 🚧 Partially Implemented

- **Binary Format**: Parsing logic exists but not fully tested
- **Result Streaming**: Basic implementation, needs maxRows handling
- **SQL-to-Cypher Translation**: Core functionality exists, needs database integration

### ❌ Not Yet Implemented

- **COPY Protocol**: Bulk data transfer (COPY IN/OUT)
- **PortalSuspended**: Partial result set support
- **Binary Result Encoding**: Results are currently text-only
- **Advanced Type OIDs**: Only basic types implemented

## Protocol Flow Examples

### Simple Query (Q Message)

```
Client:                Server:
-------                -------
Q: "SELECT * FROM users"
                       RowDescription
                       DataRow (x N)
                       CommandComplete
                       ReadyForQuery('I')
```

### Extended Query (Parse/Bind/Execute)

```
Client:                Server:
-------                -------
P: stmt="s1", query="SELECT * FROM users WHERE id=$1", paramTypes=[23]
                       ParseComplete
D: type='S', name="s1"
                       ParameterDescription([23])
                       RowDescription([...])
B: portal="", stmt="s1", params=["123"]
                       BindComplete
E: portal="", maxRows=0
                       DataRow (x N)
                       CommandComplete
S: Sync
                       ReadyForQuery('I')
```

### Transaction Flow

```
Client:                Server:
-------                -------
Q: "BEGIN"
                       CommandComplete("BEGIN")
                       ReadyForQuery('T')
Q: "INSERT INTO users VALUES (1, 'Alice')"
                       CommandComplete("INSERT 0 1")
                       ReadyForQuery('T')
Q: "COMMIT"
                       CommandComplete("COMMIT")
                       ReadyForQuery('I')
```

### Error Recovery

```
Client:                Server:
-------                -------
Q: "BEGIN"
                       CommandComplete("BEGIN")
                       ReadyForQuery('T')
Q: "SELECT * FROM non_existent"
                       ErrorResponse(ERROR, 42P01, "relation does not exist")
                       ReadyForQuery('E')
Q: "INSERT ..."       
                       ErrorResponse(WARNING, 25P02, "current transaction is aborted")
                       ReadyForQuery('E')
Q: "ROLLBACK"
                       CommandComplete("ROLLBACK")
                       ReadyForQuery('I')
```

## Message Reference

### Client Messages

| Type | Name | Description |
|------|------|-------------|
| 'Q' | Query | Simple query protocol |
| 'P' | Parse | Parse SQL statement into prepared statement |
| 'B' | Bind | Bind parameters to prepared statement |
| 'E' | Execute | Execute portal |
| 'D' | Describe | Request description of statement or portal |
| 'C' | Close | Close statement or portal |
| 'S' | Sync | Synchronize extended query protocol |
| 'X' | Terminate | Close connection |

### Server Messages

| Type | Name | Description |
|------|------|-------------|
| 'R' | Authentication | Authentication request/response |
| 'S' | ParameterStatus | Runtime parameter status |
| 'K' | BackendKeyData | Cancellation key data |
| 'Z' | ReadyForQuery | Server ready for new query |
| 'T' | RowDescription | Result set column metadata |
| 'D' | DataRow | Result set data row |
| 'C' | CommandComplete | Query completion tag |
| '1' | ParseComplete | Parse operation completed |
| '2' | BindComplete | Bind operation completed |
| '3' | CloseComplete | Close operation completed |
| 't' | ParameterDescription | Parameter type metadata |
| 'n' | NoData | Query returns no data |
| 'E' | ErrorResponse | Error message |

## Type OID Reference

Common PostgreSQL type OIDs used in ParameterDescription and RowDescription:

| Type | OID | Size | Description |
|------|-----|------|-------------|
| bool | 16 | 1 | Boolean |
| int2 | 21 | 2 | 16-bit integer |
| int4 | 23 | 4 | 32-bit integer |
| int8 | 20 | 8 | 64-bit integer |
| float4 | 700 | 4 | Single precision float |
| float8 | 701 | 8 | Double precision float |
| text | 25 | -1 | Variable length text |
| varchar | 1043 | -1 | Variable length varchar |
| timestamp | 1114 | 8 | Timestamp without timezone |
| date | 1082 | 4 | Date |

## Transaction State Machine

```
        ┌──────┐
        │ IDLE │ <──────────────────┐
        └──┬───┘                     │
           │                         │
       BEGIN                     COMMIT/
           │                     ROLLBACK
           v                         │
    ┌──────────────┐                │
    │IN_TRANSACTION│ ───────────────┘
    └──────┬───────┘
           │
        ERROR
           │
           v
      ┌────────┐
      │ FAILED │
      └────┬───┘
           │
       ROLLBACK
           │
           └──────────────> [IDLE]
```

## Error Codes

Common PostgreSQL error codes (SQLSTATE):

| Code | Category | Description |
|------|----------|-------------|
| 08P01 | Connection Exception | Protocol Violation |
| 25P01 | Invalid Transaction | No Active Transaction |
| 25P02 | Invalid Transaction | In Failed Transaction |
| 26000 | Invalid Statement | Statement Not Found |
| 34000 | Invalid Cursor | Portal Not Found |
| 42601 | Syntax Error | Syntax Error or Access Rule Violation |
| 42P01 | Syntax Error | Undefined Table |
| XX000 | Internal Error | Internal Error |

## Testing

### Unit Tests

- `test_postgres_wire.cpp`: Basic protocol and SQL translation tests
- `test_postgres_prepared_statements.cpp`: Prepared statement lifecycle tests
- `test_postgres_transactions.cpp`: Transaction state and control tests

### Integration Tests

To test with psql:

```bash
# Enable PostgreSQL wire protocol in build
cmake -DTHEMIS_ENABLE_POSTGRES_WIRE=ON ..
make

# Start ThemisDB with PostgreSQL port
./themis-server --postgres-port 5432

# Connect with psql
psql -h localhost -p 5432 -U themis -d themisdb
```

Example queries:

```sql
-- Simple query
SELECT version();

-- Prepared statement
PREPARE get_user (int) AS SELECT * FROM users WHERE id = $1;
EXECUTE get_user(123);

-- Transaction
BEGIN;
INSERT INTO users VALUES (1, 'Alice');
COMMIT;
```

## BI Tool Compatibility

### Tableau

Tableau uses extended query protocol extensively:
1. Connects and requests version()
2. Queries pg_catalog for schema metadata
3. Uses prepared statements for parameterized queries
4. Requires proper transaction handling

### Metabase

Metabase requirements:
1. Basic schema introspection (information_schema)
2. Simple query protocol for ad-hoc queries
3. Transaction support for data modifications

### DBeaver

DBeaver SQL IDE:
1. Full pg_catalog support for schema browser
2. Prepared statements for SQL execution
3. Transaction control in GUI

## Troubleshooting

### Common Issues

**Issue**: Client reports "protocol version not supported"
- **Solution**: Ensure server sends protocol version 3.0 (196608) in startup

**Issue**: BI tool can't see tables
- **Solution**: Implement pg_class and information_schema.tables queries

**Issue**: Prepared statements fail
- **Solution**: Check parameter count matches in Parse and Bind

**Issue**: Transaction state errors
- **Solution**: Verify ReadyForQuery sends correct status after each command

## Future Enhancements

### Priority 1: Production Ready
- [ ] Integrate query execution with actual database
- [ ] Add comprehensive error handling
- [ ] Implement result set streaming with maxRows
- [ ] Add connection pooling

### Priority 2: Advanced Features
- [ ] COPY protocol for bulk imports
- [ ] Binary result format
- [ ] Extended type OID support
- [ ] Cursor support (DECLARE, FETCH)

### Priority 3: Optimization
- [ ] Query plan caching
- [ ] Parallel query execution
- [ ] Result set compression
- [ ] Connection multiplexing

## References

- [PostgreSQL Frontend/Backend Protocol](https://www.postgresql.org/docs/current/protocol.html)
- [PostgreSQL Wire Protocol Documentation](https://www.postgresql.org/docs/current/protocol-message-formats.html)
- [PostgreSQL Error Codes](https://www.postgresql.org/docs/current/errcodes-appendix.html)
