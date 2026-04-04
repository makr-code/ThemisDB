# SaaS Multi-Tenancy with ThemisDB

## Overview

This guide demonstrates building a production-ready SaaS application with ThemisDB's multi-tenant capabilities. We'll cover data isolation strategies, tenant-aware queries, resource management, billing, security, and compliance requirements for enterprise SaaS platforms.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Schema Design](#schema-design)
3. [Data Isolation Strategies](#data-isolation-strategies)
4. [Tenant-Aware Queries](#tenant-aware-queries)
5. [Resource Quotas & Limits](#resource-quotas--limits)
6. [Billing & Usage Tracking](#billing--usage-tracking)
7. [Tenant Lifecycle](#tenant-lifecycle)
8. [Security & Compliance](#security--compliance)
9. [Performance at Scale](#performance-at-scale)
10. [Monitoring & Operations](#monitoring--operations)

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                   SaaS Application Layer                    │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │   Tenant A   │  │   Tenant B   │  │   Tenant C   │     │
│  │   Subdomain  │  │   Subdomain  │  │   Subdomain  │     │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘     │
│         │                 │                 │              │
│         └─────────────────┴─────────────────┘              │
│                         │                                   │
└─────────────────────────┼───────────────────────────────────┘
                          │
         ┌────────────────▼────────────────┐
         │   Multi-Tenant API Gateway      │
         │  - Tenant Resolution            │
         │  - Authentication               │
         │  - Rate Limiting                │
         │  - Request Routing              │
         └────────────────┬────────────────┘
                          │
         ┌────────────────▼────────────────┐
         │    Tenant Context Middleware    │
         │  - Inject tenant_id             │
         │  - Validate permissions         │
         │  - Enforce quotas               │
         └────────────────┬────────────────┘
                          │
         ┌────────────────▼────────────────┐
         │       ThemisDB Cluster          │
         │                                  │
         │  Isolation Strategy:             │
         │  ┌────────────────────────────┐ │
         │  │ Option 1: Schema-Based     │ │
         │  │ - Separate collections     │ │
         │  │ - Tenant-prefixed names    │ │
         │  │ - Best for few tenants     │ │
         │  ├────────────────────────────┤ │
         │  │ Option 2: Row-Based (RLS)  │ │
         │  │ - Shared collections       │ │
         │  │ - tenant_id field          │ │
         │  │ - Row-level security       │ │
         │  │ - Best for many tenants    │ │
         │  ├────────────────────────────┤ │
         │  │ Option 3: Database-Based   │ │
         │  │ - Separate databases       │ │
         │  │ - Complete isolation       │ │
         │  │ - Best for enterprise      │ │
         │  └────────────────────────────┘ │
         │                                  │
         │  ┌──────────────────────────┐   │
         │  │ Resource Management      │   │
         │  │ - Quota enforcement      │   │
         │  │ - Usage tracking         │   │
         │  │ - Rate limiting          │   │
         │  └──────────────────────────┘   │
         │                                  │
         │  ┌──────────────────────────┐   │
         │  │ Audit & Compliance       │   │
         │  │ - Access logs            │   │
         │  │ - Data lineage           │   │
         │  │ - GDPR compliance        │   │
         │  └──────────────────────────┘   │
         └──────────────────────────────────┘
                          │
         ┌────────────────▼────────────────┐
         │     Supporting Services         │
         │  - Billing Engine               │
         │  - Analytics Platform           │
         │  - Notification Service         │
         │  - Backup & Archive             │
         └─────────────────────────────────┘
```

## Schema Design

### Tenant Registry

```aql
// Central tenant management
CREATE COLLECTION tenants {
    type: "document",
    sharding: {
        strategy: "hash",
        key: "tenant_id",
        shards: 8
    },
    indexes: {
        unique: ["tenant_id", "subdomain"],
        composite: [
            ["status", "tier"],
            ["created_at", "status"]
        ]
    }
}

// Tenant document schema
{
    "tenant_id": "tenant_a1b2c3d4",
    "name": "Acme Corporation",
    "subdomain": "acme",
    "custom_domain": "app.acme.com",
    "tier": "enterprise",  // free, starter, professional, enterprise
    "status": "active",    // trial, active, suspended, cancelled
    "contact": {
        "company_name": "Acme Corporation",
        "email": "admin@acme.com",
        "phone": "+1-555-0123",
        "billing_email": "billing@acme.com"
    },
    "address": {
        "street": "123 Main St",
        "city": "San Francisco",
        "state": "CA",
        "postal_code": "94105",
        "country": "US"
    },
    "subscription": {
        "plan_id": "enterprise-annual",
        "started_at": "2024-01-01T00:00:00Z",
        "renews_at": "2025-01-01T00:00:00Z",
        "trial_ends_at": null,
        "billing_cycle": "annual",  // monthly, annual
        "mrr": 999.00,  // Monthly recurring revenue
        "currency": "USD"
    },
    "quotas": {
        "users": {
            "limit": 100,
            "used": 45
        },
        "storage_gb": {
            "limit": 500,
            "used": 234.5
        },
        "api_calls_per_month": {
            "limit": 1000000,
            "used": 456789
        },
        "custom_integrations": {
            "limit": 10,
            "used": 5
        }
    },
    "features": {
        "sso": true,
        "advanced_analytics": true,
        "custom_branding": true,
        "api_access": true,
        "priority_support": true,
        "data_export": true,
        "audit_logs": true,
        "custom_roles": true
    },
    "settings": {
        "timezone": "America/Los_Angeles",
        "locale": "en-US",
        "date_format": "MM/DD/YYYY",
        "data_retention_days": 2555,
        "session_timeout_minutes": 480
    },
    "compliance": {
        "gdpr_applicable": true,
        "data_residency": "us-west-2",
        "encryption_at_rest": true,
        "audit_logging": true,
        "data_processing_agreement_signed": true,
        "dpa_signed_at": "2024-01-01T10:00:00Z"
    },
    "metadata": {
        "created_at": "2024-01-01T00:00:00Z",
        "updated_at": "2024-01-20T15:30:00Z",
        "created_by": "system",
        "onboarding_completed": true,
        "onboarding_completed_at": "2024-01-01T10:30:00Z",
        "referral_source": "direct",
        "sales_rep": "john.doe@company.com"
    },
    "security": {
        "password_policy": {
            "min_length": 12,
            "require_uppercase": true,
            "require_lowercase": true,
            "require_numbers": true,
            "require_special": true,
            "max_age_days": 90
        },
        "mfa_required": true,
        "ip_whitelist": ["203.0.113.0/24"],
        "allowed_oauth_providers": ["google", "microsoft"]
    }
}
```

### Row-Level Security (RLS) Schema

```aql
// Shared collection with tenant isolation
CREATE COLLECTION projects {
    type: "document",
    sharding: {
        strategy: "hash",
        key: "tenant_id",  // Shard by tenant for locality
        shards: 16
    },
    indexes: {
        composite: [
            ["tenant_id", "created_at"],  // Critical for RLS queries
            ["tenant_id", "status"],
            ["tenant_id", "owner_id"]
        ]
    },
    // Row-level security policy
    rls_policies: [
        {
            name: "tenant_isolation",
            using: "doc.tenant_id = @current_tenant_id",
            with_check: "doc.tenant_id = @current_tenant_id"
        }
    ]
}

// Document with tenant_id
{
    "project_id": "proj_1234",
    "tenant_id": "tenant_a1b2c3d4",  // REQUIRED for RLS
    "name": "Q1 Marketing Campaign",
    "description": "Launch new product line",
    "status": "active",
    "owner_id": "user_5678",
    "team_members": ["user_5678", "user_9012"],
    "created_at": "2024-01-15T10:00:00Z",
    "updated_at": "2024-01-20T15:30:00Z",
    "metadata": {
        "budget": 50000,
        "deadline": "2024-03-31",
        "priority": "high"
    }
}

// Users collection with tenant context
CREATE COLLECTION users {
    type: "document",
    sharding: {
        strategy: "hash",
        key: "tenant_id",
        shards: 16
    },
    indexes: {
        unique: [["tenant_id", "email"]],  // Email unique per tenant
        composite: [
            ["tenant_id", "status"],
            ["tenant_id", "role"]
        ]
    }
}

{
    "user_id": "user_5678",
    "tenant_id": "tenant_a1b2c3d4",
    "email": "john@acme.com",
    "first_name": "John",
    "last_name": "Doe",
    "role": "admin",  // admin, user, viewer
    "status": "active",  // active, invited, suspended
    "permissions": [
        "projects:read",
        "projects:write",
        "users:read",
        "settings:write"
    ],
    "last_login": "2024-01-20T15:00:00Z",
    "created_at": "2024-01-01T10:00:00Z"
}
```

### Usage Tracking Schema

```aql
// Track resource usage for billing
CREATE COLLECTION usage_events {
    type: "timeseries",
    timeseries: {
        time_field: "timestamp",
        meta_field: "metadata",
        granularity: "seconds"
    },
    sharding: {
        strategy: "time_range",
        time_field: "timestamp",
        chunk_interval: "1 day",
        shards: 32
    },
    retention: {
        policy: "time_based",
        retain_days: 395  // Keep for billing history
    },
    indexes: {
        composite: [
            ["metadata.tenant_id", "timestamp"],
            ["metadata.event_type", "timestamp"]
        ]
    }
}

// Usage event document
{
    "timestamp": "2024-01-20T15:45:23Z",
    "metadata": {
        "tenant_id": "tenant_a1b2c3d4",
        "user_id": "user_5678",
        "event_type": "api_call",  // api_call, storage, compute, etc.
        "resource": "projects",
        "operation": "create"
    },
    "metrics": {
        "quantity": 1,
        "duration_ms": 125,
        "data_size_bytes": 2048,
        "compute_units": 0.1
    },
    "billable": true,
    "cost": 0.001  // USD
}
```

## Data Isolation Strategies

### Strategy 1: Row-Level Security (Recommended for Most SaaS)

```aql
// Enable RLS for tenant isolation
// All queries automatically filtered by tenant_id

// Middleware sets current tenant context
SET @current_tenant_id = @tenantId

// Query projects - automatically filtered
FOR project IN projects
    // No need to filter by tenant_id - RLS does it automatically
    FILTER project.status == "active"
    SORT project.created_at DESC
    RETURN project

// Insert - tenant_id automatically validated
INSERT {
    project_id: @projectId,
    tenant_id: @current_tenant_id,  // Must match session context
    name: @projectName,
    status: "active"
} INTO projects

// Update - can only update own tenant's data
UPDATE @projectId WITH {
    status: "completed"
} IN projects
// Automatically fails if project.tenant_id != @current_tenant_id
```

### Strategy 2: Schema-Based Isolation (For Few Large Tenants)

```aql
// Create tenant-specific collections
CREATE COLLECTION tenant_a1b2c3d4_projects {
    type: "document",
    sharding: {
        strategy: "hash",
        key: "project_id",
        shards: 4
    }
}

// Query tenant-specific collection
FOR project IN @@collection
    FILTER project.status == "active"
    RETURN project
// Bind: @collection = "tenant_a1b2c3d4_projects"

// Pros: Complete isolation, no query overhead
// Cons: Management complexity, limited tenant count
```

### Strategy 3: Database-Based Isolation (Enterprise)

```aql
// Each tenant gets own database
USE DATABASE tenant_a1b2c3d4

// Standard queries without tenant_id
FOR project IN projects
    FILTER project.status == "active"
    RETURN project

// Pros: Maximum isolation, regulatory compliance
// Cons: Resource overhead, complex operations
```

## Tenant-Aware Queries

### Basic CRUD with Tenant Context

```aql
// Set tenant context at connection level
SET @current_tenant_id = "tenant_a1b2c3d4"

// CREATE - with tenant validation
INSERT {
    project_id: CONCAT("proj_", DATE_NOW()),
    tenant_id: @current_tenant_id,
    name: @projectName,
    owner_id: @userId,
    created_at: DATE_ISO8601(DATE_NOW())
} INTO projects

// READ - tenant-filtered
FOR project IN projects
    FILTER project.tenant_id == @current_tenant_id
    FILTER project.status == "active"
    SORT project.created_at DESC
    LIMIT @offset, @limit
    RETURN project

// UPDATE - with tenant check
FOR project IN projects
    FILTER project.project_id == @projectId
    FILTER project.tenant_id == @current_tenant_id
    UPDATE project WITH {
        status: @newStatus,
        updated_at: DATE_ISO8601(DATE_NOW())
    } IN projects
    RETURN NEW

// DELETE - with tenant check
FOR project IN projects
    FILTER project.project_id == @projectId
    FILTER project.tenant_id == @current_tenant_id
    REMOVE project IN projects
```

### Cross-Tenant Queries (Admin Only)

```aql
// Admin queries across all tenants
// Requires elevated permissions

// Get tenant statistics
FOR tenant IN tenants
    LET project_count = LENGTH(
        FOR p IN projects
            FILTER p.tenant_id == tenant.tenant_id
            RETURN 1
    )
    
    LET user_count = LENGTH(
        FOR u IN users
            FILTER u.tenant_id == tenant.tenant_id
            FILTER u.status == "active"
            RETURN 1
    )
    
    LET storage_used = SUM(
        FOR e IN usage_events
            FILTER e.metadata.tenant_id == tenant.tenant_id
            FILTER e.metadata.event_type == "storage"
            FILTER e.timestamp >= DATE_SUBTRACT(DATE_NOW(), 1, "month")
            RETURN e.metrics.data_size_bytes
    )
    
    RETURN {
        tenant_id: tenant.tenant_id,
        name: tenant.name,
        tier: tenant.tier,
        projects: project_count,
        users: user_count,
        storage_gb: storage_used / (1024 * 1024 * 1024)
    }
```

### Complex Queries with Joins

```aql
// Join across tenant-aware collections
SET @current_tenant_id = @tenantId

FOR project IN projects
    FILTER project.tenant_id == @current_tenant_id
    FILTER project.status == "active"
    
    // Get project owner
    LET owner = FIRST(
        FOR u IN users
            FILTER u.tenant_id == @current_tenant_id
            FILTER u.user_id == project.owner_id
            RETURN u
    )
    
    // Get team members
    LET team = (
        FOR u IN users
            FILTER u.tenant_id == @current_tenant_id
            FILTER u.user_id IN project.team_members
            RETURN {
                user_id: u.user_id,
                name: CONCAT(u.first_name, " ", u.last_name),
                role: u.role
            }
    )
    
    // Get recent activity
    LET activity = (
        FOR e IN usage_events
            FILTER e.metadata.tenant_id == @current_tenant_id
            FILTER e.metadata.resource == "projects"
            FILTER e.metadata.resource_id == project.project_id
            FILTER e.timestamp >= DATE_SUBTRACT(DATE_NOW(), 7, "days")
            SORT e.timestamp DESC
            LIMIT 10
            RETURN e
    )
    
    RETURN {
        project: project,
        owner: owner,
        team: team,
        recent_activity: activity
    }
```

## Resource Quotas & Limits

### Quota Enforcement

```aql
// Check quota before creating resource
LET tenant = FIRST(
    FOR t IN tenants
        FILTER t.tenant_id == @tenantId
        RETURN t
)

// Check user quota
ASSERT tenant.quotas.users.used < tenant.quotas.users.limit,
    "User quota exceeded. Please upgrade your plan."

// Check storage quota
LET current_storage = SUM(
    FOR e IN usage_events
        FILTER e.metadata.tenant_id == @tenantId
        FILTER e.metadata.event_type == "storage"
        FILTER e.timestamp >= DATE_TRUNC(DATE_NOW(), "month")
        RETURN e.metrics.data_size_bytes
) / (1024 * 1024 * 1024)  // Convert to GB

ASSERT current_storage < tenant.quotas.storage_gb.limit,
    CONCAT("Storage quota exceeded: ", current_storage, "GB / ", 
           tenant.quotas.storage_gb.limit, "GB")

// Proceed with creation
INSERT @userData INTO users

// Update quota usage
UPDATE tenant WITH {
    quotas: MERGE(tenant.quotas, {
        users: {
            limit: tenant.quotas.users.limit,
            used: tenant.quotas.users.used + 1
        }
    })
} IN tenants
```

### Rate Limiting

```aql
// Check API rate limits
LET tenant_id = @tenantId
LET current_hour = DATE_TRUNC(DATE_NOW(), "hour")

LET api_calls_this_hour = LENGTH(
    FOR e IN usage_events
        FILTER e.metadata.tenant_id == tenant_id
        FILTER e.metadata.event_type == "api_call"
        FILTER e.timestamp >= current_hour
        RETURN 1
)

LET rate_limit = 10000  // calls per hour

ASSERT api_calls_this_hour < rate_limit,
    CONCAT("Rate limit exceeded: ", api_calls_this_hour, " / ", rate_limit)

// Track API call
INSERT {
    timestamp: DATE_ISO8601(DATE_NOW()),
    metadata: {
        tenant_id: tenant_id,
        event_type: "api_call",
        endpoint: @endpoint,
        method: @method
    },
    metrics: {
        quantity: 1,
        duration_ms: @durationMs
    }
} INTO usage_events
```

### Soft vs Hard Limits

```cpp
// C++ implementation of quota checking
#include <themis/client.hpp>

class QuotaManager {
public:
    enum class LimitType {
        Soft,  // Warning, allow temporarily
        Hard   // Strict enforcement
    };
    
    struct QuotaCheck {
        bool allowed;
        bool warning;
        std::string message;
        double usage_percent;
    };
    
    QuotaCheck check_quota(
        const std::string& tenant_id,
        const std::string& resource,
        int requested_amount = 1
    ) {
        auto tenant = db.query_one<Tenant>(
            "FOR t IN tenants FILTER t.tenant_id == @id RETURN t",
            {{"id", tenant_id}}
        );
        
        auto quota = tenant.quotas[resource];
        auto limit = quota["limit"].get<int>();
        auto used = quota["used"].get<int>();
        auto limit_type = quota["type"].get<std::string>();
        
        double usage_percent = (double)(used + requested_amount) / limit * 100.0;
        
        QuotaCheck result;
        result.usage_percent = usage_percent;
        
        if (limit_type == "hard") {
            // Hard limit - strict enforcement
            result.allowed = (used + requested_amount) <= limit;
            result.warning = usage_percent > 80.0;
            result.message = result.allowed ? "" : 
                "Hard limit reached. Please upgrade your plan.";
        } else {
            // Soft limit - allow with warning
            result.allowed = true;
            result.warning = (used + requested_amount) > limit;
            result.message = result.warning ? 
                "Soft limit exceeded. Consider upgrading." : "";
        }
        
        return result;
    }
};
```

## Billing & Usage Tracking

### Usage Aggregation

```aql
// Calculate monthly usage for billing
LET tenant_id = @tenantId
LET billing_period_start = DATE_TRUNC(DATE_NOW(), "month")
LET billing_period_end = DATE_ADD(billing_period_start, 1, "month")

LET usage_summary = {
    tenant_id: tenant_id,
    period: {
        start: billing_period_start,
        end: billing_period_end
    },
    
    // API calls
    api_calls: LENGTH(
        FOR e IN usage_events
            FILTER e.metadata.tenant_id == tenant_id
            FILTER e.metadata.event_type == "api_call"
            FILTER e.timestamp >= billing_period_start
            FILTER e.timestamp < billing_period_end
            RETURN 1
    ),
    
    // Storage (average for month)
    storage_gb: (
        FOR e IN usage_events
            FILTER e.metadata.tenant_id == tenant_id
            FILTER e.metadata.event_type == "storage"
            FILTER e.timestamp >= billing_period_start
            FILTER e.timestamp < billing_period_end
            COLLECT AGGREGATE avg_storage = AVG(e.metrics.data_size_bytes)
            RETURN avg_storage / (1024 * 1024 * 1024)
    )[0],
    
    // Compute time
    compute_hours: (
        FOR e IN usage_events
            FILTER e.metadata.tenant_id == tenant_id
            FILTER e.metadata.event_type == "compute"
            FILTER e.timestamp >= billing_period_start
            FILTER e.timestamp < billing_period_end
            RETURN e.metrics.duration_ms / (1000 * 60 * 60)
    ),
    
    // Active users
    active_users: LENGTH(
        FOR u IN users
            FILTER u.tenant_id == tenant_id
            FILTER u.status == "active"
            RETURN 1
    ),
    
    // Total cost
    total_cost: SUM(
        FOR e IN usage_events
            FILTER e.metadata.tenant_id == tenant_id
            FILTER e.billable == true
            FILTER e.timestamp >= billing_period_start
            FILTER e.timestamp < billing_period_end
            RETURN e.cost
    )
}

RETURN usage_summary
```

### Billing Invoice Generation

```aql
// Generate invoice for tenant
LET tenant = FIRST(FOR t IN tenants FILTER t.tenant_id == @tenantId RETURN t)
LET usage = @usageSummary  // From previous query

// Calculate line items
LET line_items = [
    {
        description: "Base subscription",
        quantity: 1,
        unit_price: 99.00,
        amount: 99.00
    },
    {
        description: CONCAT("API calls (", usage.api_calls, " calls)"),
        quantity: usage.api_calls,
        unit_price: 0.001,
        amount: usage.api_calls * 0.001
    },
    {
        description: CONCAT("Storage (", ROUND(usage.storage_gb, 2), " GB)"),
        quantity: ROUND(usage.storage_gb, 2),
        unit_price: 0.10,
        amount: ROUND(usage.storage_gb * 0.10, 2)
    },
    {
        description: CONCAT("Active users (", usage.active_users, " users)"),
        quantity: usage.active_users,
        unit_price: 5.00,
        amount: usage.active_users * 5.00
    }
]

LET subtotal = SUM(line_items[*].amount)
LET tax = subtotal * 0.08  // 8% tax
LET total = subtotal + tax

// Create invoice
INSERT {
    invoice_id: CONCAT("INV-", DATE_FORMAT(DATE_NOW(), "%Y%m%d"), "-", @tenantId),
    tenant_id: tenant.tenant_id,
    billing_period: usage.period,
    line_items: line_items,
    subtotal: subtotal,
    tax: tax,
    total: total,
    currency: "USD",
    status: "pending",
    due_date: DATE_ADD(DATE_NOW(), 14, "days"),
    created_at: DATE_ISO8601(DATE_NOW())
} INTO invoices RETURN NEW
```

### Usage-Based Pricing

```aql
// Calculate cost based on usage tiers
LET api_calls = @apiCalls

LET cost = (
    api_calls <= 10000 ? api_calls * 0.001 :
    api_calls <= 100000 ? (10000 * 0.001) + ((api_calls - 10000) * 0.0008) :
    api_calls <= 1000000 ? (10000 * 0.001) + (90000 * 0.0008) + ((api_calls - 100000) * 0.0005) :
    (10000 * 0.001) + (90000 * 0.0008) + (900000 * 0.0005) + ((api_calls - 1000000) * 0.0003)
)

RETURN {
    api_calls: api_calls,
    cost: cost,
    effective_rate: cost / api_calls
}
```

## Tenant Lifecycle

### Tenant Provisioning

```aql
// Complete tenant onboarding workflow
BEGIN TRANSACTION

// 1. Create tenant record
LET tenant = INSERT {
    tenant_id: @tenantId,
    name: @companyName,
    subdomain: @subdomain,
    tier: "trial",
    status: "trial",
    contact: @contact,
    subscription: {
        plan_id: "trial",
        started_at: DATE_ISO8601(DATE_NOW()),
        trial_ends_at: DATE_ISO8601(DATE_ADD(DATE_NOW(), 14, "days"))
    },
    quotas: {
        users: {limit: 5, used: 0},
        storage_gb: {limit: 10, used: 0},
        api_calls_per_month: {limit: 10000, used: 0}
    },
    features: {
        sso: false,
        advanced_analytics: false,
        api_access: true,
        priority_support: false
    },
    created_at: DATE_ISO8601(DATE_NOW()),
    metadata: {
        onboarding_completed: false
    }
} INTO tenants RETURN NEW

// 2. Create admin user
LET admin_user = INSERT {
    user_id: CONCAT("user_", DATE_NOW()),
    tenant_id: tenant.tenant_id,
    email: @adminEmail,
    first_name: @firstName,
    last_name: @lastName,
    role: "admin",
    status: "active",
    permissions: ["*:*"],  // All permissions
    created_at: DATE_ISO8601(DATE_NOW())
} INTO users RETURN NEW

// 3. Create default workspace/project
INSERT {
    project_id: CONCAT("proj_", DATE_NOW()),
    tenant_id: tenant.tenant_id,
    name: "Getting Started",
    description: "Your first project",
    owner_id: admin_user.user_id,
    status: "active",
    created_at: DATE_ISO8601(DATE_NOW())
} INTO projects

// 4. Initialize settings
INSERT {
    tenant_id: tenant.tenant_id,
    category: "general",
    settings: {
        welcome_message: "Welcome to your new workspace!",
        theme: "light",
        notifications_enabled: true
    }
} INTO tenant_settings

// 5. Track onboarding event
INSERT {
    timestamp: DATE_ISO8601(DATE_NOW()),
    metadata: {
        tenant_id: tenant.tenant_id,
        event_type: "tenant_provisioned"
    }
} INTO usage_events

COMMIT TRANSACTION

RETURN {
    tenant: tenant,
    admin_user: admin_user,
    message: "Tenant provisioned successfully"
}
```

### Tenant Upgrade/Downgrade

```aql
// Upgrade tenant to new tier
LET tenant = FIRST(FOR t IN tenants FILTER t.tenant_id == @tenantId RETURN t)
LET new_tier = @newTier  // "professional", "enterprise", etc.

// Get tier configuration
LET tier_config = FIRST(
    FOR config IN tier_configurations
        FILTER config.tier == new_tier
        RETURN config
)

BEGIN TRANSACTION

// Update tenant record
UPDATE tenant WITH {
    tier: new_tier,
    status: "active",
    subscription: MERGE(tenant.subscription, {
        plan_id: tier_config.plan_id,
        upgraded_at: DATE_ISO8601(DATE_NOW()),
        renews_at: DATE_ISO8601(DATE_ADD(DATE_NOW(), 1, "year")),
        mrr: tier_config.price
    }),
    quotas: tier_config.quotas,
    features: tier_config.features
} IN tenants

// Log upgrade event
INSERT {
    timestamp: DATE_ISO8601(DATE_NOW()),
    metadata: {
        tenant_id: @tenantId,
        event_type: "tier_upgraded",
        previous_tier: tenant.tier,
        new_tier: new_tier
    }
} INTO usage_events

COMMIT TRANSACTION

RETURN {message: "Tenant upgraded successfully", new_tier: new_tier}
```

### Tenant Suspension/Cancellation

```aql
// Suspend tenant (non-payment, violation, etc.)
BEGIN TRANSACTION

UPDATE @tenantId WITH {
    status: "suspended",
    suspension: {
        suspended_at: DATE_ISO8601(DATE_NOW()),
        reason: @reason,
        suspended_by: @adminId
    }
} IN tenants

// Invalidate all user sessions
FOR user IN users
    FILTER user.tenant_id == @tenantId
    UPDATE user WITH {
        status: "suspended",
        sessions_invalidated: true
    } IN users

// Log suspension
INSERT {
    timestamp: DATE_ISO8601(DATE_NOW()),
    metadata: {
        tenant_id: @tenantId,
        event_type: "tenant_suspended",
        reason: @reason
    }
} INTO usage_events

COMMIT TRANSACTION

// Cancel tenant (data retention period applies)
BEGIN TRANSACTION

UPDATE @tenantId WITH {
    status: "cancelled",
    cancellation: {
        cancelled_at: DATE_ISO8601(DATE_NOW()),
        data_deletion_scheduled: DATE_ISO8601(DATE_ADD(DATE_NOW(), 90, "days")),
        reason: @reason
    }
} IN tenants

// Archive data (move to cold storage)
// Actual implementation would move data to archive storage

COMMIT TRANSACTION
```

## Security & Compliance

### Row-Level Security Implementation

```aql
// Define RLS policies at collection level
CREATE COLLECTION documents {
    rls_policies: [
        {
            name: "tenant_isolation",
            // Policy applied to all SELECT queries
            using: "doc.tenant_id = @current_tenant_id",
            // Policy applied to INSERT/UPDATE
            with_check: "doc.tenant_id = @current_tenant_id"
        },
        {
            name: "user_access",
            using: "doc.tenant_id = @current_tenant_id AND (doc.owner_id = @current_user_id OR doc.visibility = 'public')"
        }
    ]
}

// Policies automatically enforced
SET @current_tenant_id = "tenant_a1b2c3d4"
SET @current_user_id = "user_5678"

FOR doc IN documents
    // No explicit FILTER needed - RLS automatically applies:
    // FILTER doc.tenant_id == @current_tenant_id
    // FILTER doc.owner_id == @current_user_id OR doc.visibility == 'public'
    RETURN doc
```

### Audit Logging

```aql
// Comprehensive audit trail
CREATE COLLECTION audit_logs {
    type: "timeseries",
    retention: {
        policy: "time_based",
        retain_days: 2555  // 7 years for compliance
    }
}

// Log all data access
INSERT {
    timestamp: DATE_ISO8601(DATE_NOW()),
    metadata: {
        tenant_id: @tenantId,
        user_id: @userId,
        action: @action,  // read, create, update, delete
        resource_type: @resourceType,
        resource_id: @resourceId
    },
    details: {
        ip_address: @ipAddress,
        user_agent: @userAgent,
        session_id: @sessionId,
        changed_fields: @changedFields,  // For updates
        previous_values: @previousValues
    },
    result: {
        success: @success,
        error_message: @errorMessage
    }
} INTO audit_logs

// Query audit logs
FOR log IN audit_logs
    FILTER log.metadata.tenant_id == @tenantId
    FILTER log.timestamp >= @startDate
    FILTER log.timestamp <= @endDate
    FILTER log.metadata.action IN @actions
    SORT log.timestamp DESC
    LIMIT @offset, @limit
    RETURN log
```

### Data Encryption

```cpp
// C++ encryption at application level
#include <themis/encryption.hpp>

class TenantDataEncryption {
private:
    std::map<std::string, std::vector<uint8_t>> tenant_keys;
    
public:
    // Load tenant-specific encryption key
    void load_tenant_key(const std::string& tenant_id) {
        // In production, fetch from secure key management service
        auto key = key_management_service.get_key(tenant_id);
        tenant_keys[tenant_id] = key;
    }
    
    // Encrypt sensitive data before storage
    std::string encrypt(const std::string& tenant_id, const std::string& plaintext) {
        auto key = tenant_keys[tenant_id];
        return themis::encrypt_aes_256_gcm(plaintext, key);
    }
    
    // Decrypt when retrieving
    std::string decrypt(const std::string& tenant_id, const std::string& ciphertext) {
        auto key = tenant_keys[tenant_id];
        return themis::decrypt_aes_256_gcm(ciphertext, key);
    }
};

// Usage
TenantDataEncryption encryption;
encryption.load_tenant_key(tenant_id);

// Store encrypted data
auto encrypted_ssn = encryption.encrypt(tenant_id, customer.ssn);
db.update("customers", customer_id, {
    {"ssn_encrypted", encrypted_ssn}
});

// Retrieve and decrypt
auto customer = db.get("customers", customer_id);
auto decrypted_ssn = encryption.decrypt(tenant_id, customer["ssn_encrypted"]);
```

### GDPR Compliance

```aql
// Data subject access request (DSAR)
FOR doc IN projects
    FILTER doc.tenant_id == @tenantId
    FILTER doc.owner_id == @userId OR @userEmail IN doc.team_members[*].email
    RETURN {
        type: "project",
        id: doc.project_id,
        data: doc,
        collected_at: doc.created_at
    }

UNION

FOR doc IN documents
    FILTER doc.tenant_id == @tenantId
    FILTER doc.created_by == @userId
    RETURN {
        type: "document",
        id: doc.document_id,
        data: doc,
        collected_at: doc.created_at
    }

// Right to be forgotten (data deletion)
BEGIN TRANSACTION

// Anonymize user data
FOR user IN users
    FILTER user.tenant_id == @tenantId
    FILTER user.user_id == @userId
    UPDATE user WITH {
        email: CONCAT("deleted-", user.user_id, "@deleted.local"),
        first_name: "[DELETED]",
        last_name: "[DELETED]",
        phone: null,
        gdpr_deleted: true,
        deleted_at: DATE_ISO8601(DATE_NOW())
    } IN users

// Remove from projects
FOR project IN projects
    FILTER project.tenant_id == @tenantId
    FILTER @userId IN project.team_members
    UPDATE project WITH {
        team_members: REMOVE_VALUE(project.team_members, @userId)
    } IN projects

// Log deletion
INSERT {
    timestamp: DATE_ISO8601(DATE_NOW()),
    metadata: {
        tenant_id: @tenantId,
        event_type: "gdpr_deletion",
        user_id: @userId
    }
} INTO audit_logs

COMMIT TRANSACTION
```

## Performance at Scale

### Sharding Strategy

```yaml
# Optimal sharding for multi-tenant SaaS
sharding:
  # Row-based isolation
  projects:
    strategy: hash
    shard_key: tenant_id  # Critical: shard by tenant for locality
    shard_count: 32
    replication_factor: 3
  
  users:
    strategy: hash
    shard_key: tenant_id
    shard_count: 32
  
  # For very large tenants, consider dedicated shards
  large_tenant_projects:
    strategy: dedicated
    tenants: ["enterprise_tenant_1", "enterprise_tenant_2"]
    shard_count: 16
```

### Query Optimization

```aql
// Always include tenant_id in filters for optimal performance
// BAD: Missing tenant_id - scans all shards
FOR project IN projects
    FILTER project.status == "active"
    RETURN project

// GOOD: Include tenant_id - targets specific shard
FOR project IN projects
    FILTER project.tenant_id == @tenantId  // ← Critical for performance
    FILTER project.status == "active"
    RETURN project

// Use composite indexes
CREATE INDEX idx_tenant_status ON projects (tenant_id, status)

// Batch operations for efficiency
FOR tenant_id IN @tenantIds
    LET projects = (
        FOR p IN projects
            FILTER p.tenant_id == tenant_id
            FILTER p.status == "active"
            RETURN p
    )
    RETURN {
        tenant_id: tenant_id,
        projects: projects
    }
```

### Connection Pooling

```cpp
// Tenant-aware connection pooling
class TenantConnectionPool {
private:
    std::map<std::string, std::shared_ptr<ConnectionPool>> tenant_pools;
    
public:
    auto get_connection(const std::string& tenant_id) {
        if (tenant_pools.find(tenant_id) == tenant_pools.end()) {
            // Create pool for this tenant
            tenant_pools[tenant_id] = std::make_shared<ConnectionPool>(
                connection_string,
                PoolConfig{
                    .min_size = 2,
                    .max_size = 10,
                    .max_idle_time = std::chrono::minutes(5)
                }
            );
            
            // Set tenant context for all connections in pool
            tenant_pools[tenant_id]->set_default_params({
                {"current_tenant_id", tenant_id}
            });
        }
        
        return tenant_pools[tenant_id]->acquire();
    }
};
```

## Monitoring & Operations

### Tenant Health Dashboard

```aql
// Real-time tenant health metrics
FOR tenant IN tenants
    FILTER tenant.status == "active"
    
    LET health_metrics = {
        // Usage metrics
        active_users: LENGTH(
            FOR u IN users
                FILTER u.tenant_id == tenant.tenant_id
                FILTER u.last_login >= DATE_SUBTRACT(DATE_NOW(), 7, "days")
                RETURN 1
        ),
        
        // Performance metrics
        avg_api_latency_ms: (
            FOR e IN usage_events
                FILTER e.metadata.tenant_id == tenant.tenant_id
                FILTER e.metadata.event_type == "api_call"
                FILTER e.timestamp >= DATE_SUBTRACT(DATE_NOW(), 1, "hour")
                COLLECT AGGREGATE avg = AVG(e.metrics.duration_ms)
                RETURN avg
        )[0],
        
        // Error rate
        error_rate: (
            LET total = LENGTH(
                FOR e IN usage_events
                    FILTER e.metadata.tenant_id == tenant.tenant_id
                    FILTER e.timestamp >= DATE_SUBTRACT(DATE_NOW(), 1, "hour")
                    RETURN 1
            )
            LET errors = LENGTH(
                FOR e IN usage_events
                    FILTER e.metadata.tenant_id == tenant.tenant_id
                    FILTER e.metadata.status >= 400
                    FILTER e.timestamp >= DATE_SUBTRACT(DATE_NOW(), 1, "hour")
                    RETURN 1
            )
            RETURN total > 0 ? errors / total : 0
        )[0],
        
        // Quota usage
        quota_usage: {
            users: tenant.quotas.users.used / tenant.quotas.users.limit,
            storage: tenant.quotas.storage_gb.used / tenant.quotas.storage_gb.limit,
            api_calls: tenant.quotas.api_calls_per_month.used / tenant.quotas.api_calls_per_month.limit
        }
    }
    
    // Health score (0-100)
    LET health_score = (
        (health_metrics.active_users > 0 ? 25 : 0) +
        (health_metrics.avg_api_latency_ms < 500 ? 25 : health_metrics.avg_api_latency_ms < 1000 ? 15 : 0) +
        (health_metrics.error_rate < 0.01 ? 25 : health_metrics.error_rate < 0.05 ? 15 : 0) +
        (health_metrics.quota_usage.users < 0.9 ? 25 : 15)
    )
    
    RETURN {
        tenant_id: tenant.tenant_id,
        name: tenant.name,
        tier: tenant.tier,
        health_score: health_score,
        metrics: health_metrics,
        alerts: (
            health_score < 50 ? ["degraded_performance"] :
            health_metrics.quota_usage.users > 0.9 ? ["quota_warning"] :
            []
        )
    }
```

### Alerting

```aql
// Monitor for tenants approaching limits
FOR tenant IN tenants
    FILTER tenant.status == "active"
    
    LET quota_warnings = (
        FOR quota_name IN ATTRIBUTES(tenant.quotas)
            LET quota = tenant.quotas[quota_name]
            LET usage_percent = quota.used / quota.limit
            FILTER usage_percent > 0.8
            RETURN {
                resource: quota_name,
                usage_percent: usage_percent,
                limit: quota.limit,
                used: quota.used
            }
    )
    
    FILTER LENGTH(quota_warnings) > 0
    
    // Generate alert
    INSERT {
        alert_type: "quota_warning",
        tenant_id: tenant.tenant_id,
        severity: quota_warnings[0].usage_percent > 0.95 ? "critical" : "warning",
        details: quota_warnings,
        created_at: DATE_ISO8601(DATE_NOW())
    } INTO alerts
    
    RETURN {
        tenant: tenant.name,
        warnings: quota_warnings
    }
```

## Best Practices

1. **Data Isolation**
   - Use RLS for most SaaS applications (best balance)
   - Reserve database-based isolation for enterprise/regulated tenants
   - Always include tenant_id in WHERE clauses for performance

2. **Security**
   - Implement RLS policies at database level
   - Encrypt sensitive data at rest and in transit
   - Log all data access for audit compliance
   - Use tenant-specific encryption keys

3. **Performance**
   - Shard by tenant_id for data locality
   - Create composite indexes: (tenant_id, other_fields)
   - Use connection pooling with tenant context
   - Monitor and optimize slow queries

4. **Quotas & Billing**
   - Implement soft limits with warnings
   - Track usage in real-time
   - Provide self-service upgrade paths
   - Generate detailed usage reports

5. **Operations**
   - Monitor tenant health metrics
   - Alert on quota thresholds
   - Automate tenant provisioning
   - Implement graceful degradation

## Related Documentation

- [Sharding Strategy](../architecture/sharding.md)
- [Security Best Practices](../security/best-practices.md)
- [AQL Reference](../aql/README.md)
- [Multi-Tenancy Guide](../architecture/multi-tenancy.md)

## Example Projects

- [SaaS Template](../../examples/saas-template/)
- [CRM System](../../examples/17_crm/)
- [Project Management](../../examples/16_kanban_board/)

## Conclusion

ThemisDB provides robust multi-tenancy support through RLS, flexible sharding, and comprehensive quota management. The platform scales from small SaaS startups to enterprise applications with thousands of tenants while maintaining strong isolation guarantees and regulatory compliance.
