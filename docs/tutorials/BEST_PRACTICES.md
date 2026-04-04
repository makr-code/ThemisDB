# Best Practices Guide

Production-ready patterns and best practices for ThemisDB. Learn from real-world experience to build robust, performant, and secure database applications.

## 🎯 What You'll Learn

- ✅ Query optimization patterns
- ✅ Security best practices
- ✅ Performance tuning strategies
- ✅ Error handling patterns
- ✅ Transaction management
- ✅ Monitoring and observability
- ✅ Code organization

**Prerequisites:** All previous tutorials  
**Time Required:** 40 minutes  
**Difficulty:** Intermediate to Advanced

---

## Table of Contents

1. [Query Optimization](#query-optimization)
2. [Security Best Practices](#security-best-practices)
3. [Performance Best Practices](#performance-best-practices)
4. [Error Handling](#error-handling)
5. [Transaction Patterns](#transaction-patterns)
6. [Monitoring](#monitoring)
7. [Code Organization](#code-organization)

---

## Query Optimization

### 1. Always Use Indexes for Queries

**❌ Bad: Full table scan**
```bash
# No index on 'city' - scans all entities!
curl -X POST http://localhost:8080/query \
  -d '{"table": "users", "predicates": [{"column": "city", "value": "Berlin"}]}'
```

**✅ Good: Use indexed column**
```bash
# First create index
curl -X POST http://localhost:8080/index/create \
  -d '{"table": "users", "column": "city", "type": "btree"}'

# Now query is fast
curl -X POST http://localhost:8080/query \
  -d '{"table": "users", "predicates": [{"column": "city", "value": "Berlin"}], "use_index": true}'
```

**Performance impact:** 100-1000x faster with index!

### 2. Use Composite Indexes for Multi-Column Queries

**❌ Bad: Multiple single-column indexes**
```bash
POST /index/create {"table": "orders", "column": "user_id"}
POST /index/create {"table": "orders", "column": "status"}
# Query can only use one index efficiently
```

**✅ Good: Composite index**
```bash
POST /index/create {
  "table": "orders",
  "columns": ["user_id", "status", "created_at"],
  "type": "btree"
}
# Optimizes: WHERE user_id=? AND status=? ORDER BY created_at
```

### 3. Use Query Projection

**❌ Bad: Fetch everything**
```bash
curl http://localhost:8080/entities/products:laptop-001
# Returns all attributes including large description
```

**✅ Good: Fetch only what you need**
```bash
curl -X POST http://localhost:8080/entities/products:laptop-001/attributes \
  -d '{"fields": ["name", "price", "stock"]}'
# 10x faster, 90% less bandwidth
```

### 4. Limit Result Sets

**❌ Bad: Unlimited results**
```bash
curl -X POST http://localhost:8080/query \
  -d '{"table": "products", "predicates": [...]}'
# Could return millions of rows!
```

**✅ Good: Always paginate**
```bash
curl -X POST http://localhost:8080/query \
  -d '{
    "table": "products",
    "predicates": [...],
    "limit": 50,
    "offset": 0
  }'
```

### 5. Use EXPLAIN to Understand Query Plans

```bash
curl -X POST http://localhost:8080/query/explain \
  -d '{
    "table": "products",
    "predicates": [
      {"column": "category", "value": "electronics"},
      {"column": "price", "operator": "<", "value": 1000}
    ]
  }'
```

**Example output:**
```json
{
  "plan": "index_scan",
  "index_used": "products_category_price_idx",
  "estimated_rows": 150,
  "estimated_cost": 42
}
```

---

## Security Best Practices

### 1. Always Use TLS in Production

**❌ Bad: Plain HTTP**
```bash
curl http://themisdb.example.com/entities/users:alice
# Credentials and data sent in clear text!
```

**✅ Good: HTTPS with TLS 1.3**
```bash
curl https://themisdb.example.com/entities/users:alice \
  --cacert ca.crt \
  --cert client.crt \
  --key client.key
```

**Server configuration:**
```yaml
# config.yaml
tls:
  enabled: true
  cert_file: /etc/themis/server.crt
  key_file: /etc/themis/server.key
  ca_file: /etc/themis/ca.crt
  min_version: "TLS1.3"
```

### 2. Implement Authentication

**❌ Bad: No auth**
```bash
curl http://localhost:8080/entities/sensitive:data
# Anyone can access!
```

**✅ Good: API key authentication**
```bash
curl https://localhost:8080/entities/sensitive:data \
  -H "Authorization: Bearer sk_prod_abc123xyz"
```

**Better: mTLS (Mutual TLS)**
```yaml
# config.yaml
auth:
  mode: mtls
  require_client_cert: true
  authorized_cns:
    - "client1.example.com"
    - "client2.example.com"
```

### 3. Use Role-Based Access Control (RBAC)

```bash
# Create role with limited permissions
curl -X POST https://localhost:8080/admin/roles \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  -d '{
    "role": "read_only_user",
    "permissions": ["read:users", "read:products"]
  }'

# Assign role to API key
curl -X POST https://localhost:8080/admin/api-keys \
  -d '{
    "name": "App Backend",
    "role": "read_only_user"
  }'
```

### 4. Encrypt Sensitive Data

**Field-level encryption:**
```python
from cryptography.fernet import Fernet
import requests

# Generate key (store securely!)
key = Fernet.generate_key()
cipher = Fernet(key)

# Encrypt sensitive field
ssn = "123-45-6789"
encrypted_ssn = cipher.encrypt(ssn.encode()).decode()

# Store encrypted
requests.put("http://localhost:8080/entities/users:alice", json={
    "blob": json.dumps({
        "name": "Alice",
        "ssn_encrypted": encrypted_ssn  # Encrypted!
    })
})
```

### 5. Audit Logging

**Enable audit logs:**
```yaml
# config.yaml
audit:
  enabled: true
  log_file: /var/log/themis/audit.log
  events:
    - create
    - update
    - delete
    - query
  include_data: false  # Don't log sensitive data
```

**Monitor audit logs:**
```bash
tail -f /var/log/themis/audit.log | jq '.event, .entity, .user, .timestamp'
```

### 6. Rate Limiting

**Prevent abuse:**
```yaml
# config.yaml
rate_limit:
  enabled: true
  requests_per_second: 100
  burst: 200
  by: ip_address
```

---

## Performance Best Practices

### 1. Use Connection Pooling

**❌ Bad: New connection per request**
```python
def get_user(user_id):
    response = requests.get(f"http://localhost:8080/entities/{user_id}")
    return response.json()
# Creates new TCP connection every time!
```

**✅ Good: Reuse connections**
```python
import requests

# Create session (connection pool)
session = requests.Session()
session.mount('http://', requests.adapters.HTTPAdapter(
    pool_connections=10,
    pool_maxsize=20,
    max_retries=3
))

def get_user(user_id):
    response = session.get(f"http://localhost:8080/entities/{user_id}")
    return response.json()
# Reuses connections - 10x faster!
```

### 2. Batch Operations

**❌ Bad: Individual operations in loop**
```python
for product in products:
    create_product(product)  # 1000 network calls
```

**✅ Good: Batch operation**
```python
batch_create_products(products)  # 1 network call
```

### 3. Use Caching

**Application-level caching:**
```python
from functools import lru_cache
import requests

@lru_cache(maxsize=1000)
def get_product(product_id):
    response = requests.get(f"http://localhost:8080/entities/{product_id}")
    return response.json()

# First call: fetches from DB
product = get_product("products:laptop-001")

# Second call: returns from cache
product = get_product("products:laptop-001")  # Instant!
```

**Redis caching layer:**
```python
import redis
import requests
import json

redis_client = redis.Redis(host='localhost', port=6379)

def get_product_cached(product_id):
    # Try cache first
    cached = redis_client.get(product_id)
    if cached:
        return json.loads(cached)
    
    # Cache miss - fetch from DB
    response = requests.get(f"http://localhost:8080/entities/{product_id}")
    data = response.json()
    
    # Store in cache (TTL: 5 minutes)
    redis_client.setex(product_id, 300, json.dumps(data))
    
    return data
```

### 4. Denormalize Read-Heavy Data

```json
// orders:order-001 - Denormalized for fast display
{
  "entity_id": "orders:order-001",
  "attributes": {
    "user_id": "users:alice",
    "user_name": "Alice Johnson",    // Denormalized!
    "user_email": "alice@example.com", // Denormalized!
    "total": 99.99
  }
}
```

**Trade-off:** Faster reads, but must update multiple places on user name change.

### 5. Monitor Performance Metrics

```bash
# Check database metrics
curl http://localhost:4318/metrics | grep -E "query_duration|cache_hit_rate|index_usage"
```

**Key metrics to watch:**
- Query duration (p50, p95, p99)
- Cache hit rate (target: >80%)
- Index usage rate
- Transaction duration
- Connection pool utilization

---

## Error Handling

### 1. Always Check HTTP Status Codes

**❌ Bad: Assume success**
```python
response = requests.get(f"http://localhost:8080/entities/{entity_id}")
data = response.json()  # Could fail!
```

**✅ Good: Check status**
```python
response = requests.get(f"http://localhost:8080/entities/{entity_id}")
if response.status_code == 200:
    data = response.json()
elif response.status_code == 404:
    print(f"Entity {entity_id} not found")
else:
    print(f"Error: {response.status_code} - {response.text}")
```

### 2. Implement Retry Logic

```python
import time
from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry

def create_session_with_retries():
    session = requests.Session()
    
    retry_strategy = Retry(
        total=3,
        backoff_factor=1,  # 1s, 2s, 4s delays
        status_forcelist=[429, 500, 502, 503, 504],
        method_whitelist=["GET", "PUT", "POST", "DELETE"]
    )
    
    adapter = HTTPAdapter(max_retries=retry_strategy)
    session.mount("http://", adapter)
    session.mount("https://", adapter)
    
    return session

session = create_session_with_retries()
```

### 3. Handle Concurrent Updates (Optimistic Locking)

```python
def update_product_stock(product_id, quantity_delta):
    max_retries = 3
    
    for attempt in range(max_retries):
        # Read current state
        response = requests.get(f"http://localhost:8080/entities/{product_id}")
        entity = response.json()
        current_version = entity['version']
        current_stock = entity['attributes']['stock']
        
        # Calculate new stock
        new_stock = current_stock + quantity_delta
        if new_stock < 0:
            raise ValueError("Insufficient stock")
        
        # Update with version check
        update_response = requests.put(
            f"http://localhost:8080/entities/{product_id}",
            headers={"If-Match": f'"version-{current_version}"'},
            json={"attributes": {"stock": new_stock}}
        )
        
        if update_response.status_code == 200:
            return True  # Success!
        elif update_response.status_code == 409:  # Conflict
            print(f"Version conflict, retrying... (attempt {attempt + 1})")
            time.sleep(0.1 * (2 ** attempt))  # Exponential backoff
        else:
            raise Exception(f"Update failed: {update_response.text}")
    
    raise Exception("Max retries exceeded")
```

### 4. Use Circuit Breaker Pattern

```python
from pybreaker import CircuitBreaker

db_breaker = CircuitBreaker(
    fail_max=5,           # Open after 5 failures
    timeout_duration=60,  # Keep open for 60 seconds
    name="themisdb"
)

@db_breaker
def query_database(query):
    response = requests.post("http://localhost:8080/query", json=query)
    response.raise_for_status()
    return response.json()

# Usage
try:
    result = query_database({"table": "users", ...})
except CircuitBreakerError:
    print("Database circuit breaker open - using fallback")
    result = get_cached_result()
```

---

## Transaction Patterns

### 1. Use Transactions for Related Operations

**❌ Bad: No transaction**
```python
# Create order
create_entity(order)

# Decrement stock
update_entity(product, {"stock": new_stock})

# If second operation fails, order exists but stock unchanged!
```

**✅ Good: Transaction**
```python
# Start transaction
tx_response = requests.post("http://localhost:8080/tx/begin")
tx_id = tx_response.json()['tx_id']

try:
    # Create order within transaction
    requests.put(
        f"http://localhost:8080/tx/{tx_id}/entities/{order_id}",
        json=order_data
    )
    
    # Update stock within same transaction
    requests.patch(
        f"http://localhost:8080/tx/{tx_id}/entities/{product_id}",
        json={"attributes": {"stock": new_stock}}
    )
    
    # Commit - all or nothing!
    requests.post(f"http://localhost:8080/tx/{tx_id}/commit")
    
except Exception as e:
    # Rollback on error
    requests.post(f"http://localhost:8080/tx/{tx_id}/rollback")
    raise
```

### 2. Keep Transactions Short

**❌ Bad: Long-running transaction**
```python
tx_id = begin_transaction()
# ... 30 seconds of processing ...
# ... network calls ...
# ... file I/O ...
commit_transaction(tx_id)
# Locks held for 30 seconds!
```

**✅ Good: Minimal transaction scope**
```python
# Do expensive work outside transaction
order_data = prepare_order_data()  # 30 seconds
stock_updates = calculate_stock_updates()

# Transaction only for database operations
tx_id = begin_transaction()
create_order(tx_id, order_data)       # 10ms
update_stocks(tx_id, stock_updates)   # 10ms
commit_transaction(tx_id)
# Locks held for 20ms!
```

### 3. Handle Deadlocks

```python
def transfer_with_deadlock_retry(from_account, to_account, amount, max_retries=3):
    for attempt in range(max_retries):
        try:
            tx_id = begin_transaction()
            
            # Deduct from source
            deduct_balance(tx_id, from_account, amount)
            
            # Add to destination
            add_balance(tx_id, to_account, amount)
            
            commit_transaction(tx_id)
            return True
            
        except DeadlockException:
            rollback_transaction(tx_id)
            if attempt < max_retries - 1:
                time.sleep(random.uniform(0.1, 0.5))  # Random backoff
            else:
                raise
```

---

## Monitoring

### 1. Set Up Metrics Collection

**Prometheus configuration:**
```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'themisdb'
    static_configs:
      - targets: ['localhost:4318']
    scrape_interval: 15s
```

**Key metrics to monitor:**
```bash
# Query performance
themis_query_duration_seconds{quantile="0.99"}
themis_query_total

# Cache performance
themis_cache_hit_rate
themis_cache_size_bytes

# Database health
themis_active_connections
themis_transaction_duration_seconds
themis_index_usage_total
```

### 2. Set Up Alerting

**Alert rules:**
```yaml
# alerts.yml
groups:
  - name: themisdb
    rules:
      - alert: HighQueryLatency
        expr: themis_query_duration_seconds{quantile="0.99"} > 1.0
        for: 5m
        annotations:
          summary: "High query latency detected"
      
      - alert: LowCacheHitRate
        expr: themis_cache_hit_rate < 0.7
        for: 10m
        annotations:
          summary: "Cache hit rate below 70%"
```

### 3. Implement Health Checks

```python
import requests
import time

def health_check():
    """Comprehensive health check"""
    try:
        # Check basic connectivity
        response = requests.get("http://localhost:8080/health", timeout=5)
        if response.status_code != 200:
            return False, "Health endpoint failed"
        
        # Check write capability
        test_entity = f"health_check:{int(time.time())}"
        write_response = requests.put(
            f"http://localhost:8080/entities/{test_entity}",
            json={"blob": '{"test": true}'},
            timeout=5
        )
        if write_response.status_code != 200:
            return False, "Write test failed"
        
        # Clean up
        requests.delete(f"http://localhost:8080/entities/{test_entity}")
        
        return True, "All checks passed"
        
    except Exception as e:
        return False, f"Health check error: {str(e)}"
```

### 4. Log Slow Queries

**Server configuration:**
```yaml
# config.yaml
logging:
  slow_query_threshold_ms: 1000  # Log queries slower than 1s
  slow_query_log: /var/log/themis/slow_queries.log
```

**Analyze slow queries:**
```bash
cat /var/log/themis/slow_queries.log | \
  jq -r '[.duration_ms, .query.table, .query.predicates] | @csv' | \
  sort -t, -k1 -rn | \
  head -20
```

---

## Code Organization

### 1. Create a Database Client Class

```python
import requests
from typing import Dict, List, Optional

class ThemisDBClient:
    def __init__(self, base_url: str, api_key: Optional[str] = None):
        self.base_url = base_url.rstrip('/')
        self.session = requests.Session()
        if api_key:
            self.session.headers['Authorization'] = f'Bearer {api_key}'
    
    def create_entity(self, entity_id: str, data: Dict) -> Dict:
        """Create an entity"""
        response = self.session.put(
            f"{self.base_url}/entities/{entity_id}",
            json={"blob": json.dumps(data)}
        )
        response.raise_for_status()
        return response.json()
    
    def get_entity(self, entity_id: str) -> Optional[Dict]:
        """Get an entity"""
        response = self.session.get(f"{self.base_url}/entities/{entity_id}")
        if response.status_code == 404:
            return None
        response.raise_for_status()
        return response.json()
    
    def query(self, table: str, predicates: List[Dict], limit: int = 100) -> List[Dict]:
        """Query entities"""
        response = self.session.post(
            f"{self.base_url}/query",
            json={
                "table": table,
                "predicates": predicates,
                "limit": limit,
                "return": "entities"
            }
        )
        response.raise_for_status()
        return response.json()['entities']
    
    def batch_create(self, entities: List[Dict]) -> Dict:
        """Batch create entities"""
        response = self.session.post(
            f"{self.base_url}/batch/create",
            json={"entities": entities}
        )
        response.raise_for_status()
        return response.json()

# Usage
client = ThemisDBClient("http://localhost:8080", api_key="sk_...")
user = client.get_entity("users:alice")
```

### 2. Use Repository Pattern

```python
class UserRepository:
    def __init__(self, client: ThemisDBClient):
        self.client = client
    
    def create_user(self, user_id: str, name: str, email: str) -> Dict:
        """Create a user"""
        data = {"name": name, "email": email}
        return self.client.create_entity(f"users:{user_id}", data)
    
    def get_user(self, user_id: str) -> Optional[Dict]:
        """Get user by ID"""
        entity = self.client.get_entity(f"users:{user_id}")
        if entity:
            return json.loads(entity['blob'])
        return None
    
    def find_users_by_city(self, city: str) -> List[Dict]:
        """Find all users in a city"""
        entities = self.client.query(
            table="users",
            predicates=[{"column": "city", "value": city}]
        )
        return [json.loads(e['blob']) for e in entities]

# Usage
repo = UserRepository(client)
user = repo.get_user("alice")
berlin_users = repo.find_users_by_city("Berlin")
```

### 3. Configuration Management

```python
# config.py
import os
from dataclasses import dataclass

@dataclass
class DatabaseConfig:
    url: str
    api_key: str
    timeout: int
    max_retries: int
    
    @classmethod
    def from_env(cls):
        return cls(
            url=os.getenv("THEMIS_URL", "http://localhost:8080"),
            api_key=os.getenv("THEMIS_API_KEY", ""),
            timeout=int(os.getenv("THEMIS_TIMEOUT", "30")),
            max_retries=int(os.getenv("THEMIS_MAX_RETRIES", "3"))
        )

# Usage
config = DatabaseConfig.from_env()
client = ThemisDBClient(config.url, config.api_key)
```

---

## Production Checklist

Before deploying to production:

**Security:**
- [ ] TLS enabled with valid certificates
- [ ] Authentication configured
- [ ] RBAC roles defined
- [ ] Sensitive data encrypted
- [ ] Audit logging enabled
- [ ] Rate limiting configured

**Performance:**
- [ ] Connection pooling implemented
- [ ] Caching strategy in place
- [ ] All query paths have indexes
- [ ] Batch operations used for bulk data
- [ ] Query result limits enforced

**Reliability:**
- [ ] Error handling implemented
- [ ] Retry logic with backoff
- [ ] Circuit breakers configured
- [ ] Health checks implemented
- [ ] Backup strategy defined

**Monitoring:**
- [ ] Metrics collection configured
- [ ] Alerts set up
- [ ] Slow query logging enabled
- [ ] Dashboard created
- [ ] On-call rotation defined

**Operations:**
- [ ] Deployment playbook documented
- [ ] Rollback procedure tested
- [ ] Disaster recovery plan created
- [ ] Capacity planning done
- [ ] Team trained

---

## What You've Learned ✅

- ✅ Query optimization techniques
- ✅ Security hardening strategies
- ✅ Performance tuning patterns
- ✅ Robust error handling
- ✅ Transaction best practices
- ✅ Monitoring and observability
- ✅ Clean code organization

---

## Next Steps

1. **Review:** [Security Documentation](../security/)
2. **Deep Dive:** [Performance Tuning Guide](../performance/PERFORMANCE_GUIDE.md)
3. **Deploy:** [Production Deployment Guide](../deployment/PRODUCTION_DEPLOYMENT.md)

---

**Ready for production?** See [Operations Guide](../production/OPERATIONS.md) →
