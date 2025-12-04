# Native Binary Client Libraries - Multi-Language Roadmap

**Ziel**: Alle Major-Sprachen erhalten native binary wire protocol clients (5-10x schneller als HTTP/REST)

**Datum**: 4. Dezember 2025  
**Status**: Python Complete - 4 Languages Pending

---

## Overview

| Language | Status | Priority | Target Performance | Maintainer |
|----------|--------|----------|-------------------|------------|
| **Python** | ✅ Complete | Critical | 5-10x vs httpx | @core-team |
| **JavaScript/Node.js** | 🔄 In Progress | High | 5-10x vs axios | TBD |
| **Java** | 📋 Planned | High | 5-10x vs OkHttp | TBD |
| **Go** | 📋 Planned | Medium | 5-10x vs net/http | TBD |
| **Rust** | 📋 Planned | Medium | 5-10x vs reqwest | TBD |
| **C#/.NET** | 🔮 Future | Low | 5-10x vs HttpClient | TBD |
| **C++** | 🔮 Future | Low | Direct library link | TBD |

---

## 1. JavaScript/TypeScript Client (`themis-native.js`)

### Architecture
```
clients/javascript/
├── package.json
├── tsconfig.json
├── src/
│   ├── index.ts                  # Main export
│   ├── ThemisNativeClient.ts     # Client implementation
│   ├── WireFrame.ts              # Binary framing
│   ├── OpCode.ts                 # Operation codes
│   ├── proto/
│   │   └── themis_wire_v1.proto  # Symlink to ../../src/network/
│   └── generated/
│       └── themis_wire_v1_pb.js  # protobuf.js generated
├── test/
│   └── integration.test.ts
└── README.md
```

### Key Dependencies
```json
{
  "name": "@themis/native-client",
  "version": "1.0.0",
  "dependencies": {
    "protobufjs": "^7.2.5",
    "crc-32": "^1.2.2"
  },
  "devDependencies": {
    "typescript": "^5.3.0",
    "@types/node": "^20.10.0",
    "vitest": "^1.0.0"
  }
}
```

### Core Implementation (TypeScript)
```typescript
// src/ThemisNativeClient.ts
import * as net from 'net';
import * as protobuf from 'protobufjs';
import { CRC32 } from 'crc-32';

export class ThemisNativeClient {
  private socket: net.Socket;
  private host: string;
  private port: number;
  private authenticated: boolean = false;

  constructor(host: string = 'localhost', port: number = 8766) {
    this.host = host;
    this.port = port;
  }

  async connect(username: string, password: string): Promise<void> {
    return new Promise((resolve, reject) => {
      this.socket = net.createConnection({ host: this.host, port: this.port });
      
      this.socket.on('connect', async () => {
        try {
          await this.sendHello();
          await this.authenticate(username, password);
          resolve();
        } catch (err) {
          reject(err);
        }
      });

      this.socket.on('error', reject);
    });
  }

  async get(model: string, collection: string, uuid: string): Promise<any> {
    const request = {
      model,
      collection,
      uuid
    };
    
    const response = await this.sendReceive(OpCode.GET, request, 'GetResponse');
    return response.found ? JSON.parse(response.jsonData) : null;
  }

  async vectorSearch(
    collection: string,
    vector: number[],
    k: number = 10,
    distanceMetric: string = 'cosine'
  ): Promise<any[]> {
    const request = {
      collection,
      queryVector: vector,
      topK: k,
      distanceMetric
    };

    const response = await this.sendReceive(
      OpCode.VECTOR_SEARCH,
      request,
      'VectorSearchResponse'
    );

    return response.matches.map(m => ({
      uuid: m.uuid,
      distance: m.distance,
      entity: JSON.parse(m.jsonData)
    }));
  }

  private async sendReceive(
    opcode: OpCode,
    message: any,
    responseType: string
  ): Promise<any> {
    // Serialize with Protocol Buffers
    const root = await protobuf.load('proto/themis_wire_v1.proto');
    const MessageType = root.lookupType(message.constructor.name);
    const buffer = MessageType.encode(message).finish();

    // Build wire frame
    const header = Buffer.alloc(12);
    header.writeUInt32BE(0x544D4442, 0);  // Magic "TMDB"
    header.writeUInt8(1, 4);              // Version
    header.writeUInt16BE(opcode, 5);      // OpCode
    header.writeUInt8(0, 7);              // Flags
    header.writeUInt32BE(buffer.length, 8); // Payload length

    const checksum = CRC32.buf(buffer);
    const checksumBuffer = Buffer.alloc(4);
    checksumBuffer.writeUInt32BE(checksum, 0);

    // Send frame
    this.socket.write(Buffer.concat([header, buffer, checksumBuffer]));

    // Receive response
    return this.receiveMessage(responseType);
  }

  private async receiveMessage(expectedType: string): Promise<any> {
    // Read header (12 bytes)
    const header = await this.readExact(12);
    const payloadLength = header.readUInt32BE(8);

    // Read payload
    const payload = await this.readExact(payloadLength);

    // Read checksum
    const checksum = await this.readExact(4);

    // Verify checksum
    const computed = CRC32.buf(payload);
    if (computed !== checksum.readUInt32BE(0)) {
      throw new Error('Checksum mismatch');
    }

    // Deserialize
    const root = await protobuf.load('proto/themis_wire_v1.proto');
    const MessageType = root.lookupType(expectedType);
    return MessageType.decode(payload);
  }

  private async readExact(n: number): Promise<Buffer> {
    return new Promise((resolve, reject) => {
      let buffer = Buffer.alloc(0);
      const onData = (chunk: Buffer) => {
        buffer = Buffer.concat([buffer, chunk]);
        if (buffer.length >= n) {
          this.socket.off('data', onData);
          resolve(buffer.slice(0, n));
        }
      };
      this.socket.on('data', onData);
    });
  }
}
```

### Performance Target
- **GET operation**: 0.3ms (vs 1.5ms HTTP)
- **Vector search**: 2.0ms (vs 10ms HTTP)
- **Query (AQL)**: 1.5ms (vs 8ms HTTP)

### Testing
```typescript
// test/integration.test.ts
import { describe, it, expect, beforeAll, afterAll } from 'vitest';
import { ThemisNativeClient } from '../src';

describe('ThemisNativeClient Integration', () => {
  let client: ThemisNativeClient;

  beforeAll(async () => {
    client = new ThemisNativeClient('localhost', 8766);
    await client.connect('admin', 'admin123');
  });

  it('should perform GET operation in <1ms', async () => {
    const start = performance.now();
    const entity = await client.get('Document', 'users', 'user123');
    const duration = performance.now() - start;
    
    expect(duration).toBeLessThan(1.0);
    expect(entity).toBeDefined();
  });

  afterAll(async () => {
    await client.close();
  });
});
```

---

## 2. Java Client (`themis-native-java`)

### Architecture
```
clients/java/
├── pom.xml
├── src/
│   ├── main/
│   │   ├── java/
│   │   │   └── com/themisdb/client/
│   │   │       ├── ThemisNativeClient.java
│   │   │       ├── WireFrame.java
│   │   │       ├── OpCode.java
│   │   │       └── ConnectionPool.java
│   │   └── proto/
│   │       └── themis_wire_v1.proto
│   └── test/
│       └── java/com/themisdb/client/
│           └── IntegrationTest.java
└── README.md
```

### Maven Dependencies (`pom.xml`)
```xml
<dependencies>
  <dependency>
    <groupId>com.google.protobuf</groupId>
    <artifactId>protobuf-java</artifactId>
    <version>3.25.1</version>
  </dependency>
  <dependency>
    <groupId>org.apache.commons</groupId>
    <artifactId>commons-pool2</artifactId>
    <version>2.12.0</version>
  </dependency>
</dependencies>

<build>
  <plugins>
    <plugin>
      <groupId>org.xolstice.maven.plugins</groupId>
      <artifactId>protobuf-maven-plugin</artifactId>
      <version>0.6.1</version>
    </plugin>
  </plugins>
</build>
```

### Core Implementation
```java
// src/main/java/com/themisdb/client/ThemisNativeClient.java
package com.themisdb.client;

import java.io.*;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.util.zip.CRC32;

public class ThemisNativeClient implements AutoCloseable {
    private final String host;
    private final int port;
    private Socket socket;
    private DataInputStream input;
    private DataOutputStream output;
    private boolean authenticated = false;

    public ThemisNativeClient(String host, int port) {
        this.host = host;
        this.port = port;
    }

    public void connect(String username, String password) throws IOException {
        socket = new Socket(host, port);
        input = new DataInputStream(socket.getInputStream());
        output = new DataOutputStream(socket.getOutputStream());
        
        sendHello();
        authenticate(username, password);
    }

    public Entity get(String model, String collection, String uuid) throws IOException {
        GetRequest request = GetRequest.newBuilder()
            .setModel(model)
            .setCollection(collection)
            .setUuid(uuid)
            .build();
        
        GetResponse response = sendReceive(OpCode.GET, request, GetResponse.parser());
        return response.getFound() ? Entity.fromJson(response.getJsonData()) : null;
    }

    public List<VectorMatch> vectorSearch(
        String collection,
        float[] vector,
        int k,
        DistanceMetric metric
    ) throws IOException {
        VectorSearchRequest request = VectorSearchRequest.newBuilder()
            .setCollection(collection)
            .addAllQueryVector(Floats.asList(vector))
            .setTopK(k)
            .setDistanceMetric(metric)
            .build();

        VectorSearchResponse response = sendReceive(
            OpCode.VECTOR_SEARCH,
            request,
            VectorSearchResponse.parser()
        );

        return response.getMatchesList().stream()
            .map(m -> new VectorMatch(m.getUuid(), m.getDistance(), Entity.fromJson(m.getJsonData())))
            .collect(Collectors.toList());
    }

    private <T> T sendReceive(
        OpCode opcode,
        Message request,
        Parser<T> responseParser
    ) throws IOException {
        // Serialize request
        byte[] payload = request.toByteArray();

        // Build wire frame header
        ByteBuffer header = ByteBuffer.allocate(12);
        header.putInt(0x544D4442);           // Magic "TMDB"
        header.put((byte) 1);                // Version
        header.putShort(opcode.getValue());  // OpCode
        header.put((byte) 0);                // Flags
        header.putInt(payload.length);       // Payload length

        // Compute checksum
        CRC32 crc = new CRC32();
        crc.update(payload);
        int checksum = (int) crc.getValue();

        // Send frame
        output.write(header.array());
        output.write(payload);
        output.writeInt(checksum);
        output.flush();

        // Receive response
        return receiveMessage(responseParser);
    }

    private <T> T receiveMessage(Parser<T> parser) throws IOException {
        // Read header
        byte[] headerBytes = readExact(12);
        ByteBuffer header = ByteBuffer.wrap(headerBytes);
        int magic = header.getInt();
        if (magic != 0x544D4442) {
            throw new IOException("Invalid magic: " + Integer.toHexString(magic));
        }
        
        header.get(); // version
        header.getShort(); // opcode
        header.get(); // flags
        int payloadLength = header.getInt();

        // Read payload
        byte[] payload = readExact(payloadLength);

        // Read checksum
        int checksum = input.readInt();

        // Verify checksum
        CRC32 crc = new CRC32();
        crc.update(payload);
        if ((int) crc.getValue() != checksum) {
            throw new IOException("Checksum mismatch");
        }

        // Parse response
        return parser.parseFrom(payload);
    }

    private byte[] readExact(int n) throws IOException {
        byte[] buffer = new byte[n];
        int offset = 0;
        while (offset < n) {
            int read = input.read(buffer, offset, n - offset);
            if (read == -1) throw new EOFException();
            offset += read;
        }
        return buffer;
    }

    @Override
    public void close() throws IOException {
        if (socket != null) {
            socket.close();
        }
    }
}
```

### Connection Pooling
```java
// src/main/java/com/themisdb/client/ConnectionPool.java
import org.apache.commons.pool2.impl.GenericObjectPool;

public class ThemisConnectionPool {
    private final GenericObjectPool<ThemisNativeClient> pool;

    public ThemisConnectionPool(String host, int port, int maxConnections) {
        ThemisClientFactory factory = new ThemisClientFactory(host, port);
        pool = new GenericObjectPool<>(factory);
        pool.setMaxTotal(maxConnections);
    }

    public ThemisNativeClient borrowClient() throws Exception {
        return pool.borrowObject();
    }

    public void returnClient(ThemisNativeClient client) {
        pool.returnObject(client);
    }
}
```

---

## 3. Go Client (`themis-native-go`)

### Architecture
```
clients/go/
├── go.mod
├── themis/
│   ├── client.go
│   ├── wire_frame.go
│   ├── opcode.go
│   └── proto/
│       └── themis_wire_v1.pb.go
├── examples/
│   └── simple_get.go
└── README.md
```

### Dependencies (`go.mod`)
```go
module github.com/themisdb/themis-go

go 1.21

require (
    google.golang.org/protobuf v1.31.0
    github.com/klauspost/compress v1.17.0  // LZ4
)
```

### Core Implementation
```go
// themis/client.go
package themis

import (
    "encoding/binary"
    "hash/crc32"
    "net"
    pb "github.com/themisdb/themis-go/proto"
    "google.golang.org/protobuf/proto"
)

type Client struct {
    conn          net.Conn
    authenticated bool
    username      string
}

func NewClient(host string, port int) (*Client, error) {
    conn, err := net.Dial("tcp", fmt.Sprintf("%s:%d", host, port))
    if err != nil {
        return nil, err
    }
    return &Client{conn: conn}, nil
}

func (c *Client) Connect(username, password string) error {
    if err := c.sendHello(); err != nil {
        return err
    }
    return c.authenticate(username, password)
}

func (c *Client) Get(model, collection, uuid string) (*Entity, error) {
    req := &pb.GetRequest{
        Model:      model,
        Collection: collection,
        Uuid:       uuid,
    }
    
    resp := &pb.GetResponse{}
    if err := c.sendReceive(OpCodeGet, req, resp); err != nil {
        return nil, err
    }
    
    if !resp.Found {
        return nil, nil
    }
    
    return EntityFromJSON(resp.JsonData), nil
}

func (c *Client) VectorSearch(
    collection string,
    vector []float32,
    k int,
    metric pb.DistanceMetric,
) ([]*VectorMatch, error) {
    req := &pb.VectorSearchRequest{
        Collection:     collection,
        QueryVector:    vector,
        TopK:          int32(k),
        DistanceMetric: metric,
    }
    
    resp := &pb.VectorSearchResponse{}
    if err := c.sendReceive(OpCodeVectorSearch, req, resp); err != nil {
        return nil, err
    }
    
    matches := make([]*VectorMatch, len(resp.Matches))
    for i, m := range resp.Matches {
        matches[i] = &VectorMatch{
            UUID:     m.Uuid,
            Distance: m.Distance,
            Entity:   EntityFromJSON(m.JsonData),
        }
    }
    return matches, nil
}

func (c *Client) sendReceive(opcode OpCode, req, resp proto.Message) error {
    // Serialize request
    payload, err := proto.Marshal(req)
    if err != nil {
        return err
    }
    
    // Build header
    header := make([]byte, 12)
    binary.BigEndian.PutUint32(header[0:4], 0x544D4442) // Magic
    header[4] = 1                                        // Version
    binary.BigEndian.PutUint16(header[5:7], uint16(opcode))
    header[7] = 0                                        // Flags
    binary.BigEndian.PutUint32(header[8:12], uint32(len(payload)))
    
    // Compute checksum
    checksum := crc32.ChecksumIEEE(payload)
    checksumBytes := make([]byte, 4)
    binary.BigEndian.PutUint32(checksumBytes, checksum)
    
    // Send frame
    if _, err := c.conn.Write(header); err != nil {
        return err
    }
    if _, err := c.conn.Write(payload); err != nil {
        return err
    }
    if _, err := c.conn.Write(checksumBytes); err != nil {
        return err
    }
    
    // Receive response
    return c.receiveMessage(resp)
}

func (c *Client) receiveMessage(resp proto.Message) error {
    // Read header
    header := make([]byte, 12)
    if _, err := io.ReadFull(c.conn, header); err != nil {
        return err
    }
    
    payloadLength := binary.BigEndian.Uint32(header[8:12])
    
    // Read payload
    payload := make([]byte, payloadLength)
    if _, err := io.ReadFull(c.conn, payload); err != nil {
        return err
    }
    
    // Read checksum
    checksumBytes := make([]byte, 4)
    if _, err := io.ReadFull(c.conn, checksumBytes); err != nil {
        return err
    }
    
    // Verify checksum
    checksum := binary.BigEndian.Uint32(checksumBytes)
    if crc32.ChecksumIEEE(payload) != checksum {
        return errors.New("checksum mismatch")
    }
    
    // Unmarshal
    return proto.Unmarshal(payload, resp)
}

func (c *Client) Close() error {
    return c.conn.Close()
}
```

### Goroutine-Safe Connection Pool
```go
// themis/pool.go
type ConnectionPool struct {
    connections chan *Client
    host        string
    port        int
}

func NewConnectionPool(host string, port, poolSize int) *ConnectionPool {
    pool := &ConnectionPool{
        connections: make(chan *Client, poolSize),
        host:        host,
        port:        port,
    }
    
    for i := 0; i < poolSize; i++ {
        client, _ := NewClient(host, port)
        pool.connections <- client
    }
    
    return pool
}

func (p *ConnectionPool) Get() *Client {
    return <-p.connections
}

func (p *ConnectionPool) Put(c *Client) {
    p.connections <- c
}
```

---

## 4. Rust Client (`themis-native-rs`)

### Architecture
```
clients/rust/
├── Cargo.toml
├── src/
│   ├── lib.rs
│   ├── client.rs
│   ├── wire_frame.rs
│   ├── opcode.rs
│   └── proto/
│       └── themis_wire_v1.rs
└── examples/
    └── simple_get.rs
```

### Dependencies (`Cargo.toml`)
```toml
[package]
name = "themis-client"
version = "1.0.0"
edition = "2021"

[dependencies]
tokio = { version = "1.35", features = ["full"] }
prost = "0.12"
crc32fast = "1.3"
bytes = "1.5"

[build-dependencies]
prost-build = "0.12"
```

### Core Implementation
```rust
// src/client.rs
use tokio::net::TcpStream;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use prost::Message;
use crc32fast::Hasher;

pub struct ThemisClient {
    stream: TcpStream,
    authenticated: bool,
}

impl ThemisClient {
    pub async fn connect(host: &str, port: u16) -> Result<Self, Box<dyn std::error::Error>> {
        let stream = TcpStream::connect((host, port)).await?;
        Ok(Self {
            stream,
            authenticated: false,
        })
    }

    pub async fn authenticate(&mut self, username: &str, password: &str) -> Result<(), Box<dyn std::error::Error>> {
        self.send_hello().await?;
        
        let req = proto::AuthRequest {
            username: username.to_string(),
            password_hash: hash_password(password),
            namespace: "default".to_string(),
        };
        
        let resp: proto::AuthSuccess = self.send_receive(OpCode::AuthRequest, req).await?;
        self.authenticated = true;
        Ok(())
    }

    pub async fn get(&mut self, model: &str, collection: &str, uuid: &str) -> Result<Option<Entity>, Box<dyn std::error::Error>> {
        let req = proto::GetRequest {
            model: model.to_string(),
            collection: collection.to_string(),
            uuid: uuid.to_string(),
        };
        
        let resp: proto::GetResponse = self.send_receive(OpCode::Get, req).await?;
        
        if resp.found {
            Ok(Some(Entity::from_json(&resp.json_data)))
        } else {
            Ok(None)
        }
    }

    pub async fn vector_search(
        &mut self,
        collection: &str,
        vector: Vec<f32>,
        k: i32,
        metric: proto::DistanceMetric,
    ) -> Result<Vec<VectorMatch>, Box<dyn std::error::Error>> {
        let req = proto::VectorSearchRequest {
            collection: collection.to_string(),
            query_vector: vector,
            top_k: k,
            distance_metric: metric as i32,
        };
        
        let resp: proto::VectorSearchResponse = self.send_receive(OpCode::VectorSearch, req).await?;
        
        Ok(resp.matches.into_iter().map(|m| VectorMatch {
            uuid: m.uuid,
            distance: m.distance,
            entity: Entity::from_json(&m.json_data),
        }).collect())
    }

    async fn send_receive<Req, Resp>(&mut self, opcode: OpCode, req: Req) -> Result<Resp, Box<dyn std::error::Error>>
    where
        Req: Message,
        Resp: Message + Default,
    {
        // Serialize request
        let mut payload = Vec::new();
        req.encode(&mut payload)?;
        
        // Build header
        let mut header = vec![0u8; 12];
        header[0..4].copy_from_slice(&0x544D4442u32.to_be_bytes()); // Magic
        header[4] = 1;                                               // Version
        header[5..7].copy_from_slice(&(opcode as u16).to_be_bytes());
        header[7] = 0;                                               // Flags
        header[8..12].copy_from_slice(&(payload.len() as u32).to_be_bytes());
        
        // Compute checksum
        let mut hasher = Hasher::new();
        hasher.update(&payload);
        let checksum = hasher.finalize();
        
        // Send frame
        self.stream.write_all(&header).await?;
        self.stream.write_all(&payload).await?;
        self.stream.write_all(&checksum.to_be_bytes()).await?;
        
        // Receive response
        self.receive_message().await
    }

    async fn receive_message<Resp: Message + Default>(&mut self) -> Result<Resp, Box<dyn std::error::Error>> {
        // Read header
        let mut header = vec![0u8; 12];
        self.stream.read_exact(&mut header).await?;
        
        let payload_length = u32::from_be_bytes([header[8], header[9], header[10], header[11]]) as usize;
        
        // Read payload
        let mut payload = vec![0u8; payload_length];
        self.stream.read_exact(&mut payload).await?;
        
        // Read checksum
        let mut checksum_bytes = [0u8; 4];
        self.stream.read_exact(&mut checksum_bytes).await?;
        let checksum = u32::from_be_bytes(checksum_bytes);
        
        // Verify checksum
        let mut hasher = Hasher::new();
        hasher.update(&payload);
        if hasher.finalize() != checksum {
            return Err("Checksum mismatch".into());
        }
        
        // Decode
        Ok(Resp::decode(&payload[..])?)
    }
}
```

---

## 5. Implementation Timeline

### Month 1: JavaScript + Java (High Priority)
- **Week 1**: JavaScript/TypeScript implementation + testing
- **Week 2**: Java implementation + connection pooling
- **Week 3**: Integration tests + benchmarks
- **Week 4**: Documentation + npm/Maven publishing

### Month 2: Go + Rust (Medium Priority)
- **Week 1**: Go implementation + goroutine-safe pooling
- **Week 2**: Rust async implementation with Tokio
- **Week 3**: Integration tests + benchmarks
- **Week 4**: Documentation + crates.io publishing

### Month 3: C# + Performance Tuning
- **Week 1-2**: C#/.NET implementation
- **Week 3**: Cross-language performance benchmarks
- **Week 4**: Publish all clients + unified documentation

---

## 6. Success Metrics

### Performance Targets (vs HTTP clients):
| Operation | HTTP Latency | Native Target | Speedup |
|-----------|--------------|---------------|---------|
| GET | 1.5ms | 0.3ms | **5x** |
| PUT | 2.0ms | 0.4ms | **5x** |
| Query (AQL) | 8ms | 1.5ms | **5.3x** |
| Vector Search | 10ms | 2ms | **5x** |
| Geo Query | 6ms | 1ms | **6x** |

### Code Quality:
- ✅ 90%+ test coverage
- ✅ Zero-copy deserialization (where possible)
- ✅ Connection pooling (Java, Go, C#)
- ✅ Async I/O (JavaScript, Rust)
- ✅ Type-safe APIs (TypeScript, Java, Rust)

---

## Next Steps

1. **Immediate**: Start JavaScript/TypeScript client implementation
2. **Week 1**: Complete JavaScript + publish to npm as `@themis/native-client`
3. **Week 2**: Complete Java + publish to Maven Central
4. **Week 3**: Benchmarks showing 5-10x improvements across all languages
5. **Month 3**: All 6 languages production-ready + unified docs

**Status**: Ready to implement JavaScript client first 🚀
