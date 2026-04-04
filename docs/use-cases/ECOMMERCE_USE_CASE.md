# E-Commerce Platform with ThemisDB

## Overview

This guide demonstrates how to build a production-ready e-commerce platform using ThemisDB's multi-model capabilities. We'll leverage full-text search, semantic search, ACID transactions, graph queries, and real-time analytics to create a scalable online retail system.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Schema Design](#schema-design)
3. [Product Catalog with Search](#product-catalog-with-search)
4. [Inventory Management](#inventory-management)
5. [Order Processing](#order-processing)
6. [Recommendation System](#recommendation-system)
7. [Customer Analytics](#customer-analytics)
8. [Performance Optimization](#performance-optimization)
9. [Monitoring & Metrics](#monitoring--metrics)
10. [Best Practices](#best-practices)

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer                        │
├─────────────────────────────────────────────────────────────┤
│  Web Frontend  │  Mobile App  │  Admin Portal │  APIs       │
└────────┬────────┴──────┬──────┴───────┬───────┴──────┬──────┘
         │               │              │              │
         └───────────────┴──────────────┴──────────────┘
                         │
         ┌───────────────▼────────────────┐
         │      Load Balancer/CDN         │
         └───────────────┬────────────────┘
                         │
         ┌───────────────▼────────────────┐
         │       ThemisDB Cluster         │
         │  ┌──────────────────────────┐  │
         │  │  Product Catalog (Doc)   │  │
         │  │  - Full-text Search      │  │
         │  │  - Vector Embeddings     │  │
         │  ├──────────────────────────┤  │
         │  │  Inventory (KV + Time)   │  │
         │  │  - Multi-warehouse       │  │
         │  │  - Real-time Updates     │  │
         │  ├──────────────────────────┤  │
         │  │  Orders (Doc + Graph)    │  │
         │  │  - ACID Transactions     │  │
         │  │  - Order History         │  │
         │  ├──────────────────────────┤  │
         │  │  Customers (Graph)       │  │
         │  │  - Relationships         │  │
         │  │  - Social Graph          │  │
         │  ├──────────────────────────┤  │
         │  │  Analytics (Time-Series) │  │
         │  │  - Real-time Metrics     │  │
         │  │  - Historical Trends     │  │
         │  └──────────────────────────┘  │
         └────────────────────────────────┘
                         │
         ┌───────────────▼────────────────┐
         │   External Services            │
         │  - Payment Gateway             │
         │  - Shipping Providers          │
         │  - Email/SMS Notifications     │
         │  - LLM for Recommendations     │
         └────────────────────────────────┘
```

## Schema Design

### Product Collection

```aql
// Create product collection with full-text and vector search
CREATE COLLECTION products {
    type: "document",
    sharding: {
        strategy: "hash",
        key: "product_id",
        shards: 8
    },
    indexes: {
        fulltext: ["name", "description", "category"],
        vector: {
            field: "embedding",
            dimensions: 768,
            metric: "cosine",
            index_type: "hnsw",
            m: 16,
            ef_construction: 200
        },
        composite: [
            ["category", "price"],
            ["brand", "rating"]
        ]
    }
}

// Product document schema
{
    "product_id": "PROD-001",
    "sku": "LAPTOP-XPS-13",
    "name": "Dell XPS 13 Laptop",
    "description": "Premium ultrabook with 11th Gen Intel Core...",
    "category": "Electronics/Computers/Laptops",
    "subcategory": "Ultrabooks",
    "brand": "Dell",
    "price": 1299.99,
    "currency": "USD",
    "cost": 950.00,
    "images": [
        {"url": "https://cdn.example.com/prod001-1.jpg", "primary": true},
        {"url": "https://cdn.example.com/prod001-2.jpg", "primary": false}
    ],
    "specifications": {
        "processor": "Intel Core i7-1165G7",
        "ram": "16GB LPDDR4x",
        "storage": "512GB NVMe SSD",
        "display": "13.4\" FHD+ (1920x1200)",
        "weight": "2.64 lbs",
        "battery": "52Wh"
    },
    "attributes": {
        "color": "Platinum Silver",
        "warranty": "1 year",
        "condition": "new"
    },
    "embedding": [0.123, -0.456, 0.789, ...],  // 768-dimensional vector
    "tags": ["ultrabook", "business", "premium", "lightweight"],
    "rating": 4.5,
    "review_count": 142,
    "status": "active",
    "created_at": "2024-01-15T10:00:00Z",
    "updated_at": "2024-01-20T15:30:00Z",
    "metadata": {
        "seo_title": "Dell XPS 13 - Premium Ultrabook",
        "seo_description": "...",
        "slug": "dell-xps-13-laptop"
    }
}
```

### Inventory Collection

```aql
// Multi-warehouse inventory tracking
CREATE COLLECTION inventory {
    type: "document",
    sharding: {
        strategy: "composite",
        keys: ["product_id", "warehouse_id"],
        shards: 16
    },
    indexes: {
        composite: [
            ["product_id", "warehouse_id"],
            ["warehouse_id", "quantity"]
        ]
    }
}

// Inventory document
{
    "inventory_id": "INV-001",
    "product_id": "PROD-001",
    "sku": "LAPTOP-XPS-13",
    "warehouse_id": "WH-EAST-01",
    "warehouse_name": "East Coast Distribution Center",
    "location": {
        "zone": "A",
        "aisle": "12",
        "shelf": "3B"
    },
    "quantity": 45,
    "reserved": 8,      // Reserved for pending orders
    "available": 37,    // quantity - reserved
    "reorder_point": 10,
    "reorder_quantity": 50,
    "last_restock": "2024-01-18T08:00:00Z",
    "next_restock_expected": "2024-01-25T00:00:00Z",
    "supplier_id": "SUPP-123",
    "cost_per_unit": 950.00,
    "updated_at": "2024-01-20T14:22:33Z"
}
```

### Orders Collection

```aql
// Order management with transaction support
CREATE COLLECTION orders {
    type: "document",
    sharding: {
        strategy: "hash",
        key: "order_id",
        shards: 16
    },
    indexes: {
        composite: [
            ["customer_id", "created_at"],
            ["status", "created_at"],
            ["payment_status", "fulfillment_status"]
        ]
    }
}

// Order document
{
    "order_id": "ORD-2024-000123",
    "order_number": "123",
    "customer_id": "CUST-456",
    "customer": {
        "email": "customer@example.com",
        "first_name": "John",
        "last_name": "Doe"
    },
    "items": [
        {
            "product_id": "PROD-001",
            "sku": "LAPTOP-XPS-13",
            "name": "Dell XPS 13 Laptop",
            "quantity": 1,
            "unit_price": 1299.99,
            "discount": 0.00,
            "tax": 104.00,
            "subtotal": 1299.99,
            "total": 1403.99,
            "warehouse_id": "WH-EAST-01"
        }
    ],
    "subtotal": 1299.99,
    "tax": 104.00,
    "shipping": 0.00,
    "discount": 0.00,
    "total": 1403.99,
    "currency": "USD",
    "shipping_address": {
        "first_name": "John",
        "last_name": "Doe",
        "address1": "123 Main St",
        "address2": "Apt 4B",
        "city": "New York",
        "state": "NY",
        "zip": "10001",
        "country": "US",
        "phone": "+1-555-0123"
    },
    "billing_address": { /* same structure */ },
    "payment": {
        "method": "credit_card",
        "provider": "stripe",
        "transaction_id": "ch_3AbCdEfGhIjKl",
        "status": "completed",
        "paid_at": "2024-01-20T15:45:12Z"
    },
    "fulfillment": {
        "status": "processing",
        "warehouse_id": "WH-EAST-01",
        "tracking_number": null,
        "carrier": null,
        "shipped_at": null,
        "delivered_at": null
    },
    "status": "processing",
    "created_at": "2024-01-20T15:45:00Z",
    "updated_at": "2024-01-20T15:45:12Z",
    "notes": []
}
```

### Customer Graph

```aql
// Customer nodes and relationships
CREATE GRAPH customer_graph {
    vertices: ["customers", "products", "categories"],
    edges: ["purchases", "views", "favorites", "reviews", "friendships"]
}

// Customer vertex
{
    "_id": "customers/CUST-456",
    "customer_id": "CUST-456",
    "email": "customer@example.com",
    "first_name": "John",
    "last_name": "Doe",
    "joined_at": "2023-06-15T10:00:00Z",
    "loyalty_tier": "gold",
    "lifetime_value": 5420.50,
    "preferences": {
        "categories": ["Electronics", "Books"],
        "brands": ["Dell", "Apple", "Samsung"],
        "price_range": {"min": 50, "max": 2000}
    },
    "demographics": {
        "age_range": "30-40",
        "location": "New York, NY"
    }
}

// Purchase edge
{
    "_from": "customers/CUST-456",
    "_to": "products/PROD-001",
    "edge_type": "purchased",
    "order_id": "ORD-2024-000123",
    "quantity": 1,
    "price": 1299.99,
    "purchased_at": "2024-01-20T15:45:00Z"
}
```

## Product Catalog with Search

### Full-Text Search

```aql
// Basic full-text search
FOR product IN products
    SEARCH ANALYZER(
        product.name IN TOKENS("laptop ultrabook", "text_en") OR
        product.description IN TOKENS("laptop ultrabook", "text_en"),
        "text_en"
    )
    SORT BM25(product) DESC
    LIMIT 20
    RETURN {
        product_id: product.product_id,
        name: product.name,
        price: product.price,
        rating: product.rating,
        score: BM25(product)
    }

// Advanced search with filters
FOR product IN products
    SEARCH ANALYZER(
        product.name IN TOKENS(@searchTerm, "text_en") OR
        product.description IN TOKENS(@searchTerm, "text_en"),
        "text_en"
    )
    FILTER product.price >= @minPrice AND product.price <= @maxPrice
    FILTER product.category IN @categories
    FILTER product.rating >= @minRating
    FILTER product.status == "active"
    SORT BM25(product) DESC, product.rating DESC
    LIMIT @offset, @limit
    RETURN product

// Faceted search for filtering UI
LET results = (
    FOR product IN products
        SEARCH ANALYZER(product.name IN TOKENS(@searchTerm, "text_en"), "text_en")
        FILTER product.status == "active"
        RETURN product
)

RETURN {
    products: (FOR p IN results LIMIT @offset, @limit RETURN p),
    facets: {
        categories: (
            FOR p IN results
                COLLECT category = p.category WITH COUNT INTO count
                SORT count DESC
                RETURN {name: category, count: count}
        ),
        brands: (
            FOR p IN results
                COLLECT brand = p.brand WITH COUNT INTO count
                SORT count DESC
                LIMIT 20
                RETURN {name: brand, count: count}
        ),
        price_ranges: (
            FOR p IN results
                COLLECT range = FLOOR(p.price / 100) * 100 WITH COUNT INTO count
                SORT range
                RETURN {
                    min: range,
                    max: range + 100,
                    count: count
                }
        )
    },
    total: LENGTH(results)
}
```

### Semantic Search with Vectors

```aql
// Semantic product search using vector embeddings
// First, generate embedding for search query using LLM
LET query_embedding = LLM_EMBED(@searchQuery, {
    model: "all-MiniLM-L6-v2",
    pooling: "mean"
})

// Find similar products using HNSW index
FOR product IN products
    LET similarity = COSINE_SIMILARITY(product.embedding, query_embedding)
    FILTER similarity > 0.7
    SORT similarity DESC
    LIMIT 20
    RETURN {
        product_id: product.product_id,
        name: product.name,
        description: product.description,
        price: product.price,
        similarity: similarity
    }

// Hybrid search: combine full-text and semantic
LET fulltext_results = (
    FOR product IN products
        SEARCH ANALYZER(product.name IN TOKENS(@query, "text_en"), "text_en")
        LIMIT 50
        RETURN {
            product: product,
            score: BM25(product),
            type: "fulltext"
        }
)

LET query_embedding = LLM_EMBED(@query, {model: "all-MiniLM-L6-v2"})
LET semantic_results = (
    FOR product IN products
        LET similarity = COSINE_SIMILARITY(product.embedding, query_embedding)
        FILTER similarity > 0.6
        LIMIT 50
        RETURN {
            product: product,
            score: similarity,
            type: "semantic"
        }
)

// Merge and re-rank results
FOR result IN UNION(fulltext_results, semantic_results)
    COLLECT product_id = result.product.product_id
    AGGREGATE 
        product = FIRST(result.product),
        total_score = SUM(result.score),
        methods = UNIQUE(result.type)
    SORT total_score DESC
    LIMIT 20
    RETURN {
        product: product,
        relevance_score: total_score,
        matched_by: methods
    }
```

### Category Navigation

```aql
// Browse products by category hierarchy
FOR product IN products
    FILTER product.category LIKE CONCAT(@category, '%')
    FILTER product.status == "active"
    SORT product.rating DESC, product.review_count DESC
    LIMIT @offset, @limit
    RETURN product

// Get category tree with product counts
FOR product IN products
    FILTER product.status == "active"
    COLLECT category = product.category WITH COUNT INTO product_count
    LET parts = SPLIT(category, '/')
    RETURN {
        category: category,
        level: LENGTH(parts),
        parent: LENGTH(parts) > 1 ? JOIN(SLICE(parts, 0, -1), '/') : null,
        name: parts[-1],
        product_count: product_count
    }
```

## Inventory Management

### Real-Time Inventory Tracking

```aql
// Get current inventory for a product across all warehouses
FOR inv IN inventory
    FILTER inv.product_id == @productId
    RETURN {
        warehouse_id: inv.warehouse_id,
        warehouse_name: inv.warehouse_name,
        available: inv.available,
        reserved: inv.reserved,
        reorder_needed: inv.available < inv.reorder_point
    }

// Check product availability for order
LET product_checks = (
    FOR item IN @orderItems
        LET total_available = SUM(
            FOR inv IN inventory
                FILTER inv.product_id == item.product_id
                FILTER inv.warehouse_id IN @targetWarehouses
                RETURN inv.available
        )
        RETURN {
            product_id: item.product_id,
            requested: item.quantity,
            available: total_available,
            can_fulfill: total_available >= item.quantity
        }
)

RETURN {
    can_fulfill_order: ALL(product_checks[*].can_fulfill),
    items: product_checks
}
```

### Inventory Reservation (ACID Transaction)

```aql
// Reserve inventory when order is placed
BEGIN TRANSACTION

// 1. Check and reserve inventory
FOR item IN @orderItems
    // Find warehouse with sufficient stock
    LET warehouse_inv = FIRST(
        FOR inv IN inventory
            FILTER inv.product_id == item.product_id
            FILTER inv.warehouse_id IN @preferredWarehouses
            FILTER inv.available >= item.quantity
            SORT inv.available DESC
            LIMIT 1
            RETURN inv
    )
    
    // Reserve the quantity
    UPDATE warehouse_inv WITH {
        reserved: warehouse_inv.reserved + item.quantity,
        available: warehouse_inv.available - item.quantity,
        updated_at: DATE_ISO8601(DATE_NOW())
    } IN inventory
    
    // Track reservation
    INSERT {
        reservation_id: CONCAT("RES-", DATE_NOW()),
        product_id: item.product_id,
        warehouse_id: warehouse_inv.warehouse_id,
        order_id: @orderId,
        quantity: item.quantity,
        created_at: DATE_ISO8601(DATE_NOW()),
        expires_at: DATE_ISO8601(DATE_ADD(DATE_NOW(), 30, "minutes"))
    } INTO reservations

// 2. Create order
INSERT @orderData INTO orders

COMMIT TRANSACTION

// Release reservation on order cancellation
BEGIN TRANSACTION

FOR reservation IN reservations
    FILTER reservation.order_id == @orderId
    
    // Return inventory to available
    FOR inv IN inventory
        FILTER inv.product_id == reservation.product_id
        FILTER inv.warehouse_id == reservation.warehouse_id
        UPDATE inv WITH {
            reserved: inv.reserved - reservation.quantity,
            available: inv.available + reservation.quantity,
            updated_at: DATE_ISO8601(DATE_NOW())
        } IN inventory
    
    // Remove reservation
    REMOVE reservation IN reservations

COMMIT TRANSACTION
```

### Low Stock Alerts

```aql
// Find products needing reorder
FOR inv IN inventory
    FILTER inv.available < inv.reorder_point
    LET product = FIRST(
        FOR p IN products
            FILTER p.product_id == inv.product_id
            RETURN p
    )
    RETURN {
        product_id: inv.product_id,
        product_name: product.name,
        warehouse_id: inv.warehouse_id,
        current_stock: inv.available,
        reorder_point: inv.reorder_point,
        reorder_quantity: inv.reorder_quantity,
        estimated_stockout: DATE_ADD(
            DATE_NOW(),
            FLOOR(inv.available / product.daily_sales_avg),
            "days"
        )
    }

// Inventory turnover analysis
FOR inv IN inventory
    LET product = FIRST(FOR p IN products FILTER p.product_id == inv.product_id RETURN p)
    LET sales = (
        FOR order IN orders
            FILTER order.created_at > DATE_SUBTRACT(DATE_NOW(), 30, "days")
            FOR item IN order.items
                FILTER item.product_id == inv.product_id
                FILTER item.warehouse_id == inv.warehouse_id
                RETURN item.quantity
    )
    LET total_sold = SUM(sales)
    RETURN {
        product_id: inv.product_id,
        warehouse_id: inv.warehouse_id,
        current_stock: inv.available + inv.reserved,
        sold_30days: total_sold,
        turnover_rate: total_sold / (inv.available + inv.reserved),
        days_of_stock: (inv.available + inv.reserved) / (total_sold / 30)
    }
```

## Order Processing

### Create Order with Transaction

```aql
BEGIN TRANSACTION

// 1. Validate customer
LET customer = FIRST(FOR c IN customers FILTER c.customer_id == @customerId RETURN c)
ASSERT customer != null, "Customer not found"

// 2. Validate and reserve inventory
LET order_items = (
    FOR item IN @cartItems
        LET product = FIRST(FOR p IN products FILTER p.product_id == item.product_id RETURN p)
        ASSERT product != null, CONCAT("Product not found: ", item.product_id)
        ASSERT product.status == "active", "Product not available"
        
        // Find warehouse with stock
        LET warehouse_inv = FIRST(
            FOR inv IN inventory
                FILTER inv.product_id == item.product_id
                FILTER inv.available >= item.quantity
                SORT inv.available DESC
                LIMIT 1
                RETURN inv
        )
        ASSERT warehouse_inv != null, CONCAT("Insufficient stock for: ", product.name)
        
        // Reserve inventory
        UPDATE warehouse_inv WITH {
            reserved: warehouse_inv.reserved + item.quantity,
            available: warehouse_inv.available - item.quantity
        } IN inventory
        
        RETURN {
            product_id: product.product_id,
            sku: product.sku,
            name: product.name,
            quantity: item.quantity,
            unit_price: product.price,
            discount: 0,
            tax: product.price * item.quantity * 0.08,
            subtotal: product.price * item.quantity,
            total: product.price * item.quantity * 1.08,
            warehouse_id: warehouse_inv.warehouse_id
        }
)

// 3. Calculate order totals
LET subtotal = SUM(order_items[*].subtotal)
LET tax = SUM(order_items[*].tax)
LET shipping = @shippingCost
LET discount = @discountAmount
LET total = subtotal + tax + shipping - discount

// 4. Create order
LET order = INSERT {
    order_id: CONCAT("ORD-", DATE_FORMAT(DATE_NOW(), "%Y"), "-", LPAD(SEQUENCE("orders"), 6, "0")),
    customer_id: @customerId,
    customer: {
        email: customer.email,
        first_name: customer.first_name,
        last_name: customer.last_name
    },
    items: order_items,
    subtotal: subtotal,
    tax: tax,
    shipping: shipping,
    discount: discount,
    total: total,
    currency: "USD",
    shipping_address: @shippingAddress,
    billing_address: @billingAddress,
    payment: {
        method: @paymentMethod,
        provider: @paymentProvider,
        status: "pending"
    },
    fulfillment: {
        status: "pending"
    },
    status: "pending_payment",
    created_at: DATE_ISO8601(DATE_NOW()),
    updated_at: DATE_ISO8601(DATE_NOW())
} INTO orders RETURN NEW

// 5. Create customer->product edges for recommendation engine
FOR item IN order_items
    INSERT {
        _from: CONCAT("customers/", @customerId),
        _to: CONCAT("products/", item.product_id),
        edge_type: "purchased",
        order_id: order.order_id,
        quantity: item.quantity,
        price: item.unit_price,
        purchased_at: order.created_at
    } INTO purchases

COMMIT TRANSACTION

RETURN order
```

### Order Fulfillment Workflow

```aql
// Update order status - mark as paid
UPDATE @orderId WITH {
    payment: MERGE(@currentPayment, {
        status: "completed",
        transaction_id: @transactionId,
        paid_at: DATE_ISO8601(DATE_NOW())
    }),
    status: "processing",
    updated_at: DATE_ISO8601(DATE_NOW())
} IN orders

// Generate picking list for warehouse
FOR order IN orders
    FILTER order.order_id == @orderId
    FOR item IN order.items
        FILTER item.warehouse_id == @warehouseId
        LET inv = FIRST(
            FOR i IN inventory
                FILTER i.product_id == item.product_id
                FILTER i.warehouse_id == item.warehouse_id
                RETURN i
        )
        RETURN {
            order_id: order.order_id,
            product_id: item.product_id,
            sku: item.sku,
            name: item.name,
            quantity: item.quantity,
            location: inv.location
        }

// Mark as shipped
BEGIN TRANSACTION

UPDATE @orderId WITH {
    fulfillment: MERGE(@currentFulfillment, {
        status: "shipped",
        tracking_number: @trackingNumber,
        carrier: @carrier,
        shipped_at: DATE_ISO8601(DATE_NOW())
    }),
    status: "shipped",
    updated_at: DATE_ISO8601(DATE_NOW())
} IN orders

// Update inventory - move from reserved to sold
FOR item IN @orderItems
    FOR inv IN inventory
        FILTER inv.product_id == item.product_id
        FILTER inv.warehouse_id == item.warehouse_id
        UPDATE inv WITH {
            reserved: inv.reserved - item.quantity,
            updated_at: DATE_ISO8601(DATE_NOW())
        } IN inventory

COMMIT TRANSACTION
```

## Recommendation System

### Collaborative Filtering

```aql
// Find similar customers based on purchase history
FOR customer IN customers
    FILTER customer.customer_id == @customerId
    
    // Get products this customer purchased
    LET my_products = (
        FOR v, e, p IN 1..1 OUTBOUND customer purchases
            RETURN v.product_id
    )
    
    // Find customers who bought similar products
    LET similar_customers = (
        FOR product_id IN my_products
            FOR v, e, p IN 1..1 INBOUND CONCAT("products/", product_id) purchases
                FILTER v._id != customer._id
                COLLECT customer_id = v._id WITH COUNT INTO overlap
                SORT overlap DESC
                LIMIT 20
                RETURN {customer_id: customer_id, overlap: overlap}
    )
    
    // Get products purchased by similar customers
    FOR similar IN similar_customers
        FOR v, e, p IN 1..1 OUTBOUND similar.customer_id purchases
            FILTER v.product_id NOT IN my_products
            COLLECT product_id = v.product_id 
            AGGREGATE 
                product = FIRST(v),
                score = SUM(similar.overlap),
                purchased_by_count = COUNT()
            SORT score DESC
            LIMIT 10
            RETURN {
                product_id: product.product_id,
                name: product.name,
                price: product.price,
                recommendation_score: score,
                purchased_by: purchased_by_count
            }

// Products frequently bought together
FOR order IN orders
    FILTER order.created_at > DATE_SUBTRACT(DATE_NOW(), 90, "days")
    FILTER LENGTH(order.items) >= 2
    FOR item1 IN order.items
        FILTER item1.product_id == @productId
        FOR item2 IN order.items
            FILTER item2.product_id != @productId
            COLLECT product_id = item2.product_id WITH COUNT INTO frequency
            LET product = FIRST(FOR p IN products FILTER p.product_id == product_id RETURN p)
            SORT frequency DESC
            LIMIT 5
            RETURN {
                product: product,
                bought_together_count: frequency
            }
```

### Content-Based Recommendations

```aql
// Recommend similar products using vector embeddings
LET current_product = FIRST(
    FOR p IN products
        FILTER p.product_id == @productId
        RETURN p
)

FOR product IN products
    FILTER product.product_id != @productId
    FILTER product.status == "active"
    LET similarity = COSINE_SIMILARITY(product.embedding, current_product.embedding)
    FILTER similarity > 0.8
    SORT similarity DESC
    LIMIT 10
    RETURN {
        product_id: product.product_id,
        name: product.name,
        price: product.price,
        similarity_score: similarity
    }

// Personalized recommendations using customer preferences
LET customer = FIRST(FOR c IN customers FILTER c.customer_id == @customerId RETURN c)

LET preferred_categories = customer.preferences.categories
LET preferred_brands = customer.preferences.brands
LET price_range = customer.preferences.price_range

FOR product IN products
    FILTER product.status == "active"
    FILTER product.category IN preferred_categories OR
           product.brand IN preferred_brands
    FILTER product.price >= price_range.min AND product.price <= price_range.max
    
    // Check if not already purchased
    LET already_purchased = LENGTH(
        FOR v, e IN 1..1 OUTBOUND CONCAT("customers/", @customerId) purchases
            FILTER v.product_id == product.product_id
            RETURN 1
    ) > 0
    FILTER !already_purchased
    
    LET score = (
        (product.category IN preferred_categories ? 2 : 0) +
        (product.brand IN preferred_brands ? 2 : 0) +
        (product.rating / 5) +
        (product.review_count / 100)
    )
    
    SORT score DESC, product.rating DESC
    LIMIT 20
    RETURN {
        product: product,
        recommendation_score: score
    }
```

## Customer Analytics

### Customer Lifetime Value

```aql
// Calculate customer lifetime value and segmentation
FOR customer IN customers
    LET orders = (
        FOR order IN orders
            FILTER order.customer_id == customer.customer_id
            FILTER order.status != "cancelled"
            RETURN order
    )
    
    LET total_spent = SUM(orders[*].total)
    LET order_count = LENGTH(orders)
    LET first_order = MIN(orders[*].created_at)
    LET last_order = MAX(orders[*].created_at)
    LET days_active = DATE_DIFF(first_order, last_order, "days")
    LET avg_order_value = total_spent / order_count
    
    LET segment = (
        total_spent > 5000 ? "VIP" :
        total_spent > 2000 ? "High Value" :
        total_spent > 500 ? "Regular" :
        "New"
    )
    
    RETURN {
        customer_id: customer.customer_id,
        email: customer.email,
        total_spent: total_spent,
        order_count: order_count,
        avg_order_value: avg_order_value,
        days_active: days_active,
        lifetime_value: total_spent,
        segment: segment,
        last_purchase_days_ago: DATE_DIFF(last_order, DATE_NOW(), "days")
    }

// Customer cohort analysis
FOR customer IN customers
    LET cohort_month = DATE_FORMAT(customer.joined_at, "%Y-%m")
    LET orders = (
        FOR order IN orders
            FILTER order.customer_id == customer.customer_id
            FILTER order.status != "cancelled"
            RETURN {
                month: DATE_FORMAT(order.created_at, "%Y-%m"),
                total: order.total
            }
    )
    
    COLLECT cohort = cohort_month INTO group
    RETURN {
        cohort: cohort,
        customers: LENGTH(group),
        retention_by_month: (
            FOR month IN UNIQUE(group[*].orders[*].month)
                FILTER month != null
                LET active_customers = LENGTH(
                    FOR g IN group
                        FILTER month IN g.orders[*].month
                        RETURN 1
                )
                RETURN {
                    month: month,
                    active_customers: active_customers,
                    retention_rate: active_customers / LENGTH(group)
                }
        ),
        total_revenue: SUM(group[*].orders[*][*].total)
    }
```

### Sales Analytics

```aql
// Daily sales dashboard
FOR order IN orders
    FILTER order.created_at >= DATE_SUBTRACT(DATE_NOW(), 30, "days")
    FILTER order.status != "cancelled"
    COLLECT date = DATE_FORMAT(order.created_at, "%Y-%m-%d")
    AGGREGATE
        order_count = COUNT(),
        revenue = SUM(order.total),
        avg_order_value = AVG(order.total)
    SORT date DESC
    RETURN {
        date: date,
        orders: order_count,
        revenue: revenue,
        avg_order_value: avg_order_value
    }

// Top selling products
FOR order IN orders
    FILTER order.created_at >= DATE_SUBTRACT(DATE_NOW(), 30, "days")
    FOR item IN order.items
        COLLECT product_id = item.product_id
        AGGREGATE
            name = FIRST(item.name),
            quantity_sold = SUM(item.quantity),
            revenue = SUM(item.total)
        SORT quantity_sold DESC
        LIMIT 20
        RETURN {
            product_id: product_id,
            name: name,
            quantity_sold: quantity_sold,
            revenue: revenue
        }

// Conversion funnel analysis
LET product_views = (
    FOR v, e IN 1..1 OUTBOUND "customers/*" views
        FILTER e.viewed_at >= DATE_SUBTRACT(DATE_NOW(), 7, "days")
        RETURN DISTINCT v.product_id
)

LET added_to_cart = (
    FOR cart IN shopping_carts
        FILTER cart.updated_at >= DATE_SUBTRACT(DATE_NOW(), 7, "days")
        FOR item IN cart.items
            RETURN DISTINCT item.product_id
)

LET purchased = (
    FOR order IN orders
        FILTER order.created_at >= DATE_SUBTRACT(DATE_NOW(), 7, "days")
        FOR item IN order.items
            RETURN DISTINCT item.product_id
)

RETURN {
    views: LENGTH(product_views),
    added_to_cart: LENGTH(added_to_cart),
    purchased: LENGTH(purchased),
    view_to_cart_rate: LENGTH(added_to_cart) / LENGTH(product_views),
    cart_to_purchase_rate: LENGTH(purchased) / LENGTH(added_to_cart),
    overall_conversion: LENGTH(purchased) / LENGTH(product_views)
}
```

## Performance Optimization

### Indexing Strategy

```aql
// Create composite indexes for common query patterns
CREATE INDEX idx_products_category_price ON products (category, price)
CREATE INDEX idx_products_brand_rating ON products (brand, rating)
CREATE INDEX idx_orders_customer_date ON orders (customer_id, created_at)
CREATE INDEX idx_orders_status ON orders (status, created_at)
CREATE INDEX idx_inventory_product_warehouse ON inventory (product_id, warehouse_id)

// Create vector index for semantic search
CREATE INDEX idx_products_embedding ON products (embedding) 
    TYPE vector 
    OPTIONS {
        dimensions: 768,
        metric: "cosine",
        index_type: "hnsw",
        m: 16,
        ef_construction: 200
    }

// Create full-text index
CREATE FULLTEXT INDEX idx_products_search ON products (name, description, tags)
    OPTIONS {
        analyzer: "text_en",
        features: ["frequency", "position", "offset"]
    }
```

### Query Optimization

```aql
// Use query hints for better performance
FOR product IN products
    OPTIONS {useIndex: "idx_products_category_price"}
    FILTER product.category == @category
    FILTER product.price >= @minPrice AND product.price <= @maxPrice
    SORT product.price ASC
    LIMIT @offset, @limit
    RETURN product

// Materialize frequently accessed aggregations
INSERT {
    date: DATE_FORMAT(DATE_NOW(), "%Y-%m-%d"),
    metrics: {
        total_revenue: @dailyRevenue,
        order_count: @orderCount,
        new_customers: @newCustomers,
        avg_order_value: @avgOrderValue
    },
    top_products: @topProducts,
    calculated_at: DATE_ISO8601(DATE_NOW())
} INTO daily_analytics

// Use projection to reduce data transfer
FOR product IN products
    FILTER product.category == @category
    LIMIT 100
    RETURN {
        product_id: product.product_id,
        name: product.name,
        price: product.price,
        image: product.images[0].url
    }
```

### Caching Strategy

```aql
// Cache popular products
FOR product IN products
    FILTER product.rating >= 4.5
    FILTER product.review_count >= 100
    SORT product.review_count DESC
    LIMIT 50
    LET cached = INSERT {
        cache_key: CONCAT("popular_product:", product.product_id),
        data: product,
        cached_at: DATE_ISO8601(DATE_NOW()),
        ttl: 3600  // 1 hour
    } INTO cache
    RETURN product

// Cache category navigation
FOR category IN DISTINCT(
    FOR p IN products
        FILTER p.status == "active"
        RETURN p.category
)
    LET product_count = LENGTH(
        FOR p IN products
            FILTER p.category == category
            FILTER p.status == "active"
            RETURN 1
    )
    INSERT {
        cache_key: CONCAT("category:", category),
        data: {
            name: category,
            product_count: product_count
        },
        cached_at: DATE_ISO8601(DATE_NOW()),
        ttl: 7200  // 2 hours
    } INTO cache
```

### Sharding Configuration

```yaml
# ThemisDB sharding configuration for e-commerce
sharding:
  products:
    strategy: hash
    shard_key: product_id
    shard_count: 8
    replication_factor: 3
    
  inventory:
    strategy: composite
    shard_keys: [product_id, warehouse_id]
    shard_count: 16
    replication_factor: 3
    
  orders:
    strategy: hash
    shard_key: order_id
    shard_count: 16
    replication_factor: 3
    # Archive old orders to cold storage
    archival:
      enabled: true
      after_days: 365
      archive_to: "s3://orders-archive/"
      
  customers:
    strategy: hash
    shard_key: customer_id
    shard_count: 8
    replication_factor: 3
```

## Monitoring & Metrics

### Key Performance Indicators

```aql
// Real-time dashboard metrics
RETURN {
    current_time: DATE_ISO8601(DATE_NOW()),
    orders: {
        pending: LENGTH(FOR o IN orders FILTER o.status == "pending_payment" RETURN 1),
        processing: LENGTH(FOR o IN orders FILTER o.status == "processing" RETURN 1),
        shipped_today: LENGTH(
            FOR o IN orders 
                FILTER o.fulfillment.shipped_at >= DATE_FORMAT(DATE_NOW(), "%Y-%m-%d")
                RETURN 1
        )
    },
    inventory: {
        low_stock_items: LENGTH(
            FOR inv IN inventory
                FILTER inv.available < inv.reorder_point
                RETURN 1
        ),
        out_of_stock: LENGTH(
            FOR inv IN inventory
                FILTER inv.available == 0
                RETURN 1
        )
    },
    revenue: {
        today: SUM(
            FOR o IN orders
                FILTER o.created_at >= DATE_FORMAT(DATE_NOW(), "%Y-%m-%d")
                FILTER o.status != "cancelled"
                RETURN o.total
        ),
        this_month: SUM(
            FOR o IN orders
                FILTER o.created_at >= DATE_FORMAT(DATE_NOW(), "%Y-%m-01")
                FILTER o.status != "cancelled"
                RETURN o.total
        )
    }
}
```

### Prometheus Metrics

```cpp
// C++ metrics collection example
#include <themis/metrics.hpp>

// Register metrics
auto order_counter = prometheus::BuildCounter()
    .Name("ecommerce_orders_total")
    .Help("Total number of orders")
    .Register(*registry);

auto order_value = prometheus::BuildHistogram()
    .Name("ecommerce_order_value")
    .Help("Order value distribution")
    .Register(*registry);

auto inventory_gauge = prometheus::BuildGauge()
    .Name("ecommerce_inventory_level")
    .Help("Current inventory levels")
    .Register(*registry);

// Track order creation
order_counter.Add({"status", "created"}).Increment();
order_value.Add({}).Observe(order.total);

// Update inventory metrics
for (const auto& item : order.items) {
    inventory_gauge.Add({"product_id", item.product_id}).Decrement(item.quantity);
}
```

## Best Practices

### 1. **Transaction Boundaries**
   - Keep transactions short and focused
   - Reserve inventory first, then process payment
   - Use optimistic locking for inventory updates
   - Implement compensation logic for failed transactions

### 2. **Search Performance**
   - Use full-text search for keyword matching
   - Use vector search for semantic similarity
   - Implement hybrid search for best results
   - Cache popular searches and results
   - Use query result pagination

### 3. **Inventory Management**
   - Implement reservation system for cart items
   - Use time-based reservation expiry
   - Track inventory movements in audit log
   - Set up automated reorder triggers
   - Monitor inventory turnover rates

### 4. **Data Modeling**
   - Denormalize frequently accessed data
   - Use composite indexes for multi-field queries
   - Partition large collections by date or category
   - Archive old orders to cold storage
   - Use graph edges for recommendations

### 5. **Security**
   - Encrypt sensitive customer data
   - Use row-level security for multi-tenant
   - Implement rate limiting on APIs
   - Audit all financial transactions
   - Use prepared statements to prevent injection

### 6. **Scalability**
   - Shard by customer ID or product ID
   - Use read replicas for analytics
   - Implement caching layer (Redis)
   - Use CDN for product images
   - Batch process non-critical updates

## Related Documentation

- [AQL Reference](../aql/README.md)
- [Sharding Guide](../architecture/sharding.md)
- [Vector Search](../features/vector-search.md)
- [Transaction Management](../features/transactions.md)
- [Performance Tuning](../performance/optimization.md)

## Example Projects

- [E-Commerce Catalog Example](../../examples/14_ecommerce_catalog/)
- [Recommendation Engine](../../examples/19_recommendation_engine/)
- [Inventory System](../../examples/04_inventory_system/)

## Conclusion

This e-commerce implementation demonstrates ThemisDB's ability to handle complex multi-model workloads with ACID transactions, real-time search, and graph-based recommendations. The architecture scales horizontally through sharding while maintaining strong consistency guarantees for critical operations like order processing and inventory management.

For production deployments, consider:
- Implementing circuit breakers for external services
- Setting up automated backup and disaster recovery
- Monitoring query performance and optimizing slow queries
- Implementing A/B testing for recommendations
- Using feature flags for gradual rollouts
