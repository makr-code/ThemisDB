# ThemisDB REST API - Beispiele mit Authentifizierung

**Version:** 1.0.0  
**Stand:** 6. April 2026  
**Kategorie:** Clients  
**Status:** ✅ Produktionsreif

---

## 📑 Inhaltsverzeichnis

- [API Übersicht](#api-übersicht)
- [Authentifizierung](#authentifizierung)
- [CRUD Operations](#crud-operations)
- [Query Execution](#query-execution)
- [Batch Operations](#batch-operations)
- [Performance Tipps](#performance-tipps)
- [Client-Beispiele](#client-beispiele)

---

## API Übersicht

### Base URL

```
http://localhost:8765/api/v1
```

### Content-Type

Alle Requests und Responses verwenden `application/json`.

### Standard Headers

```http
Content-Type: application/json
Accept: application/json
```

---

## Authentifizierung

### Basic Authentication

```bash
# cURL
curl -u admin:secret \
  http://localhost:8765/api/v1/collections

# Header
Authorization: Basic YWRtaW46c2VjcmV0
```

```javascript
// JavaScript (fetch)
const credentials = btoa('admin:secret');

fetch('http://localhost:8765/api/v1/collections', {
  headers: {
    'Authorization': `Basic ${credentials}`
  }
});
```

```python
# Python (requests)
import requests

response = requests.get(
    'http://localhost:8765/api/v1/collections',
    auth=('admin', 'secret')
)
```

### Token-based Authentication

#### 1. Login

```bash
# Login Request
curl -X POST http://localhost:8765/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "username": "admin",
    "password": "secret"
  }'
```

**Response:**
```json
{
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "refresh_token": "def50200abcdef...",
  "expires_in": 3600,
  "token_type": "Bearer"
}
```

#### 2. Use Token

```bash
# With Bearer Token
curl http://localhost:8765/api/v1/collections \
  -H "Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
```

```javascript
// JavaScript
const token = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...';

fetch('http://localhost:8765/api/v1/collections', {
  headers: {
    'Authorization': `Bearer ${token}`
  }
});
```

#### 3. Refresh Token

```bash
curl -X POST http://localhost:8765/api/v1/auth/refresh \
  -H "Content-Type: application/json" \
  -d '{
    "refresh_token": "def50200abcdef..."
  }'
```

**Response:**
```json
{
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "expires_in": 3600
}
```

### API Key Authentication

```bash
# With API Key
curl http://localhost:8765/api/v1/collections \
  -H "X-API-Key: your-api-key-here"
```

---

## CRUD Operations

### Create (Insert)

```bash
# Single Document
curl -X POST http://localhost:8765/api/v1/collections/users/documents \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "_key": "user-123",
    "name": "Alice",
    "age": 30,
    "email": "alice@example.com"
  }'
```

**Response:**
```json
{
  "success": true,
  "key": "user-123",
  "id": "users/user-123",
  "rev": "_gVPKuLa---"
}
```

```javascript
// JavaScript
async function createUser(user) {
  const response = await fetch(
    'http://localhost:8765/api/v1/collections/users/documents',
    {
      method: 'POST',
      headers: {
        'Authorization': `Bearer ${token}`,
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(user)
    }
  );
  
  return await response.json();
}

const user = {
  _key: 'user-123',
  name: 'Alice',
  age: 30,
  email: 'alice@example.com'
};

const result = await createUser(user);
```

```python
# Python
import requests

def create_user(user, token):
    response = requests.post(
        'http://localhost:8765/api/v1/collections/users/documents',
        headers={
            'Authorization': f'Bearer {token}',
            'Content-Type': 'application/json'
        },
        json=user
    )
    return response.json()

user = {
    '_key': 'user-123',
    'name': 'Alice',
    'age': 30,
    'email': 'alice@example.com'
}

result = create_user(user, token)
```

### Read (Get)

```bash
# Get Single Document
curl http://localhost:8765/api/v1/collections/users/documents/user-123 \
  -H "Authorization: Bearer $TOKEN"
```

**Response:**
```json
{
  "_key": "user-123",
  "_id": "users/user-123",
  "_rev": "_gVPKuLa---",
  "name": "Alice",
  "age": 30,
  "email": "alice@example.com"
}
```

```javascript
// JavaScript
async function getUser(key) {
  const response = await fetch(
    `http://localhost:8765/api/v1/collections/users/documents/${key}`,
    {
      headers: {
        'Authorization': `Bearer ${token}`
      }
    }
  );
  
  if (response.status === 404) {
    return null;
  }
  
  return await response.json();
}

const user = await getUser('user-123');
```

### Update

```bash
# Partial Update (PATCH)
curl -X PATCH http://localhost:8765/api/v1/collections/users/documents/user-123 \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "age": 31,
    "last_login": "2026-01-24T14:30:00Z"
  }'

# Full Replace (PUT)
curl -X PUT http://localhost:8765/api/v1/collections/users/documents/user-123 \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "name": "Alice Smith",
    "age": 31,
    "email": "alice.smith@example.com"
  }'
```

```javascript
// JavaScript - Partial Update
async function updateUser(key, updates) {
  const response = await fetch(
    `http://localhost:8765/api/v1/collections/users/documents/${key}`,
    {
      method: 'PATCH',
      headers: {
        'Authorization': `Bearer ${token}`,
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(updates)
    }
  );
  
  return await response.json();
}

await updateUser('user-123', {
  age: 31,
  last_login: new Date().toISOString()
});
```

### Delete

```bash
# Delete Document
curl -X DELETE http://localhost:8765/api/v1/collections/users/documents/user-123 \
  -H "Authorization: Bearer $TOKEN"
```

**Response:**
```json
{
  "success": true,
  "deleted": true
}
```

```javascript
// JavaScript
async function deleteUser(key) {
  const response = await fetch(
    `http://localhost:8765/api/v1/collections/users/documents/${key}`,
    {
      method: 'DELETE',
      headers: {
        'Authorization': `Bearer ${token}`
      }
    }
  );
  
  return response.ok;
}

await deleteUser('user-123');
```

---

## Query Execution

### Simple Query

```bash
curl -X POST http://localhost:8765/api/v1/query \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "query": "FOR doc IN users FILTER doc.age > 25 RETURN doc",
    "limit": 10
  }'
```

**Response:**
```json
{
  "entities": [
    {"_key": "user-123", "name": "Alice", "age": 30},
    {"_key": "user-125", "name": "Charlie", "age": 35}
  ],
  "count": 2,
  "has_more": false
}
```

### Parameterized Query

```bash
curl -X POST http://localhost:8765/api/v1/query \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "query": "FOR doc IN users FILTER doc.age > @min_age LIMIT @limit RETURN doc",
    "bind_vars": {
      "min_age": 25,
      "limit": 10
    }
  }'
```

```javascript
// JavaScript
async function queryUsers(minAge, limit) {
  const response = await fetch(
    'http://localhost:8765/api/v1/query',
    {
      method: 'POST',
      headers: {
        'Authorization': `Bearer ${token}`,
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        query: 'FOR doc IN users FILTER doc.age > @min_age LIMIT @limit RETURN doc',
        bind_vars: {
          min_age: minAge,
          limit: limit
        }
      })
    }
  );
  
  return await response.json();
}

const result = await queryUsers(25, 10);
console.log(result.entities);
```

```python
# Python
def query_users(min_age, limit, token):
    response = requests.post(
        'http://localhost:8765/api/v1/query',
        headers={
            'Authorization': f'Bearer {token}',
            'Content-Type': 'application/json'
        },
        json={
            'query': 'FOR doc IN users FILTER doc.age > @min_age LIMIT @limit RETURN doc',
            'bind_vars': {
                'min_age': min_age,
                'limit': limit
            }
        }
    )
    return response.json()

result = query_users(25, 10, token)
```

### Cursor-based Pagination

```bash
# First Page
curl -X POST http://localhost:8765/api/v1/query \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "query": "FOR doc IN large_collection RETURN doc",
    "batch_size": 100,
    "use_cursor": true
  }'
```

**Response:**
```json
{
  "entities": [...],
  "count": 100,
  "has_more": true,
  "cursor": "12345-abcdef"
}
```

```bash
# Next Page
curl -X POST http://localhost:8765/api/v1/query/cursor/12345-abcdef \
  -H "Authorization: Bearer $TOKEN"
```

---

## Batch Operations

### Batch Insert

```bash
curl -X POST http://localhost:8765/api/v1/collections/users/documents/batch \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "documents": [
      {"_key": "user-124", "name": "Bob", "age": 25},
      {"_key": "user-125", "name": "Charlie", "age": 35}
    ]
  }'
```

**Response:**
```json
{
  "success_count": 2,
  "error_count": 0,
  "results": [
    {"key": "user-124", "success": true},
    {"key": "user-125", "success": true}
  ]
}
```

### Batch Get

```bash
curl -X POST http://localhost:8765/api/v1/collections/users/documents/batch/get \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "keys": ["user-123", "user-124", "user-125"]
  }'
```

**Response:**
```json
{
  "found": [
    {"_key": "user-123", "name": "Alice", "age": 30},
    {"_key": "user-124", "name": "Bob", "age": 25}
  ],
  "missing": ["user-125"]
}
```

### Batch Delete

```bash
curl -X DELETE http://localhost:8765/api/v1/collections/users/documents/batch \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "keys": ["user-124", "user-125"]
  }'
```

---

## Performance Tipps

### Connection Reuse

```javascript
// ✅ Good: Reuse connection with keep-alive
const agent = new https.Agent({
  keepAlive: true,
  maxSockets: 50
});

const client = axios.create({
  baseURL: 'http://localhost:8765/api/v1',
  httpsAgent: agent,
  headers: {
    'Authorization': `Bearer ${token}`
  }
});

// Use client for all requests
await client.get('/collections/users/documents/user-123');
```

### Batch Operations

```javascript
// ❌ Bad: Multiple single requests
for (const key of keys) {
  await fetch(`http://localhost:8765/api/v1/collections/users/documents/${key}`);
}

// ✅ Good: Single batch request
await fetch('http://localhost:8765/api/v1/collections/users/documents/batch/get', {
  method: 'POST',
  body: JSON.stringify({ keys })
});
```

### Compression

```bash
# Request compression
curl -X POST http://localhost:8765/api/v1/query \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -H "Content-Encoding: gzip" \
  --data-binary @query.json.gz

# Response compression
curl http://localhost:8765/api/v1/query \
  -H "Authorization: Bearer $TOKEN" \
  -H "Accept-Encoding: gzip, deflate"
```

---

## Client-Beispiele

### Node.js (axios)

```javascript
const axios = require('axios');

class ThemisClient {
  constructor(baseURL, token) {
    this.client = axios.create({
      baseURL,
      headers: {
        'Authorization': `Bearer ${token}`,
        'Content-Type': 'application/json'
      }
    });
    
    // Add request retry
    this.client.interceptors.response.use(
      response => response,
      async error => {
        if (error.response?.status === 401) {
          // Token expired, refresh and retry
          await this.refreshToken();
          return this.client.request(error.config);
        }
        throw error;
      }
    );
  }
  
  async get(collection, key) {
    const response = await this.client.get(
      `/collections/${collection}/documents/${key}`
    );
    return response.data;
  }
  
  async insert(collection, document) {
    const response = await this.client.post(
      `/collections/${collection}/documents`,
      document
    );
    return response.data;
  }
  
  async query(aql, bindVars = {}) {
    const response = await this.client.post('/query', {
      query: aql,
      bind_vars: bindVars
    });
    return response.data;
  }
  
  async refreshToken() {
    // Implement token refresh logic
  }
}

// Usage
const client = new ThemisClient('http://localhost:8765/api/v1', token);

const user = await client.get('users', 'user-123');
await client.insert('users', { _key: 'user-124', name: 'Bob' });

const result = await client.query(
  'FOR doc IN users FILTER doc.age > @min_age RETURN doc',
  { min_age: 25 }
);
```

### Python (requests)

```python
import requests
from typing import Dict, List, Optional

class ThemisClient:
    def __init__(self, base_url: str, token: str):
        self.base_url = base_url
        self.session = requests.Session()
        self.session.headers.update({
            'Authorization': f'Bearer {token}',
            'Content-Type': 'application/json'
        })
    
    def get(self, collection: str, key: str) -> Optional[Dict]:
        url = f'{self.base_url}/collections/{collection}/documents/{key}'
        response = self.session.get(url)
        
        if response.status_code == 404:
            return None
        
        response.raise_for_status()
        return response.json()
    
    def insert(self, collection: str, document: Dict) -> Dict:
        url = f'{self.base_url}/collections/{collection}/documents'
        response = self.session.post(url, json=document)
        response.raise_for_status()
        return response.json()
    
    def query(self, aql: str, bind_vars: Dict = None) -> Dict:
        url = f'{self.base_url}/query'
        payload = {'query': aql}
        if bind_vars:
            payload['bind_vars'] = bind_vars
        
        response = self.session.post(url, json=payload)
        response.raise_for_status()
        return response.json()
    
    def batch_get(self, collection: str, keys: List[str]) -> Dict:
        url = f'{self.base_url}/collections/{collection}/documents/batch/get'
        response = self.session.post(url, json={'keys': keys})
        response.raise_for_status()
        return response.json()

# Usage
client = ThemisClient('http://localhost:8765/api/v1', token)

user = client.get('users', 'user-123')
client.insert('users', {'_key': 'user-124', 'name': 'Bob'})

result = client.query(
    'FOR doc IN users FILTER doc.age > @min_age RETURN doc',
    {'min_age': 25}
)
```

### Go

```go
package main

import (
    "bytes"
    "encoding/json"
    "fmt"
    "net/http"
)

type ThemisClient struct {
    BaseURL    string
    Token      string
    HTTPClient *http.Client
}

func NewThemisClient(baseURL, token string) *ThemisClient {
    return &ThemisClient{
        BaseURL:    baseURL,
        Token:      token,
        HTTPClient: &http.Client{},
    }
}

func (c *ThemisClient) Get(collection, key string) (map[string]interface{}, error) {
    url := fmt.Sprintf("%s/collections/%s/documents/%s", c.BaseURL, collection, key)
    
    req, err := http.NewRequest("GET", url, nil)
    if err != nil {
        return nil, err
    }
    
    req.Header.Set("Authorization", "Bearer "+c.Token)
    
    resp, err := c.HTTPClient.Do(req)
    if err != nil {
        return nil, err
    }
    defer resp.Body.Close()
    
    var result map[string]interface{}
    err = json.NewDecoder(resp.Body).Decode(&result)
    return result, err
}

func (c *ThemisClient) Query(aql string, bindVars map[string]interface{}) (map[string]interface{}, error) {
    url := c.BaseURL + "/query"
    
    payload := map[string]interface{}{
        "query": aql,
    }
    if bindVars != nil {
        payload["bind_vars"] = bindVars
    }
    
    body, err := json.Marshal(payload)
    if err != nil {
        return nil, err
    }
    
    req, err := http.NewRequest("POST", url, bytes.NewBuffer(body))
    if err != nil {
        return nil, err
    }
    
    req.Header.Set("Authorization", "Bearer "+c.Token)
    req.Header.Set("Content-Type", "application/json")
    
    resp, err := c.HTTPClient.Do(req)
    if err != nil {
        return nil, err
    }
    defer resp.Body.Close()
    
    var result map[string]interface{}
    err = json.NewDecoder(resp.Body).Decode(&result)
    return result, err
}

func main() {
    client := NewThemisClient("http://localhost:8765/api/v1", token)
    
    user, _ := client.Get("users", "user-123")
    fmt.Println(user)
    
    result, _ := client.Query(
        "FOR doc IN users FILTER doc.age > @min_age RETURN doc",
        map[string]interface{}{"min_age": 25},
    )
    fmt.Println(result)
}
```

---

## Siehe auch

- [C++ Client SDK](clients_cpp_sdk.md)
- [C# Client SDK](clients_csharp_sdk.md)
- [Python SDK](clients_python_sdk.md)
- [HTTP API Reference](../apis/HTTP_API_REFERENCE.md)
- [Authentication Guide](../security/AUTHENTICATION_GUIDE.md)
