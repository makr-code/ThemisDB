# Changefeed Authentication and Long-Lived Connections

**Version:** 1.5.0  
**Status:** Production Ready  
**Last Updated:** April 2026

---

## Overview

ThemisDB changefeed (CDC - Change Data Capture) provides real-time event streaming with comprehensive authentication and authorization. This document describes authentication behavior for both polling and long-lived SSE (Server-Sent Events) connections.

---

## Authentication Model

### Fail-Closed Security

All changefeed endpoints enforce fail-closed authentication:
- Missing tokens are **denied** with `401 Unauthorized`
- Invalid tokens are **denied** with `401 Unauthorized`
- Insufficient scopes are **denied** with `403 Forbidden`

### Required Scopes

| Endpoint | Required Scope | Description |
|----------|---------------|-------------|
| `GET /changefeed` | `cdc:read` | Poll for events |
| `GET /changefeed/stream` | `cdc:read` | SSE streaming |
| `GET /changefeed/stats` | `cdc:admin` | Statistics |
| `POST /changefeed/retention` | `cdc:admin` | Configure retention |

---

## Short-Lived Polling (GET /changefeed)

### Request Pattern

```bash
curl -X GET "https://themis.example.com/changefeed?from_seq=100&limit=50" \
  -H "Authorization: Bearer $JWT_TOKEN"
```

### Authentication Flow

1. Client sends request with Bearer token
2. Server validates token and checks `cdc:read` scope
3. Server returns events if authorized
4. Connection closes after response

### Token Expiration

For polling, each request is independently authenticated:
- Use a valid token for each poll
- Implement token refresh in client
- Handle 401 errors by refreshing token and retrying

---

## Long-Lived SSE Streaming (GET /changefeed/stream)

### Connection Lifecycle

SSE streaming maintains a persistent HTTP connection:

```
Client → Server: GET /changefeed/stream + Authorization header
Server → Client: Validates token, checks cdc:read scope
Server → Client: Opens SSE connection (HTTP 200, Content-Type: text/event-stream)
Server → Client: Streams events continuously
Server → Client: Sends heartbeats every N seconds
...
Connection maintained until:
  - Client disconnects
  - Server terminates (max_seconds limit)
  - Network failure
```

### Authentication at Connection Time

**Important:** Authentication occurs **once** at connection establishment:

1. Client sends SSE request with Bearer token
2. Server validates token and checks `cdc:read` scope
3. If authorized, SSE connection is established
4. Token is **not** revalidated during the stream
5. Connection remains authenticated for its duration

### Token Expiration During Streaming

**Challenge:** JWT tokens typically expire after 15-60 minutes, but SSE connections may run for hours or days.

**Solution:** Implement reconnection with token refresh:

```python
import requests
import time

def stream_changefeed(get_token_fn):
    """Stream changefeed with automatic reconnection on token expiry."""
    last_sequence = 0
    
    while True:
        try:
            token = get_token_fn()  # Get fresh token
            headers = {
                'Authorization': f'Bearer {token}',
                'Accept': 'text/event-stream'
            }
            
            url = f'https://themis.example.com/changefeed/stream'
            params = {
                'from_seq': last_sequence,
                'keep_alive': 'true',
                'retry_ms': 3000
            }
            
            with requests.get(url, headers=headers, params=params, stream=True) as resp:
                if resp.status_code == 401:
                    # Token expired or invalid - refresh and retry
                    print("Authentication failed, refreshing token...")
                    time.sleep(1)
                    continue
                
                resp.raise_for_status()
                
                for line in resp.iter_lines():
                    if line:
                        line = line.decode('utf-8')
                        if line.startswith('id: '):
                            last_sequence = int(line[4:])
                        elif line.startswith('data: '):
                            event_data = line[6:]
                            process_event(event_data)
                            
        except requests.exceptions.RequestException as e:
            print(f"Connection error: {e}, reconnecting...")
            time.sleep(5)  # Wait before reconnect
```

### Recommended Client Pattern

1. **Token Management**
   - Store token expiration time
   - Refresh token before expiration (e.g., 5 minutes early)
   - Implement exponential backoff for retries

2. **Connection Management**
   - Track last processed sequence number
   - Resume from last sequence on reconnect
   - Handle network interruptions gracefully

3. **Heartbeat Monitoring**
   - Server sends heartbeat comments (`: heartbeat\n\n`)
   - Client should expect heartbeats every 30-60 seconds
   - Disconnect and reconnect if no heartbeat received in 2x interval

### Example: JavaScript/Node.js Client

**Note:** Browser `EventSource` does not support custom headers. For authenticated SSE in browsers, use `fetch()` with ReadableStream or include the token in the URL (less secure). The example below uses Node.js with `eventsource` package which supports headers.

```javascript
// Node.js example using 'eventsource' npm package
// npm install eventsource

const EventSource = require('eventsource');

class ChangefeedClient {
  constructor(baseUrl, getTokenFn) {
    this.baseUrl = baseUrl;
    this.getTokenFn = getTokenFn;
    this.lastSequence = 0;
    this.reconnectDelay = 1000;
    this.maxReconnectDelay = 60000;
    this.eventSource = null;
  }
  
  async connect() {
    while (true) {
      try {
        const token = await this.getTokenFn();
        const url = `${this.baseUrl}/changefeed/stream?from_seq=${this.lastSequence}`;
        
        // Node.js eventsource package supports headers
        this.eventSource = new EventSource(url, {
          headers: {
            'Authorization': `Bearer ${token}`
          }
        });
        
        this.eventSource.addEventListener('message', (event) => {
          const data = JSON.parse(event.data);
          this.lastSequence = data.sequence;
          this.handleEvent(data);
        });
        
        this.eventSource.addEventListener('error', (error) => {
          console.error('SSE error:', error);
          if (this.eventSource) {
            this.eventSource.close();
          }
          
          // Reconnect with exponential backoff
          setTimeout(() => this.connect(), this.reconnectDelay);
          this.reconnectDelay = Math.min(this.reconnectDelay * 2, this.maxReconnectDelay);
        });
        
        this.eventSource.addEventListener('open', () => {
          console.log('SSE connection established');
          this.reconnectDelay = 1000; // Reset backoff
        });
        
        break; // Exit retry loop on successful connection
        
      } catch (error) {
        console.error('Connection failed:', error);
        await new Promise(resolve => setTimeout(resolve, this.reconnectDelay));
        this.reconnectDelay = Math.min(this.reconnectDelay * 2, this.maxReconnectDelay);
      }
    }
  }
  
  disconnect() {
    if (this.eventSource) {
      this.eventSource.close();
      this.eventSource = null;
    }
  }
  
  handleEvent(event) {
    console.log('Received event:', event);
    // Process event
  }
}
```

---

## Security Considerations

### 1. Token Validation at Connection Time

**Current Behavior:**
- Token is validated when SSE connection is established
- Token is **not** continuously revalidated during streaming
- If token expires mid-stream, connection remains open

**Rationale:**
- Continuous validation would require complex token passing in SSE
- Server-side token revalidation would add overhead
- Client reconnection provides natural refresh point

**Best Practice:**
- Use short-lived tokens (15-60 minutes)
- Implement automatic reconnection before token expiry
- Monitor for 401 responses indicating token issues

### 2. Scope Enforcement

**At Connection:**
```cpp
// changefeed_api_handler.cpp
auto auth_result = auth_->authorize(*token, "cdc:read");
if (!auth_result.authorized) {
    return 403 Forbidden;
}
```

**During Stream:**
- No additional scope checks
- Authorization granted for connection duration

### 3. Revocation Handling

**Token Revocation:**
- If a token is revoked server-side, active SSE connections remain open
- Revoked token will fail on next reconnection attempt
- Administrators can close specific connections if needed

**User Role Changes:**
- Role/scope changes do not affect active connections
- Changes take effect on next connection

**Recommendation:**
- Set reasonable connection limits (e.g., 30-60 minutes max)
- Force reconnection periodically to revalidate permissions

---

## Server Configuration

### Connection Limits

```cpp
// Example SSE configuration with overridable defaults
ChangefeedApiHandler handler(
    storage,
    changefeed,
    sse_manager,
    auth,
    feature_cdc_enabled
);

// Default values (can be overridden via query parameters)
const int default_max_seconds = 30;  // Default: 30 seconds (overridable via max_seconds param)
const int default_heartbeat_ms = 30000;  // Default: 30 seconds (overridable via heartbeat_ms param)
```

### Query Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `from_seq` | uint64 | 0 | Start sequence number |
| `key_prefix` | string | "" | Filter by key prefix |
| `keep_alive` | bool | true | Enable long-lived connection |
| `max_seconds` | int | 30 | Max connection duration (seconds) |
| `heartbeat_ms` | int | 30000 | Heartbeat interval (milliseconds) |
| `retry_ms` | int | 3000 | Suggested retry delay for client |

---

## Monitoring

### Connection Metrics

SSE connection manager tracks:
- Active connections count
- Heartbeats sent
- Events delivered
- Connection duration
- Disconnection reasons

### Authentication Metrics

AuthMiddleware tracks:
- Authorization attempts (`cdc:read`, `cdc:admin`)
- Successes vs. failures
- Denied reasons (invalid token, insufficient scope)

### Audit Logging

All authentication events are logged:
```
[INFO] SSE connection established: conn=abc123, user=alice@example.com, scope=cdc:read
[WARN] Authorization failed for changefeed endpoint - user: bob@example.com, required scope: cdc:admin, reason: insufficient_scope
[INFO] SSE stream completed: conn=abc123, events=1542, heartbeats=120, duration_s=3600
```

---

## Testing

### Test Authentication Failure

```bash
# Missing token
curl -X GET "https://themis.example.com/changefeed/stream"
# Expected: 401 Unauthorized

# Invalid token
curl -X GET "https://themis.example.com/changefeed/stream" \
  -H "Authorization: Bearer invalid_token"
# Expected: 401 Unauthorized

# Valid token, wrong scope
curl -X GET "https://themis.example.com/changefeed/stats" \
  -H "Authorization: Bearer $READ_ONLY_TOKEN"
# Expected: 403 Forbidden (needs cdc:admin, not just cdc:read)
```

### Test Long-Lived Connection

```bash
# Stream with 5-minute max connection
curl -X GET "https://themis.example.com/changefeed/stream?from_seq=0&max_seconds=300" \
  -H "Authorization: Bearer $JWT_TOKEN" \
  -H "Accept: text/event-stream"

# Monitor for heartbeats
# Expected: ": heartbeat\n\n" every 30 seconds
```

---

## Migration Recommendations

### For Production Deployments

1. **Enable Authentication**
   ```cpp
   auto auth = std::make_shared<AuthMiddleware>();
   auth->enableJWT(jwt_config);
   ```

2. **Configure Token Expiry**
   - Set JWT expiry to 30-60 minutes
   - Implement token refresh mechanism

3. **Update Client Code**
   - Add Authorization header
   - Implement reconnection logic
   - Track last sequence number

4. **Set Connection Limits**
   - `max_seconds`: 1-2 hours maximum
   - `heartbeat_ms`: 30-60 seconds
   - Force periodic reconnection

5. **Monitor Metrics**
   - Track authentication failures
   - Alert on anomalous patterns
   - Review audit logs

---

## FAQ

**Q: What happens if my token expires during SSE streaming?**  
A: The connection remains open. The server does not revalidate tokens during streaming. You should implement client-side reconnection before token expiry.

**Q: How do I know when to reconnect?**  
A: Monitor your token expiration time and reconnect 5-10 minutes before expiry. Also reconnect if you stop receiving heartbeats.

**Q: Can I use the same token for multiple SSE connections?**  
A: Yes, but each connection independently validates the token at connection time.

**Q: What if my user's permissions are revoked?**  
A: Active connections remain open. Revoked permissions take effect on next connection attempt.

**Q: How do I resume after disconnection?**  
A: Track the `last_sequence` from events and use `from_seq` parameter on reconnect to resume from where you left off.

---

## References

- [API Authentication and Authorization](./api_authentication_authorization.md)
- [Changefeed Implementation](../../src/server/changefeed_api_handler.cpp)
- [SSE Connection Manager](../../include/server/sse_connection_manager.h)
- [Production Hardening Checklist](./PRODUCTION_HARDENING_CHECKLIST.md)

---

**Implementation Status:**
- Authentication enforcement: v1.4.0+
- Fail-closed behavior: Production ready
- SSE long-lived connections: Production ready
- Reconnection best practices: Documented v1.5.0
