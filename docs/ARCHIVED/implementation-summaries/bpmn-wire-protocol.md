# BPMN Process Engine Wire Protocol Support

This document describes the BPMN process engine wire protocol and HTTP API support added to ThemisDB.

## Overview

ThemisDB now supports BPMN (Business Process Model and Notation) process execution through both wire protocol (binary TCP) and HTTP REST API endpoints. This allows clients to:

1. Start process instances
2. Complete user tasks
3. Query process instance state and history

## Wire Protocol Support

### Opcodes

Three new opcodes have been added to the ThemisDB wire protocol (v1):

- `0x60` - `BPMN_START_PROCESS`: Start a new process instance
- `0x61` - `BPMN_TASK_COMPLETE`: Complete a user task
- `0x62` - `BPMN_QUERY_INSTANCE`: Query process instance state

### Message Format

All BPMN messages use JSON payload format over the wire protocol frame:

#### Frame Header (12 bytes)
```
[MAGIC:4][VERSION:1][OPCODE:1][FLAGS:2][PAYLOAD_SIZE:4]
```

- MAGIC: 0x544D4442 ("TMDB")
- VERSION: 0x01
- OPCODE: 0x60, 0x61, or 0x62
- FLAGS: Standard flags (SKIP_CHECKSUM, COMPRESSED, ENCRYPTED)
- PAYLOAD_SIZE: Size of JSON payload in bytes

#### Start Process Request (Opcode 0x60)

```json
{
  "process_definition_key": "orderProcess",
  "variables": {
    "orderId": "123",
    "amount": 1000
  },
  "business_key": "order-123"
}
```

#### Start Process Response

```json
{
  "process_instance_id": "inst-abc123def456",
  "status": 0,
  "status_string": "RUNNING",
  "active_task_ids": ["inst-abc123def456:userTask1"]
}
```

Process Status Codes:
- `0` - RUNNING
- `1` - COMPLETED
- `2` - FAILED
- `3` - SUSPENDED
- `4` - TERMINATED

#### Task Complete Request (Opcode 0x61)

```json
{
  "task_id": "inst-abc123:userTask1",
  "variables": {
    "approved": true,
    "comment": "Looks good"
  },
  "assignee": "user@example.com"
}
```

Task ID format: `{instance_id}:{node_id}`

#### Task Complete Response

```json
{
  "success": true,
  "next_task_id": "inst-abc123:userTask2",
  "error": ""
}
```

#### Query Instance Request (Opcode 0x62)

```json
{
  "process_instance_id": "inst-abc123",
  "include_variables": true,
  "include_history": true
}
```

#### Query Instance Response

```json
{
  "status": 0,
  "active_tasks": [
    {
      "task_id": "inst-abc123:userTask1",
      "task_name": "userTask1",
      "task_type": "userTask",
      "assignee": "",
      "created_at_ns": 1234567890000000
    }
  ],
  "variables": {
    "orderId": "123",
    "amount": 1000
  },
  "history": [
    {
      "event_type": "node_visited",
      "timestamp_ns": 1234567890000000,
      "data": {
        "node_id": "startEvent1"
      }
    }
  ],
  "start_time_ns": 1234567890000000,
  "end_time_ns": 0
}
```

## HTTP REST API

### Endpoints

#### POST /api/v1/bpmn/process/start

Start a new process instance.

**Request Body:**
```json
{
  "process_definition_key": "orderProcess",
  "variables": {
    "orderId": "123",
    "amount": 1000
  },
  "business_key": "order-123"
}
```

**Response (200 OK):**
```json
{
  "process_instance_id": "inst-abc123",
  "status": 0,
  "status_string": "RUNNING",
  "active_task_ids": ["inst-abc123:userTask1"]
}
```

**Error Responses:**
- `400 Bad Request` - Missing or invalid parameters
- `401 Unauthorized` - Authentication required
- `500 Internal Server Error` - Process engine error
- `503 Service Unavailable` - Process engine not available

#### POST /api/v1/bpmn/task/:taskId/complete

Complete a user task and advance the process.

**URL Parameters:**
- `taskId` - Task identifier in format `{instance_id}:{node_id}`

**Request Body:**
```json
{
  "variables": {
    "approved": true,
    "comment": "Looks good"
  }
}
```

**Response (200 OK):**
```json
{
  "success": true,
  "next_task_id": "inst-abc123:userTask2",
  "error": ""
}
```

**Error Responses:**
- `400 Bad Request` - Invalid task ID format
- `401 Unauthorized` - Authentication required
- `404 Not Found` - Task or instance not found
- `500 Internal Server Error` - Task completion failed

#### GET /api/v1/bpmn/instance/:instanceId

Query the state of a process instance.

**URL Parameters:**
- `instanceId` - Process instance identifier

**Query Parameters:**
- `include_variables` - Include process variables in response (default: true)
- `include_history` - Include execution history in response (default: false)

**Response (200 OK):**
```json
{
  "status": 0,
  "active_tasks": [...],
  "variables": {...},
  "history": [...],
  "start_time_ns": 1234567890000000,
  "end_time_ns": 0
}
```

**Error Responses:**
- `401 Unauthorized` - Authentication required
- `404 Not Found` - Instance not found
- `503 Service Unavailable` - Process engine not available

## Authentication & Authorization

All BPMN operations require authentication:

1. **Wire Protocol**: Client must complete the authentication handshake (opcodes 0x03-0x06) before sending BPMN requests.

2. **HTTP API**: Client must provide a Bearer token in the Authorization header:
   ```
   Authorization: Bearer <token>
   ```

Authorization scopes:
- `bpmn:start` - Start process instances
- `bpmn:complete` - Complete tasks
- `bpmn:read` - Query instance state

## Implementation Details

### Server Components

1. **WireProtocolServer** (`src/network/wire_protocol_server.cpp`)
   - Handles binary wire protocol connections
   - Dispatches BPMN opcodes to handler methods
   - Integrates with ProcessGraphManager

2. **BpmnApiHandler** (`src/server/bpmn_api_handler.cpp`)
   - Handles HTTP REST API endpoints
   - Validates requests and authorization
   - Converts between HTTP and ProcessGraphManager data formats

3. **ProcessGraphManager** (`src/index/process_graph.cpp`)
   - Core BPMN execution engine
   - Manages process instances, tokens, and state
   - Supports BPMN and EPK process models

### Data Storage

Process data is stored in system collections:
- `_process_definitions` - Process model definitions
- `_process_nodes` - BPMN/EPK nodes
- `_process_edges` - Flow connections
- `_process_instances` - Running instances
- `_process_tokens` - Execution tokens
- `_process_history` - Audit trail

### Configuration

To enable BPMN support, the HTTP server must be initialized with a ProcessGraphManager instance:

```cpp
auto process_graph = std::make_shared<ProcessGraphManager>(*storage);
auto http_server = std::make_unique<HttpServer>(
    config, storage, secondary_index, graph_index, 
    vector_index, tx_manager, process_graph
);
```

The wire protocol server also requires a ProcessGraphManager:

```cpp
auto wire_server = std::make_shared<WireProtocolServer>(
    wire_config, storage, secondary_index, graph_index,
    vector_index, tx_manager, process_graph
);
```

## Example Usage

### Using Wire Protocol (TypeScript Client)

```typescript
import { ThemisClient } from '@themis/client';

const client = new ThemisClient({ host: 'localhost', port: 8766 });
await client.connect();
await client.authenticate('user', 'password');

// Start a process
const startResp = await client.bpmn.startProcess({
  process_definition_key: 'orderProcess',
  variables: { orderId: '123', amount: 1000 }
});

console.log('Started instance:', startResp.process_instance_id);

// Complete a task
const completeResp = await client.bpmn.completeTask({
  task_id: startResp.active_task_ids[0],
  variables: { approved: true }
});

// Query instance
const queryResp = await client.bpmn.queryInstance({
  process_instance_id: startResp.process_instance_id,
  include_history: true
});

console.log('Instance status:', queryResp.status);
```

### Using HTTP API (curl)

```bash
# Get auth token
TOKEN=$(curl -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"user","password":"password"}' | jq -r '.token')

# Start a process
curl -X POST http://localhost:8080/api/v1/bpmn/process/start \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "process_definition_key": "orderProcess",
    "variables": {
      "orderId": "123",
      "amount": 1000
    }
  }'

# Complete a task
curl -X POST http://localhost:8080/api/v1/bpmn/task/inst-abc123:userTask1/complete \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "variables": {
      "approved": true,
      "comment": "Approved"
    }
  }'

# Query instance
curl -X GET "http://localhost:8080/api/v1/bpmn/instance/inst-abc123?include_history=true" \
  -H "Authorization: Bearer $TOKEN"
```

## Testing

To test BPMN functionality:

1. Ensure a process definition is registered in the database
2. Use the HTTP API or wire protocol to start a process instance
3. Check that the instance is created and active tasks are returned
4. Complete tasks and verify process advancement
5. Query instance state to verify history and variables

## Future Enhancements

Potential improvements for future releases:

1. **Protobuf serialization**: Migrate from JSON to binary protobuf format for wire protocol payloads
2. **Timer events**: Support for BPMN timer events and boundary events
3. **Message correlation**: Support for message catching events and correlation keys
4. **Subprocess support**: Complete implementation of call activities and subprocesses
5. **Process versioning**: Support for multiple versions of the same process definition
6. **Task assignment**: Enhanced task assignment and claiming functionality
7. **Process monitoring**: Real-time metrics and dashboards for process execution
8. **BPMN import**: Direct import of BPMN 2.0 XML files

## Related Documentation

- [Process Graph Architecture](../docs/process-graph.md)
- [Wire Protocol Specification](../docs/wire-protocol-v1.md)
- [HTTP API Reference](../docs/api-reference.md)
- [Authentication & Authorization](../docs/auth.md)
