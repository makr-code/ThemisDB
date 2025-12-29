# Kapitel 21: Client Libraries & Drivers

## 21.1 Einführung in ThemisDB Client-Ökosystem

ThemisDB bietet eine umfassende Palette an Client-Libraries und Treibern für verschiedene Programmiersprachen und Plattformen. Diese ermöglichen es Entwicklern, nahtlos mit ThemisDB zu interagieren, unabhängig vom gewählten Tech-Stack.

### 21.1.1 Architektur der Client-Libraries

Die ThemisDB Client-Architecture basiert auf einem mehrschichtigen Ansatz:

**1. Protokoll-Layer:**
- **Binary Protocol:** Hochperformantes binäres Protokoll für maximale Effizienz
- **HTTP/REST API:** RESTful Interface für einfache Integration
- **WebSocket:** Bidirektionale Kommunikation für Realtime-Features

**2. Connection Management:**
- **Connection Pooling:** Effiziente Verwaltung von Datenbankverbindungen
- **Automatic Reconnection:** Automatisches Wiederherstellen nach Verbindungsabbruch
- **Load Balancing:** Verteilung von Anfragen über mehrere Server

**3. Query Interface:**
- **AQL Builder:** Fluent API zum Erstellen von Queries
- **ORM Support:** Object-Relational Mapping für typsichere Datenmodelle
- **Raw Query:** Direkter Zugriff auf AQL für maximale Flexibilität

### 21.1.2 Unterstützte Sprachen

ThemisDB bietet offizielle Clients für:

- **Python** (themisdb-python) - Vollständiger Feature-Support
- **JavaScript/TypeScript** (themisdb-js) - Node.js und Browser
- **Java** (themisdb-java) - Enterprise-ready JDBC-Treiber
- **Go** (themisdb-go) - High-performance native Client
- **C#/.NET** (ThemisDB.NET) - .NET Standard 2.0+
- **Rust** (themisdb-rs) - Memory-safe native Client
- **Ruby** (themisdb-ruby) - Rails-freundlich
- **PHP** (themisdb-php) - Laravel & Symfony Support

## 21.2 Python Client (themisdb-python)

Der Python Client ist die am weitesten entwickelte und feature-reichste Client-Library für ThemisDB.

### 21.2.1 Installation & Setup

```bash
# Installation via pip
pip install themisdb

# Mit optionalen Dependencies
pip install themisdb[async,pandas]

# Development Version
pip install git+https://github.com/themisdb/themisdb-python.git
```

**Basis-Verbindung:**

```python
from themisdb import ThemisDB

# Einfache Verbindung
db = ThemisDB("localhost", 7687)

# Mit Authentifizierung
db = ThemisDB(
    host="localhost",
    port=7687,
    username="admin",
    password="secure_password",
    database="mydb"
)

# Connection String
db = ThemisDB.from_uri("themis://admin:password@localhost:7687/mydb")
```

### 21.2.2 CRUD-Operationen

```python
# CREATE - Dokument einfügen
user = db.insert("users", {
    "name": "Alice",
    "email": "alice@example.com",
    "age": 30
})
print(f"Created user with ID: {user['id']}")

# READ - Einzelnes Dokument
user = db.find_one("users", {"email": "alice@example.com"})

# UPDATE - Dokument aktualisieren
db.update("users", 
    {"email": "alice@example.com"},
    {"$set": {"age": 31}}
)

# DELETE - Dokument löschen
db.delete("users", {"email": "alice@example.com"})
```

### 21.2.3 Query Builder API

```python
from themisdb import Query

# Fluent Query Building
users = (Query(db, "users")
    .filter(age__gte=18)
    .filter(active=True)
    .order_by("-created_at")
    .limit(10)
    .all())

# Komplexe Queries
active_admins = (Query(db, "users")
    .filter(role="admin", status="active")
    .select("id", "name", "email")
    .join("profiles", on="user_id")
    .all())

# Aggregationen
stats = (Query(db, "orders")
    .filter(status="completed")
    .group_by("customer_id")
    .aggregate({
        "total_amount": "SUM(amount)",
        "order_count": "COUNT(*)",
        "avg_amount": "AVG(amount)"
    }))
```

### 21.2.4 ORM (Object-Relational Mapping)

```python
from themisdb.orm import Model, Field

class User(Model):
    __collection__ = "users"
    
    name = Field(str, required=True)
    email = Field(str, unique=True)
    age = Field(int, min=0, max=150)
    created_at = Field(datetime, auto_now_add=True)
    updated_at = Field(datetime, auto_now=True)
    
    def __repr__(self):
        return f"User({self.name}, {self.email})"

# ORM Operationen
# Create
user = User(name="Bob", email="bob@example.com", age=25)
user.save()

# Read
users = User.objects.filter(age__gte=18).all()
bob = User.objects.get(email="bob@example.com")

# Update
bob.age = 26
bob.save()

# Delete
bob.delete()

# Beziehungen
class Post(Model):
    __collection__ = "posts"
    
    title = Field(str)
    content = Field(str)
    author = Field(User, foreign_key=True)  # Foreign Key zu User
    tags = Field(list)

# Zugriff auf Related Objects
post = Post.objects.first()
print(f"Author: {post.author.name}")  # Automatisches Laden
```

### 21.2.5 Async/Await Support

```python
import asyncio
from themisdb import AsyncThemisDB

async def main():
    # Async Connection
    db = AsyncThemisDB("localhost", 7687)
    
    # Async Queries
    users = await db.find("users", {"active": True})
    
    # Concurrent Operations
    results = await asyncio.gather(
        db.find("users", {"role": "admin"}),
        db.find("orders", {"status": "pending"}),
        db.find("products", {"in_stock": True})
    )
    
    await db.close()

asyncio.run(main())
```

### 21.2.6 Transaktionen

```python
# Context Manager für Transaktionen
with db.transaction() as tx:
    # Geld von Account A nach B überweisen
    account_a = tx.find_one("accounts", {"id": "A"})
    account_b = tx.find_one("accounts", {"id": "B"})
    
    if account_a["balance"] >= 100:
        tx.update("accounts", {"id": "A"}, 
                  {"$inc": {"balance": -100}})
        tx.update("accounts", {"id": "B"}, 
                  {"$inc": {"balance": 100}})
        tx.commit()
    else:
        tx.rollback()

# Async Transactions
async with db.transaction() as tx:
    await tx.insert("logs", {"action": "transfer", "amount": 100})
    await tx.commit()
```

### 21.2.7 Pandas Integration

```python
import pandas as pd

# Query zu DataFrame
df = db.to_dataframe("SELECT * FROM users WHERE age > 18")

# DataFrame zu ThemisDB
df.to_themis(db, collection="users", if_exists="append")

# Aggregationen mit Pandas
user_stats = (
    db.to_dataframe("users")
    .groupby("country")
    .agg({
        "id": "count",
        "age": ["mean", "median"],
        "income": "sum"
    })
)
```

## 21.3 JavaScript/TypeScript Client

### 21.3.1 Installation

```bash
# NPM
npm install themisdb

# Yarn
yarn add themisdb

# TypeScript Definitionen (bereits enthalten)
```

### 21.3.2 Node.js Usage

```javascript
const { ThemisDB } = require('themisdb');

// Connection
const db = new ThemisDB({
    host: 'localhost',
    port: 7687,
    username: 'admin',
    password: 'password',
    database: 'mydb'
});

// Async/Await
async function getUsers() {
    const users = await db.collection('users')
        .find({ active: true })
        .sort({ name: 1 })
        .limit(10)
        .toArray();
    
    return users;
}

// Promises
db.collection('users')
    .findOne({ email: 'user@example.com' })
    .then(user => console.log(user))
    .catch(err => console.error(err));
```

### 21.3.3 TypeScript Support

```typescript
interface User {
    id: string;
    name: string;
    email: string;
    age: number;
    active: boolean;
}

const db = new ThemisDB<{
    users: User;
    posts: Post;
}>({
    host: 'localhost',
    port: 7687
});

// Type-safe Queries
const users: User[] = await db.collection('users')
    .find<User>({ age: { $gte: 18 } })
    .toArray();

// Type-safe Inserts
await db.collection('users').insertOne({
    name: "Alice",
    email: "alice@example.com",
    age: 30,
    active: true
});
```

### 21.3.4 Browser Usage

```html
<!DOCTYPE html>
<html>
<head>
    <script src="https://cdn.themisdb.io/themisdb-browser.min.js"></script>
</head>
<body>
    <script>
        const db = new ThemisDB({
            url: 'https://api.example.com/themis',
            apiKey: 'your-api-key'
        });
        
        // WebSocket für Realtime
        db.collection('messages')
            .watch()
            .on('insert', (doc) => {
                console.log('New message:', doc);
            });
    </script>
</body>
</html>
```

### 21.3.4 React Integration

```jsx
import React, { useState, useEffect } from 'react';
import { useThemisDB } from 'themisdb/react';

function UserList() {
    const { data, loading, error } = useThemisDB(
        'users',
        { active: true },
        { sort: { name: 1 } }
    );
    
    if (loading) return <div>Loading...</div>;
    if (error) return <div>Error: {error.message}</div>;
    
    return (
        <ul>
            {data.map(user => (
                <li key={user.id}>{user.name}</li>
            ))}
        </ul>
    );
}
```

## 21.4 Java Client & JDBC Driver

### 21.4.1 Maven Dependency

```xml
<dependency>
    <groupId>io.themisdb</groupId>
    <artifactId>themisdb-java</artifactId>
    <version>1.3.4</version>
</dependency>

<!-- JDBC Driver -->
<dependency>
    <groupId>io.themisdb</groupId>
    <artifactId>themisdb-jdbc</artifactId>
    <version>1.3.4</version>
</dependency>
```

### 21.4.2 Native Client API

```java
import io.themisdb.ThemisDB;
import io.themisdb.Document;

public class Example {
    public static void main(String[] args) {
        // Connection
        ThemisDB db = ThemisDB.connect()
            .host("localhost")
            .port(7687)
            .username("admin")
            .password("password")
            .database("mydb")
            .build();
        
        // Insert
        Document user = new Document()
            .append("name", "Alice")
            .append("email", "alice@example.com")
            .append("age", 30);
        
        db.collection("users").insertOne(user);
        
        // Query
        List<Document> users = db.collection("users")
            .find(Filters.eq("active", true))
            .sort(Sorts.ascending("name"))
            .limit(10)
            .into(new ArrayList<>());
        
        db.close();
    }
}
```

### 21.4.3 JDBC Integration

```java
import java.sql.*;

public class JdbcExample {
    public static void main(String[] args) throws SQLException {
        // JDBC Connection
        String url = "jdbc:themis://localhost:7687/mydb";
        Properties props = new Properties();
        props.setProperty("user", "admin");
        props.setProperty("password", "password");
        
        Connection conn = DriverManager.getConnection(url, props);
        
        // Prepared Statement
        String sql = "SELECT * FROM users WHERE age > ? ORDER BY name";
        PreparedStatement stmt = conn.prepareStatement(sql);
        stmt.setInt(1, 18);
        
        ResultSet rs = stmt.executeQuery();
        while (rs.next()) {
            System.out.println(
                rs.getString("name") + " - " + 
                rs.getInt("age")
            );
        }
        
        rs.close();
        stmt.close();
        conn.close();
    }
}
```

### 21.4.4 Spring Data Integration

```java
import org.springframework.data.annotation.Id;
import org.springframework.data.themis.repository.ThemisRepository;

// Entity
@Document(collection = "users")
public class User {
    @Id
    private String id;
    private String name;
    private String email;
    private Integer age;
    
    // Getters and Setters
}

// Repository
public interface UserRepository extends ThemisRepository<User, String> {
    List<User> findByAge(Integer age);
    List<User> findByNameContaining(String name);
    Optional<User> findByEmail(String email);
    
    @Query("{ 'age': { '$gte': ?0 } }")
    List<User> findAdults(int minAge);
}

// Service
@Service
public class UserService {
    @Autowired
    private UserRepository userRepository;
    
    public List<User> getActiveUsers() {
        return userRepository.findByAge(18);
    }
}
```

## 21.5 Go Client

### 21.5.1 Installation

```bash
go get github.com/themisdb/themisdb-go
```

### 21.5.2 Basic Usage

```go
package main

import (
    "context"
    "fmt"
    "github.com/themisdb/themisdb-go"
)

func main() {
    // Connect
    client, err := themisdb.Connect(
        context.Background(),
        "localhost:7687",
        themisdb.WithAuth("admin", "password"),
        themisdb.WithDatabase("mydb"),
    )
    if err != nil {
        panic(err)
    }
    defer client.Close()
    
    // Insert
    user := map[string]interface{}{
        "name":  "Alice",
        "email": "alice@example.com",
        "age":   30,
    }
    
    result, err := client.Collection("users").InsertOne(
        context.Background(),
        user,
    )
    
    // Query
    cursor, err := client.Collection("users").Find(
        context.Background(),
        themisdb.M{"active": true},
    )
    
    var users []map[string]interface{}
    if err := cursor.All(context.Background(), &users); err != nil {
        panic(err)
    }
}
```

### 21.5.3 Struct Mapping

```go
type User struct {
    ID        string    `themis:"_id,omitempty"`
    Name      string    `themis:"name"`
    Email     string    `themis:"email"`
    Age       int       `themis:"age"`
    Active    bool      `themis:"active"`
    CreatedAt time.Time `themis:"created_at"`
}

// Insert with Struct
user := User{
    Name:   "Bob",
    Email:  "bob@example.com",
    Age:    25,
    Active: true,
}

_, err := client.Collection("users").InsertOne(ctx, user)

// Find with Struct
var users []User
cursor, _ := client.Collection("users").Find(ctx, themisdb.M{"age": themisdb.M{"$gte": 18}})
cursor.All(ctx, &users)
```

## 21.6 C#/.NET Client

### 21.6.1 NuGet Installation

```bash
dotnet add package ThemisDB.Driver
```

### 21.6.2 Basic Operations

```csharp
using ThemisDB;

// Connection
var client = new ThemisClient("localhost:7687");
var db = client.GetDatabase("mydb");

// Insert
var user = new BsonDocument
{
    { "name", "Alice" },
    { "email", "alice@example.com" },
    { "age", 30 }
};

db.GetCollection("users").InsertOne(user);

// Query
var filter = Builders<BsonDocument>.Filter.Eq("active", true);
var users = db.GetCollection("users")
    .Find(filter)
    .SortBy(u => u["name"])
    .Limit(10)
    .ToList();
```

### 21.6.3 POCO Mapping

```csharp
public class User
{
    public ObjectId Id { get; set; }
    public string Name { get; set; }
    public string Email { get; set; }
    public int Age { get; set; }
    public bool Active { get; set; }
}

// Typed Collection
var collection = db.GetCollection<User>("users");

// Insert
var user = new User
{
    Name = "Alice",
    Email = "alice@example.com",
    Age = 30,
    Active = true
};

collection.InsertOne(user);

// Query
var adults = collection
    .Find(u => u.Age >= 18)
    .SortBy(u => u.Name)
    .ToList();
```

## 21.7 Connection Pooling & Performance

### 21.7.1 Connection Pool Konfiguration

**Python:**
```python
db = ThemisDB(
    "localhost", 7687,
    pool_size=20,              # Max connections
    pool_timeout=30,           # Timeout in Sekunden
    pool_max_idle_time=300,    # Max idle time
    pool_recycle=3600          # Connection recycling
)
```

**JavaScript:**
```javascript
const db = new ThemisDB({
    host: 'localhost',
    port: 7687,
    poolSize: 20,
    poolTimeout: 30000,
    idleTimeout: 300000
});
```

**Java:**
```java
ThemisDB db = ThemisDB.connect()
    .host("localhost")
    .port(7687)
    .poolSize(20)
    .poolTimeout(30000)
    .build();
```

### 21.7.2 Performance Best Practices

**1. Batch Operations:**
```python
# Ineffizient - Einzelne Inserts
for user in users:
    db.insert("users", user)

# Effizient - Batch Insert
db.insert_many("users", users)
```

**2. Index Hints:**
```python
# Mit Index Hint
users = db.find("users", 
    {"age": {"$gte": 18}},
    hint="age_idx"
)
```

**3. Projection (Nur benötigte Felder):**
```python
# Nur Name und Email laden
users = db.find("users", 
    {"active": True},
    projection={"name": 1, "email": 1}
)
```

**4. Cursor Batching:**
```python
# Große Resultsets in Batches
cursor = db.find("users", {}, batch_size=1000)
for batch in cursor.batches():
    process_batch(batch)
```

## 21.8 Error Handling & Retry Logic

### 21.8.1 Exception Hierarchie

```python
from themisdb.exceptions import (
    ThemisDBError,           # Base Exception
    ConnectionError,          # Verbindungsfehler
    QueryError,              # Query-Fehler
    TimeoutError,            # Timeout
    DuplicateKeyError,       # Unique Constraint
    ValidationError          # Schema Validation
)

try:
    db.insert("users", user)
except DuplicateKeyError:
    print("User already exists")
except ValidationError as e:
    print(f"Invalid data: {e.details}")
except ConnectionError:
    print("Cannot connect to database")
```

### 21.8.2 Automatic Retry

```python
from themisdb.retry import RetryPolicy

db = ThemisDB(
    "localhost", 7687,
    retry_policy=RetryPolicy(
        max_attempts=3,
        backoff_multiplier=2,
        initial_delay=1.0,
        max_delay=30.0,
        retry_on=[ConnectionError, TimeoutError]
    )
)
```

## 21.9 Monitoring & Logging

### 21.9.1 Query Logging

```python
import logging

# Enable Query Logging
logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger('themisdb')

db = ThemisDB("localhost", 7687, log_queries=True)

# Custom Logger
db.set_logger(logger, log_slow_queries=True, slow_threshold=1.0)
```

### 21.9.2 Performance Metrics

```python
# Performance Monitoring
with db.profiler() as prof:
    users = db.find("users", {"age": {"$gte": 18}})
    
print(f"Query time: {prof.elapsed}ms")
print(f"Documents returned: {prof.docs_returned}")
print(f"Documents scanned: {prof.docs_scanned}")
```

## 21.10 Security & Authentication

### 21.10.1 TLS/SSL Verbindungen

```python
# Python
db = ThemisDB(
    "secure.example.com", 7687,
    ssl=True,
    ssl_ca_cert="/path/to/ca.pem",
    ssl_certfile="/path/to/client-cert.pem",
    ssl_keyfile="/path/to/client-key.pem"
)

# JavaScript
const db = new ThemisDB({
    host: 'secure.example.com',
    port: 7687,
    ssl: true,
    sslCA: fs.readFileSync('ca.pem'),
    sslCert: fs.readFileSync('client-cert.pem'),
    sslKey: fs.readFileSync('client-key.pem')
});
```

### 21.10.2 Token-basierte Authentifizierung

```python
# JWT Token
db = ThemisDB(
    "api.example.com", 7687,
    auth_token="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
    token_refresh_callback=get_new_token
)

def get_new_token():
    # Token renewal logic
    return request_new_token()
```

## 21.11 Migration zwischen Clients

### 21.11.1 Von MongoDB zu ThemisDB

```python
# MongoDB Code
from pymongo import MongoClient
mongo_client = MongoClient('mongodb://localhost:27017/')
users = mongo_client.mydb.users.find({'age': {'$gte': 18}})

# ThemisDB äquivalent
from themisdb import ThemisDB
db = ThemisDB('localhost', 7687)
users = db.find('users', {'age': {'$gte': 18}})
```

Die API ist weitgehend kompatibel, was Migration erleichtert.

## 21.12 Zusammenfassung

ThemisDB bietet eine umfassende Client-Library-Unterstützung mit:

- **Multi-Language Support:** Offizielle Clients für 8+ Sprachen
- **Consistent API:** Ähnliche Konzepte über alle Clients
- **High Performance:** Connection Pooling, Batch Operations
- **Type Safety:** ORM und Typed Clients
- **Production Ready:** Error Handling, Retry Logic, Monitoring
- **Framework Integration:** Spring Data, Django ORM, etc.

**Best Practices:**
1. Verwenden Sie Connection Pooling in Production
2. Nutzen Sie Batch Operations für große Datenmengen
3. Implementieren Sie Retry Logic für transiente Fehler
4. Aktivieren Sie Query Logging während Development
5. Verwenden Sie Type-Safe Clients (TypeScript, Java mit Generics)

Im nächsten Kapitel betrachten wir die Integration von ThemisDB in populäre Frameworks wie Django, Spring Boot, Express.js und mehr.
