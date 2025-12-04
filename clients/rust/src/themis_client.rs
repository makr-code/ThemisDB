use std::collections::HashMap;
use std::time::Duration;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::TcpStream;
use tokio::sync::Mutex;
use serde_json::{json, JsonValue};
use std::sync::Arc;

// Wire Protocol v1 Constants
const WIRE_MAGIC: u32 = 0x544D4442; // "TMDB"
const WIRE_VERSION: u8 = 0x01;
const HEADER_SIZE: usize = 12;
const CHECKSUM_SIZE: usize = 4;
const MAX_PAYLOAD_SIZE: usize = 64 * 1024 * 1024; // 64MB

// OpCode definitions
pub mod opcode {
    pub const HELLO: u8 = 0x01;
    pub const HELLO_ACK: u8 = 0x02;
    pub const AUTH_REQUEST: u8 = 0x03;
    pub const AUTH_RESPONSE: u8 = 0x04;
    pub const AUTH_SUCCESS: u8 = 0x05;
    pub const AUTH_FAILURE: u8 = 0x06;

    pub const GET: u8 = 0x10;
    pub const PUT: u8 = 0x11;
    pub const DELETE: u8 = 0x12;
    pub const BATCH_GET: u8 = 0x13;
    pub const BATCH_PUT: u8 = 0x14;

    pub const QUERY_AQL: u8 = 0x20;
    pub const QUERY_RESULT: u8 = 0x21;
    pub const QUERY_CURSOR: u8 = 0x22;
    pub const CURSOR_NEXT: u8 = 0x23;
    pub const CURSOR_CLOSE: u8 = 0x24;

    pub const TRANSACTION_BEGIN: u8 = 0x30;
    pub const TRANSACTION_COMMIT: u8 = 0x31;
    pub const TRANSACTION_ABORT: u8 = 0x32;

    pub const VECTOR_SEARCH: u8 = 0x40;
    pub const GRAPH_TRAVERSE: u8 = 0x41;

    pub const GEO_QUERY: u8 = 0x50;
    pub const TIMESERIES_QUERY: u8 = 0x51;

    pub const BPMN_START_PROCESS: u8 = 0x60;
    pub const BPMN_TASK_COMPLETE: u8 = 0x61;
    pub const BPMN_QUERY_INSTANCE: u8 = 0x62;

    pub const ERROR: u8 = 0xF0;
    pub const OK: u8 = 0xF1;
    pub const PING: u8 = 0xFE;
    pub const CLOSE: u8 = 0xFF;
}

// Message Flags
pub mod flags {
    pub const NONE: u16 = 0x0000;
    pub const SKIP_CHECKSUM: u16 = 0x0001;
    pub const COMPRESSED: u16 = 0x0002;
    pub const ENCRYPTED: u16 = 0x0004;
}

// Error types
#[derive(Debug)]
pub enum ThemisDBError {
    ConnectionError(String),
    AuthenticationError(String),
    ProtocolError(String),
    TimeoutError,
    SerializationError(String),
}

impl std::fmt::Display for ThemisDBError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            ThemisDBError::ConnectionError(msg) => write!(f, "Connection error: {}", msg),
            ThemisDBError::AuthenticationError(msg) => write!(f, "Authentication error: {}", msg),
            ThemisDBError::ProtocolError(msg) => write!(f, "Protocol error: {}", msg),
            ThemisDBError::TimeoutError => write!(f, "Request timeout"),
            ThemisDBError::SerializationError(msg) => write!(f, "Serialization error: {}", msg),
        }
    }
}

impl std::error::Error for ThemisDBError {}

pub type Result<T> = std::result::Result<T, ThemisDBError>;

// WireFrame structure
pub struct WireFrame {
    pub version: u8,
    pub opcode: u8,
    pub flags: u16,
    pub sequence: u32,
    pub payload: Vec<u8>,
}

impl WireFrame {
    pub fn new(opcode: u8, payload: Vec<u8>, flags: u16, sequence: u32) -> Self {
        WireFrame {
            version: WIRE_VERSION,
            opcode,
            flags,
            sequence,
            payload,
        }
    }

    pub fn to_bytes(&self) -> Result<Vec<u8>> {
        let mut bytes = Vec::new();

        // Write header
        bytes.extend_from_slice(&WIRE_MAGIC.to_be_bytes());
        bytes.push(self.version);
        bytes.push(self.opcode);
        bytes.extend_from_slice(&self.flags.to_be_bytes());
        bytes.extend_from_slice(&(self.payload.len() as u32).to_be_bytes());

        // Write payload
        bytes.extend_from_slice(&self.payload);

        // Calculate and write checksum
        if (self.flags & flags::SKIP_CHECKSUM) == 0 {
            let checksum = crc32fast::hash(&bytes);
            bytes.extend_from_slice(&checksum.to_be_bytes());
        }

        Ok(bytes)
    }

    pub fn from_bytes(data: &[u8], offset: usize) -> Result<(Self, usize)> {
        if data.len() - offset < HEADER_SIZE {
            return Err(ThemisDBError::ProtocolError("Incomplete frame".to_string()));
        }

        let mut pos = offset;

        // Read header
        let magic = u32::from_be_bytes([data[pos], data[pos + 1], data[pos + 2], data[pos + 3]]);
        pos += 4;

        if magic != WIRE_MAGIC {
            return Err(ThemisDBError::ProtocolError(format!("Invalid magic: 0x{:x}", magic)));
        }

        let version = data[pos];
        pos += 1;

        let opcode = data[pos];
        pos += 1;

        let flags = u16::from_be_bytes([data[pos], data[pos + 1]]);
        pos += 2;

        let payload_size = u32::from_be_bytes([data[pos], data[pos + 1], data[pos + 2], data[pos + 3]]);
        pos += 4;

        let payload_size = payload_size as usize;
        if payload_size > MAX_PAYLOAD_SIZE {
            return Err(ThemisDBError::ProtocolError(format!(
                "Payload too large: {}",
                payload_size
            )));
        }

        // Check if we have complete payload
        if data.len() < offset + HEADER_SIZE + payload_size {
            return Err(ThemisDBError::ProtocolError("Incomplete payload".to_string()));
        }

        // Read payload
        let payload = data[pos..pos + payload_size].to_vec();
        pos += payload_size;

        // Read and verify checksum
        if (flags & flags::SKIP_CHECKSUM) == 0 {
            if data.len() < pos + CHECKSUM_SIZE {
                return Err(ThemisDBError::ProtocolError("Missing checksum".to_string()));
            }

            let checksum = u32::from_be_bytes([data[pos], data[pos + 1], data[pos + 2], data[pos + 3]]);
            pos += CHECKSUM_SIZE;

            let header_and_payload = &data[offset..offset + HEADER_SIZE + payload_size];
            let calculated_checksum = crc32fast::hash(header_and_payload);

            if calculated_checksum != checksum {
                return Err(ThemisDBError::ProtocolError("Checksum mismatch".to_string()));
            }
        }

        Ok((
            WireFrame {
                version,
                opcode,
                flags,
                sequence: 0,
                payload,
            },
            pos,
        ))
    }
}

// ThemisDB Client
pub struct ThemisDBClient {
    host: String,
    port: u16,
    username: String,
    password: String,
    stream: Arc<Mutex<Option<TcpStream>>>,
    authenticated: Arc<Mutex<bool>>,
    sequence: Arc<Mutex<u32>>,
    pending_requests: Arc<Mutex<HashMap<u32, tokio::sync::oneshot::Sender<WireFrame>>>>,
}

impl ThemisDBClient {
    pub fn new(host: impl Into<String>, port: u16, username: impl Into<String>, password: impl Into<String>) -> Self {
        ThemisDBClient {
            host: host.into(),
            port,
            username: username.into(),
            password: password.into(),
            stream: Arc::new(Mutex::new(None)),
            authenticated: Arc::new(Mutex::new(false)),
            sequence: Arc::new(Mutex::new(0)),
            pending_requests: Arc::new(Mutex::new(HashMap::new())),
        }
    }

    pub async fn connect(&self) -> Result<()> {
        let addr = format!("{}:{}", self.host, self.port);
        let stream = TcpStream::connect(&addr)
            .await
            .map_err(|e| ThemisDBError::ConnectionError(e.to_string()))?;

        {
            let mut s = self.stream.lock().await;
            *s = Some(stream);
        }

        self.authenticate().await?;

        // Start receive loop
        let stream_clone = self.stream.clone();
        let pending_requests_clone = self.pending_requests.clone();
        tokio::spawn(async move {
            Self::receive_loop(stream_clone, pending_requests_clone).await;
        });

        Ok(())
    }

    pub async fn disconnect(&self) -> Result<()> {
        let mut stream = self.stream.lock().await;
        if let Some(mut s) = stream.take() {
            let _ = s.shutdown().await;
        }
        Ok(())
    }

    async fn authenticate(&self) -> Result<()> {
        // Send HELLO
        let hello_frame = WireFrame::new(
            opcode::HELLO,
            b"ThemisDB/1.0".to_vec(),
            flags::NONE,
            0,
        );
        self.send_frame(&hello_frame).await?;

        // Send AUTH_REQUEST
        let credentials = format!("{}:{}", self.username, self.password);
        let auth_frame = WireFrame::new(
            opcode::AUTH_REQUEST,
            credentials.into_bytes(),
            flags::NONE,
            0,
        );
        self.send_frame(&auth_frame).await?;

        *self.authenticated.lock().await = true;
        Ok(())
    }

    pub async fn get(&self, key: &str) -> Result<JsonValue> {
        let payload = json!({ "key": key });
        let frame = WireFrame::new(
            opcode::GET,
            serde_json::to_vec(&payload)
                .map_err(|e| ThemisDBError::SerializationError(e.to_string()))?,
            flags::NONE,
            self.next_sequence().await,
        );

        let response = self.send_and_wait(&frame).await?;
        serde_json::from_slice(&response.payload)
            .map_err(|e| ThemisDBError::SerializationError(e.to_string()))
    }

    pub async fn put(&self, key: &str, value: JsonValue) -> Result<()> {
        let payload = json!({ "key": key, "value": value });
        let frame = WireFrame::new(
            opcode::PUT,
            serde_json::to_vec(&payload)
                .map_err(|e| ThemisDBError::SerializationError(e.to_string()))?,
            flags::NONE,
            self.next_sequence().await,
        );

        self.send_and_wait(&frame).await?;
        Ok(())
    }

    pub async fn delete(&self, key: &str) -> Result<()> {
        let payload = json!({ "key": key });
        let frame = WireFrame::new(
            opcode::DELETE,
            serde_json::to_vec(&payload)
                .map_err(|e| ThemisDBError::SerializationError(e.to_string()))?,
            flags::NONE,
            self.next_sequence().await,
        );

        self.send_and_wait(&frame).await?;
        Ok(())
    }

    pub async fn query(&self, aql: &str, options: Option<JsonValue>) -> Result<JsonValue> {
        let mut payload = json!({ "aql": aql });
        if let Some(opts) = options {
            payload["options"] = opts;
        }

        let frame = WireFrame::new(
            opcode::QUERY_AQL,
            serde_json::to_vec(&payload)
                .map_err(|e| ThemisDBError::SerializationError(e.to_string()))?,
            flags::NONE,
            self.next_sequence().await,
        );

        let response = self.send_and_wait(&frame).await?;
        serde_json::from_slice(&response.payload)
            .map_err(|e| ThemisDBError::SerializationError(e.to_string()))
    }

    pub async fn vector_search(&self, collection: &str, vector: Vec<f64>, options: Option<JsonValue>) -> Result<JsonValue> {
        let mut opts = options.unwrap_or_else(|| json!({}));
        if !opts.get("top_k").is_some() {
            opts["top_k"] = json!(10);
        }
        if !opts.get("metric").is_some() {
            opts["metric"] = json!("cosine");
        }

        let payload = json!({
            "collection": collection,
            "vector": vector,
            "options": opts
        });

        let frame = WireFrame::new(
            opcode::VECTOR_SEARCH,
            serde_json::to_vec(&payload)
                .map_err(|e| ThemisDBError::SerializationError(e.to_string()))?,
            flags::NONE,
            self.next_sequence().await,
        );

        let response = self.send_and_wait(&frame).await?;
        serde_json::from_slice(&response.payload)
            .map_err(|e| ThemisDBError::SerializationError(e.to_string()))
    }

    pub async fn geo_query(&self, collection: &str, lat: f64, lon: f64, radius_km: f64, options: Option<JsonValue>) -> Result<JsonValue> {
        let mut payload = json!({
            "collection": collection,
            "lat": lat,
            "lon": lon,
            "radius_km": radius_km
        });
        if let Some(opts) = options {
            payload["options"] = opts;
        }

        let frame = WireFrame::new(
            opcode::GEO_QUERY,
            serde_json::to_vec(&payload)
                .map_err(|e| ThemisDBError::SerializationError(e.to_string()))?,
            flags::NONE,
            self.next_sequence().await,
        );

        let response = self.send_and_wait(&frame).await?;
        serde_json::from_slice(&response.payload)
            .map_err(|e| ThemisDBError::SerializationError(e.to_string()))
    }

    pub async fn timeseries_query(&self, collection: &str, start_time: &str, end_time: &str, options: Option<JsonValue>) -> Result<JsonValue> {
        let mut payload = json!({
            "collection": collection,
            "start_time": start_time,
            "end_time": end_time
        });
        if let Some(opts) = options {
            payload["options"] = opts;
        }

        let frame = WireFrame::new(
            opcode::TIMESERIES_QUERY,
            serde_json::to_vec(&payload)
                .map_err(|e| ThemisDBError::SerializationError(e.to_string()))?,
            flags::NONE,
            self.next_sequence().await,
        );

        let response = self.send_and_wait(&frame).await?;
        serde_json::from_slice(&response.payload)
            .map_err(|e| ThemisDBError::SerializationError(e.to_string()))
    }

    // Helper methods

    async fn next_sequence(&self) -> u32 {
        let mut seq = self.sequence.lock().await;
        *seq += 1;
        *seq
    }

    async fn send_frame(&self, frame: &WireFrame) -> Result<()> {
        let bytes = frame.to_bytes()?;
        let mut stream = self.stream.lock().await;
        if let Some(s) = stream.as_mut() {
            s.write_all(&bytes)
                .await
                .map_err(|e| ThemisDBError::ConnectionError(e.to_string()))?;
        }
        Ok(())
    }

    async fn send_and_wait(&self, frame: &WireFrame) -> Result<WireFrame> {
        let (tx, rx) = tokio::sync::oneshot::channel();

        {
            let mut pending = self.pending_requests.lock().await;
            pending.insert(frame.sequence, tx);
        }

        self.send_frame(frame).await?;

        tokio::time::timeout(Duration::from_secs(5), rx)
            .await
            .map_err(|_| ThemisDBError::TimeoutError)?
            .map_err(|_| ThemisDBError::ProtocolError("Response channel closed".to_string()))
    }

    async fn receive_loop(
        stream: Arc<Mutex<Option<TcpStream>>>,
        pending_requests: Arc<Mutex<HashMap<u32, tokio::sync::oneshot::Sender<WireFrame>>>>,
    ) {
        let mut buffer = vec![0u8; 1024 * 1024];
        let mut receive_buffer = Vec::new();

        loop {
            let mut stream_guard = stream.lock().await;
            if let Some(s) = stream_guard.as_mut() {
                match s.read(&mut buffer).await {
                    Ok(0) => break,
                    Ok(n) => {
                        drop(stream_guard);

                        receive_buffer.extend_from_slice(&buffer[..n]);

                        let mut offset = 0;
                        while offset < receive_buffer.len() {
                            match WireFrame::from_bytes(&receive_buffer, offset) {
                                Ok((frame, new_offset)) => {
                                    offset = new_offset;

                                    if frame.sequence > 0 {
                                        let mut pending = pending_requests.lock().await;
                                        if let Some(tx) = pending.remove(&frame.sequence) {
                                            let _ = tx.send(frame);
                                        }
                                    }
                                }
                                Err(_) => break,
                            }
                        }

                        receive_buffer = receive_buffer[offset..].to_vec();
                    }
                    Err(_) => break,
                }
            } else {
                break;
            }
        }
    }
}
