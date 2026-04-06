# ThemisDB Transaction Management Best Practices

**Version:** 1.4.0  
**Last Updated:** 2026-04-06  
**Target Audience:** Application Developers, Database Administrators, Software Architects

---

## Table of Contents

1. [Transaction Isolation Levels](#transaction-isolation-levels)
2. [Transaction Sizing](#transaction-sizing)
3. [Conflict Handling Strategies](#conflict-handling-strategies)
4. [Common Transaction Patterns](#common-transaction-patterns)
5. [Anti-patterns to Avoid](#anti-patterns-to-avoid)
6. [Performance Optimization](#performance-optimization)

---

## Transaction Isolation Levels

### ReadCommitted vs Snapshot Detailed Comparison

**ReadCommitted Isolation:**

```cpp
// ReadCommitted: Reads latest committed data
auto txn = tm->begin(IsolationLevel::ReadCommitted);

// Time T1: Read user
auto user = txn->get("user:123");  // Reads version at seq=100

// Time T2: Another transaction commits (seq=101)
// Modifies "user:123" → new version at seq=101

// Time T3: Read same user again in same transaction
auto user2 = txn->get("user:123");  // Reads version at seq=101 (different!)

// Non-repeatable read: user != user2
txn->commit();
```

**Snapshot Isolation:**

```cpp
// Snapshot: Consistent point-in-time view
auto txn = tm->begin(IsolationLevel::Snapshot);
// Creates snapshot at seq=100

// Time T1: Read user
auto user = txn->get("user:123");  // Reads version at seq=100

// Time T2: Another transaction commits (seq=101)
// Modifies "user:123" → new version at seq=101

// Time T3: Read same user again
auto user2 = txn->get("user:123");  // Still reads version at seq=100

// Repeatable read: user == user2 (consistent snapshot)
txn->commit();
```

**Key Differences:**

| Aspect | ReadCommitted | Snapshot |
|--------|---------------|----------|
| **Snapshot Created** | No | Yes (at BEGIN) |
| **Read Consistency** | Latest committed data | Point-in-time view |
| **Repeatable Reads** | ❌ No | ✅ Yes |
| **Phantom Reads** | ✅ Possible | ❌ Not possible |
| **Performance** | ✅ Faster (10-20%) | ⚠️ Baseline |
| **Memory Usage** | ✅ Lower | ⚠️ Higher (snapshot overhead) |
| **Snapshot Cleanup** | N/A | Required |

### When to Use Each Level

**Use ReadCommitted For:**

✅ **Point Queries**
```python
# Single-key lookup
def get_user(user_id):
    txn = db.begin(IsolationLevel.ReadCommitted)
    user = txn.get(f"user:{user_id}")
    txn.commit()
    return user
```

✅ **Simple Updates**
```python
# Update single record
def update_user_email(user_id, new_email):
    txn = db.begin(IsolationLevel.ReadCommitted)
    user = txn.get(f"user:{user_id}")
    user['email'] = new_email
    txn.put(f"user:{user_id}", user)
    txn.commit()
```

✅ **High-Throughput OLTP**
```python
# Process payment (no complex business logic)
def process_payment(payment_id):
    txn = db.begin(IsolationLevel.ReadCommitted)
    payment = txn.get(f"payment:{payment_id}")
    payment['status'] = 'processed'
    txn.put(f"payment:{payment_id}", payment)
    txn.commit()
```

**Use Snapshot For:**

✅ **Multi-Row Reads**
```python
# Generate report with consistent data
def generate_user_report(user_id):
    txn = db.begin(IsolationLevel.Snapshot)  # Consistent view
    
    user = txn.get(f"user:{user_id}")
    orders = txn.query("SELECT * FROM orders WHERE user_id = ?", user_id)
    payments = txn.query("SELECT * FROM payments WHERE user_id = ?", user_id)
    
    # All data from same snapshot → consistent
    report = calculate_report(user, orders, payments)
    txn.commit()
    return report
```

✅ **Business Logic Validation**
```python
# Transfer money (requires consistent balance check)
def transfer_money(from_account, to_account, amount):
    txn = db.begin(IsolationLevel.Snapshot)
    
    # Read balances (must be consistent)
    from_balance = txn.get(f"balance:{from_account}")
    to_balance = txn.get(f"balance:{to_account}")
    
    # Validate (consistent snapshot ensures no race condition)
    if from_balance < amount:
        txn.rollback()
        raise InsufficientFundsError()
    
    # Update both accounts
    txn.put(f"balance:{from_account}", from_balance - amount)
    txn.put(f"balance:{to_account}", to_balance + amount)
    
    txn.commit()
```

✅ **Analytics Queries**
```python
# Long-running analytics (needs consistent view)
def calculate_daily_revenue(date):
    txn = db.begin(IsolationLevel.Snapshot)
    txn.setTimeout(300000)  # 5 minutes
    
    # All queries see same snapshot
    orders = txn.query("SELECT * FROM orders WHERE date = ?", date)
    refunds = txn.query("SELECT * FROM refunds WHERE date = ?", date)
    
    revenue = sum(o.amount for o in orders) - sum(r.amount for r in refunds)
    
    txn.commit()
    return revenue
```

### Performance Implications

**Benchmark Results:**

```python
# Workload: 10K transactions, 16 threads, 1KB values

isolation_levels = {
    'ReadCommitted': {
        'throughput_tps': 52000,
        'latency_p50_ms': 0.3,
        'latency_p99_ms': 1.2,
        'memory_mb': 512,
    },
    'Snapshot': {
        'throughput_tps': 43000,  # ~17% slower
        'latency_p50_ms': 0.37,   # +23% latency
        'latency_p99_ms': 1.5,    # +25% latency
        'memory_mb': 768,         # +50% memory
    },
}

for level, metrics in isolation_levels.items():
    print(f"{level}: {metrics}")
```

**Cost Analysis:**

```python
def isolation_level_cost_analysis(workload):
    """
    Estimate performance cost of Snapshot vs ReadCommitted.
    """
    baseline_tps = workload['tps']
    
    # Snapshot overhead
    snapshot_overhead_pct = 0.17  # 17% throughput reduction
    memory_overhead_mb = 256  # Additional 256MB for snapshots
    
    snapshot_tps = baseline_tps * (1 - snapshot_overhead_pct)
    
    return {
        'ReadCommitted': {
            'tps': baseline_tps,
            'memory_mb': workload['memory_mb']
        },
        'Snapshot': {
            'tps': snapshot_tps,
            'memory_mb': workload['memory_mb'] + memory_overhead_mb,
            'cost_pct': snapshot_overhead_pct * 100
        }
    }

# Example
result = isolation_level_cost_analysis({
    'tps': 50000,
    'memory_mb': 512
})
print(result)
# Output: Snapshot costs ~17% throughput, +256MB memory
```

### Isolation Level Trade-offs Table

| Criteria | ReadCommitted | Snapshot | Winner |
|----------|---------------|----------|--------|
| **Throughput** | 52K TPS | 43K TPS | ReadCommitted |
| **Latency (p50)** | 0.3ms | 0.37ms | ReadCommitted |
| **Latency (p99)** | 1.2ms | 1.5ms | ReadCommitted |
| **Memory Usage** | 512MB | 768MB | ReadCommitted |
| **Read Consistency** | ❌ Non-repeatable | ✅ Repeatable | Snapshot |
| **Phantom Prevention** | ❌ No | ✅ Yes | Snapshot |
| **Complex Queries** | ❌ Inconsistent | ✅ Consistent | Snapshot |
| **Analytics** | ❌ Not recommended | ✅ Required | Snapshot |
| **OLTP** | ✅ Ideal | ⚠️ Acceptable | ReadCommitted |
| **Financial Logic** | ⚠️ Risk of inconsistency | ✅ Safe | Snapshot |

---

## Transaction Sizing

### Optimal Transaction Sizes

**Guidelines:**

| Transaction Size | Operations | Duration | Use Case | Conflict Risk |
|------------------|------------|----------|----------|---------------|
| **Micro** | 1-10 ops | < 1ms | Point queries, cache updates | ✅ Very Low |
| **Small** | 10-100 ops | 1-10ms | User requests, API calls | ✅ Low |
| **Medium** | 100-1K ops | 10-100ms | Batch updates, background jobs | ⚠️ Medium |
| **Large** | 1K-10K ops | 100ms-1s | Data migrations, bulk imports | ⚠️ High |
| **XLarge** | > 10K ops | > 1s | ❌ Avoid (split into smaller) | ❌ Very High |

**Example Sizing:**

```python
# Micro transaction: Single operation
def get_user(user_id):
    txn = db.begin()
    user = txn.get(f"user:{user_id}")
    txn.commit()
    return user  # ~1 operation, < 1ms

# Small transaction: Few operations
def create_order(user_id, items):
    txn = db.begin()
    order_id = generate_id()
    
    # Create order
    txn.put(f"order:{order_id}", {
        'user_id': user_id,
        'items': items,
        'status': 'pending'
    })
    
    # Create order items
    for item in items:
        txn.put(f"order_item:{order_id}:{item['id']}", item)
    
    # Update user order count
    user = txn.get(f"user:{user_id}")
    user['order_count'] += 1
    txn.put(f"user:{user_id}", user)
    
    txn.commit()
    return order_id  # ~5-10 operations, 5-10ms

# Medium transaction: Batch processing
def process_daily_orders(orders):
    BATCH_SIZE = 100
    
    for i in range(0, len(orders), BATCH_SIZE):
        batch = orders[i:i + BATCH_SIZE]
        
        txn = db.begin()
        for order in batch:
            order['status'] = 'processed'
            txn.put(f"order:{order['id']}", order)
        txn.commit()
        # ~100 operations, 50-100ms per transaction
```

### Small Transactions vs Large Batches

**Small Transaction Approach:**

```python
# Pros: Low conflict risk, fast commits
# Cons: Higher overhead, more network roundtrips

def import_users_small_txn(users):
    for user in users:
        txn = db.begin()
        txn.put(f"user:{user['id']}", user)
        txn.commit()
        # Each user in separate transaction
    
# Performance:
# - 1000 users: ~5 seconds
# - Conflict risk: Very low
# - Memory usage: Very low
```

**Large Batch Approach:**

```python
# Pros: Lower overhead, fewer commits
# Cons: Higher conflict risk, longer lock duration

def import_users_large_batch(users):
    txn = db.begin()
    for user in users:
        txn.put(f"user:{user['id']}", user)
    txn.commit()
    # All users in single transaction

# Performance:
# - 1000 users: ~2 seconds
# - Conflict risk: High (holds locks longer)
# - Memory usage: High (large WriteBatch)
```

**Balanced Approach (Recommended):**

```python
# Best of both worlds: Medium-sized batches

def import_users_balanced(users, batch_size=100):
    for i in range(0, len(users), batch_size):
        batch = users[i:i + batch_size]
        
        txn = db.begin()
        for user in batch:
            txn.put(f"user:{user['id']}", user)
        txn.commit()
        
        # Allow other transactions to proceed
        time.sleep(0.01)

# Performance:
# - 1000 users: ~2.5 seconds
# - Conflict risk: Low (shorter lock duration)
# - Memory usage: Medium (manageable WriteBatch)
```

### Batch Operation Patterns

**Pattern 1: Chunked Import**

```python
def chunked_import(data, chunk_size=500):
    """
    Import large dataset in chunks.
    """
    total = len(data)
    processed = 0
    
    for i in range(0, total, chunk_size):
        chunk = data[i:i + chunk_size]
        
        txn = db.begin()
        for item in chunk:
            txn.put(f"item:{item['id']}", item)
        
        try:
            txn.commit()
            processed += len(chunk)
            print(f"Progress: {processed}/{total} ({processed/total*100:.1f}%)")
        except ConflictError:
            print(f"Conflict on chunk {i}, retrying...")
            # Retry logic
```

**Pattern 2: Parallel Batching**

```python
from concurrent.futures import ThreadPoolExecutor

def parallel_batch_insert(data, num_workers=4, batch_size=100):
    """
    Parallel batch insert with multiple connections.
    """
    def process_batch(batch):
        txn = db.begin()
        for item in batch:
            txn.put(f"item:{item['id']}", item)
        txn.commit()
        return len(batch)
    
    # Partition data into batches
    batches = [
        data[i:i + batch_size]
        for i in range(0, len(data), batch_size)
    ]
    
    # Process in parallel
    with ThreadPoolExecutor(max_workers=num_workers) as executor:
        results = executor.map(process_batch, batches)
    
    return sum(results)
```

**Pattern 3: Streaming Import**

```python
def streaming_import(data_stream, batch_size=100):
    """
    Import streaming data with batching.
    """
    batch = []
    
    for item in data_stream:
        batch.append(item)
        
        if len(batch) >= batch_size:
            txn = db.begin()
            for b in batch:
                txn.put(f"item:{b['id']}", b)
            txn.commit()
            batch = []  # Clear batch
    
    # Flush remaining items
    if batch:
        txn = db.begin()
        for b in batch:
            txn.put(f"item:{b['id']}", b)
        txn.commit()
```

### Multi-Statement Transaction Examples

**Example 1: Order Processing**

```python
def process_order_transaction(order_id):
    """
    Multi-step order processing in single transaction.
    """
    txn = db.begin(IsolationLevel.Snapshot)
    
    try:
        # Step 1: Get order
        order = txn.get(f"order:{order_id}")
        
        # Step 2: Validate inventory
        for item in order['items']:
            inventory = txn.get(f"inventory:{item['product_id']}")
            if inventory['quantity'] < item['quantity']:
                raise InsufficientInventoryError()
        
        # Step 3: Deduct inventory
        for item in order['items']:
            inventory = txn.get(f"inventory:{item['product_id']}")
            inventory['quantity'] -= item['quantity']
            txn.put(f"inventory:{item['product_id']}", inventory)
        
        # Step 4: Update order status
        order['status'] = 'confirmed'
        txn.put(f"order:{order_id}", order)
        
        # Step 5: Create shipment
        shipment_id = generate_id()
        txn.put(f"shipment:{shipment_id}", {
            'order_id': order_id,
            'status': 'pending'
        })
        
        # All or nothing
        txn.commit()
        return shipment_id
        
    except Exception as e:
        txn.rollback()
        raise
```

**Example 2: Money Transfer**

```python
def transfer_money(from_account, to_account, amount):
    """
    Atomic money transfer.
    """
    txn = db.begin(IsolationLevel.Snapshot)
    
    try:
        # Read both account balances
        from_balance = txn.get(f"balance:{from_account}")
        to_balance = txn.get(f"balance:{to_account}")
        
        # Validate balance
        if from_balance['amount'] < amount:
            raise InsufficientFundsError()
        
        # Debit from source
        from_balance['amount'] -= amount
        from_balance['last_updated'] = datetime.now()
        txn.put(f"balance:{from_account}", from_balance)
        
        # Credit to destination
        to_balance['amount'] += amount
        to_balance['last_updated'] = datetime.now()
        txn.put(f"balance:{to_account}", to_balance)
        
        # Record transaction
        transfer_id = generate_id()
        txn.put(f"transfer:{transfer_id}", {
            'from': from_account,
            'to': to_account,
            'amount': amount,
            'timestamp': datetime.now()
        })
        
        # Commit atomically
        txn.commit()
        return transfer_id
        
    except Exception as e:
        txn.rollback()
        raise
```

---

## Conflict Handling Strategies

### Write-Write Conflict Detection

**Detection Mechanism:**

```cpp
// ThemisDB detects write-write conflicts via RocksDB TransactionDB

Transaction T1:
  BEGIN
  PUT "user:123" = "Alice"  // Acquires write lock
  // ... long-running operation ...
  COMMIT  // Releases lock

Transaction T2 (concurrent):
  BEGIN
  PUT "user:123" = "Bob"  // Tries to acquire write lock
  // BLOCKED or TIMEOUT (conflict detected)
  
// T2 receives Status::Conflict error
```

**Application Handling:**

```python
def update_with_conflict_detection(key, value):
    """
    Update with explicit conflict detection.
    """
    txn = db.begin()
    
    try:
        # Attempt write
        txn.put(key, value)
        txn.commit()
        return True
        
    except ConflictError as e:
        txn.rollback()
        print(f"Conflict detected on key: {key}")
        print(f"Error: {e}")
        return False
```

### Automatic Retry Mechanisms

**Simple Retry:**

```python
def execute_with_retry(operation, max_attempts=3):
    """
    Retry operation on conflict.
    """
    for attempt in range(max_attempts):
        try:
            return operation()
        except ConflictError:
            if attempt == max_attempts - 1:
                raise  # Give up
            print(f"Conflict, retrying ({attempt + 1}/{max_attempts})")
            time.sleep(0.1)  # Brief delay

# Usage
def update_counter(counter_id):
    def operation():
        txn = db.begin()
        counter = txn.get(f"counter:{counter_id}")
        counter['value'] += 1
        txn.put(f"counter:{counter_id}", counter)
        txn.commit()
        return counter['value']
    
    return execute_with_retry(operation)
```

**Configuration-Based Retry:**

```yaml
# ThemisDB configuration
transaction:
  enable_auto_retry: true
  max_retry_attempts: 3
  retry_backoff_ms: 10
```

### Manual Conflict Resolution

**Optimistic Locking with Version:**

```python
def update_with_version_check(key, update_fn, max_attempts=10):
    """
    Optimistic locking using version field.
    """
    for attempt in range(max_attempts):
        txn = db.begin()
        
        try:
            # Read current version
            data = txn.get(key)
            current_version = data.get('version', 0)
            
            # Apply update
            updated_data = update_fn(data)
            updated_data['version'] = current_version + 1
            
            # Write with version check
            # (Application-level check, not DB-level)
            existing = txn.get(key)
            if existing['version'] != current_version:
                # Version changed, conflict!
                txn.rollback()
                continue
            
            txn.put(key, updated_data)
            txn.commit()
            return updated_data
            
        except ConflictError:
            txn.rollback()
            # Retry
    
    raise MaxRetriesExceededError()

# Usage
def increment_likes(post_id):
    def update(post):
        post['likes'] += 1
        return post
    
    return update_with_version_check(f"post:{post_id}", update)
```

**Last-Write-Wins Strategy:**

```python
def last_write_wins_update(key, value):
    """
    Last write wins (no conflict detection).
    """
    txn = db.begin(IsolationLevel.ReadCommitted)
    
    # No read, just write
    txn.put(key, value)
    txn.commit()
    
    # No conflicts (overwrites any concurrent writes)
```

### Exponential Backoff Implementation

**Basic Exponential Backoff:**

```python
import time
import random

def exponential_backoff_retry(operation, max_attempts=5):
    """
    Retry with exponential backoff.
    """
    base_delay = 0.1  # 100ms
    max_delay = 10.0  # 10 seconds
    
    for attempt in range(max_attempts):
        try:
            return operation()
            
        except ConflictError:
            if attempt == max_attempts - 1:
                raise
            
            # Calculate backoff delay
            delay = min(base_delay * (2 ** attempt), max_delay)
            
            # Add jitter to prevent thundering herd
            jitter = delay * random.random()
            
            sleep_time = delay + jitter
            print(f"Retry {attempt + 1}/{max_attempts} after {sleep_time:.2f}s")
            time.sleep(sleep_time)

# Usage
result = exponential_backoff_retry(lambda: update_counter("global_counter"))
```

**Advanced Backoff with Metrics:**

```python
class ExponentialBackoffRetry:
    def __init__(self, max_attempts=5, base_delay=0.1, max_delay=10.0):
        self.max_attempts = max_attempts
        self.base_delay = base_delay
        self.max_delay = max_delay
        self.metrics = {'total_attempts': 0, 'conflicts': 0, 'successes': 0}
    
    def execute(self, operation):
        for attempt in range(self.max_attempts):
            self.metrics['total_attempts'] += 1
            
            try:
                result = operation()
                self.metrics['successes'] += 1
                return result
                
            except ConflictError as e:
                self.metrics['conflicts'] += 1
                
                if attempt == self.max_attempts - 1:
                    raise
                
                delay = min(self.base_delay * (2 ** attempt), self.max_delay)
                jitter = delay * random.random()
                time.sleep(delay + jitter)
    
    def get_metrics(self):
        return self.metrics

# Usage with monitoring
retry_handler = ExponentialBackoffRetry(max_attempts=5)
result = retry_handler.execute(lambda: update_user("user:123"))
print(f"Metrics: {retry_handler.get_metrics()}")
```

---

## Common Transaction Patterns

### CRUD Patterns

**Create:**

```python
def create_entity(entity_type, entity_id, data):
    """
    Create new entity.
    """
    txn = db.begin()
    
    # Check if exists
    exists = txn.exists(f"{entity_type}:{entity_id}")
    if exists:
        txn.rollback()
        raise EntityAlreadyExistsError()
    
    # Create entity
    data['created_at'] = datetime.now()
    data['updated_at'] = datetime.now()
    txn.put(f"{entity_type}:{entity_id}", data)
    
    # Update index
    txn.put(f"index:{entity_type}", lambda idx: idx.add(entity_id))
    
    txn.commit()
    return entity_id
```

**Read:**

```python
def read_entity(entity_type, entity_id):
    """
    Read entity (no transaction needed for single read).
    """
    txn = db.begin(IsolationLevel.ReadCommitted)
    entity = txn.get(f"{entity_type}:{entity_id}")
    txn.commit()
    return entity
```

**Update:**

```python
def update_entity(entity_type, entity_id, updates):
    """
    Update entity with optimistic locking.
    """
    def operation():
        txn = db.begin(IsolationLevel.Snapshot)
        
        # Read current state
        entity = txn.get(f"{entity_type}:{entity_id}")
        
        # Apply updates
        for key, value in updates.items():
            entity[key] = value
        entity['updated_at'] = datetime.now()
        
        # Write back
        txn.put(f"{entity_type}:{entity_id}", entity)
        txn.commit()
        
        return entity
    
    return execute_with_retry(operation)
```

**Delete:**

```python
def delete_entity(entity_type, entity_id):
    """
    Delete entity and clean up references.
    """
    txn = db.begin()
    
    # Delete entity
    txn.delete(f"{entity_type}:{entity_id}")
    
    # Clean up index
    txn.put(f"index:{entity_type}", lambda idx: idx.remove(entity_id))
    
    # Soft delete alternative: Mark as deleted
    # entity = txn.get(f"{entity_type}:{entity_id}")
    # entity['deleted'] = True
    # entity['deleted_at'] = datetime.now()
    # txn.put(f"{entity_type}:{entity_id}", entity)
    
    txn.commit()
```

### Distributed Transactions (SAGA Pattern)

**SAGA Pattern Implementation:**

```python
class Saga:
    """
    SAGA pattern for distributed transactions.
    """
    def __init__(self):
        self.steps = []
        self.compensations = []
    
    def add_step(self, forward_fn, compensate_fn):
        """
        Add step with compensation.
        """
        self.steps.append(forward_fn)
        self.compensations.append(compensate_fn)
    
    def execute(self):
        """
        Execute SAGA with automatic compensation on failure.
        """
        executed_steps = []
        
        try:
            # Execute forward steps
            for i, step in enumerate(self.steps):
                result = step()
                executed_steps.append(i)
                
        except Exception as e:
            # Rollback executed steps in reverse order
            print(f"SAGA failed at step {len(executed_steps)}: {e}")
            
            for step_index in reversed(executed_steps):
                compensate_fn = self.compensations[step_index]
                try:
                    compensate_fn()
                except Exception as comp_error:
                    print(f"Compensation failed: {comp_error}")
            
            raise
        
        return True

# Example: Multi-service order processing
def process_order_saga(order_id):
    saga = Saga()
    
    # Step 1: Reserve inventory
    def reserve_inventory():
        return inventory_service.reserve(order_id)
    
    def release_inventory():
        inventory_service.release(order_id)
    
    saga.add_step(reserve_inventory, release_inventory)
    
    # Step 2: Charge payment
    def charge_payment():
        return payment_service.charge(order_id)
    
    def refund_payment():
        payment_service.refund(order_id)
    
    saga.add_step(charge_payment, refund_payment)
    
    # Step 3: Create shipment
    def create_shipment():
        return shipment_service.create(order_id)
    
    def cancel_shipment():
        shipment_service.cancel(order_id)
    
    saga.add_step(create_shipment, cancel_shipment)
    
    # Execute SAGA
    return saga.execute()
```

### Compensating Transactions

**Pattern:**

```python
def transfer_with_compensation(from_account, to_account, amount):
    """
    Transfer money with explicit compensation logic.
    """
    compensation_needed = False
    compensation_data = {}
    
    txn = db.begin()
    
    try:
        # Forward transaction
        from_balance = txn.get(f"balance:{from_account}")
        from_balance['amount'] -= amount
        txn.put(f"balance:{from_account}", from_balance)
        compensation_data['from_account'] = from_account
        compensation_data['amount'] = amount
        compensation_needed = True
        
        to_balance = txn.get(f"balance:{to_account}")
        to_balance['amount'] += amount
        txn.put(f"balance:{to_account}", to_balance)
        
        txn.commit()
        return True
        
    except Exception as e:
        txn.rollback()
        
        # Compensate if needed
        if compensation_needed:
            compensate_txn = db.begin()
            from_balance = compensate_txn.get(f"balance:{from_account}")
            from_balance['amount'] += compensation_data['amount']
            compensate_txn.put(f"balance:{from_account}", from_balance)
            compensate_txn.commit()
        
        raise
```

### Atomic Multi-Key Updates

**Pattern:**

```python
def atomic_multi_key_update(updates):
    """
    Update multiple keys atomically.
    """
    txn = db.begin(IsolationLevel.Snapshot)
    
    try:
        # Read all keys first (for validation)
        current_values = {}
        for key in updates.keys():
            current_values[key] = txn.get(key)
        
        # Validate updates
        for key, new_value in updates.items():
            if not validate_update(current_values[key], new_value):
                raise ValidationError(f"Invalid update for key: {key}")
        
        # Apply all updates
        for key, new_value in updates.items():
            txn.put(key, new_value)
        
        # All or nothing
        txn.commit()
        return True
        
    except Exception as e:
        txn.rollback()
        raise
```

---

## Anti-patterns to Avoid

### Long-Running Transactions

**❌ Anti-pattern:**

```python
# BAD: Long-running transaction
def bad_batch_process():
    txn = db.begin()
    
    # Process millions of records (takes hours!)
    for i in range(10_000_000):
        record = fetch_record(i)
        processed = process(record)
        txn.put(f"record:{i}", processed)
    
    txn.commit()  # Holds locks for hours!
```

**✅ Solution:**

```python
# GOOD: Chunked processing
def good_batch_process():
    CHUNK_SIZE = 1000
    
    for chunk_start in range(0, 10_000_000, CHUNK_SIZE):
        txn = db.begin()
        
        for i in range(chunk_start, min(chunk_start + CHUNK_SIZE, 10_000_000)):
            record = fetch_record(i)
            processed = process(record)
            txn.put(f"record:{i}", processed)
        
        txn.commit()  # Releases locks quickly
        time.sleep(0.01)  # Let other transactions proceed
```

### Nested Transactions

**❌ Anti-pattern:**

```python
# BAD: Nested transactions (not supported)
def bad_nested_txn():
    outer_txn = db.begin()
    
    # Do some work
    outer_txn.put("key1", "value1")
    
    # Try to nest (WILL FAIL!)
    inner_txn = db.begin()  # Error or undefined behavior
    inner_txn.put("key2", "value2")
    inner_txn.commit()
    
    outer_txn.commit()
```

**✅ Solution:**

```python
# GOOD: Sequential transactions
def good_sequential_txn():
    # First transaction
    txn1 = db.begin()
    txn1.put("key1", "value1")
    txn1.commit()
    
    # Second transaction
    txn2 = db.begin()
    txn2.put("key2", "value2")
    txn2.commit()
```

### Deadlock-Prone Patterns

**❌ Anti-pattern:**

```python
# BAD: Different lock order in concurrent transactions
# Transaction 1:
def transfer_a_to_b():
    txn = db.begin()
    a = txn.get("account:A")  # Lock A
    b = txn.get("account:B")  # Lock B
    # ... transfer logic ...
    txn.commit()

# Transaction 2:
def transfer_b_to_a():
    txn = db.begin()
    b = txn.get("account:B")  # Lock B
    a = txn.get("account:A")  # Lock A (DEADLOCK!)
    # ... transfer logic ...
    txn.commit()
```

**✅ Solution:**

```python
# GOOD: Consistent lock order
def transfer(from_account, to_account, amount):
    # Always lock in alphabetical order
    first_account = min(from_account, to_account)
    second_account = max(from_account, to_account)
    
    txn = db.begin()
    
    first = txn.get(f"account:{first_account}")
    second = txn.get(f"account:{second_account}")
    
    # Determine which is from/to
    if from_account == first_account:
        first['balance'] -= amount
        second['balance'] += amount
    else:
        first['balance'] += amount
        second['balance'] -= amount
    
    txn.put(f"account:{first_account}", first)
    txn.put(f"account:{second_account}", second)
    
    txn.commit()
```

### Resource Exhaustion Patterns

**❌ Anti-pattern:**

```python
# BAD: Unbounded transaction growth
def bad_unlimited_txn():
    txn = db.begin()
    
    # Read unbounded data
    for record in unbounded_stream():  # Could be millions of records!
        txn.put(f"record:{record.id}", record)
    
    txn.commit()  # OOM or transaction too large error
```

**✅ Solution:**

```python
# GOOD: Bounded transaction size
def good_bounded_txn(max_txn_size=1000):
    txn = db.begin()
    count = 0
    
    for record in stream():
        txn.put(f"record:{record.id}", record)
        count += 1
        
        if count >= max_txn_size:
            txn.commit()
            txn = db.begin()  # Start new transaction
            count = 0
    
    if count > 0:
        txn.commit()  # Commit remaining
```

---

## Performance Optimization

### Batch Insert/Update Patterns

**Optimized Batch Insert:**

```python
def optimized_batch_insert(entities, batch_size=1000):
    """
    High-performance batch insert.
    """
    total = len(entities)
    inserted = 0
    start_time = time.time()
    
    for i in range(0, total, batch_size):
        batch = entities[i:i + batch_size]
        
        # Use ReadCommitted for faster writes
        txn = db.begin(IsolationLevel.ReadCommitted)
        
        for entity in batch:
            txn.put(f"entity:{entity['id']}", entity)
        
        txn.commit()
        
        inserted += len(batch)
        
        # Progress reporting
        if inserted % 10000 == 0:
            elapsed = time.time() - start_time
            rate = inserted / elapsed
            eta = (total - inserted) / rate
            print(f"Inserted {inserted}/{total} ({rate:.0f} ops/sec, ETA: {eta:.0f}s)")
    
    elapsed = time.time() - start_time
    print(f"Completed: {total} entities in {elapsed:.2f}s ({total/elapsed:.0f} ops/sec)")
```

### Connection Pooling Recommendations

**Configuration:**

```python
from themisdb import ConnectionPool

# Create connection pool
pool = ConnectionPool(
    host='localhost',
    port=8765,
    min_connections=5,   # Minimum pool size
    max_connections=50,  # Maximum pool size
    max_idle_time=300,   # 5 minutes idle timeout
    health_check_interval=30  # Health check every 30s
)

# Use connection from pool
def handle_request(request):
    with pool.get_connection() as conn:
        # Connection automatically returned to pool
        result = conn.query(request.sql)
        return result

# Metrics
print(pool.get_stats())
# Output: {'total': 50, 'active': 25, 'idle': 25, 'wait_count': 5}
```

**Best Practices:**

| Scenario | Min Connections | Max Connections | Rationale |
|----------|-----------------|-----------------|-----------|
| **Low Traffic** | 2-5 | 10-20 | Minimize idle connections |
| **Medium Traffic** | 10-20 | 50-100 | Balance responsiveness and resources |
| **High Traffic** | 20-50 | 100-200 | Prevent connection creation overhead |
| **Burst Traffic** | 10 | 500+ | Handle spikes |

### Statement Preparation/Caching

**Query Preparation:**

```python
# Prepare frequently used queries
prepared_queries = {
    'get_user': db.prepare("SELECT * FROM users WHERE id = ?"),
    'get_orders': db.prepare("SELECT * FROM orders WHERE user_id = ? AND status = ?"),
    'update_user': db.prepare("UPDATE users SET email = ? WHERE id = ?"),
}

# Execute prepared query
def get_user(user_id):
    return prepared_queries['get_user'].execute(user_id)

# Performance benefit: ~10-20% faster than ad-hoc queries
```

### Timeout Configuration

**Workload-Specific Timeouts:**

```yaml
# Production configuration
transaction:
  # Default timeout for all transactions
  default_timeout_ms: 30000  # 30 seconds
  
  # Per-operation type timeouts
  operation_timeouts:
    point_read: 1000      # 1 second
    range_scan: 5000      # 5 seconds
    batch_write: 60000    # 60 seconds
    analytics: 300000     # 5 minutes
  
  # Lock timeouts
  lock_timeout_ms: 10000  # 10 seconds
```

**Application-Level Override:**

```python
# Short timeout for interactive request
def get_user_for_api(user_id):
    txn = db.begin()
    txn.setTimeout(1000)  # 1 second timeout
    user = txn.get(f"user:{user_id}")
    txn.commit()
    return user

# Long timeout for batch job
def batch_export():
    txn = db.begin()
    txn.setTimeout(300000)  # 5 minutes
    data = txn.query("SELECT * FROM large_table")
    txn.commit()
    return data
```

---

## Related Documentation

- [Production Deployment Guide](../deployment/PRODUCTION_DEPLOYMENT_GUIDE.md)
- [MVCC Tuning Guide](MVCC_TUNING_GUIDE.md)
- [RocksDB Optimization Guide](../storage/ROCKSDB_OPTIMIZATION_GUIDE.md)
- [Operational Procedures](../operations/OPERATIONAL_PROCEDURES.md)
- [Monitoring Setup Guide](../operations/MONITORING_SETUP_GUIDE.md)
- [Troubleshooting Guide](../operations/TROUBLESHOOTING_GUIDE.md)

---

**Document Version:** 1.0  
**ThemisDB Compatibility:** 1.4.0+  
**Last Reviewed:** 2026-01-18
