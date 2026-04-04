# Interactive Examples

Learn ThemisDB by doing! This guide provides hands-on code examples, try-it-yourself exercises, and real-world scenarios you can run immediately.

## 🎯 What You'll Find

- ✅ Quick code snippets with explanations
- ✅ Try-it-yourself exercises
- ✅ Common patterns and recipes
- ✅ Real-world scenarios
- ✅ Links to runnable example projects

**Prerequisites:** [Getting Started Tutorial](GETTING_STARTED_TUTORIAL.md)  
**Time Required:** Varies by example  
**Difficulty:** Beginner to Advanced

---

## Table of Contents

1. [Quick Start Examples](#quick-start-examples)
2. [Common Patterns](#common-patterns)
3. [Real-World Scenarios](#real-world-scenarios)
4. [Interactive Exercises](#interactive-exercises)
5. [Example Projects](#example-projects)

---

## Quick Start Examples

### Example 1: Hello World

**Goal:** Create, read, update, and delete your first entity

```bash
# 1. Create a user
curl -X PUT http://localhost:8080/entities/users:john \
  -H "Content-Type: application/json" \
  -d '{
    "blob": "{\"name\":\"John Doe\",\"email\":\"john@example.com\",\"age\":28}"
  }'

# 2. Read the user
curl http://localhost:8080/entities/users:john

# 3. Update the user
curl -X PUT http://localhost:8080/entities/users:john \
  -H "Content-Type: application/json" \
  -d '{
    "blob": "{\"name\":\"John Doe\",\"email\":\"john@example.com\",\"age\":29}"
  }'

# 4. Delete the user
curl -X DELETE http://localhost:8080/entities/users:john
```

**Expected Output:**
```json
// Step 1: {"status":"success","entity":"users:john","version":1}
// Step 2: {"entity_id":"users:john","version":1,"blob":"{...}"}
// Step 3: {"status":"success","entity":"users:john","version":2}
// Step 4: {"status":"success","entity":"users:john","deleted":true}
```

**Try It Yourself:**
1. Change the entity to create a product instead
2. Add more attributes (category, price, stock)
3. Create 3 products and query them

---

### Example 2: Batch Insert Products

**Goal:** Insert 100 products efficiently

**Python:**
```python
import requests
import json

def create_products(count=100):
    base_url = "http://localhost:8080"
    
    # Prepare batch
    entities = []
    for i in range(count):
        entity = {
            "entity_id": f"products:item-{i:04d}",
            "blob": json.dumps({
                "name": f"Product {i}",
                "price": 10.0 + (i % 50),
                "stock": 100 + (i % 100),
                "category": f"category-{i % 5}"
            })
        }
        entities.append(entity)
    
    # Batch create
    response = requests.post(
        f"{base_url}/batch/create",
        json={"entities": entities}
    )
    
    result = response.json()
    print(f"Created {result['created']} products")
    return result

# Run it!
if __name__ == "__main__":
    result = create_products(100)
```

**Expected Output:**
```
Created 100 products
```

**Try It Yourself:**
1. Increase to 1000 products
2. Add more attributes (description, tags, ratings)
3. Measure the time taken

---

### Example 3: Query with Index

**Goal:** Create an index and perform fast queries

```bash
# 1. Create index on category
curl -X POST http://localhost:8080/index/create \
  -H "Content-Type: application/json" \
  -d '{
    "table": "products",
    "column": "category",
    "type": "btree"
  }'

# 2. Query products by category
curl -X POST http://localhost:8080/query \
  -H "Content-Type: application/json" \
  -d '{
    "table": "products",
    "predicates": [
      {
        "column": "category",
        "value": "category-0"
      }
    ],
    "use_index": true,
    "return": "entities"
  }'

# 3. Check index usage
curl http://localhost:4318/metrics | grep index_usage
```

**Try It Yourself:**
1. Create a composite index on (category, price)
2. Query products in category-0 with price < 30
3. Use EXPLAIN to see query plan

---

## Common Patterns

### Pattern 1: Upsert (Create or Update)

**Problem:** Need to create an entity if it doesn't exist, or update if it does.

**Solution:**
```python
import requests
import json

def upsert_entity(entity_id: str, data: dict):
    """Create or update an entity"""
    url = f"http://localhost:8080/entities/{entity_id}/upsert"
    
    response = requests.post(url, json={"blob": json.dumps(data)})
    return response.json()

# Usage
user_data = {"name": "Alice", "email": "alice@example.com", "age": 30}
result = upsert_entity("users:alice", user_data)
print(result)  # Creates or updates Alice
```

**Try It Yourself:**
1. Run it once (creates)
2. Change age to 31 and run again (updates)
3. Verify version incremented

---

### Pattern 2: Atomic Counter

**Problem:** Need to increment/decrement a value atomically (thread-safe).

**Solution:**
```python
def decrement_stock(product_id: str, quantity: int = 1):
    """Atomically decrement product stock"""
    url = f"http://localhost:8080/entities/{product_id}/increment"
    
    response = requests.post(url, json={
        "attribute": "stock",
        "delta": -quantity
    })
    
    return response.json()

# Usage: Sell 3 units
result = decrement_stock("products:item-0001", 3)
print(f"New stock: {result['attributes']['stock']}")
```

**Try It Yourself:**
1. Create a product with stock=100
2. Run decrement_stock 10 times in parallel
3. Verify final stock is exactly 90 (no race condition!)

---

### Pattern 3: Pagination

**Problem:** Query returns too many results, need to paginate.

**Solution:**
```python
def get_all_products_paginated(page_size=50):
    """Get all products with pagination"""
    base_url = "http://localhost:8080"
    all_products = []
    offset = 0
    
    while True:
        response = requests.post(
            f"{base_url}/query",
            json={
                "table": "products",
                "predicates": [],
                "limit": page_size,
                "offset": offset,
                "return": "entities"
            }
        )
        
        result = response.json()
        products = result['entities']
        
        if not products:
            break  # No more results
        
        all_products.extend(products)
        offset += page_size
        print(f"Fetched {len(products)} products (total: {len(all_products)})")
    
    return all_products

# Usage
all_products = get_all_products_paginated()
print(f"Total products: {len(all_products)}")
```

**Try It Yourself:**
1. Adjust page_size to 10 and observe more iterations
2. Add a filter (e.g., category="electronics")
3. Sort by price descending

---

### Pattern 4: Soft Delete

**Problem:** Need to "delete" entities but keep them recoverable.

**Solution:**
```python
def soft_delete(entity_id: str):
    """Mark entity as deleted without removing it"""
    url = f"http://localhost:8080/entities/{entity_id}"
    
    response = requests.patch(url, json={
        "attributes": {
            "deleted": True,
            "deleted_at": datetime.now().isoformat()
        }
    })
    
    return response.json()

def query_active_only(table: str):
    """Query only non-deleted entities"""
    response = requests.post(
        "http://localhost:8080/query",
        json={
            "table": table,
            "predicates": [
                {"column": "deleted", "operator": "!=", "value": True}
            ],
            "return": "entities"
        }
    )
    return response.json()['entities']

# Usage
soft_delete("users:john")  # Marks as deleted
active_users = query_active_only("users")  # Doesn't include John
```

**Try It Yourself:**
1. Soft delete a few entities
2. Create a "restore" function that sets deleted=False
3. Add a cleanup job that hard-deletes after 30 days

---

### Pattern 5: Transaction

**Problem:** Need multiple operations to succeed or fail together.

**Solution:**
```python
def transfer_inventory(from_product: str, to_product: str, quantity: int):
    """Transfer inventory between products atomically"""
    base_url = "http://localhost:8080"
    
    # Start transaction
    tx_response = requests.post(f"{base_url}/tx/begin")
    tx_id = tx_response.json()['tx_id']
    
    try:
        # Decrement source
        requests.post(
            f"{base_url}/tx/{tx_id}/entities/{from_product}/increment",
            json={"attribute": "stock", "delta": -quantity}
        )
        
        # Increment destination
        requests.post(
            f"{base_url}/tx/{tx_id}/entities/{to_product}/increment",
            json={"attribute": "stock", "delta": quantity}
        )
        
        # Commit transaction
        requests.post(f"{base_url}/tx/{tx_id}/commit")
        return True
        
    except Exception as e:
        # Rollback on error
        requests.post(f"{base_url}/tx/{tx_id}/rollback")
        raise

# Usage
transfer_inventory("products:item-0001", "products:item-0002", 10)
```

**Try It Yourself:**
1. Verify stock changes in both products
2. Simulate a failure (e.g., insufficient stock) and verify rollback
3. Run multiple transfers in parallel

---

## Real-World Scenarios

### Scenario 1: E-Commerce Checkout

**Requirements:**
- Create order
- Decrement product stock
- Update user's order history
- All or nothing (transaction)

**Solution:**
```python
import requests
import json
from datetime import datetime

def checkout(user_id: str, cart_items: list):
    """Complete checkout with transaction"""
    base_url = "http://localhost:8080"
    
    # Calculate total
    total = sum(item['price'] * item['quantity'] for item in cart_items)
    
    # Start transaction
    tx_response = requests.post(f"{base_url}/tx/begin")
    tx_id = tx_response.json()['tx_id']
    
    try:
        # Create order
        order_id = f"orders:order-{datetime.now().strftime('%Y%m%d%H%M%S')}"
        order_data = {
            "user_id": user_id,
            "items": cart_items,
            "total": total,
            "status": "pending",
            "created_at": datetime.now().isoformat()
        }
        
        requests.put(
            f"{base_url}/tx/{tx_id}/entities/{order_id}",
            json={"blob": json.dumps(order_data)}
        )
        
        # Decrement stock for each item
        for item in cart_items:
            requests.post(
                f"{base_url}/tx/{tx_id}/entities/{item['product_id']}/increment",
                json={"attribute": "stock", "delta": -item['quantity']}
            )
        
        # Update user's order list
        requests.patch(
            f"{base_url}/tx/{tx_id}/entities/{user_id}",
            json={"attributes": {"last_order": order_id}}
        )
        
        # Commit
        requests.post(f"{base_url}/tx/{tx_id}/commit")
        
        return {"status": "success", "order_id": order_id}
        
    except Exception as e:
        requests.post(f"{base_url}/tx/{tx_id}/rollback")
        return {"status": "error", "message": str(e)}

# Usage
cart = [
    {"product_id": "products:laptop-001", "quantity": 1, "price": 1299.99},
    {"product_id": "products:mouse-001", "quantity": 2, "price": 29.99}
]

result = checkout("users:alice", cart)
print(result)
```

**Try It Yourself:**
1. Add stock validation before checkout
2. Implement order cancellation (reverse the operations)
3. Add email notification (outside transaction)

---

### Scenario 2: Social Media Feed

**Requirements:**
- Get posts from followed users
- Sort by time
- Paginate results
- Include user details

**Solution:**
```python
def get_user_feed(user_id: str, page_size=20, offset=0):
    """Get personalized feed for user"""
    base_url = "http://localhost:8080"
    
    # 1. Get list of followed users
    follows_response = requests.post(
        f"{base_url}/query",
        json={
            "table": "follows",
            "predicates": [{"column": "from_user", "value": user_id}],
            "return": "entities"
        }
    )
    
    followed_user_ids = [
        json.loads(f['blob'])['to_user'] 
        for f in follows_response.json()['entities']
    ]
    
    # 2. Get posts from followed users
    posts_response = requests.post(
        f"{base_url}/query",
        json={
            "table": "posts",
            "predicates": [
                {"column": "author_id", "operator": "IN", "value": followed_user_ids}
            ],
            "order_by": "created_at",
            "order": "DESC",
            "limit": page_size,
            "offset": offset,
            "return": "entities"
        }
    )
    
    posts = [json.loads(p['blob']) for p in posts_response.json()['entities']]
    
    return {
        "posts": posts,
        "count": len(posts),
        "has_more": len(posts) == page_size
    }

# Usage
feed = get_user_feed("users:alice", page_size=10)
for post in feed['posts']:
    print(f"{post['author_name']}: {post['content']}")
```

**Try It Yourself:**
1. Add filtering (e.g., only posts with images)
2. Cache the followed users list
3. Implement "load more" functionality

---

### Scenario 3: Search with Filters

**Requirements:**
- Search products by multiple criteria
- Support price range
- Filter by category
- Sort by relevance

**Solution:**
```python
def search_products(
    query: str = "",
    category: str = None,
    min_price: float = None,
    max_price: float = None,
    sort_by: str = "relevance",
    limit: int = 50
):
    """Advanced product search"""
    base_url = "http://localhost:8080"
    
    predicates = []
    
    # Category filter
    if category:
        predicates.append({"column": "category", "value": category})
    
    # Price range
    if min_price is not None:
        predicates.append({"column": "price", "operator": ">=", "value": min_price})
    if max_price is not None:
        predicates.append({"column": "price", "operator": "<=", "value": max_price})
    
    # Text search (if supported)
    if query:
        predicates.append({"column": "name", "operator": "CONTAINS", "value": query})
    
    # Build query
    query_data = {
        "table": "products",
        "predicates": predicates,
        "limit": limit,
        "return": "entities"
    }
    
    # Add sorting
    if sort_by == "price_asc":
        query_data["order_by"] = "price"
        query_data["order"] = "ASC"
    elif sort_by == "price_desc":
        query_data["order_by"] = "price"
        query_data["order"] = "DESC"
    elif sort_by == "popularity":
        query_data["order_by"] = "sales_count"
        query_data["order"] = "DESC"
    
    response = requests.post(f"{base_url}/query", json=query_data)
    products = [json.loads(p['blob']) for p in response.json()['entities']]
    
    return products

# Usage
products = search_products(
    query="laptop",
    category="electronics",
    min_price=500,
    max_price=2000,
    sort_by="price_asc"
)

for product in products:
    print(f"{product['name']}: ${product['price']}")
```

**Try It Yourself:**
1. Add more filters (brand, rating, in_stock)
2. Implement faceted search (count per category)
3. Add vector search for semantic matching

---

## Interactive Exercises

### Exercise 1: Build a Todo App Backend

**Goal:** Implement CRUD operations for a todo application

**Requirements:**
- Create todo items
- Mark as complete
- Filter by status
- Sort by priority

**Starter Code:**
```python
class TodoAPI:
    def __init__(self, base_url):
        self.base_url = base_url
    
    def create_todo(self, title, description, priority="medium"):
        # TODO: Implement
        pass
    
    def get_todo(self, todo_id):
        # TODO: Implement
        pass
    
    def update_todo(self, todo_id, **kwargs):
        # TODO: Implement
        pass
    
    def complete_todo(self, todo_id):
        # TODO: Implement
        pass
    
    def list_todos(self, status=None, sort_by="created_at"):
        # TODO: Implement
        pass
    
    def delete_todo(self, todo_id):
        # TODO: Implement
        pass

# Test your implementation
api = TodoAPI("http://localhost:8080")
todo = api.create_todo("Learn ThemisDB", "Complete all tutorials", priority="high")
print(f"Created: {todo}")
```

**Solution:** See [examples/02_todo_app/](../../examples/02_todo_app/)

---

### Exercise 2: Implement Inventory Management

**Goal:** Build an inventory system with stock tracking

**Requirements:**
- Add products
- Track stock levels
- Atomic stock operations
- Low stock alerts

**Hints:**
- Use atomic increment/decrement for stock
- Index on stock level for low-stock queries
- Use transactions for multi-product operations

**Starter Code:**
```python
class InventoryManager:
    def add_product(self, product_id, name, initial_stock):
        # TODO: Implement
        pass
    
    def receive_stock(self, product_id, quantity):
        # TODO: Implement atomic increment
        pass
    
    def sell_product(self, product_id, quantity):
        # TODO: Implement atomic decrement with validation
        pass
    
    def check_low_stock(self, threshold=10):
        # TODO: Query products with stock < threshold
        pass
    
    def transfer_stock(self, from_product, to_product, quantity):
        # TODO: Use transaction
        pass
```

**Solution:** See [examples/04_inventory_system/](../../examples/04_inventory_system/)

---

### Exercise 3: Social Network Graph

**Goal:** Model relationships and traversals

**Requirements:**
- Follow/unfollow users
- Get followers and following
- Find mutual friends
- Suggest friends (friends of friends)

**Hints:**
- Use graph edges for relationships
- Create indexes on both from and to fields
- Use graph traversal API

**Starter Code:**
```python
class SocialGraph:
    def follow(self, user_id, target_user_id):
        # TODO: Create edge
        pass
    
    def unfollow(self, user_id, target_user_id):
        # TODO: Delete edge
        pass
    
    def get_followers(self, user_id):
        # TODO: Query incoming edges
        pass
    
    def get_following(self, user_id):
        # TODO: Query outgoing edges
        pass
    
    def find_mutual_friends(self, user1, user2):
        # TODO: Find intersection of following lists
        pass
    
    def suggest_friends(self, user_id, limit=10):
        # TODO: Friends of friends, excluding already following
        pass
```

**Solution:** See [examples/06_graph_social_network/](../../examples/06_graph_social_network/)

---

## Example Projects

ThemisDB includes 20+ complete example applications. Here's a curated selection:

### Beginner Examples

| Example | Description | Key Concepts |
|---------|-------------|--------------|
| [01_hello_world](../../examples/01_hello_world/) | Basic CRUD with GUI | Entity management, GUI |
| [02_todo_app](../../examples/02_todo_app/) | Todo list application | CRUD, filtering, sorting |
| [03_contact_manager](../../examples/03_contact_manager/) | Contact management | Search, categories |
| [11_blog_wiki](../../examples/11_blog_wiki/) | Simple blog/wiki | Documents, tags |

### Intermediate Examples

| Example | Description | Key Concepts |
|---------|-------------|--------------|
| [04_inventory_system](../../examples/04_inventory_system/) | Inventory tracking | Atomic ops, transactions |
| [05_time_series_monitor](../../examples/05_time_series_monitor/) | Time-series data | Batch insert, aggregation |
| [14_ecommerce_catalog](../../examples/14_ecommerce_catalog/) | E-commerce catalog | Search, filters, pagination |
| [17_crm](../../examples/17_crm/) | Customer relationship mgmt | Complex queries, reports |

### Advanced Examples

| Example | Description | Key Concepts |
|---------|-------------|--------------|
| [06_graph_social_network](../../examples/06_graph_social_network/) | Social network | Graph traversal, edges |
| [07_vector_search_documents](../../examples/07_vector_search_documents/) | Document similarity | Vector search, embeddings |
| [09_iot_sensor_network](../../examples/09_iot_sensor_network/) | IoT data processing | Time-series, batch ops |
| [19_recommendation_engine](../../examples/19_recommendation_engine/) | Product recommendations | Vector search, graph |

### Complete Applications

| Example | Description | Lines of Code |
|---------|-------------|---------------|
| [08_dms_erp_system](../../examples/08_dms_erp_system/) | DMS/ERP system | ~2000 LOC |
| [18_realtime_chat](../../examples/18_realtime_chat/) | Real-time chat | ~1500 LOC |
| [21_coding_platform](../../examples/21_coding_platform/) | Coding platform | ~2500 LOC |

**Run an Example:**
```bash
cd examples/02_todo_app
pip install -r requirements.txt
python main.py
```

---

## Community Examples

Share your examples! Submit a PR to add your projects:

**Template:**
```markdown
### Your Project Name
- **Author:** Your Name
- **Description:** Brief description
- **GitHub:** Link to repo
- **Concepts:** Key concepts demonstrated
```

---

## What's Next?

- **Build Something:** Pick an example and customize it
- **Contribute:** Share your examples with the community
- **Learn More:** [Best Practices Guide](BEST_PRACTICES.md)

---

**Questions?** Ask in [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
