# API Reference - DMS/ERP System

> **Historischer Stand:** 2026-01-31 — Inhalte nicht gegen aktuelle Quellen geprüft.

## 📋 Übersicht

Vollständige REST API-Dokumentation für das DMS/ERP-System basierend auf ThemisDB.

**Base URL:** `http://localhost:8080/api/v1`

## 🔐 Authentication

### Authentifizierungs-Methoden

Alle API-Requests benötigen Authentication via:

#### 1. API Key (Header)
```http
GET /api/v1/documents
Authorization: Bearer <api_key>
```

#### 2. Session Token (Cookie)
```http
GET /api/v1/documents
Cookie: session_token=<token>
```

#### 3. OAuth2 (optional)
```http
GET /api/v1/documents
Authorization: Bearer <oauth_token>
```

### Login
```http
POST /api/v1/auth/login
Content-Type: application/json

{
    "username": "user@example.com",
    "password": "secure_password",
    "mfa_code": "123456"  // Optional, falls MFA aktiviert
}
```

**Response:**
```json
{
    "status": "success",
    "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
    "refresh_token": "rt_abc123...",
    "expires_in": 3600,
    "user": {
        "id": "user_uuid",
        "username": "user@example.com",
        "roles": ["employee", "manager"]
    }
}
```

### Logout
```http
POST /api/v1/auth/logout
Authorization: Bearer <token>
```

**Response:**
```json
{
    "status": "success",
    "message": "Logged out successfully"
}
```

### Token Refresh
```http
POST /api/v1/auth/refresh
Content-Type: application/json

{
    "refresh_token": "rt_abc123..."
}
```

## 📄 Documents API

### List Documents

**Endpoint:** `GET /api/v1/documents`

**Query Parameters:**
- `page` (int, default: 1) - Seitennummer
- `per_page` (int, default: 50, max: 100) - Dokumente pro Seite
- `type` (string) - Filter nach Dokumenttyp (invoice, contract, etc.)
- `status` (string) - Filter nach Workflow-Status
- `owner` (string) - Filter nach Besitzer
- `tags` (array) - Filter nach Tags
- `sort_by` (string) - Sortierfeld (created, updated, title)
- `sort_order` (string) - Sortierrichtung (asc, desc)
- `search` (string) - Volltext-Suche

**Example:**
```http
GET /api/v1/documents?type=invoice&status=approved&page=1&per_page=20
Authorization: Bearer <token>
```

**Response:**
```json
{
    "status": "success",
    "data": [
        {
            "id": "doc_uuid_1",
            "title": "Rechnung_2025_001.pdf",
            "type": "invoice",
            "version": 3,
            "current_version_id": "version_uuid",
            "metadata": {
                "invoice_number": "2025-001",
                "amount": 1299.99,
                "customer": "Firma XYZ",
                "date": "2025-12-22"
            },
            "tags": ["rechnung", "2025", "gezahlt"],
            "owner": {
                "id": "user_uuid",
                "name": "Max Mustermann"
            },
            "workflow_state": "approved",
            "created_at": "2025-12-22T10:00:00Z",
            "updated_at": "2025-12-22T14:30:00Z"
        }
    ],
    "pagination": {
        "page": 1,
        "per_page": 20,
        "total_items": 150,
        "total_pages": 8,
        "has_next": true,
        "has_prev": false
    }
}
```

### Get Document

**Endpoint:** `GET /api/v1/documents/{document_id}`

**Response:**
```json
{
    "status": "success",
    "data": {
        "id": "doc_uuid_1",
        "title": "Rechnung_2025_001.pdf",
        "type": "invoice",
        "version": 3,
        "current_version_id": "version_uuid",
        "metadata": {
            "invoice_number": "2025-001",
            "amount": 1299.99,
            "customer": "Firma XYZ",
            "date": "2025-12-22"
        },
        "tags": ["rechnung", "2025", "gezahlt"],
        "owner": {
            "id": "user_uuid",
            "name": "Max Mustermann"
        },
        "permissions": [
            {
                "user_id": "user1",
                "role": "read",
                "granted_at": "2025-12-22T10:00:00Z"
            }
        ],
        "workflow_state": "approved",
        "workflow_history": [
            {
                "state": "draft",
                "user": "user_uuid",
                "timestamp": "2025-12-22T10:00:00Z"
            },
            {
                "state": "review",
                "user": "manager_uuid",
                "timestamp": "2025-12-22T12:00:00Z"
            },
            {
                "state": "approved",
                "user": "director_uuid",
                "timestamp": "2025-12-22T14:30:00Z"
            }
        ],
        "created_at": "2025-12-22T10:00:00Z",
        "updated_at": "2025-12-22T14:30:00Z"
    }
}
```

### Create Document

**Endpoint:** `POST /api/v1/documents`

**Request:**
```http
POST /api/v1/documents
Content-Type: multipart/form-data
Authorization: Bearer <token>

{
    "title": "Neue Rechnung",
    "type": "invoice",
    "file": <binary_data>,
    "metadata": {
        "invoice_number": "2025-002",
        "amount": 599.99,
        "customer": "Kunde ABC"
    },
    "tags": ["rechnung", "2025"],
    "workflow_id": "workflow_uuid"
}
```

**Response:**
```json
{
    "status": "success",
    "data": {
        "id": "doc_uuid_2",
        "title": "Neue Rechnung",
        "type": "invoice",
        "version": 1,
        "file_url": "/files/doc_uuid_2/v1",
        "workflow_state": "draft",
        "created_at": "2025-12-22T15:00:00Z"
    }
}
```

### Update Document

**Endpoint:** `PATCH /api/v1/documents/{document_id}`

**Request:**
```json
{
    "title": "Aktualisierte Rechnung",
    "metadata": {
        "amount": 649.99
    },
    "tags": ["rechnung", "2025", "aktualisiert"]
}
```

**Response:**
```json
{
    "status": "success",
    "data": {
        "id": "doc_uuid_2",
        "version": 2,
        "updated_at": "2025-12-22T15:30:00Z"
    }
}
```

### Delete Document

**Endpoint:** `DELETE /api/v1/documents/{document_id}`

**Query Parameters:**
- `permanent` (bool, default: false) - Permanentes Löschen vs. Soft-Delete

**Response:**
```json
{
    "status": "success",
    "message": "Document deleted successfully",
    "deleted_at": "2025-12-22T16:00:00Z"
}
```

### Upload New Version

**Endpoint:** `POST /api/v1/documents/{document_id}/versions`

**Request:**
```http
POST /api/v1/documents/{document_id}/versions
Content-Type: multipart/form-data
Authorization: Bearer <token>

{
    "file": <binary_data>,
    "comment": "Korrigierte Version"
}
```

**Response:**
```json
{
    "status": "success",
    "data": {
        "version": 4,
        "version_id": "version_uuid_4",
        "file_url": "/files/doc_uuid_1/v4",
        "created_at": "2025-12-22T16:30:00Z"
    }
}
```

### Get Document Versions

**Endpoint:** `GET /api/v1/documents/{document_id}/versions`

**Response:**
```json
{
    "status": "success",
    "data": [
        {
            "version": 1,
            "version_id": "version_uuid_1",
            "file_size": 1024000,
            "created_by": "user_uuid",
            "created_at": "2025-12-22T10:00:00Z",
            "comment": "Initial version"
        },
        {
            "version": 2,
            "version_id": "version_uuid_2",
            "file_size": 1056000,
            "created_by": "user_uuid",
            "created_at": "2025-12-22T12:00:00Z",
            "comment": "Updated metadata"
        }
    ]
}
```

### Download Document

**Endpoint:** `GET /api/v1/documents/{document_id}/download`

**Query Parameters:**
- `version` (int, optional) - Spezifische Version (default: aktuelle)

**Response:**
```http
HTTP/1.1 200 OK
Content-Type: application/pdf
Content-Disposition: attachment; filename="Rechnung_2025_001.pdf"
Content-Length: 1024000

<binary_data>
```

## 🔄 Workflow API

### Execute Workflow Action

**Endpoint:** `POST /api/v1/documents/{document_id}/workflow/action`

**Request:**
```json
{
    "action": "submit",
    "comment": "Bitte um Genehmigung",
    "metadata": {
        "urgency": "high"
    }
}
```

**Response:**
```json
{
    "status": "success",
    "data": {
        "previous_state": "draft",
        "new_state": "review",
        "assigned_to": "manager_uuid",
        "transition_id": "transition_uuid"
    }
}
```

### Get Workflow Status

**Endpoint:** `GET /api/v1/documents/{document_id}/workflow/status`

**Response:**
```json
{
    "status": "success",
    "data": {
        "current_state": "review",
        "assigned_to": {
            "id": "manager_uuid",
            "name": "Manager Müller"
        },
        "available_actions": ["approve", "reject", "request_changes"],
        "pending_since": "2025-12-22T10:00:00Z",
        "timeout_at": "2025-12-25T10:00:00Z",
        "history": [
            {
                "from_state": "draft",
                "to_state": "review",
                "action": "submit",
                "user": "user_uuid",
                "timestamp": "2025-12-22T10:00:00Z"
            }
        ]
    }
}
```

### Get Pending Tasks

**Endpoint:** `GET /api/v1/workflow/tasks`

**Query Parameters:**
- `status` (string) - Filter nach Status (pending, completed, cancelled)
- `assigned_to` (string) - Filter nach zugewiesenem User

**Response:**
```json
{
    "status": "success",
    "data": [
        {
            "id": "task_uuid",
            "type": "approval_request",
            "document": {
                "id": "doc_uuid",
                "title": "Rechnung_2025_001.pdf"
            },
            "assigned_to": "manager_uuid",
            "status": "pending",
            "priority": "high",
            "due_date": "2025-12-25T10:00:00Z",
            "created_at": "2025-12-22T10:00:00Z"
        }
    ]
}
```

## 🔍 Search API

### Full-Text Search

**Endpoint:** `POST /api/v1/search/fulltext`

**Request:**
```json
{
    "query": "Rechnung Firma XYZ 2025",
    "filters": {
        "type": "invoice",
        "date_range": {
            "from": "2025-01-01",
            "to": "2025-12-31"
        }
    },
    "page": 1,
    "per_page": 20
}
```

**Response:**
```json
{
    "status": "success",
    "data": [
        {
            "id": "doc_uuid_1",
            "title": "Rechnung_2025_001.pdf",
            "relevance_score": 0.95,
            "highlights": [
                "...Firma <mark>XYZ</mark> <mark>Rechnung</mark> Nr. 2025-001..."
            ],
            "type": "invoice",
            "created_at": "2025-12-22T10:00:00Z"
        }
    ],
    "pagination": {
        "page": 1,
        "per_page": 20,
        "total_items": 5
    }
}
```

### Vector Search (Similarity)

**Endpoint:** `POST /api/v1/search/similar`

**Request:**
```json
{
    "document_id": "doc_uuid_1",
    "top_k": 10,
    "threshold": 0.7
}
```

**Response:**
```json
{
    "status": "success",
    "data": [
        {
            "id": "doc_uuid_5",
            "title": "Ähnliche Rechnung",
            "similarity_score": 0.92,
            "type": "invoice"
        }
    ]
}
```

## 👥 Users & Permissions API

### List Users

**Endpoint:** `GET /api/v1/users`

**Response:**
```json
{
    "status": "success",
    "data": [
        {
            "id": "user_uuid_1",
            "username": "max.mustermann",
            "email": "max@example.com",
            "full_name": "Max Mustermann",
            "roles": ["employee", "manager"],
            "status": "active",
            "created_at": "2025-01-01T00:00:00Z"
        }
    ]
}
```

### Get User

**Endpoint:** `GET /api/v1/users/{user_id}`

**Response:**
```json
{
    "status": "success",
    "data": {
        "id": "user_uuid_1",
        "username": "max.mustermann",
        "email": "max@example.com",
        "full_name": "Max Mustermann",
        "roles": ["employee", "manager"],
        "permissions": [
            {
                "resource": "documents",
                "actions": ["read", "write", "delete"]
            },
            {
                "resource": "workflows",
                "actions": ["approve"]
            }
        ],
        "supervisor_id": "user_uuid_5",
        "department": "Finance",
        "status": "active",
        "last_login": "2025-12-22T09:00:00Z",
        "created_at": "2025-01-01T00:00:00Z"
    }
}
```

### Create User

**Endpoint:** `POST /api/v1/users`

**Request:**
```json
{
    "username": "new.user",
    "email": "new.user@example.com",
    "full_name": "New User",
    "password": "secure_password_123",
    "roles": ["employee"],
    "department": "IT"
}
```

**Response:**
```json
{
    "status": "success",
    "data": {
        "id": "user_uuid_new",
        "username": "new.user",
        "created_at": "2025-12-22T16:00:00Z"
    }
}
```

### Update User Permissions

**Endpoint:** `PATCH /api/v1/users/{user_id}/permissions`

**Request:**
```json
{
    "add_roles": ["manager"],
    "remove_roles": [],
    "add_permissions": [
        {
            "resource": "workflows",
            "actions": ["approve", "reject"]
        }
    ]
}
```

**Response:**
```json
{
    "status": "success",
    "message": "Permissions updated successfully"
}
```

### Grant Document Access

**Endpoint:** `POST /api/v1/documents/{document_id}/permissions`

**Request:**
```json
{
    "user_id": "user_uuid_3",
    "role": "read",  // read, write, admin
    "expires_at": "2026-01-01T00:00:00Z"  // Optional
}
```

**Response:**
```json
{
    "status": "success",
    "message": "Access granted successfully"
}
```

### Revoke Document Access

**Endpoint:** `DELETE /api/v1/documents/{document_id}/permissions/{user_id}`

**Response:**
```json
{
    "status": "success",
    "message": "Access revoked successfully"
}
```

## 📊 Audit Log API

### Get Audit Logs

**Endpoint:** `GET /api/v1/audit/logs`

**Query Parameters:**
- `resource_type` (string) - documents, users, workflows
- `resource_id` (string) - Spezifische Ressource
- `user_id` (string) - Filter nach User
- `action` (string) - create, update, delete, read
- `from_date` (datetime) - Startdatum
- `to_date` (datetime) - Enddatum
- `page` (int)
- `per_page` (int)

**Response:**
```json
{
    "status": "success",
    "data": [
        {
            "id": "audit_uuid",
            "timestamp": "2025-12-22T10:00:00Z",
            "user": {
                "id": "user_uuid",
                "username": "max.mustermann"
            },
            "action": "update",
            "resource_type": "document",
            "resource_id": "doc_uuid_1",
            "changes": {
                "title": {
                    "old": "Alte Rechnung",
                    "new": "Neue Rechnung"
                }
            },
            "ip_address": "192.168.1.100",
            "user_agent": "Mozilla/5.0..."
        }
    ],
    "pagination": {
        "page": 1,
        "per_page": 50,
        "total_items": 1500
    }
}
```

### Export Audit Logs

**Endpoint:** `POST /api/v1/audit/export`

**Request:**
```json
{
    "format": "csv",  // csv, json, pdf
    "filters": {
        "from_date": "2025-01-01",
        "to_date": "2025-12-31",
        "user_id": "user_uuid"
    }
}
```

**Response:**
```json
{
    "status": "success",
    "data": {
        "export_id": "export_uuid",
        "download_url": "/api/v1/audit/exports/export_uuid",
        "expires_at": "2025-12-23T16:00:00Z"
    }
}
```

## 📈 Analytics API

### Get Document Statistics

**Endpoint:** `GET /api/v1/analytics/documents/stats`

**Response:**
```json
{
    "status": "success",
    "data": {
        "total_documents": 1500,
        "by_type": {
            "invoice": 800,
            "contract": 400,
            "report": 300
        },
        "by_status": {
            "draft": 50,
            "review": 100,
            "approved": 1200,
            "rejected": 150
        },
        "storage_used_mb": 15000,
        "avg_approval_time_hours": 18.5
    }
}
```

### Get Workflow Metrics

**Endpoint:** `GET /api/v1/analytics/workflows/metrics`

**Query Parameters:**
- `from_date` (datetime)
- `to_date` (datetime)
- `workflow_id` (string)

**Response:**
```json
{
    "status": "success",
    "data": {
        "total_workflows": 500,
        "completed": 450,
        "in_progress": 40,
        "timeout": 10,
        "avg_completion_time_hours": 24.5,
        "bottlenecks": [
            {
                "state": "manager_approval",
                "avg_time_hours": 48,
                "count": 200
            }
        ]
    }
}
```

## ⚠️ Error Responses

### Standard Error Format

```json
{
    "status": "error",
    "error": {
        "code": "RESOURCE_NOT_FOUND",
        "message": "Document with ID 'doc_uuid_99' not found",
        "details": {
            "resource_type": "document",
            "resource_id": "doc_uuid_99"
        },
        "timestamp": "2025-12-22T16:00:00Z",
        "request_id": "req_abc123"
    }
}
```

### Error Codes

| Code | HTTP Status | Beschreibung |
|------|-------------|--------------|
| `AUTHENTICATION_REQUIRED` | 401 | Keine oder ungültige Authentifizierung |
| `PERMISSION_DENIED` | 403 | Keine Berechtigung für diese Aktion |
| `RESOURCE_NOT_FOUND` | 404 | Ressource nicht gefunden |
| `VALIDATION_ERROR` | 400 | Ungültige Request-Daten |
| `CONFLICT` | 409 | Ressourcen-Konflikt (z.B. Version) |
| `RATE_LIMIT_EXCEEDED` | 429 | Zu viele Requests |
| `INTERNAL_ERROR` | 500 | Interner Serverfehler |

## 🔒 Rate Limiting

**Limits:**
- Authentifizierte Requests: 1000/Stunde
- Unauthentifizierte Requests: 100/Stunde
- File Upload: 100/Tag
- Search API: 500/Stunde

**Response Headers:**
```http
X-RateLimit-Limit: 1000
X-RateLimit-Remaining: 950
X-RateLimit-Reset: 1640185200
```

**Rate Limit Error:**
```json
{
    "status": "error",
    "error": {
        "code": "RATE_LIMIT_EXCEEDED",
        "message": "Rate limit exceeded. Retry after 300 seconds.",
        "retry_after": 300
    }
}
```

## 🔔 Webhooks

### Register Webhook

**Endpoint:** `POST /api/v1/webhooks`

**Request:**
```json
{
    "url": "https://example.com/webhook",
    "events": [
        "document.created",
        "document.updated",
        "workflow.state_changed"
    ],
    "secret": "webhook_secret_123"
}
```

**Response:**
```json
{
    "status": "success",
    "data": {
        "id": "webhook_uuid",
        "url": "https://example.com/webhook",
        "events": ["document.created", "document.updated", "workflow.state_changed"],
        "created_at": "2025-12-22T16:00:00Z"
    }
}
```

### Webhook Payload Example

```json
{
    "event": "document.created",
    "timestamp": "2025-12-22T16:00:00Z",
    "data": {
        "id": "doc_uuid_new",
        "title": "Neue Rechnung",
        "type": "invoice",
        "owner": "user_uuid"
    },
    "signature": "sha256=abc123..."
}
```

## 📝 SDK Examples

### Python
```python
import requests

class ThemisDBClient:
    def __init__(self, base_url, api_key):
        self.base_url = base_url
        self.headers = {"Authorization": f"Bearer {api_key}"}
    
    def list_documents(self, **filters):
        response = requests.get(
            f"{self.base_url}/documents",
            headers=self.headers,
            params=filters
        )
        return response.json()
    
    def create_document(self, title, file_path, **metadata):
        with open(file_path, 'rb') as f:
            files = {'file': f}
            data = {'title': title, 'metadata': metadata}
            response = requests.post(
                f"{self.base_url}/documents",
                headers=self.headers,
                files=files,
                data=data
            )
        return response.json()

# Verwendung
client = ThemisDBClient("http://localhost:8080/api/v1", "your_api_key")
docs = client.list_documents(type="invoice", status="approved")
```

### JavaScript
```javascript
class ThemisDBClient {
    constructor(baseUrl, apiKey) {
        this.baseUrl = baseUrl;
        this.apiKey = apiKey;
    }
    
    async listDocuments(filters = {}) {
        const params = new URLSearchParams(filters);
        const response = await fetch(
            `${this.baseUrl}/documents?${params}`,
            {
                headers: {
                    'Authorization': `Bearer ${this.apiKey}`
                }
            }
        );
        return await response.json();
    }
    
    async createDocument(title, file, metadata = {}) {
        const formData = new FormData();
        formData.append('title', title);
        formData.append('file', file);
        formData.append('metadata', JSON.stringify(metadata));
        
        const response = await fetch(
            `${this.baseUrl}/documents`,
            {
                method: 'POST',
                headers: {
                    'Authorization': `Bearer ${this.apiKey}`
                },
                body: formData
            }
        );
        return await response.json();
    }
}

// Verwendung
const client = new ThemisDBClient('http://localhost:8080/api/v1', 'your_api_key');
const docs = await client.listDocuments({ type: 'invoice', status: 'approved' });
```

## 🎓 Best Practices

1. **Authentifizierung**
   - Verwende API Keys für Server-to-Server
   - Verwende OAuth2 für User-Facing Apps
   - Rotiere API Keys regelmäßig

2. **Fehlerbehandlung**
   - Prüfe immer HTTP-Status
   - Parse Error-Response für Details
   - Implementiere Retry-Logic mit Exponential Backoff

3. **Performance**
   - Nutze Pagination für große Resultsets
   - Cache häufig abgerufene Daten
   - Verwende Batch-Operationen wo möglich

4. **Sicherheit**
   - Verwende HTTPS immer
   - Validiere Webhook-Signaturen
   - Speichere API Keys sicher (z.B. Environment Variables)

## 📚 Weitere Dokumentation

- [WORKFLOW_DESIGN.md](WORKFLOW_DESIGN.md) - Workflow-Patterns
- [SECURITY.md](SECURITY.md) - Sicherheitskonzepte
- [ADMIN_GUIDE.md](ADMIN_GUIDE.md) - Administration
