> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# DMS/ERP System - Enterprise Integration Guide

> **Historischer Stand:** 2026-01-31 — Inhalte nicht gegen aktuelle Quellen geprüft.

## Overview

This guide demonstrates how to build a production-ready Document Management System (DMS) integrated with ERP functionality using ThemisDB's multi-model capabilities.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Web Application Layer                    │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │   UI     │  │   API    │  │  Auth    │  │  Search  │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
└────────────────────────┬─────────────────────────────────────┘
                         │
┌────────────────────────▼─────────────────────────────────────┐
│                  Business Logic Layer                         │
│  ┌──────────────┐  ┌─────────────┐  ┌──────────────────┐   │
│  │ Document Mgmt│  │  Workflow   │  │  Audit & Track   │   │
│  └──────────────┘  └─────────────┘  └──────────────────┘   │
└────────────────────────┬─────────────────────────────────────┘
                         │
┌────────────────────────▼─────────────────────────────────────┐
│                      ThemisDB Layer                          │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │Document  │  │  Graph   │  │  Vector  │  │Time-Series│   │
│  │ Storage  │  │Relations │  │ Search   │  │  Audit   │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
└──────────────────────────────────────────────────────────────┘
```

## Features

### Core DMS Functionality
- ✅ Document storage and versioning
- ✅ Full-text search with vector similarity
- ✅ Hierarchical folder structure (graph model)
- ✅ Metadata extraction and indexing
- ✅ Access control and permissions
- ✅ Document workflow and approval
- ✅ OCR for scanned documents
- ✅ Preview generation

### ERP Integration
- ✅ Invoice processing and matching
- ✅ Purchase order tracking
- ✅ Contract management
- ✅ Vendor/Customer document linking
- ✅ Compliance and retention policies
- ✅ Multi-language support
- ✅ Audit trail and versioning

### Advanced Features
- ✅ AI-powered document classification
- ✅ Automatic metadata extraction
- ✅ Duplicate detection
- ✅ Email integration (IMAP/SMTP)
- ✅ Mobile access (REST API)
- ✅ Cloud backup integration
- ✅ Multi-tenancy support

## Data Model

### Document Entity

```python
{
    "id": "doc_uuid",
    "type": "document",
    "title": "Invoice #12345",
    "filename": "invoice_12345.pdf",
    "mime_type": "application/pdf",
    "size_bytes": 524288,
    "hash_sha256": "abc123...",
    
    # Storage
    "storage_path": "/documents/2025/02/invoice_12345.pdf",
    "thumbnail_path": "/thumbnails/invoice_12345_thumb.jpg",
    
    # Metadata
    "metadata": {
        "author": "John Doe",
        "created_date": "2025-02-07T10:30:00Z",
        "modified_date": "2025-02-07T10:30:00Z",
        "document_date": "2025-02-05",
        "language": "en",
        "page_count": 3,
        "keywords": ["invoice", "payment", "vendor"],
        "custom": {
            "invoice_number": "INV-12345",
            "vendor_id": "VEND-001",
            "total_amount": 5000.00,
            "currency": "USD",
            "due_date": "2025-03-05"
        }
    },
    
    # Classification
    "category": "invoice",
    "sub_category": "purchase_invoice",
    "department": "accounting",
    "classification_confidence": 0.95,
    
    # Version control
    "version": 2,
    "version_history": [
        {
            "version": 1,
            "timestamp": "2025-02-07T10:30:00Z",
            "user": "user123",
            "action": "created",
            "storage_path": "/archive/doc_uuid_v1.pdf"
        },
        {
            "version": 2,
            "timestamp": "2025-02-07T11:00:00Z",
            "user": "user456",
            "action": "updated",
            "storage_path": "/documents/2025/02/invoice_12345.pdf"
        }
    ],
    
    # Access control
    "owner": "user123",
    "permissions": {
        "read": ["user123", "user456", "group:accounting"],
        "write": ["user123"],
        "delete": ["user123", "group:admin"]
    },
    
    # Workflow
    "workflow_state": "approved",
    "workflow_history": [
        {
            "state": "pending_review",
            "timestamp": "2025-02-07T10:30:00Z",
            "user": "user123"
        },
        {
            "state": "approved",
            "timestamp": "2025-02-07T14:00:00Z",
            "user": "user789",
            "comment": "Approved by manager"
        }
    ],
    
    # Search optimization
    "content_embedding": [0.123, -0.456, ...],  # 512D vector
    "extracted_text": "Full text content from OCR...",
    "entities": [
        {"type": "amount", "value": "5000.00", "confidence": 0.98},
        {"type": "date", "value": "2025-02-05", "confidence": 0.95},
        {"type": "vendor", "value": "ACME Corp", "confidence": 0.92}
    ],
    
    # Audit
    "created_at": "2025-02-07T10:30:00Z",
    "created_by": "user123",
    "modified_at": "2025-02-07T11:00:00Z",
    "modified_by": "user456",
    "access_count": 15,
    "last_accessed": "2025-02-07T15:30:00Z"
}
```

### Folder/Collection Entity

```python
{
    "id": "folder_uuid",
    "type": "folder",
    "name": "2025 Invoices",
    "path": "/Accounting/Invoices/2025",
    "parent_id": "parent_folder_uuid",
    
    # Metadata
    "description": "All invoices for fiscal year 2025",
    "tags": ["invoices", "2025", "accounting"],
    
    # Access control
    "owner": "user123",
    "permissions": {
        "read": ["group:accounting", "group:management"],
        "write": ["group:accounting"],
        "admin": ["user123"]
    },
    
    # Statistics
    "document_count": 150,
    "total_size_bytes": 78643200,
    "last_updated": "2025-02-07T15:00:00Z",
    
    # Retention policy
    "retention": {
        "policy": "financial_7years",
        "expire_date": "2032-12-31",
        "auto_delete": false
    }
}
```

### Workflow Definition

```python
{
    "id": "workflow_uuid",
    "name": "Invoice Approval Workflow",
    "description": "Standard invoice approval process",
    
    "states": [
        {
            "id": "draft",
            "name": "Draft",
            "is_initial": true,
            "allowed_transitions": ["submitted"]
        },
        {
            "id": "submitted",
            "name": "Submitted for Review",
            "allowed_transitions": ["approved", "rejected", "needs_info"]
        },
        {
            "id": "needs_info",
            "name": "Needs More Information",
            "allowed_transitions": ["submitted"]
        },
        {
            "id": "approved",
            "name": "Approved",
            "is_final": true,
            "allowed_transitions": []
        },
        {
            "id": "rejected",
            "name": "Rejected",
            "is_final": true,
            "allowed_transitions": []
        }
    ],
    
    "rules": [
        {
            "from_state": "submitted",
            "to_state": "approved",
            "condition": "amount <= 1000",
            "required_role": "manager"
        },
        {
            "from_state": "submitted",
            "to_state": "approved",
            "condition": "amount > 1000",
            "required_role": "director",
            "additional_approvals": 2
        }
    ],
    
    "notifications": [
        {
            "trigger": "state_change",
            "from_state": "submitted",
            "to_state": "approved",
            "recipients": ["document_owner", "accounting_team"],
            "template": "invoice_approved"
        }
    ]
}
```

## Implementation

### 1. Document Upload and Processing

```python
from themis_client import ThemisClient
import hashlib
import mimetypes
from PIL import Image
import pytesseract
import fitz  # PyMuPDF

class DocumentProcessor:
    def __init__(self, client: ThemisClient):
        self.client = client
        
    def upload_document(self, file_path, folder_id, metadata=None):
        """Upload and process a new document"""
        
        # 1. Calculate hash
        with open(file_path, 'rb') as f:
            file_hash = hashlib.sha256(f.read()).hexdigest()
        
        # 2. Check for duplicates
        existing = self.client.query(f"""
            FOR doc IN documents
                FILTER doc.hash_sha256 == '{file_hash}'
                RETURN doc
        """)
        
        if existing:
            return {"status": "duplicate", "existing_id": existing[0]['id']}
        
        # 3. Extract text content
        mime_type = mimetypes.guess_type(file_path)[0]
        extracted_text = ""
        
        if mime_type == "application/pdf":
            extracted_text = self._extract_pdf_text(file_path)
        elif mime_type.startswith("image/"):
            extracted_text = self._extract_image_text(file_path)
        
        # 4. Generate embedding for vector search
        embedding = self._generate_embedding(extracted_text)
        
        # 5. Classify document
        classification = self._classify_document(extracted_text)
        
        # 6. Extract entities
        entities = self._extract_entities(extracted_text)
        
        # 7. Generate thumbnail
        thumbnail_path = self._generate_thumbnail(file_path)
        
        # 8. Create document record
        document = {
            "type": "document",
            "filename": os.path.basename(file_path),
            "mime_type": mime_type,
            "size_bytes": os.path.getsize(file_path),
            "hash_sha256": file_hash,
            "storage_path": self._store_file(file_path),
            "thumbnail_path": thumbnail_path,
            "extracted_text": extracted_text,
            "content_embedding": embedding,
            "category": classification['category'],
            "classification_confidence": classification['confidence'],
            "entities": entities,
            "metadata": metadata or {},
            "version": 1,
            "created_at": datetime.utcnow().isoformat(),
            "created_by": self.client.current_user,
            "workflow_state": "draft"
        }
        
        # 9. Insert into ThemisDB
        doc_id = self.client.insert("documents", document)
        
        # 10. Create folder relationship
        if folder_id:
            self.client.query(f"""
                INSERT {{
                    _from: 'folders/{folder_id}',
                    _to: 'documents/{doc_id}',
                    type: 'contains'
                }} INTO document_edges
            """)
        
        return {"status": "success", "document_id": doc_id}
    
    def _extract_pdf_text(self, file_path):
        """Extract text from PDF using PyMuPDF"""
        doc = fitz.open(file_path)
        text = ""
        for page in doc:
            text += page.get_text()
        return text
    
    def _extract_image_text(self, file_path):
        """Extract text from image using OCR"""
        image = Image.open(file_path)
        text = pytesseract.image_to_string(image)
        return text
    
    def _generate_embedding(self, text):
        """Generate vector embedding for semantic search"""
        # In production, use a proper embedding model (e.g., sentence-transformers)
        # This is a placeholder
        return [0.0] * 512
    
    def _classify_document(self, text):
        """Classify document into categories"""
        # Use ML model for classification
        # Placeholder implementation
        if "invoice" in text.lower():
            return {"category": "invoice", "confidence": 0.95}
        return {"category": "other", "confidence": 0.5}
    
    def _extract_entities(self, text):
        """Extract entities (amounts, dates, names) from text"""
        # Use NER model or regex patterns
        # Placeholder implementation
        entities = []
        
        # Extract amounts
        import re
        amounts = re.findall(r'\$?\d+[,.]?\d*', text)
        for amount in amounts:
            entities.append({
                "type": "amount",
                "value": amount,
                "confidence": 0.9
            })
        
        return entities
    
    def _generate_thumbnail(self, file_path):
        """Generate thumbnail for preview"""
        # Implementation depends on file type
        return "/thumbnails/placeholder.jpg"
    
    def _store_file(self, file_path):
        """Store file in filesystem or object storage"""
        # In production, upload to S3/Azure/GCS
        return f"/documents/{os.path.basename(file_path)}"
```

### 2. Search Implementation

```python
class DocumentSearch:
    def __init__(self, client: ThemisClient):
        self.client = client
    
    def search(self, query, filters=None, limit=20):
        """Multi-modal search across documents"""
        
        results = []
        
        # 1. Full-text search
        text_results = self._full_text_search(query, filters, limit)
        
        # 2. Vector similarity search
        vector_results = self._vector_search(query, filters, limit)
        
        # 3. Metadata search
        metadata_results = self._metadata_search(query, filters, limit)
        
        # 4. Combine and rank results
        all_results = self._merge_results(
            text_results, vector_results, metadata_results
        )
        
        return all_results[:limit]
    
    def _full_text_search(self, query, filters, limit):
        """Search in extracted text content"""
        aql = f"""
            FOR doc IN documents
                SEARCH ANALYZER(doc.extracted_text IN TOKENS('{query}', 'text_en'), 'text_en')
                {self._build_filter_clause(filters)}
                SORT BM25(doc) DESC
                LIMIT {limit}
                RETURN {{
                    doc: doc,
                    score: BM25(doc),
                    match_type: 'fulltext'
                }}
        """
        return self.client.query(aql)
    
    def _vector_search(self, query, filters, limit):
        """Semantic similarity search using embeddings"""
        # Generate embedding for query
        query_embedding = self._generate_embedding(query)
        
        aql = f"""
            FOR doc IN documents
                LET similarity = COSINE_SIMILARITY(doc.content_embedding, {query_embedding})
                FILTER similarity > 0.7
                {self._build_filter_clause(filters)}
                SORT similarity DESC
                LIMIT {limit}
                RETURN {{
                    doc: doc,
                    score: similarity,
                    match_type: 'semantic'
                }}
        """
        return self.client.query(aql)
    
    def _metadata_search(self, query, filters, limit):
        """Search in metadata fields"""
        aql = f"""
            FOR doc IN documents
                FILTER doc.metadata.invoice_number == '{query}'
                    OR doc.metadata.vendor_id == '{query}'
                    OR doc.category == '{query}'
                {self._build_filter_clause(filters)}
                LIMIT {limit}
                RETURN {{
                    doc: doc,
                    score: 1.0,
                    match_type: 'metadata'
                }}
        """
        return self.client.query(aql)
    
    def _build_filter_clause(self, filters):
        """Build AQL filter clause from filters dict"""
        if not filters:
            return ""
        
        clauses = []
        if 'category' in filters:
            clauses.append(f"doc.category == '{filters['category']}'")
        if 'date_from' in filters:
            clauses.append(f"doc.created_at >= '{filters['date_from']}'")
        if 'date_to' in filters:
            clauses.append(f"doc.created_at <= '{filters['date_to']}'")
        
        return "FILTER " + " AND ".join(clauses) if clauses else ""
    
    def _merge_results(self, *result_sets):
        """Merge and deduplicate results from different search methods"""
        seen = set()
        merged = []
        
        for result_set in result_sets:
            for result in result_set:
                doc_id = result['doc']['id']
                if doc_id not in seen:
                    seen.add(doc_id)
                    merged.append(result)
        
        # Sort by score
        merged.sort(key=lambda x: x['score'], reverse=True)
        
        return merged
```

### 3. Workflow Engine

```python
class WorkflowEngine:
    def __init__(self, client: ThemisClient):
        self.client = client
    
    def transition_state(self, document_id, new_state, user_id, comment=""):
        """Transition document to new workflow state"""
        
        # 1. Get current document state
        doc = self.client.get("documents", document_id)
        current_state = doc.get('workflow_state')
        
        # 2. Get workflow definition
        workflow = self._get_workflow_for_document(doc)
        
        # 3. Validate transition
        if not self._is_valid_transition(workflow, current_state, new_state):
            raise ValueError(f"Invalid transition from {current_state} to {new_state}")
        
        # 4. Check permissions
        if not self._user_has_permission(workflow, current_state, new_state, user_id):
            raise PermissionError("User does not have permission for this transition")
        
        # 5. Update document
        self.client.update("documents", document_id, {
            "workflow_state": new_state,
            "modified_at": datetime.utcnow().isoformat(),
            "modified_by": user_id,
            "workflow_history": doc.get('workflow_history', []) + [{
                "state": new_state,
                "timestamp": datetime.utcnow().isoformat(),
                "user": user_id,
                "comment": comment
            }]
        })
        
        # 6. Send notifications
        self._send_notifications(workflow, current_state, new_state, document_id)
        
        # 7. Execute actions
        self._execute_actions(workflow, new_state, document_id)
        
        return {"status": "success", "new_state": new_state}
    
    def _get_workflow_for_document(self, doc):
        """Get workflow definition based on document category"""
        category = doc.get('category', 'default')
        workflow = self.client.query(f"""
            FOR wf IN workflows
                FILTER wf.document_category == '{category}'
                RETURN wf
        """)
        return workflow[0] if workflow else self._get_default_workflow()
    
    def _is_valid_transition(self, workflow, from_state, to_state):
        """Check if transition is allowed in workflow"""
        for state in workflow['states']:
            if state['id'] == from_state:
                return to_state in state.get('allowed_transitions', [])
        return False
    
    def _user_has_permission(self, workflow, from_state, to_state, user_id):
        """Check if user has permission for transition"""
        # Check workflow rules
        for rule in workflow.get('rules', []):
            if rule['from_state'] == from_state and rule['to_state'] == to_state:
                required_role = rule.get('required_role')
                if required_role and not self._user_has_role(user_id, required_role):
                    return False
        return True
    
    def _user_has_role(self, user_id, role):
        """Check if user has specific role"""
        # Query user roles from database
        user = self.client.get("users", user_id)
        return role in user.get('roles', [])
    
    def _send_notifications(self, workflow, from_state, to_state, document_id):
        """Send notifications based on workflow rules"""
        for notification in workflow.get('notifications', []):
            if (notification['from_state'] == from_state and 
                notification['to_state'] == to_state):
                self._send_notification(
                    notification['recipients'],
                    notification['template'],
                    document_id
                )
    
    def _send_notification(self, recipients, template, document_id):
        """Send email/SMS notification"""
        # Implementation depends on notification system
        pass
    
    def _execute_actions(self, workflow, state, document_id):
        """Execute automated actions for state"""
        # e.g., auto-archive, generate reports, etc.
        pass
    
    def _get_default_workflow(self):
        """Get default workflow if none specified"""
        return {
            "states": [
                {"id": "draft", "is_initial": True, "allowed_transitions": ["published"]},
                {"id": "published", "is_final": True, "allowed_transitions": []}
            ],
            "rules": [],
            "notifications": []
        }
```

## Integration with Cloud Backup

```python
from sharding.cloud_backup import CloudBackupCoordinator, CloudBackupConfig

# Configure cloud backup
cloud_backup_config = CloudBackupConfig(
    provider="s3",
    s3_bucket="themisdb-dms-backups",
    s3_region="us-east-1",
    local_backup_dir="/var/lib/themisdb/backups",
    enable_encryption=True,
    retention_period=timedelta(days=90)
)

# Initialize backup coordinator
backup_coordinator = CloudBackupCoordinator(
    cloud_agent=cloud_agent,
    backup_manager=backup_manager,
    config=cloud_backup_config
)

# Schedule automatic backups
def schedule_backups():
    """Schedule daily backups"""
    import schedule
    
    def backup_job():
        backup_id = f"dms-backup-{datetime.now().strftime('%Y%m%d-%H%M%S')}"
        shard_ids = ["shard1", "shard2", "shard3"]
        
        success = backup_coordinator.createBackup(backup_id, shard_ids)
        if success:
            print(f"Backup {backup_id} completed successfully")
        else:
            print(f"Backup {backup_id} failed")
    
    # Run backup daily at 2 AM
    schedule.every().day.at("02:00").do(backup_job)
    
    while True:
        schedule.run_pending()
        time.sleep(60)

# Start backup scheduler in background thread
import threading
backup_thread = threading.Thread(target=schedule_backups, daemon=True)
backup_thread.start()
```

## Performance Optimization

### Indexing Strategy

```python
# Create indexes for fast lookups
client.create_index("documents", ["hash_sha256"], unique=True)
client.create_index("documents", ["category", "created_at"])
client.create_index("documents", ["metadata.invoice_number"])
client.create_index("documents", ["workflow_state"])

# Vector index for semantic search
client.create_vector_index("documents", "content_embedding", {
    "type": "hnsw",
    "dimension": 512,
    "metric": "cosine"
})

# Full-text search view
client.create_analyzer("text_en", {
    "type": "text",
    "locale": "en",
    "case": "lower",
    "stopwords": ["the", "a", "an"],
    "accent": false,
    "stemming": true
})
```

### Caching Strategy

```python
from functools import lru_cache
import redis

# Redis cache for frequent queries
redis_client = redis.Redis(host='localhost', port=6379, db=0)

@lru_cache(maxsize=1000)
def get_workflow(category):
    """Cache workflow definitions"""
    cache_key = f"workflow:{category}"
    cached = redis_client.get(cache_key)
    
    if cached:
        return json.loads(cached)
    
    workflow = client.query(f"""
        FOR wf IN workflows
            FILTER wf.document_category == '{category}'
            RETURN wf
    """)[0]
    
    redis_client.setex(cache_key, 3600, json.dumps(workflow))
    return workflow
```

## Security Best Practices

1. **Access Control**: Implement row-level security
2. **Encryption**: Encrypt documents at rest and in transit
3. **Audit Logging**: Log all document access and modifications
4. **Compliance**: Implement retention policies (GDPR, HIPAA)
5. **Backup**: Regular backups with cloud replication
6. **Authentication**: Use OAuth2/SAML for SSO
7. **Authorization**: Role-based access control (RBAC)

## Testing

```python
import unittest

class TestDocumentUpload(unittest.TestCase):
    def setUp(self):
        self.client = ThemisClient("http://localhost:18765")
        self.processor = DocumentProcessor(self.client)
    
    def test_upload_pdf(self):
        result = self.processor.upload_document(
            "test_invoice.pdf",
            folder_id="test_folder",
            metadata={"invoice_number": "TEST-001"}
        )
        self.assertEqual(result['status'], 'success')
    
    def test_duplicate_detection(self):
        # Upload same file twice
        result1 = self.processor.upload_document("test.pdf", "folder1")
        result2 = self.processor.upload_document("test.pdf", "folder2")
        
        self.assertEqual(result2['status'], 'duplicate')
        self.assertEqual(result2['existing_id'], result1['document_id'])
```

## Monitoring

```yaml
# Prometheus metrics
themis_documents_total: Total number of documents
themis_documents_upload_duration_seconds: Document upload latency
themis_documents_search_duration_seconds: Search query latency
themis_documents_storage_bytes: Total storage used
themis_documents_by_category: Documents by category
themis_workflow_transitions_total: Workflow state transitions
```

## Conclusion

This enterprise integration guide demonstrates how to build a production-ready DMS/ERP system using ThemisDB's multi-model capabilities. The implementation includes:

- Document management with versioning
- Full-text and vector search
- Workflow engine with approval processes
- Enterprise features (OCR, classification, entity extraction)
- Cloud backup integration
- Security and compliance
- Performance optimization
- Monitoring and observability

For questions or support, contact the ThemisDB team or visit our documentation at https://themisdb.io/docs

---

**Version**: 1.0  
**Last Updated**: February 7, 2026  
**Maintainer**: ThemisDB Team
