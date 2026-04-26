# ThemisDB SDK Enhancement Guide

## Overview

This document describes the comprehensive enhancements made to ThemisDB Client SDKs across multiple languages. All enhancements are **100% backwards compatible** and opt-in via configuration.

## Enhanced Features

### 1. Circuit Breaker Pattern

The circuit breaker pattern prevents cascading failures by temporarily blocking requests when a service is experiencing issues.

**States:**
- `CLOSED`: Normal operation, requests flow through
- `OPEN`: Blocking requests due to failures
- `HALF_OPEN`: Testing if service has recovered

**Implementations:**
- ✅ JavaScript/TypeScript
- ✅ Go
- ✅ Java
- ✅ Python (async)

### 2. Retry with Exponential Backoff

Automatic retry of failed requests with exponential backoff to handle transient failures.

**Implementations:**
- ✅ JavaScript/TypeScript (existing, now integrated with circuit breaker)
- ✅ Go (new)
- ✅ Java (new)
- ✅ Python (existing, now with circuit breaker integration)
- ✅ Rust (existing)
- ✅ Ruby (existing)
- ✅ PHP (existing)

### 3. Request/Response Logging

Comprehensive logging of all HTTP requests and responses for debugging and monitoring.

**Implementations:**
- ✅ JavaScript/TypeScript
- ✅ Go
- ✅ Java
- ✅ Python

### 4. Connection Pooling

Efficient HTTP connection management with configurable pool sizes.

**Implementations:**
- ✅ Python (existing, HTTP/2 with httpx)
- ✅ PHP (existing)
- ⚠️  JavaScript/TypeScript (browser fetch API, implicit pooling)
- ⚠️  Go (standard HTTP client, implicit pooling)

---

## Usage Examples

### JavaScript/TypeScript

```typescript
import { ThemisClient } from "@themisdb/client";

const client = new ThemisClient({
  endpoints: ["http://localhost:8080"],
  namespace: "default",
  maxRetries: 3,
  
  // Circuit breaker configuration
  circuitBreaker: {
    enabled: true,
    failureThreshold: 5,
    resetTimeout: 60000, // 60 seconds
    halfOpenMaxRequests: 3
  },
  
  // Logging configuration
  logging: {
    enabled: true,
    logRequests: true,
    logResponses: true,
    logger: (message, level) => {
      console.log(`[${level}] ${message}`);
    }
  }
});

// Check circuit breaker state
const state = client.getCircuitBreakerState();
console.log(`Circuit breaker state: ${state}`);

// Normal operations
const user = await client.get("mymodel", "users", "user-123");
```

### Go

```go
package main

import (
    "context"
    "log"
    "time"
    
    themisdb "github.com/makr-code/ThemisDB/clients/go"
)

func main() {
    client := themisdb.NewClient(themisdb.Config{
        Endpoints:  []string{"http://localhost:8080"},
        Timeout:    30 * time.Second,
        MaxRetries: 3,
        
        // Circuit breaker configuration
        CircuitBreaker: &themisdb.CircuitBreakerConfig{
            Enabled:             true,
            FailureThreshold:    5,
            ResetTimeout:        60 * time.Second,
            HalfOpenMaxRequests: 3,
        },
        
        // Logging configuration
        Logging: &themisdb.LoggingConfig{
            Enabled:      true,
            LogRequests:  true,
            LogResponses: true,
            Logger: func(msg, level string) {
                log.Printf("[%s] %s", level, msg)
            },
        },
    })
    
    ctx := context.Background()
    
    // Check circuit breaker state
    state := client.GetCircuitBreakerState()
    log.Printf("Circuit breaker state: %s", state)
    
    // Normal operations
    var user map[string]interface{}
    err := client.Get(ctx, "mymodel", "users", "user-123", &user)
    if err != nil {
        log.Fatal(err)
    }
}
```

### Java

```java
import com.themisdb.client.ThemisClient;
import com.themisdb.client.ClientConfig;
import java.time.Duration;
import java.util.Arrays;

public class Example {
    public static void main(String[] args) throws Exception {
        // Build configuration
        ClientConfig config = new ClientConfig.Builder()
            .maxRetries(3)
            .timeout(Duration.ofSeconds(30))
            .circuitBreaker(
                new ClientConfig.CircuitBreakerConfig.Builder()
                    .enabled(true)
                    .failureThreshold(5)
                    .resetTimeout(Duration.ofSeconds(60))
                    .halfOpenMaxRequests(3)
                    .build()
            )
            .logging(
                new ClientConfig.LoggingConfig.Builder()
                    .enabled(true)
                    .logRequests(true)
                    .logResponses(true)
                    .logger((message, level) -> {
                        System.out.println("[" + level + "] " + message);
                    })
                    .build()
            )
            .build();
        
        // Create client
        ThemisClient client = new ThemisClient(
            Arrays.asList("http://localhost:8080"),
            Duration.ofSeconds(30),
            config
        );
        
        // Check circuit breaker state
        String state = client.getCircuitBreakerState();
        System.out.println("Circuit breaker state: " + state);
        
        // Normal operations
        User user = client.get("mymodel", "users", "user-123", User.class);
    }
}
```

### Python (Async)

```python
import asyncio
import logging
from themis.async_client import AsyncThemisClient, CircuitBreaker

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

async def main():
    # Create circuit breaker
    circuit_breaker = CircuitBreaker(
        failure_threshold=5,
        reset_timeout=60.0,
        half_open_max_requests=3
    )
    
    # Create client with all features
    client = AsyncThemisClient(
        endpoints=["http://localhost:8080"],
        namespace="default",
        timeout=30.0,
        max_connections=100,
        max_retries=3,
        circuit_breaker=circuit_breaker,
        enable_logging=True,
        log_requests=True,
        log_responses=True,
        logger=logger
    )
    
    async with client:
        # Check circuit breaker state
        state = client.circuit_breaker_state
        logger.info(f"Circuit breaker state: {state}")
        
        # Normal operations
        user = await client.get("mymodel", "users", "user-123")
        print(user)

if __name__ == "__main__":
    asyncio.run(main())
```

### Rust

```rust
use themisdb_sdk::{ThemisClientConfig, CircuitBreakerConfig, LoggingConfig};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Create configuration with circuit breaker and logging
    let config = ThemisClientConfig {
        endpoints: vec!["http://localhost:8080".to_string()],
        namespace: "default".to_string(),
        timeout_ms: 30_000,
        metadata_endpoint: None,
        max_retries: 3,
        circuit_breaker: Some(CircuitBreakerConfig {
            enabled: true,
            failure_threshold: 5,
            reset_timeout_secs: 60,
            half_open_max_requests: 3,
        }),
        logging: Some(LoggingConfig {
            enabled: true,
            log_requests: true,
            log_responses: true,
        }),
    };
    
    let client = themisdb_sdk::ThemisClient::new(config)?;
    
    // Check circuit breaker state
    if let Some(state) = client.get_circuit_breaker_state().await {
        println!("Circuit breaker state: {:?}", state);
    }
    
    // Normal operations
    let user = client.get("mymodel", "users", "user-123").await?;
    println!("{:?}", user);
    
    Ok(())
}
```

### Ruby

```ruby
require 'themisdb'

# Create client with circuit breaker and logging
client = ThemisDB::Client.new(
  ['http://localhost:8080'],
  namespace: 'default',
  max_retries: 3,
  circuit_breaker: {
    enabled: true,
    failure_threshold: 5,
    reset_timeout: 60,
    half_open_max_requests: 3
  },
  logging: {
    enabled: true,
    log_requests: true,
    log_responses: true
  }
)

# Check circuit breaker state
puts "Circuit breaker state: #{client.circuit_breaker_state}"

# Normal operations
user = client.get('mymodel', 'users', 'user-123')
puts user.inspect
```

### PHP

```php
<?php
require_once 'vendor/autoload.php';

use ThemisDB\ThemisClient;

// Create client with circuit breaker and logging
$client = new ThemisClient(
    ['http://localhost:8080'],
    [
        'namespace' => 'default',
        'max_retries' => 3,
        'circuit_breaker' => [
            'enabled' => true,
            'failure_threshold' => 5,
            'reset_timeout' => 60,
            'half_open_max_requests' => 3,
        ],
        'logging' => [
            'enabled' => true,
            'log_requests' => true,
            'log_responses' => true,
        ],
    ]
);

// Check circuit breaker state
$state = $client->getCircuitBreakerState();
echo "Circuit breaker state: {$state}\n";

// Normal operations
$user = $client->get('mymodel', 'users', 'user-123');
print_r($user);
```

### C#

```csharp
using ThemisDB.Client;
using System;
using System.Threading.Tasks;

class Program
{
    static async Task Main(string[] args)
    {
        // Create configuration with all features
        var config = new ClientConfig
        {
            MaxRetries = 3,
            Timeout = TimeSpan.FromSeconds(30),
            CircuitBreaker = new ClientConfig.CircuitBreakerConfig
            {
                Enabled = true,
                FailureThreshold = 5,
                ResetTimeout = TimeSpan.FromSeconds(60),
                HalfOpenMaxRequests = 3
            },
            Logging = new ClientConfig.LoggingConfig
            {
                Enabled = true,
                LogRequests = true,
                LogResponses = true,
                Logger = (message, level) => Console.WriteLine($"[{level}] {message}")
            },
            ConnectionPool = new ClientConfig.ConnectionPoolConfig
            {
                MaxConnections = 100,
                MaxConnectionsPerEndpoint = 50,
                IdleTimeout = TimeSpan.FromSeconds(30),
                KeepAliveTimeout = TimeSpan.FromSeconds(60)
            }
        };
        
        // Create client
        using var client = new ThemisClient(
            new[] { "http://localhost:8080" },
            config
        );
        
        // Check circuit breaker state
        var state = client.GetCircuitBreakerState();
        Console.WriteLine($"Circuit breaker state: {state}");
        
        // Normal operations
        var user = await client.GetAsync<dynamic>("mymodel", "users", "user-123");
        Console.WriteLine(user);
    }
}
```

---

## Configuration Options

### Circuit Breaker

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `enabled` | boolean | false | Enable circuit breaker |
| `failureThreshold` | integer | 5 | Number of failures before opening circuit |
| `resetTimeout` | duration | 60s | How long to wait before attempting reset |
| `halfOpenMaxRequests` | integer | 3 | Max requests in half-open state |

### Logging

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `enabled` | boolean | false | Enable logging |
| `logRequests` | boolean | false | Log all HTTP requests |
| `logResponses` | boolean | false | Log all HTTP responses |
| `logger` | function | console/stdout | Custom logger function |

### Retry

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `maxRetries` | integer | 3 | Maximum number of retry attempts |
| `backoff` | string | exponential | Backoff strategy (exponential) |
| `baseDelay` | duration | 50-100ms | Base delay for exponential backoff |

---

## Backwards Compatibility

All enhancements are **100% backwards compatible**:

1. **Default Configuration**: New features are disabled by default
2. **Opt-in**: All features must be explicitly enabled via configuration
3. **Existing Constructors**: All existing constructors remain unchanged
4. **No Breaking Changes**: Existing code continues to work without modifications

### Migration Path

**Before (still works):**
```typescript
const client = new ThemisClient({
  endpoints: ["http://localhost:8080"]
});
```

**After (with enhancements):**
```typescript
const client = new ThemisClient({
  endpoints: ["http://localhost:8080"],
  circuitBreaker: { enabled: true },  // Optional
  logging: { enabled: true }          // Optional
});
```

---

## Best Practices

### 1. Circuit Breaker Thresholds

- **Development**: `failureThreshold: 10`, `resetTimeout: 30s`
- **Production**: `failureThreshold: 5`, `resetTimeout: 60s`
- **Critical Systems**: `failureThreshold: 3`, `resetTimeout: 120s`

### 2. Logging

- **Development**: Enable all logging
- **Staging**: Log requests only for debugging
- **Production**: Disable or use custom logger with sampling

### 3. Retry Configuration

- **Fast APIs**: `maxRetries: 2`, short timeout
- **Slow APIs**: `maxRetries: 3-5`, longer timeout
- **Idempotent Operations**: Higher retry counts safe
- **Non-idempotent Operations**: Lower retry counts recommended

---

## Monitoring Circuit Breaker

All SDKs provide a method to check circuit breaker state:

- **JavaScript/TypeScript**: `client.getCircuitBreakerState()`
- **Go**: `client.GetCircuitBreakerState()`
- **Java**: `client.getCircuitBreakerState()`
- **Python**: `client.circuit_breaker_state` (property)

**States:**
- `"CLOSED"`: Normal operation
- `"OPEN"`: Circuit is open, requests blocked
- `"HALF_OPEN"`: Testing recovery
- `null`/`""`: Circuit breaker not enabled

**Example Monitoring:**

```typescript
// JavaScript
setInterval(() => {
  const state = client.getCircuitBreakerState();
  if (state === "OPEN") {
    console.error("Circuit breaker is OPEN - service degraded");
    // Send alert
  }
}, 5000);
```

---

## Performance Impact

### Circuit Breaker
- **Memory**: ~100 bytes per client instance
- **CPU**: Negligible (simple state machine)
- **Latency**: <1ms overhead per request

### Logging
- **Development**: Minimal impact with console logging
- **Production**: Use async loggers to minimize impact
- **Recommendation**: Enable selectively in production

### Retry
- **Bandwidth**: May increase due to retries
- **Latency**: Increases on failures (exponential backoff)
- **Recommendation**: Tune `maxRetries` based on SLA

---

## SDK Feature Matrix

| Feature | JS/TS | Go | Java | Python | Rust | C# | Ruby | PHP | Swift |
|---------|-------|----|----|--------|------|----|----|-----|-------|
| Circuit Breaker | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| Retry + Backoff | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| Logging | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| Connection Pool | ⚠️  | ⚠️  | ✅ | ✅ | ❌ | ✅ | ❌ | ✅ | ❌ |
| Async/Await | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ | ❌ | ✅ |
| Binary Protocol | ❌ | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |

**Legend:**
- ✅ Implemented
- ⚠️  Implicit/Built-in
- ❌ Not implemented

**Updated: April 2026**
- All major SDKs (JS, Go, Java, Python, C#, Rust, Ruby, PHP) now have circuit breaker, retry, and logging
- Java and C# have explicit connection pooling configuration
- Go and Java have binary protocol support

---

## Troubleshooting

### Circuit Breaker Opens Frequently

1. Check if `failureThreshold` is too low
2. Verify service health
3. Increase `resetTimeout` to give service more recovery time
4. Check logs for actual errors

### Excessive Retries

1. Reduce `maxRetries` for non-critical operations
2. Check if errors are retryable (5xx vs 4xx)
3. Consider implementing request idempotency

### High Latency

1. Circuit breaker may be in `HALF_OPEN` state
2. Retry backoff may be too aggressive
3. Check if logging is impacting performance

---

## Future Enhancements

Planned for future releases:

1. **Remaining SDKs**: Add circuit breaker and logging to Rust, C#, Ruby, PHP, Swift
2. **Binary Protocol**: Expand binary protocol support to all SDKs
3. **Metrics**: Built-in metrics collection (Prometheus, StatsD)
4. **Tracing**: OpenTelemetry integration
5. **Rate Limiting**: Client-side rate limiting
6. **Caching**: Response caching layer

---

## Contributing

To add these features to other SDKs:

1. Implement `CircuitBreaker` class/struct
2. Add configuration options to client constructor
3. Wrap HTTP requests with retry + circuit breaker logic
4. Add logging at appropriate points
5. Maintain backwards compatibility
6. Add tests for new features
7. Update documentation

---

## Support

For issues or questions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://themisdb.io/docs
- Discord: https://discord.gg/themisdb

---

## Changelog

### Version 1.8.0-rc1 (April 2026)
- ✨ Added circuit breaker pattern to JavaScript, Go, Java, Python SDKs
- ✨ Added comprehensive logging to JavaScript, Go, Java, Python SDKs
- ✨ Enhanced retry mechanisms with circuit breaker integration
- ✨ All changes are backwards compatible
- 📚 Added comprehensive SDK enhancement documentation

---

*Last Updated: April 2026*
