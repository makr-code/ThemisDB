# Chapter 11: Realtime Data Streaming mit Change Data Capture

## 11.1 Einführung: Die Streaming-Architektur

Change Data Capture (CDC) ist eine kritische Komponente moderner Datenarchitekturen, die Echtzeit-Datenströme ermöglicht. ThemisDB implementiert CDC als natives Feature, das automatisch alle Mutationen (CREATE, UPDATE, DELETE) in einem append-only Event Log aufzeichnet. Dies ist ein fundamentaler Unterschied zu polyglot Systemen, die externe Tools wie Debezium benötigen, um Änderungen aus dem Write-Ahead Log (WAL) zu extrahieren.

### Warum CDC in ThemisDB?

**Architektonischer Vorteil:** Da ThemisDB alle Datenmodelle (relational, graph, document, vector) in einem einheitlichen "Base Entity"-Format speichert (siehe Chapter 2), kann ein einzelner CDC-Stream **alle** Datentypen erfassen. In einem Polyglot-Setup müsste man separate CDC-Pipelines für PostgreSQL, Neo4j und ChromaDB betreiben, was die Komplexität vervielfacht und die Konsistenz gefährdet.

**Use Cases:**
1. **Real-Time Analytics:** Streaming von Datenänderungen in ein OLAP-System (z.B. ClickHouse)
2. **Event Sourcing:** Rekonstruktion des Anwendungszustands aus dem Event Log
3. **Materialized Views:** Automatische Aktualisierung von Aggregationen
4. **Audit Trails:** BSI-konforme Nachvollziehbarkeit aller Datenänderungen
5. **Cross-System Sync:** Spiegelung von Daten in externe Systeme (Elasticsearch, Redis Cache)
6. **Kafka Integration:** Streaming in ein zentrales Event-Bus-System

```mermaid
graph TB
    subgraph "ThemisDB CDC Architecture"
        App[Application] -->|Write| DB[(ThemisDB<br/>Multi-Model Storage)]
        
        DB -->|Automatically Capture| CDC[CDC Event Log<br/>Append-Only]
        
        CDC -->|Stream 1| Analytics[(ClickHouse<br/>OLAP Analytics)]
        CDC -->|Stream 2| Search[(Elasticsearch<br/>Full-Text Search)]
        CDC -->|Stream 3| Cache[(Redis<br/>Cache Layer)]
        CDC -->|Stream 4| Kafka[Kafka<br/>Event Bus]
        CDC -->|Stream 5| Audit[Audit System<br/>Compliance]
    end
    
    style DB fill:#667eea
    style CDC fill:#f093fb
    style Analytics fill:#43e97b
    style Search fill:#4facfe
    style Cache fill:#ffd32a
    style Kafka fill:#ff6348
    style Audit fill:#95e1d3
```

Abb. 11.1: Real-time-Streaming-Architektur

## 11.2 CDC-Architektur und Datenmodell

### 11.2.1 Event-Struktur

Jedes CDC-Event in ThemisDB folgt einem standardisierten Schema, das maximale Flexibilität bietet:

```json
{
  "sequence": 42,
  "type": "PUT",
  "key": "user:alice",
  "value": "{\"name\":\"Alice\",\"email\":\"alice@example.com\"}",
  "timestamp_ms": 1730294567123,
  "metadata": {
    "table": "user",
    "pk": "alice",
    "operation_type": "update",
    "user_id": "admin@example.com"
  }
}
```

**Feld-Semantik:**

- **sequence** (uint64): Monoton steigende ID, die **strikte Ordnung** garantiert. Diese Eigenschaft ist kritisch für die Konsistenz: Consumer können sicher sein, dass Event N vor Event N+1 aufgetreten ist. Die Sequence-Nummer wird atomar in RocksDB verwaltet (`changefeed_sequence`-Schlüssel) und nutzt RocksDB's MVCC-Garantien.

- **type** (enum): `PUT` (Create/Update) oder `DELETE`. Zukünftige Erweiterungen könnten `TRANSACTION_COMMIT`/`TRANSACTION_ROLLBACK` für Transaktionsgrenzen hinzufügen, was Event Sourcing-Patterns vereinfachen würde.

- **key** (string): Vollständiger Entitätsschlüssel im Format `collection:primary_key`. Dies ermöglicht effiziente Filterung nach Präfixen (z.B. alle User-Events via `user:`-Filter).

- **value** (string|null): Bei PUT: JSON-serialisierte Entität. Bei DELETE: `null`. Die Speicherung als String (nicht als natives JSON-Objekt) minimiert den Serialisierungsaufwand im Hot Path. Consumer können mit simdjson/serde_json parsen.

- **timestamp_ms** (uint64): Millisekunden seit Unix Epoch. Wichtig: Dieser Timestamp basiert auf der Serverzeit zum Zeitpunkt des Commits, nicht auf Client-Zeit. Für verteilte Systeme sollte NTP-Synchronisation sichergestellt sein.

- **metadata** (JSON object): Frei erweiterbar. Standardfelder (`table`, `pk`) werden automatisch gesetzt. Anwendungen können zusätzliche Kontextinformationen hinzufügen (z.B. `user_id` für Audit-Trails, `source_ip` für Sicherheitsanalysen).

### 11.2.2 Physische Speicherung

CDC-Events werden im selben RocksDB-Backend wie die Base Entities gespeichert, jedoch in einem separaten Schlüsselraum:

**Key-Format:** `changefeed:{sequence_number}`

Die Sequence-Nummer wird Zero-padded (z.B. `changefeed:0000000000000042`), um lexikographische Sortierung zu garantieren. Dies ist kritisch für die Scan-Performance: Ein RocksDB-Iterator kann effizient von `changefeed:{from_seq}` bis `changefeed:{to_seq}` iterieren, ohne die Daten neu sortieren zu müssen.

**Column Family Strategy:**

- **Default:** Events werden in der Default Column Family gespeichert. Vorteil: Atomare Transaktionen mit den Base Entities sind trivial (ein RocksDB WriteBatch kann beide aktualisieren). Nachteil: Events und Entitäten konkurrieren um den Block Cache.

- **Dedicated CF (zukünftig):** Eine separate Column Family (`cf_changefeed`) würde isolierte Tuning-Parameter erlauben (z.B. aggressivere Compaction für Events, da sie nie aktualisiert werden).

### 11.2.3 Sequence-Management

Die atomare Sequenzvergabe ist der kritische Pfad für CDC-Schreiboperationen:

```cpp
// Pseudo-Code: Atomare Sequence-Vergabe
rocksdb::WriteBatch batch;
uint64_t seq = increment_sequence_counter(batch); // Atomic increment
std::string event_key = format("changefeed:{:020d}", seq);
batch.Put(event_key, serialize_event(event));
db->Write(batch);  // ACID: Sequence & Event atomar committed
```

```mermaid
sequenceDiagram
    participant App as Application
    participant TM as Transaction Manager
    participant CDC as CDC Logger
    participant RDB as RocksDB
    
    App->>TM: BEGIN TRANSACTION
    TM->>App: transaction_id
    
    App->>TM: UPDATE users SET name='Alice'
    Note over TM: Sammelt Änderungen
    
    App->>TM: INSERT INTO orders ...
    Note over TM: Sammelt weitere Änderungen
    
    App->>TM: COMMIT
    
    TM->>CDC: Erstelle CDC Events
    CDC->>CDC: Generiere Sequence Numbers<br/>(atomar)
    
    CDC->>RDB: WriteBatch {<br/>  Base Entity Updates<br/>  CDC Event #42<br/>  CDC Event #43<br/>}
    
    RDB-->>CDC: [OK] ATOMIC COMMIT
    CDC-->>TM: [OK] Events persistent
    TM-->>App: [OK] COMMIT erfolreich
    
    Note over RDB: ALLE Änderungen atomar:<br/>Base Entities + CDC Events
```

Abb. 11.2: Change-Stream-Processing

**Performance-Trade-off:** Die zentrale Sequenzvergabe ist ein Bottleneck bei hohen Schreibraten (>100K writes/sec). Zukünftige Optimierungen:

1. **Batch Allocation:** Reserviere Sequence-Blöcke (z.B. 1.000 Sequences auf einmal), reduziere Contentions um Faktor 1000.
2. **Sharded Sequences:** In Sharding-Setups (Chapter 16) kann jeder Shard eigene Sequences verwalten. Globale Ordnung wird via `(shard_id, local_sequence)` Tupel erreicht.

### CDC-Optionen

```python
# Nur bestimmte Spalten tracken
stream = db.cdc.create_stream(
    name="user_updates",
    table="users",
    columns=["status", "last_seen"],  # Nur diese Felder
    operations=["UPDATE"]
)

# Mit Filterung
stream = db.cdc.create_stream(
    name="important_messages",
    table="messages",
    filter="priority = 'high'",
    operations=["INSERT"]
)

# Mit Transformationen
stream = db.cdc.create_stream(
    name="enriched_events",
    table="orders",
    transform=lambda event: {
        **event,
        "customer_name": db.query("""
            FOR customer IN customers 
              FILTER customer.id == @customer_id 
              LIMIT 1 
              RETURN customer.name
        """, {"customer_id": event.data["customer_id"]})[0]
    }
)
```

## Server-Sent Events (SSE)

SSE ermöglicht es, Daten vom Server an Clients zu pushen.

### SSE-Server-Setup

```python
from flask import Flask, Response
from themisdb import ThemisDB
import json
import time

app = Flask(__name__)
db = ThemisDB()

def event_stream(channel):
    """Generator für SSE-Events"""
    stream = db.cdc.create_stream(
        name=f"sse_{channel}",
        table=channel,
        operations=["INSERT", "UPDATE", "DELETE"]
    )
    
    for event in stream:
        # SSE-Format: "data: JSON\n\n"
        yield f"data: {json.dumps(event.to_dict())}\n\n"

@app.route('/stream/<channel>')
def stream(channel):
    return Response(
        event_stream(channel),
        mimetype="text/event-stream",
        headers={
            "Cache-Control": "no-cache",
            "Connection": "keep-alive",
            "X-Accel-Buffering": "no"  # Nginx
        }
    )
```

### SSE-Client-Code

```javascript
// JavaScript Client
const eventSource = new EventSource('/stream/messages');

eventSource.onmessage = function(event) {
    const data = JSON.parse(event.data);
    
    if (data.operation === 'INSERT') {
        addMessageToUI(data.after);
    } else if (data.operation === 'UPDATE') {
        updateMessageInUI(data.after);
    } else if (data.operation === 'DELETE') {
        removeMessageFromUI(data.before.id);
    }
};

eventSource.onerror = function(error) {
    console.error('SSE error:', error);
    // Automatische Reconnection
};
```

## WebSocket-Integration

Für bidirektionale Kommunikation sind WebSockets ideal.

### WebSocket-Server

```python
from flask_socketio import SocketIO, emit, join_room, leave_room
from themisdb import ThemisDB

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*")
db = ThemisDB()

# User-Session-Tracking
active_users = {}

@socketio.on('connect')
def handle_connect():
    print(f'Client connected: {request.sid}')

@socketio.on('join_channel')
def handle_join(data):
    channel = data['channel']
    username = data['username']
    
    join_room(channel)
    active_users[request.sid] = {
        'username': username,
        'channel': channel
    }
    
    # Benachrichtige andere
    emit('user_joined', {
        'username': username,
        'timestamp': time.time()
    }, room=channel, skip_sid=request.sid)
    
    # CDC-Stream für diesen Channel
    start_cdc_stream(channel)

def start_cdc_stream(channel):
    """Background-Thread für CDC → WebSocket"""
    stream = db.cdc.create_stream(
        name=f"ws_{channel}",
        table="messages",
        filter=f"channel = '{channel}'"
    )
    
    def emit_changes():
        for event in stream:
            socketio.emit('message_update', 
                         event.to_dict(), 
                         room=channel)
    
    # In separatem Thread
    socketio.start_background_task(emit_changes)

@socketio.on('send_message')
def handle_message(data):
    channel = active_users[request.sid]['channel']
    username = active_users[request.sid]['username']
    
    # In DB schreiben
    db.execute("""
        INSERT {
          channel: @channel,
          username: @username,
          content: @content,
          timestamp: @timestamp
        } INTO messages
    """, {"channel": channel, "username": username, "content": data['content'], "timestamp": time.time()})
    
    # CDC wird automatisch notifizieren
```

### WebSocket-Client

```javascript
const socket = io('http://localhost:5000');

// Channel beitreten
socket.emit('join_channel', {
    channel: 'general',
    username: 'Alice'
});

// Nachrichten senden
function sendMessage(content) {
    socket.emit('send_message', {
        content: content
    });
}

// Nachrichten empfangen
socket.on('message_update', function(event) {
    if (event.operation === 'INSERT') {
        displayMessage(event.after);
    }
});

// Nutzer beigetreten
socket.on('user_joined', function(data) {
    console.log(`${data.username} ist beigetreten`);
});
```

## Example: Realtime Chat (examples/18_realtime_chat)

Vollständige Chat-Anwendung mit Channels, Private Messages, Online-Status und Typing-Indikatoren.

### Datenmodell

```python
from themisdb import ThemisDB
from datetime import datetime

db = ThemisDB()

# Schema erstellen
db.execute("""
    CREATE TABLE users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT UNIQUE NOT NULL,
        email TEXT UNIQUE NOT NULL,
        avatar_url TEXT,
        status TEXT DEFAULT 'offline',  -- online, offline, away
        last_seen TIMESTAMP,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )
""")

db.execute("""
    CREATE TABLE channels (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT UNIQUE NOT NULL,
        description TEXT,
        is_private BOOLEAN DEFAULT FALSE,
        created_by INTEGER REFERENCES users(id),
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )
""")

db.execute("""
    CREATE TABLE channel_members (
        channel_id INTEGER REFERENCES channels(id),
        user_id INTEGER REFERENCES users(id),
        joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        role TEXT DEFAULT 'member',  -- admin, moderator, member
        PRIMARY KEY (channel_id, user_id)
    )
""")

db.execute("""
    CREATE TABLE messages (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        channel_id INTEGER REFERENCES channels(id),
        user_id INTEGER REFERENCES users(id),
        content TEXT NOT NULL,
        message_type TEXT DEFAULT 'text',  -- text, image, file, system
        reply_to INTEGER REFERENCES messages(id),
        edited_at TIMESTAMP,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        INDEX idx_channel_time (channel_id, created_at),
        INDEX idx_user (user_id)
    )
""")

db.execute("""
    CREATE TABLE reactions (
        message_id INTEGER REFERENCES messages(id),
        user_id INTEGER REFERENCES users(id),
        emoji TEXT NOT NULL,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        PRIMARY KEY (message_id, user_id, emoji)
    )
""")

db.execute("""
    CREATE TABLE direct_messages (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        from_user_id INTEGER REFERENCES users(id),
        to_user_id INTEGER REFERENCES users(id),
        content TEXT NOT NULL,
        read_at TIMESTAMP,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        INDEX idx_participants (from_user_id, to_user_id, created_at)
    )
""")

db.execute("""
    CREATE TABLE typing_indicators (
        channel_id INTEGER REFERENCES channels(id),
        user_id INTEGER REFERENCES users(id),
        started_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        PRIMARY KEY (channel_id, user_id)
    )
""")
```

### Chat-Backend

Das Chat-Backend implementiert eine Echtzeit-Messaging-Plattform mit Flask-SocketIO und ThemisDB. Die Architektur trennt sauber zwischen WebSocket-Events (für Echtzeit-Updates) und REST-API (für Datenabfragen). Der `ChatService` kapselt alle Datenbankoperationen, während die Socket-Handler die Echtzeitkommunikation zwischen Clients koordinieren.

> **📁 Vollständiger Code:** `examples/realtime_chat/backend/app.py` (ca. 400 Zeilen)

**Kernkomponenten des Chat-Backends:**

```python
from flask import Flask, request, jsonify
from flask_socketio import SocketIO, emit, join_room
from themisdb import ThemisDB

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*")
db = ThemisDB()

# Aktive Verbindungen verwalten
connections = {}  # sid -> {user_id, username, channels}
```

**Zentrale ChatService-Klasse (Auszug):**

```python
class ChatService:
    @staticmethod
    def get_channel_messages(channel_id, limit=50, before_id=None):
        """Lädt Nachrichten mit Pagination"""
        query = """
        FOR msg IN messages
            FILTER msg.channel_id == @channel_id
            """ + ("AND msg._key < @before_id" if before_id else "") + """
            SORT msg.timestamp DESC
            LIMIT @limit
            RETURN msg
        """
        return db.query(query, {
            "channel_id": channel_id,
            "before_id": before_id,
            "limit": limit
        })
    
    @staticmethod
    def send_message(channel_id, user_id, username, text):
        """Speichert Nachricht und triggert CDC"""
        message = {
            "channel_id": channel_id,
            "user_id": user_id,
            "username": username,
            "text": text,
            "timestamp": datetime.now().isoformat()
        }
        # Insert triggert automatisch CDC-Event!
        return db.insert("messages", message)
```

**WebSocket-Handler für Echtzeit-Events:**

```python
@socketio.on('connect')
def handle_connect():
    """Client verbindet sich"""
    user_id = request.args.get('userId')
    username = request.args.get('username')
    
    connections[request.sid] = {
        "user_id": user_id,
        "username": username,
        "channels": []
    }
    
    emit('connected', {'status': 'ok'})

@socketio.on('send_message')
def handle_send_message(data):
    """Client sendet Nachricht"""
    msg = ChatService.send_message(
        data['channel_id'],
        connections[request.sid]['user_id'],
        connections[request.sid]['username'],
        data['text']
    )
    
    # Broadcast an alle im Channel
    emit('new_message', msg, room=data['channel_id'])
```

**REST-API-Endpunkte (Auszug):**

```python
@app.route('/api/channels', methods=['GET'])
def get_channels():
    """Liste aller Channels für User"""
    user_id = request.args.get('user_id')
    channels = ChatService.get_user_channels(user_id)
    return jsonify(channels)

@app.route('/api/messages/<channel_id>', methods=['GET'])
def get_messages(channel_id):
    """Nachrichten mit Pagination laden"""
    limit = int(request.args.get('limit', 50))
    before_id = request.args.get('before_id')
    messages = ChatService.get_channel_messages(
        channel_id, limit, before_id
    )
    return jsonify(messages)
```

**CDC-Integration für Echtzeit-Sync:**

Die vollständige Implementierung enthält zusätzlich:
- Channel-Management (erstellen, beitreten, verlassen)
- User-Presence-Tracking (online/offline Status)
- Typing-Indicators
- Read-Receipts
- Message-Threading
- File-Upload-Handling

Siehe vollständige Datei für alle Features und Error-Handling.

### Chat-Frontend (React)

Das React-Frontend implementiert eine moderne Chat-Oberfläche mit Echtzeit-Updates über Socket.IO. Die Komponente verwaltet den Verbindungs-State, empfängt Live-Updates (neue Nachrichten, Typing-Indicators, User-Status) und bietet eine intuitive UI für Chat-Interaktionen. Alle State-Updates erfolgen reaktiv über React Hooks.

> **📁 Vollständiger Code:** `examples/realtime_chat/frontend/ChatApp.jsx` (ca. 200 Zeilen)

**State-Management und Socket-Setup:**

```jsx
import React, { useState, useEffect, useRef } from 'react';
import io from 'socket.io-client';

const ChatApp = ({ userId, username }) => {
    // State für Channels, Messages, Typing-Indicators
    const [socket, setSocket] = useState(null);
    const [channels, setChannels] = useState([]);
    const [activeChannel, setActiveChannel] = useState(null);
    const [messages, setMessages] = useState([]);
    const [typingUsers, setTypingUsers] = useState(new Set());
    
    useEffect(() => {
        // Socket-Verbindung initialisieren
        const newSocket = io('http://localhost:5000');
        setSocket(newSocket);
        
        // Authentifizieren
        newSocket.emit('authenticate', { user_id: userId, username });
        
        return () => newSocket.close();
    }, [userId, username]);
```

**Event-Handler für Echtzeit-Updates:**

```jsx
        // Neue Nachricht empfangen
        newSocket.on('new_message', (data) => {
            setMessages(prev => [...prev, data.message]);
            scrollToBottom();
        });
        
        // Typing-Indicator
        newSocket.on('user_typing', (data) => {
            setTypingUsers(prev => new Set([...prev, data.username]));
        });
        
        // User-Presence
        newSocket.on('user_joined', (data) => {
            setOnlineUsers(prev => [...prev, data]);
        });
```

**Nachricht senden mit Typing-Feedback:**

```jsx
    const sendMessage = () => {
        if (!newMessage.trim()) return;
        
        socket.emit('send_message', {
            channel_id: activeChannel.id,
            text: newMessage,
            message_type: 'text'
        });
        
        setNewMessage('');
        stopTyping();
    };
    
    const handleTyping = () => {
        socket.emit('user_typing', { channel_id: activeChannel.id });
        
        // Auto-stop nach 3 Sekunden
        clearTimeout(typingTimeoutRef.current);
        typingTimeoutRef.current = setTimeout(stopTyping, 3000);
    };
```

**UI-Rendering (Konzept):**

```jsx
    return (
        <div className="chat-container">
            <ChannelList 
                channels={channels}
                activeChannel={activeChannel}
                onSelectChannel={joinChannel}
            />
            <MessageList 
                messages={messages}
                typingUsers={Array.from(typingUsers)}
            />
            <MessageInput
                value={newMessage}
                onChange={(e) => { setNewMessage(e.target.value); handleTyping(); }}
                onSend={sendMessage}
            />
        </div>
    );
};
```

Die vollständige Implementierung enthält zusätzlich:
- Message-Threading (Antworten auf Nachrichten)
- Reactions (Emoji-Reaktionen)
- File-Upload mit Progress
- Message-Editing und Deletion
- Infinite-Scroll für Message-History
- Unread-Message-Counter
- Sound-Notifications

Siehe vollständige Datei für alle UI-Komponenten und Styling.

### Chat-Features

Das vollständige Chat-Example demonstriert:

1. **Echtzeit-Nachrichten**: Sofortige Zustellung via WebSocket + CDC
2. **Online-Status**: Wer ist gerade online?
3. **Typing-Indikatoren**: "Alice is typing..."
4. **Reaktionen**: Emoji-Reactions auf Nachrichten
5. **Threads**: Antworten auf bestimmte Nachrichten
6. **Nachrichtenbearbeitung**: Edit-History mit Timestamps
7. **Private Channels**: Eingeschränkter Zugriff
8. **Direktnachrichten**: 1-on-1 Chats
9. **Presence**: Last-Seen und Away-Status
10. **Pagination**: Unendliches Scrollen durch Historie

## Example: Kanban Board (examples/16_kanban_board)

Vollständige Kanban-Board-Anwendung mit Drag & Drop, Realtime-Updates und Team-Collaboration.

### Datenmodell

```python
from themisdb import ThemisDB

db = ThemisDB()

db.execute("""
    CREATE TABLE boards (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        description TEXT,
        created_by INTEGER REFERENCES users(id),
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )
""")

db.execute("""
    CREATE TABLE columns (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        board_id INTEGER REFERENCES boards(id),
        name TEXT NOT NULL,
        position INTEGER NOT NULL,
        wip_limit INTEGER,  -- Work-In-Progress Limit
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        INDEX idx_board (board_id, position)
    )
""")

db.execute("""
    CREATE TABLE cards (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        column_id INTEGER REFERENCES columns(id),
        board_id INTEGER REFERENCES boards(id),
        title TEXT NOT NULL,
        description TEXT,
        assigned_to INTEGER REFERENCES users(id),
        position INTEGER NOT NULL,
        priority TEXT DEFAULT 'medium',  -- low, medium, high, urgent
        due_date TIMESTAMP,
        created_by INTEGER REFERENCES users(id),
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        INDEX idx_column (column_id, position),
        INDEX idx_board (board_id)
    )
""")

db.execute("""
    CREATE TABLE card_activities (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        card_id INTEGER REFERENCES cards(id),
        user_id INTEGER REFERENCES users(id),
        action_type TEXT NOT NULL,  -- created, moved, assigned, commented
        details JSON,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        INDEX idx_card (card_id, created_at)
    )
""")

db.execute("""
    CREATE TABLE card_labels (
        card_id INTEGER REFERENCES cards(id),
        label TEXT NOT NULL,
        color TEXT,
        PRIMARY KEY (card_id, label)
    )
""")
```

### Kanban-Backend

```python
from flask import Flask, jsonify, request
from flask_socketio import SocketIO, emit, join_room
from themisdb import ThemisDB
import time

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*")
db = ThemisDB()

class KanbanService:
    @staticmethod
    def get_board(board_id):
        """Board mit allen Spalten und Karten laden"""
        board = db.query("""
            FOR board IN boards 
              FILTER board.id == @board_id 
              LIMIT 1 
              RETURN board
        """, {"board_id": board_id})[0]
        
        columns = db.query("""
            FOR column IN columns 
              FILTER column.board_id == @board_id
              SORT column.position ASC
              RETURN column
        """, {"board_id": board_id})
        
        for col in columns:
            col['cards'] = db.query("""
                FOR card IN cards
                  FILTER card.column_id == @column_id
                  LET assigned_name = (
                    FOR user IN users
                      FILTER card.assigned_to == user.id
                      LIMIT 1
                      RETURN user.username
                  )[0]
                  SORT card.position ASC
                  RETURN MERGE(card, {assigned_to_name: assigned_name})
            """, {"column_id": col['id']})
        
        board['columns'] = columns
        return board
    
    @staticmethod
    def move_card(card_id, to_column_id, to_position):
        """Karte verschieben"""
        with db.transaction():
            # Aktuelle Position
            current = db.query("""
                FOR card IN cards 
                  FILTER card.id == @card_id 
                  LIMIT 1 
                  RETURN {column_id: card.column_id, position: card.position}
            """, {"card_id": card_id})[0]
            
            from_column_id = current['column_id']
            from_position = current['position']
            
            # Position in alter Spalte aktualisieren
            db.execute("""
                UPDATE cards 
                SET position = position - 1
                WHERE column_id = ? AND position > ?
            """, [from_column_id, from_position])
            
            # Position in neuer Spalte freimachen
            db.execute("""
                UPDATE cards 
                SET position = position + 1
                WHERE column_id = ? AND position >= ?
            """, [to_column_id, to_position])
            
            # Karte verschieben
            db.execute("""
                UPDATE cards 
                SET column_id = ?, position = ?, updated_at = CURRENT_TIMESTAMP
                WHERE id = ?
            """, [to_column_id, to_position, card_id])
            
            # Activity loggen
            db.execute("""
                INSERT INTO card_activities (card_id, user_id, action_type, details)
                VALUES (?, ?, 'moved', ?)
            """, [card_id, request.user_id, json.dumps({
                'from_column': from_column_id,
                'to_column': to_column_id
            })])
    
    @staticmethod
    def create_card(board_id, column_id, title, description, assigned_to=None):
        """Neue Karte erstellen"""
        with db.transaction():
            # Position am Ende der Spalte
            position = db.query("""
                FOR card IN cards 
                  FILTER card.column_id == @column_id
                  COLLECT AGGREGATE max_pos = MAX(card.position)
                  RETURN COALESCE(max_pos, -1) + 1
            """, {"column_id": column_id})[0]
            
            result = db.execute("""
                INSERT {
                  board_id: @board_id,
                  column_id: @column_id,
                  title: @title,
                  description: @description,
                  assigned_to: @assigned_to,
                  position: @position,
                  created_by: @created_by
                } INTO cards
                RETURN NEW
            """, {
                "board_id": board_id,
                "column_id": column_id,
                "title": title,
                "description": description,
                "assigned_to": assigned_to,
                "position": position,
                "created_by": request.user_id
            })
            
            card = result[0]
            
            # Activity
            db.execute("""
                INSERT {
                  card_id: @card_id,
                  user_id: @user_id,
                  action_type: 'created'
                } INTO card_activities
            """, {"card_id": card['id'], "user_id": request.user_id})
            
            return card
    
    @staticmethod
    def update_card(card_id, **updates):
        """Karte aktualisieren"""
        allowed_fields = ['title', 'description', 'assigned_to', 'priority', 'due_date']
        set_clause = ', '.join([f"{field} = ?" for field in updates if field in allowed_fields])
        values = [updates[field] for field in updates if field in allowed_fields]
        values.append(card_id)
        
        db.execute(f"""
            UPDATE cards 
            SET {set_clause}, updated_at = CURRENT_TIMESTAMP
            WHERE id = ?
        """, values)
        
        # Activity
        db.execute("""
            INSERT {
              card_id: @card_id,
              user_id: @user_id,
              action_type: 'updated',
              details: @details
            } INTO card_activities
        """, {"card_id": card_id, "user_id": request.user_id, "details": json.dumps(updates)})

# WebSocket-Handler
@socketio.on('join_board')
def handle_join_board(data):
    board_id = data['board_id']
    join_room(f'board_{board_id}')
    
    # Board-Daten senden
    board = KanbanService.get_board(board_id)
    emit('board_state', board)
    
    # CDC-Stream starten
    start_board_cdc(board_id)

@socketio.on('move_card')
def handle_move_card(data):
    card_id = data['card_id']
    to_column_id = data['to_column_id']
    to_position = data['to_position']
    board_id = data['board_id']
    
    try:
        KanbanService.move_card(card_id, to_column_id, to_position)
        # CDC benachrichtigt andere Clients
    except Exception as e:
        emit('error', {'message': str(e)})

@socketio.on('create_card')
def handle_create_card(data):
    card = KanbanService.create_card(
        data['board_id'],
        data['column_id'],
        data['title'],
        data.get('description'),
        data.get('assigned_to')
    )
    # CDC benachrichtigt

@socketio.on('update_card')
def handle_update_card(data):
    card_id = data.pop('card_id')
    KanbanService.update_card(card_id, **data)
    # CDC benachrichtigt

# CDC für Board
active_board_streams = set()

def start_board_cdc(board_id):
    if board_id in active_board_streams:
        return
    
    active_board_streams.add(board_id)
    
    def cdc_worker():
        # Cards-Stream
        cards_stream = db.cdc.create_stream(
            name=f"kanban_cards_{board_id}",
            table="cards",
            filter=f"board_id = {board_id}",
            operations=["INSERT", "UPDATE", "DELETE"]
        )
        
        # Columns-Stream
        columns_stream = db.cdc.create_stream(
            name=f"kanban_columns_{board_id}",
            table="columns",
            filter=f"board_id = {board_id}",
            operations=["INSERT", "UPDATE", "DELETE"]
        )
        
        # Beide Streams kombinieren
        for event in combined_stream([cards_stream, columns_stream]):
            if event.table == 'cards':
                if event.operation == 'INSERT':
                    socketio.emit('card_created', event.after, 
                                room=f'board_{board_id}')
                elif event.operation == 'UPDATE':
                    socketio.emit('card_updated', event.after,
                                room=f'board_{board_id}')
                elif event.operation == 'DELETE':
                    socketio.emit('card_deleted', {'id': event.before['id']},
                                room=f'board_{board_id}')
            
            elif event.table == 'columns':
                # Spalten-Update
                socketio.emit('column_updated', event.after,
                            room=f'board_{board_id}')
    
    socketio.start_background_task(cdc_worker)

def combined_stream(streams):
    """Mehrere CDC-Streams kombinieren"""
    import queue
    import threading
    
    q = queue.Queue()
    
    def stream_worker(stream):
        for event in stream:
            q.put(event)
    
    for stream in streams:
        threading.Thread(target=stream_worker, args=(stream,), daemon=True).start()
    
    while True:
        yield q.get()

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000)
```

### Kanban-Frontend (React)

```jsx
// KanbanBoard.jsx
import React, { useState, useEffect } from 'react';
import { DragDropContext, Droppable, Draggable } from 'react-beautiful-dnd';
import io from 'socket.io-client';

const KanbanBoard = ({ boardId, userId }) => {
    const [socket, setSocket] = useState(null);
    const [board, setBoard] = useState(null);
    const [columns, setColumns] = useState([]);
    
    useEffect(() => {
        const newSocket = io('http://localhost:5000');
        setSocket(newSocket);
        
        newSocket.emit('join_board', { board_id: boardId });
        
        newSocket.on('board_state', (data) => {
            setBoard(data);
            setColumns(data.columns);
        });
        
        newSocket.on('card_created', (card) => {
            setColumns(prev => prev.map(col => {
                if (col.id === card.column_id) {
                    return { ...col, cards: [...col.cards, card] };
                }
                return col;
            }));
        });
        
        newSocket.on('card_updated', (card) => {
            setColumns(prev => prev.map(col => ({
                ...col,
                cards: col.cards.map(c => c.id === card.id ? card : c)
            })));
        });
        
        newSocket.on('card_deleted', (data) => {
            setColumns(prev => prev.map(col => ({
                ...col,
                cards: col.cards.filter(c => c.id !== data.id)
            })));
        });
        
        return () => newSocket.close();
    }, [boardId]);
    
    const onDragEnd = (result) => {
        if (!result.destination) return;
        
        const { source, destination } = result;
        
        if (source.droppableId === destination.droppableId &&
            source.index === destination.index) {
            return;
        }
        
        // Optimistisches Update
        const newColumns = [...columns];
        const sourceColumn = newColumns.find(c => c.id === parseInt(source.droppableId));
        const destColumn = newColumns.find(c => c.id === parseInt(destination.droppableId));
        
        const [movedCard] = sourceColumn.cards.splice(source.index, 1);
        destColumn.cards.splice(destination.index, 0, movedCard);
        
        setColumns(newColumns);
        
        // An Server senden
        socket.emit('move_card', {
            card_id: parseInt(result.draggableId),
            to_column_id: parseInt(destination.droppableId),
            to_position: destination.index,
            board_id: boardId
        });
    };
    
    if (!board) return <div>Loading...</div>;
    
    return (
        <div className="kanban-board">
            <h1>{board.name}</h1>
            
            <DragDropContext onDragEnd={onDragEnd}>
                <div className="columns">
                    {columns.map(column => (
                        <div key={column.id} className="column">
                            <h3>
                                {column.name}
                                <span className="card-count">{column.cards.length}</span>
                                {column.wip_limit && (
                                    <span className={`wip-limit ${column.cards.length > column.wip_limit ? 'exceeded' : ''}`}>
                                        / {column.wip_limit}
                                    </span>
                                )}
                            </h3>
                            
                            <Droppable droppableId={column.id.toString()}>
                                {(provided, snapshot) => (
                                    <div
                                        ref={provided.innerRef}
                                        {...provided.droppableProps}
                                        className={`cards ${snapshot.isDraggingOver ? 'dragging-over' : ''}`}
                                    >
                                        {column.cards.map((card, index) => (
                                            <Draggable
                                                key={card.id}
                                                draggableId={card.id.toString()}
                                                index={index}
                                            >
                                                {(provided, snapshot) => (
                                                    <div
                                                        ref={provided.innerRef}
                                                        {...provided.draggableProps}
                                                        {...provided.dragHandleProps}
                                                        className={`card priority-${card.priority} ${snapshot.isDragging ? 'dragging' : ''}`}
                                                    >
                                                        <h4>{card.title}</h4>
                                                        {card.description && <p>{card.description}</p>}
                                                        {card.assigned_to_name && (
                                                            <div className="assignee">
                                                                👤 {card.assigned_to_name}
                                                            </div>
                                                        )}
                                                        {card.due_date && (
                                                            <div className="due-date">
                                                                📅 {new Date(card.due_date).toLocaleDateString()}
                                                            </div>
                                                        )}
                                                    </div>
                                                )}
                                            </Draggable>
                                        ))}
                                        {provided.placeholder}
                                    </div>
                                )}
                            </Droppable>
                        </div>
                    ))}
                </div>
            </DragDropContext>
        </div>
    );
};

export default KanbanBoard;
```

### Kanban-Features

Das Kanban-Board-Example demonstriert:

1. **Drag & Drop**: Karten zwischen Spalten verschieben
2. **Realtime-Sync**: Alle Nutzer sehen Änderungen sofort
3. **WIP-Limits**: Work-In-Progress Beschränkungen
4. **Prioritäten**: Visuelle Kennzeichnung (low, medium, high, urgent)
5. **Zuweisungen**: Karten Teammitgliedern zuordnen
6. **Due Dates**: Fälligkeitsdaten mit Warnungen
7. **Labels**: Flexible Kategorisierung
8. **Activity-Log**: Vollständige Historie aller Änderungen
9. **Board-Analytics**: Durchlaufzeit, Cycle-Time, etc.
10. **Custom Columns**: Beliebige Workflow-Stufen

## Event-Driven Architecture

Realtime-Anwendungen profitieren von event-driven Design.

### Event-Bus-Pattern

```python
from themisdb import ThemisDB
from typing import Callable, List
import threading

class EventBus:
    def __init__(self, db: ThemisDB):
        self.db = db
        self.handlers = {}  # event_type -> [handlers]
        self.running = False
    
    def subscribe(self, event_type: str, handler: Callable):
        """Handler für Event-Typ registrieren"""
        if event_type not in self.handlers:
            self.handlers[event_type] = []
        self.handlers[event_type].append(handler)
    
    def publish(self, event_type: str, data: dict):
        """Event publishen"""
        self.db.execute("""
            INSERT {
              event_type: @event_type,
              data: @data,
              created_at: DATE_NOW()
            } INTO events
        """, {"event_type": event_type, "data": json.dumps(data)})
    
    def start(self):
        """Event-Processing starten"""
        self.running = True
        
        def process_events():
            stream = self.db.cdc.create_stream(
                name="event_bus",
                table="events",
                operations=["INSERT"]
            )
            
            for event in stream:
                if not self.running:
                    break
                
                event_type = event.after['event_type']
                data = json.loads(event.after['data'])
                
                # Handler aufrufen
                if event_type in self.handlers:
                    for handler in self.handlers[event_type]:
                        try:
                            handler(data)
                        except Exception as e:
                            print(f"Handler error: {e}")
        
        threading.Thread(target=process_events, daemon=True).start()
    
    def stop(self):
        self.running = False

# Verwendung
bus = EventBus(db)

# Handler registrieren
@bus.subscribe('user_registered')
def send_welcome_email(data):
    print(f"Sending email to {data['email']}")

@bus.subscribe('order_placed')
def update_inventory(data):
    print(f"Reducing stock for order {data['order_id']}")

# Events publishen
bus.publish('user_registered', {
    'user_id': 123,
    'email': 'alice@example.com'
})

bus.start()
```

### CQRS-Pattern

Command Query Responsibility Segregation für Realtime-Apps:

```python
class OrderService:
    def __init__(self, db: ThemisDB, event_bus: EventBus):
        self.db = db
        self.event_bus = event_bus
    
    # COMMAND: Write-Seite
    def place_order(self, user_id, items):
        with self.db.transaction():
            # Order erstellen
            order_id = self.db.execute("""
                INSERT {
                  user_id: @user_id,
                  status: 'pending',
                  created_at: DATE_NOW()
                } INTO orders
                RETURN NEW.id
            """, {"user_id": user_id})[0]
            
            # Items
            for item in items:
                self.db.execute("""
                    INSERT {
                      order_id: @order_id,
                      product_id: @product_id,
                      quantity: @quantity,
                      price: @price
                    } INTO order_items
                """, {
                    "order_id": order_id,
                    "product_id": item['product_id'],
                    "quantity": item['quantity'],
                    "price": item['price']
                })
            
            # Event publishen
            self.event_bus.publish('order_placed', {
                'order_id': order_id,
                'user_id': user_id,
                'items': items
            })
            
            return order_id
    
    # QUERY: Read-Seite (materialized view)
    def get_order_summary(self, order_id):
        return self.db.query("""
            SELECT 
                o.id, o.status, o.created_at,
                u.name as customer_name,
                COUNT(oi.id) as item_count,
                SUM(oi.quantity * oi.price) as total
            FROM orders o
            JOIN users u ON o.user_id = u.id
            LEFT JOIN order_items oi ON o.id = oi.order_id
            WHERE o.id = ?
            GROUP BY o.id
        """, [order_id])[0]

# Event-Handler aktualisieren materialized views
@event_bus.subscribe('order_placed')
def update_order_cache(data):
    # Cache/Read-Model aktualisieren
    pass
```

## Best Practices

### 1. Connection Management

```python
class ConnectionPool:
    def __init__(self, max_connections=100):
        self.max_connections = max_connections
        self.active_connections = {}
        self.semaphore = threading.Semaphore(max_connections)
    
    def add_connection(self, sid, user_id):
        with self.semaphore:
            if len(self.active_connections) >= self.max_connections:
                raise ValueError("Too many connections")
            self.active_connections[sid] = user_id
    
    def remove_connection(self, sid):
        if sid in self.active_connections:
            del self.active_connections[sid]
            self.semaphore.release()
```

### 2. Rate Limiting

```python
from collections import defaultdict
import time

class RateLimiter:
    def __init__(self, max_requests=10, window=60):
        self.max_requests = max_requests
        self.window = window
        self.requests = defaultdict(list)
    
    def allow(self, user_id):
        now = time.time()
        # Alte Einträge entfernen
        self.requests[user_id] = [
            t for t in self.requests[user_id]
            if now - t < self.window
        ]
        
        if len(self.requests[user_id]) >= self.max_requests:
            return False
        
        self.requests[user_id].append(now)
        return True

# Middleware
@socketio.on('send_message')
def handle_send_message(data):
    if not rate_limiter.allow(current_user_id):
        return emit('error', {'message': 'Rate limit exceeded'})
    # ... rest
```

### 3. Message Deduplication

```python
class MessageDeduplicator:
    def __init__(self, ttl=60):
        self.seen = {}  # message_id -> timestamp
        self.ttl = ttl
    
    def is_duplicate(self, message_id):
        now = time.time()
        
        # Cleanup
        self.seen = {
            mid: ts for mid, ts in self.seen.items()
            if now - ts < self.ttl
        }
        
        if message_id in self.seen:
            return True
        
        self.seen[message_id] = now
        return False
```

### 4. Graceful Shutdown

```python
import signal
import sys

def graceful_shutdown(signum, frame):
    print("Shutting down gracefully...")
    
    # Alle Clients benachrichtigen
    socketio.emit('server_shutdown', {
        'message': 'Server ist down für Wartung',
        'reconnect_in': 60
    })
    
    # CDC-Streams stoppen
    for stream_id in active_cdc_streams:
        stop_cdc_stream(stream_id)
    
    # Connections schließen
    socketio.stop()
    
    sys.exit(0)

signal.signal(signal.SIGINT, graceful_shutdown)
signal.signal(signal.SIGTERM, graceful_shutdown)
```

### 5. Monitoring & Metrics

```python
from prometheus_client import Counter, Histogram, Gauge
import time

# Metriken
messages_sent = Counter('messages_sent_total', 'Total messages sent')
message_latency = Histogram('message_latency_seconds', 'Message delivery latency')
active_connections_gauge = Gauge('active_connections', 'Number of active WebSocket connections')

@socketio.on('send_message')
def handle_send_message(data):
    start_time = time.time()
    
    # ... message handling ...
    
    messages_sent.inc()
    message_latency.observe(time.time() - start_time)

@socketio.on('connect')
def handle_connect():
    active_connections_gauge.inc()

@socketio.on('disconnect')
def handle_disconnect():
    active_connections_gauge.dec()
```

## Performance-Optimierungen

### 1. Message-Batching

```python
class MessageBatcher:
    def __init__(self, max_batch_size=100, max_wait_ms=100):
        self.max_batch_size = max_batch_size
        self.max_wait_ms = max_wait_ms
        self.batch = []
        self.last_flush = time.time()
    
    def add(self, message):
        self.batch.append(message)
        
        if len(self.batch) >= self.max_batch_size or \
           (time.time() - self.last_flush) * 1000 >= self.max_wait_ms:
            self.flush()
    
    def flush(self):
        if not self.batch:
            return
        
        # Bulk-Insert
        db.executemany("""
            INSERT INTO messages (channel_id, user_id, content)
            VALUES (?, ?, ?)
        """, [(m['channel_id'], m['user_id'], m['content']) for m in self.batch])
        
        self.batch = []
        self.last_flush = time.time()
```

### 2. Connection Pooling

```python
from themisdb import ThemisDB, ConnectionPool

# Connection Pool für DB
pool = ConnectionPool(
    min_size=10,
    max_size=50,
    max_idle_time=300
)

def get_db_connection():
    return pool.acquire()

def release_db_connection(conn):
    pool.release(conn)
```

### 3. Caching

```python
from functools import lru_cache
import time

class CachedQuery:
    def __init__(self, ttl=60):
        self.cache = {}
        self.ttl = ttl
    
    def get(self, query, params):
        key = (query, tuple(params))
        
        if key in self.cache:
            value, timestamp = self.cache[key]
            if time.time() - timestamp < self.ttl:
                return value
        
        # Cache Miss
        result = db.query(query, params)
        self.cache[key] = (result, time.time())
        return result

# Verwendung
cache = CachedQuery(ttl=30)

def get_channel_members(channel_id):
    return cache.get("""
        FOR member IN channel_members 
          FILTER member.channel_id == @channel_id 
          RETURN member
    """, {"channel_id": channel_id})
```

## 11.8 StreamingIngestManager — Hochdurchsatz-Ingest-Engine (v1.9.x)

Neben CDC-basierten Downstream-Konsumenten benötigen viele Anwendungen auch einen schnellen, backpressure-fähigen **Eingangskanal** für hochvolumige Event-Ströme.  Der `StreamingIngestManager` implementiert genau dies: ein Ring-Buffer-basierter In-Memory-Buffer, der von einem dedizierten Flush-Thread kontinuierlich nach RocksDB drainiert wird.

### Architektur

```
Producer Thread(s)
      │  ingest(key, value)
      ▼
┌─────────────────────────────┐
│  Ring Buffer (in-memory)    │  max_buffer_events = 1 000 000
│  OverflowPolicy: BLOCK/DROP │
└─────────────┬───────────────┘
              │  alle flush_interval = 10 ms
              ▼
┌─────────────────────────────┐
│  Flush Thread               │  max_batch_size = 65 536
│  rocksdb::WriteBatch        │  WAL-sync optional
└─────────────┬───────────────┘
              │
              ▼
          RocksDB
```

**Durability-Garantie:** Jedes Event wird in den WAL geschrieben, bevor `ingest()` zurückkehrt (`sync_wal = true`, Standard).  Der RocksDB-Commit erfolgt asynchron durch den Flush-Thread.

**Performance-Ziel:** ≥ 1 M Events/s bei End-to-End-Latenz ≤ 50 ms auf einem 8-Core-Knoten.

### Schnellstart

```cpp
#include "storage/streaming_ingest_manager.h"

// Konfiguration
themis::StreamingIngestManager::Config cfg;
cfg.flush_interval      = std::chrono::milliseconds(10);
cfg.max_buffer_events   = 1'000'000;
cfg.max_batch_size      = 65'536;
cfg.overflow_policy     = themis::StreamingIngestManager::OverflowPolicy::BLOCK;
cfg.sync_wal            = true;

// Instanz erstellen und starten
auto mgr = themis::StreamingIngestManager::create(rocksdb_wrapper, cfg);
mgr->start();

// Einzelnes Event einreihen
mgr->ingest("metrics:cpu:server-01", "0.72");

// Effizienteres Batch-Einreihen (Mutex nur einmal)
std::vector<themis::StreamingIngestManager::Event> batch;
for (auto& row : sensor_rows) {
    batch.push_back({row.key, row.payload});
}
auto result = mgr->ingestBatch(std::move(batch));
// result.value() == Anzahl erfolgreich eingereihter Events

// Manueller sofortiger Flush (z.B. für Tests / Graceful Shutdown)
mgr->flush();

// Statistiken
auto s = mgr->stats();
// s.events_ingested   – Gesamtanzahl akzeptierter Events
// s.events_flushed    – In RocksDB persistierte Events
// s.backpressure_waits – Wie oft ingest() auf Pufferplatz warten musste
// s.dropped_events    – Verworfene Events (nur bei OVERFLOW_DROP)

mgr->stop();  // Drainiert den Buffer, beendet den Flush-Thread
```

### Konfigurationsparameter

| Parameter | Standard | Beschreibung |
|-----------|---------|-------------|
| `flush_interval` | 10 ms | Flush-Frequenz des Background-Threads |
| `max_buffer_events` | 1 000 000 | Pufferkapazität; bei Überschreitung greift `overflow_policy` |
| `max_batch_size` | 65 536 | Max. Events pro RocksDB-WriteBatch |
| `backpressure_timeout` | 0 (unbegrenzt) | Timeout für `BLOCK`-Policy; 0 = ewig warten |
| `overflow_policy` | `BLOCK` | `BLOCK` blockiert den Aufrufer; `DROP` verwirft Events |
| `sync_wal` | `true` | Synchroner WAL-Flush pro Batch |

### OverflowPolicy

- **BLOCK:** Der Aufrufer wartet, bis Pufferplatz vorhanden ist (subject to `backpressure_timeout`).  Bei Ablauf des Timeouts wird `ERR_STORAGE_LOG_FULL` zurückgegeben.
- **DROP:** Das Event wird verworfen, `dropped_events`-Counter wird erhöht; kein Fehler.

### Integration mit CDC

StreamingIngestManager und CDC ergänzen sich:  Während CDC Downstream-Consumer über Änderungen informiert, ist StreamingIngestManager die Upstream-Schnittstelle für hochvolumige Schreibquellen wie IoT-Sensoren, Log-Aggregatoren oder Metriken-Exporteure.

```mermaid
graph LR
    Sensors -->|"ingest(key, value)"| SIM[StreamingIngestManager]
    SIM -->|WriteBatch| RDB[(RocksDB)]
    RDB -->|CDC Events| Consumers[Analytics / Alerting]
```

Abb. 11.8: StreamingIngestManager als Upstream-Ingest-Pipeline

## Zusammenfassung

In diesem Kapitel haben Sie gelernt:

1. **Change Data Capture (CDC)**: Datenänderungen verfolgen und darauf reagieren
2. **Server-Sent Events (SSE)**: Unidirektionale Push-Updates
3. **WebSockets**: Bidirektionale Realtime-Kommunikation
4. **Event-Driven Architecture**: Entkoppelte, skalierbare Systeme
5. **Realtime Chat**: Vollständige Implementierung mit allen Features
6. **Kanban Board**: Collaborative Drag & Drop mit Sync
7. **Best Practices**: Connection Management, Rate Limiting, Monitoring
8. **Performance**: Batching, Pooling, Caching
9. **StreamingIngestManager**: Hochdurchsatz-Ingest ≥ 1 M Events/s mit Backpressure

Realtime-Anwendungen mit ThemisDB sind einfach zu implementieren dank:
- Integriertem CDC-System
- MVCC für konfliktfreie Reads
- Flexible Event-Streaming-Mechanismen
- Starke Transaktionsgarantien

Im nächsten Kapitel schauen wir uns Computer Vision-Anwendungen an, wo wir Bilder speichern, analysieren und durchsuchen.

## Übungen

1. **Chat-Erweiterung**: Fügen Sie Voice Messages und File Sharing hinzu
2. **Kanban-Analytics**: Cycle Time und Lead Time berechnen
3. **Presence-System**: Implementieren Sie "Alice is viewing card #42"
4. **Notification-System**: Push-Benachrichtigungen bei @mentions
5. **Realtime-Dashboard**: Live-Metrics mit automatischer Aktualisierung
6. **Collaborative Editor**: Google Docs-ähnliche Textbearbeitung
7. **Gaming**: Einfaches Multiplayer-Spiel mit ThemisDB als Backend
8. **Live-Poll**: Echtzeit-Abstimmungssystem mit automatischen Updates
9. **Activity-Feed**: Timeline aller Aktivitäten im System
10. **Realtime-Search**: Live-Search-Results während dem Tippen

---

## 11.9 Ingestion-Modul — Multi-Source Daten-Intake (v1.5)

Das Ingestion-Modul (`include/ingestion/`, `src/ingestion/`) implementiert eine produktionsreife, mehrquellen-fähige Daten-Intake-Pipeline: Kafka, S3/GCS/Azure, REST-API, HuggingFace-Datasets, Filesystem, WebCrawler, CDC (PostgreSQL), JDBC/ODBC und ein Plugin-API für eigene Konnektoren.

### 11.9.1 IngestionBuilder — Fluent API

```cpp
#include "ingestion/ingestion_manager.h"

auto manager = themis::ingestion::IngestionBuilder("themisdb://localhost:8765")
    // ── HuggingFace Dataset ───────────────────────────────────────────
    .withHuggingFaceSource("hf-legal", "lexlms/ger_legal_data",
        { {"split", "train"}, {"token", hf_token} }, /*priority=*/8)

    // ── Filesystem (HTML/XML via pugixml) ─────────────────────────────
    .withFilesystemSource("fs-docs", "/data/documents",
        { {"recursive", "true"}, {"format", "html"} })

    // ── REST API (cursor-based pagination) ────────────────────────────
    .withApiSource("api-contracts", "https://api.example.com/contracts",
        { {"api_key", api_key}, {"pagination_mode", "cursor"},
          {"cursor_param", "next_cursor"}, {"page_size", "200"} })

    // ── Kafka Consumer ────────────────────────────────────────────────
    .withKafkaSource("kafka-orders", kafka_config,
        { {"group_id", "themis-ingestor"}, {"topic", "orders"} })

    // ── Object Storage (S3/GCS/Azure Blob) ───────────────────────────
    .withObjectStorageSource("s3-docs", s3_config,
        { {"bucket", "my-documents"}, {"prefix", "legal/2026/"} })

    // ── PostgreSQL CDC (Logical Replication) ──────────────────────────
    .withCdcSource("pg-cdc", cdc_config,
        { {"slot_name", "themis_slot"}, {"publication", "all_tables"} })

    // ── Globale Konfiguration ─────────────────────────────────────────
    .withRateLimitConfig({ .tokens_per_second = 500 })
    .withDryRun(false)
    .withSchemaValidation("hf-legal", legal_schema_json)
    .build();

// Ingestion starten
manager->start();

// Prometheus-Metriken exportieren
auto metrics = manager->exportMetrics();
// docs_processed, errors, throughput_per_sec, quarantine_queue_size

// Quelle pausieren/fortsetzen
manager->pauseSource("kafka-orders");
manager->resumeSource("kafka-orders");
```

### 11.9.2 Quarantine + Retry

```
Dokument schlägt fehl →
  Quarantine Queue (Exponentielles Back-off: 1s→2s→4s→... max 10 Versuche)
  ↓ nach max_retries
  Dead-Letter-Speicherung + Prometheus-Alert
```

**Checkpoint-basierte Incremental Ingestion:** Der Fortschritt wird pro Quelle persistiert. Neustart setzt ab letztem Checkpoint fort – kein Re-Processing.
