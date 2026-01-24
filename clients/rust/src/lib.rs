use std::{collections::HashMap, sync::Arc, time::Duration};

use reqwest::{Client, Method, Response};
use serde::{de::DeserializeOwned, Deserialize, Serialize};
use serde_json::{json, Map, Value};
use thiserror::Error;
use tokio::sync::RwLock;
use tokio::time::sleep;

const JSON_CONTENT_TYPE: &str = "application/json";

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ThemisClientConfig {
    pub endpoints: Vec<String>,
    #[serde(default = "default_namespace")]
    pub namespace: String,
    #[serde(default = "default_timeout_ms")]
    pub timeout_ms: u64,
    pub metadata_endpoint: Option<String>,
    #[serde(default = "default_max_retries")]
    pub max_retries: usize,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub circuit_breaker: Option<CircuitBreakerConfig>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub logging: Option<LoggingConfig>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CircuitBreakerConfig {
    pub enabled: bool,
    #[serde(default = "default_failure_threshold")]
    pub failure_threshold: usize,
    #[serde(default = "default_reset_timeout_secs")]
    pub reset_timeout_secs: u64,
    #[serde(default = "default_half_open_max_requests")]
    pub half_open_max_requests: usize,
}

impl Default for CircuitBreakerConfig {
    fn default() -> Self {
        Self {
            enabled: true,
            failure_threshold: default_failure_threshold(),
            reset_timeout_secs: default_reset_timeout_secs(),
            half_open_max_requests: default_half_open_max_requests(),
        }
    }
}

fn default_failure_threshold() -> usize {
    5
}

fn default_reset_timeout_secs() -> u64 {
    60
}

fn default_half_open_max_requests() -> usize {
    3
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LoggingConfig {
    pub enabled: bool,
    pub log_requests: bool,
    pub log_responses: bool,
}

impl Default for LoggingConfig {
    fn default() -> Self {
        Self {
            enabled: true,
            log_requests: true,
            log_responses: true,
        }
    }
}

impl Default for ThemisClientConfig {
    fn default() -> Self {
        Self {
            endpoints: Vec::new(),
            namespace: default_namespace(),
            timeout_ms: default_timeout_ms(),
            metadata_endpoint: None,
            max_retries: default_max_retries(),
            circuit_breaker: None,
            logging: None,
        }
    }
}

fn default_namespace() -> String {
    "default".to_string()
}

fn default_timeout_ms() -> u64 {
    30_000
}

fn default_max_retries() -> usize {
    3
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct BatchGetResult<T> {
    pub found: HashMap<String, T>,
    pub missing: Vec<String>,
    pub errors: HashMap<String, String>,
}

impl<T> Default for BatchGetResult<T> {
    fn default() -> Self {
        Self {
            found: HashMap::new(),
            missing: Vec::new(),
            errors: HashMap::new(),
        }
    }
}

#[derive(Debug, Clone, Default, Serialize, Deserialize)]
pub struct QueryOptions {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub params: Option<Map<String, Value>>,
    #[serde(default)]
    pub use_cursor: bool,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub cursor: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub batch_size: Option<u32>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct QueryResult<T> {
    pub items: Vec<T>,
    pub has_more: bool,
    pub next_cursor: Option<String>,
    pub raw: Value,
}

impl<T> Default for QueryResult<T> {
    fn default() -> Self {
        Self {
            items: Vec::new(),
            has_more: false,
            next_cursor: None,
            raw: Value::Null,
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LlmMessage {
    pub role: String,
    pub content: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub image_url: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ReasoningStep {
    pub r#type: String,
    pub content: Vec<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LlmInteraction {
    pub id: String,
    pub created_at: String,
    pub model: String,
    pub messages: Vec<LlmMessage>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub reasoning_steps: Option<Vec<ReasoningStep>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub metadata: Option<Map<String, Value>>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LlmInteractionResult {
    pub id: String,
    pub success: bool,
}

#[derive(Debug, Clone, Default, Serialize, Deserialize)]
pub struct LlmInteractionOptions {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub reasoning_steps: Option<Vec<ReasoningStep>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub metadata: Option<Map<String, Value>>,
}

#[derive(Debug, Clone, Default)]
pub struct ListLlmInteractionsOptions {
    pub model: Option<String>,
    pub limit: Option<u32>,
    pub offset: Option<u32>,
}

#[derive(Debug, Error)]
pub enum ThemisError {
    #[error("invalid configuration: {0}")]
    InvalidConfig(String),
    #[error("topology error: {0}")]
    Topology(String),
    #[error("http error: {status} {body}")]
    Http { status: u16, body: String },
    #[error(transparent)]
    Transport(#[from] reqwest::Error),
    #[error("serialization error: {0}")]
    Serde(String),
    #[error("transaction error: {0}")]
    Transaction(String),
}

pub type Result<T> = std::result::Result<T, ThemisError>;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CircuitBreakerState {
    Closed,
    Open,
    HalfOpen,
}

pub struct CircuitBreaker {
    state: Arc<RwLock<CircuitBreakerState>>,
    failure_count: Arc<RwLock<usize>>,
    success_count: Arc<RwLock<usize>>,
    next_attempt_time: Arc<RwLock<std::time::Instant>>,
    config: CircuitBreakerConfig,
}

impl CircuitBreaker {
    pub fn new(config: CircuitBreakerConfig) -> Self {
        Self {
            state: Arc::new(RwLock::new(CircuitBreakerState::Closed)),
            failure_count: Arc::new(RwLock::new(0)),
            success_count: Arc::new(RwLock::new(0)),
            next_attempt_time: Arc::new(RwLock::new(std::time::Instant::now())),
            config,
        }
    }

    pub async fn can_execute(&self) -> bool {
        let state = *self.state.read().await;
        
        match state {
            CircuitBreakerState::Closed => true,
            CircuitBreakerState::Open => {
                let next_attempt = *self.next_attempt_time.read().await;
                if std::time::Instant::now() >= next_attempt {
                    *self.state.write().await = CircuitBreakerState::HalfOpen;
                    *self.success_count.write().await = 0;
                    true
                } else {
                    false
                }
            }
            CircuitBreakerState::HalfOpen => {
                let success_count = *self.success_count.read().await;
                success_count < self.config.half_open_max_requests
            }
        }
    }

    pub async fn record_success(&self) {
        *self.failure_count.write().await = 0;
        
        let state = *self.state.read().await;
        if state == CircuitBreakerState::HalfOpen {
            let mut success_count = self.success_count.write().await;
            *success_count += 1;
            if *success_count >= self.config.half_open_max_requests {
                *self.state.write().await = CircuitBreakerState::Closed;
            }
        }
    }

    pub async fn record_failure(&self) {
        let mut failure_count = self.failure_count.write().await;
        *failure_count += 1;
        *self.success_count.write().await = 0;
        
        if *failure_count >= self.config.failure_threshold {
            *self.state.write().await = CircuitBreakerState::Open;
            let reset_duration = Duration::from_secs(self.config.reset_timeout_secs);
            *self.next_attempt_time.write().await = std::time::Instant::now() + reset_duration;
        }
    }

    pub async fn get_state(&self) -> CircuitBreakerState {
        *self.state.read().await
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
pub enum IsolationLevel {
    ReadCommitted,
    Snapshot,
}

impl Default for IsolationLevel {
    fn default() -> Self {
        IsolationLevel::ReadCommitted
    }
}

#[derive(Debug, Clone)]
pub struct TransactionOptions {
    pub isolation_level: IsolationLevel,
    pub timeout_ms: Option<u64>,
}

impl Default for TransactionOptions {
    fn default() -> Self {
        Self {
            isolation_level: IsolationLevel::default(),
            timeout_ms: None,
        }
    }
}

#[derive(Clone)]
pub struct ThemisClient {
    http: Client,
    config: ThemisClientConfig,
    topology: Arc<RwLock<Option<Vec<String>>>>,
    circuit_breaker: Option<Arc<CircuitBreaker>>,
}

impl ThemisClient {
    pub fn new(config: ThemisClientConfig) -> Result<Self> {
        if config.endpoints.is_empty() {
            return Err(ThemisError::InvalidConfig(
                "endpoints must not be empty".into(),
            ));
        }
        let client = Client::builder()
            .timeout(Duration::from_millis(config.timeout_ms))
            .build()?;
        
        let circuit_breaker = config
            .circuit_breaker
            .as_ref()
            .filter(|cb| cb.enabled)
            .map(|cb| Arc::new(CircuitBreaker::new(cb.clone())));
        
        Ok(Self {
            http: client,
            config,
            topology: Arc::new(RwLock::new(None)),
            circuit_breaker,
        })
    }
    
    /// Get the circuit breaker state
    pub async fn get_circuit_breaker_state(&self) -> Option<CircuitBreakerState> {
        if let Some(cb) = &self.circuit_breaker {
            Some(cb.get_state().await)
        } else {
            None
        }
    }
    
    /// Log a message if logging is enabled
    fn log(&self, level: &str, message: &str) {
        if let Some(logging) = &self.config.logging {
            if logging.enabled {
                println!("[ThemisDB] [{}] {}", level, message);
            }
        }
    }

    pub async fn health(&self) -> Result<Value> {
        let endpoint = self
            .config
            .endpoints
            .first()
            .cloned()
            .ok_or_else(|| ThemisError::InvalidConfig("endpoints must not be empty".into()))?;
        let url = format!("{endpoint}/health");
        let response = self.request(Method::GET, url, None).await?;
        ensure_success(response)
            .await?
            .json::<Value>()
            .await
            .map_err(|err| ThemisError::Serde(err.to_string()))
    }

    pub async fn get<T>(&self, model: &str, collection: &str, uuid: &str) -> Result<Option<T>>
    where
        T: DeserializeOwned,
    {
        let endpoint = self.resolve_endpoint(model, collection, uuid).await?;
        let key = build_entity_key(model, &self.config.namespace, collection, uuid);
        let url = format!("{endpoint}/entities/{key}");
        let response = self.request(Method::GET, url, None).await?;
        if response.status() == 404 {
            return Ok(None);
        }
        let payload = ensure_success(response)
            .await?
            .json::<Value>()
            .await
            .map_err(|err| ThemisError::Serde(err.to_string()))?;
        decode_entity::<T>(payload).map(Some)
    }

    pub async fn put<T>(&self, model: &str, collection: &str, uuid: &str, data: &T) -> Result<()>
    where
        T: Serialize,
    {
        let endpoint = self.resolve_endpoint(model, collection, uuid).await?;
        let key = build_entity_key(model, &self.config.namespace, collection, uuid);
        let url = format!("{endpoint}/entities/{key}");
        let body = json!({ "blob": encode_entity(data)? });
        let response = self
            .request(
                Method::PUT,
                url,
                Some(RequestBody {
                    body: Some(body.to_string()),
                    content_type: Some(JSON_CONTENT_TYPE.to_string()),
                }),
            )
            .await?;
        ensure_success(response).await.map(|_| ())
    }

    pub async fn delete(&self, model: &str, collection: &str, uuid: &str) -> Result<bool> {
        let endpoint = self.resolve_endpoint(model, collection, uuid).await?;
        let key = build_entity_key(model, &self.config.namespace, collection, uuid);
        let url = format!("{endpoint}/entities/{key}");
        let response = self.request(Method::DELETE, url, None).await?;
        if response.status() == 404 {
            return Ok(false);
        }
        ensure_success(response).await.map(|_| true)
    }

    pub async fn batch_get<T>(
        &self,
        model: &str,
        collection: &str,
        uuids: &[String],
    ) -> Result<BatchGetResult<T>>
    where
        T: DeserializeOwned,
    {
        let mut result = BatchGetResult::default();
        for uuid in uuids {
            match self.get::<T>(model, collection, uuid).await {
                Ok(Some(value)) => {
                    result.found.insert(uuid.clone(), value);
                }
                Ok(None) => result.missing.push(uuid.clone()),
                Err(err) => {
                    result.errors.insert(uuid.clone(), format!("{err}"));
                }
            }
        }
        Ok(result)
    }

    pub async fn query<T>(&self, aql: &str, options: QueryOptions) -> Result<QueryResult<T>>
    where
        T: DeserializeOwned,
    {
        let mut payload = Map::new();
        payload.insert("query".into(), Value::String(aql.to_string()));
        if let Some(params) = options.params {
            payload.insert("params".into(), Value::Object(params));
        }
        if options.use_cursor {
            payload.insert("use_cursor".into(), Value::Bool(true));
        }
        if let Some(cursor) = options.cursor {
            payload.insert("cursor".into(), Value::String(cursor));
        }
        if let Some(batch_size) = options.batch_size {
            payload.insert("batch_size".into(), Value::Number(batch_size.into()));
        }

        let endpoints = if is_single_shard_query(aql) {
            vec![self.resolve_query_endpoint(aql).await?]
        } else {
            self.current_endpoints().await?
        };

        let mut partials = Vec::new();
        for endpoint in endpoints {
            let url = format!("{endpoint}/query/aql");
            let response = self
                .request(
                    Method::POST,
                    url,
                    Some(RequestBody {
                        body: Some(Value::Object(payload.clone()).to_string()),
                        content_type: Some(JSON_CONTENT_TYPE.to_string()),
                    }),
                )
                .await?;
            let payload = ensure_success(response)
                .await?
                .json::<Value>()
                .await
                .map_err(|err| ThemisError::Serde(err.to_string()))?;
            partials.push(parse_query_result::<T>(payload)?);
        }

        if partials.is_empty() {
            return Ok(QueryResult::default());
        }
        if partials.len() == 1 {
            return Ok(partials.into_iter().next().unwrap());
        }
        let mut items = Vec::new();
        let mut has_more = false;
        let mut raw_partials = Vec::new();
        for mut part in partials {
            has_more |= part.has_more;
            items.append(&mut part.items);
            raw_partials.push(part.raw);
        }
        Ok(QueryResult {
            items,
            has_more,
            next_cursor: None,
            raw: Value::Array(raw_partials),
        })
    }

    pub async fn vector_search(
        &self,
        embedding: &[f32],
        filter: Option<Value>,
        top_k: Option<u32>,
    ) -> Result<Value> {
        let mut payload = Map::new();
        payload.insert(
            "vector".into(),
            Value::Array(embedding.iter().map(|v| Value::from(*v)).collect()),
        );
        if let Some(filter) = filter {
            payload.insert("filter".into(), filter);
        }
        if let Some(top_k) = top_k {
            payload.insert("k".into(), Value::from(top_k));
        }

        let endpoints = self.current_endpoints().await?;
        let mut results = Vec::new();
        let mut raw = Vec::new();
        for endpoint in endpoints {
            let url = format!("{endpoint}/vector/search");
            let response = self
                .request(
                    Method::POST,
                    url,
                    Some(RequestBody {
                        body: Some(Value::Object(payload.clone()).to_string()),
                        content_type: Some(JSON_CONTENT_TYPE.to_string()),
                    }),
                )
                .await?;
            let value = ensure_success(response)
                .await?
                .json::<Value>()
                .await
                .map_err(|err| ThemisError::Serde(err.to_string()))?;
            if let Some(items) = value.get("results").and_then(|v| v.as_array()) {
                results.extend(items.clone());
            }
            raw.push(value);
        }
        results.sort_by(|a, b| {
            score_value(b)
                .partial_cmp(&score_value(a))
                .unwrap_or(std::cmp::Ordering::Equal)
        });
        if let Some(k) = top_k {
            let limit = k as usize;
            if results.len() > limit {
                results.truncate(limit);
            }
        }
        Ok(json!({ "results": results, "partials": raw }))
    }

    // ==================== LLM API ====================

    pub async fn llm_interaction(
        &self,
        model: &str,
        messages: Vec<LlmMessage>,
        options: Option<LlmInteractionOptions>,
    ) -> Result<LlmInteractionResult> {
        let mut body = json!({
            "model": model,
            "messages": messages,
        });

        if let Some(opts) = options {
            if let Some(reasoning_steps) = opts.reasoning_steps {
                body["reasoning_steps"] = json!(reasoning_steps);
            }
            if let Some(metadata) = opts.metadata {
                body["metadata"] = json!(metadata);
            }
        }

        let endpoint = &self.config.endpoints[0];
        let url = format!("{}/llm/interaction", endpoint.trim_end_matches('/'));
        let response = self
            .request(
                Method::POST,
                url,
                Some(RequestBody {
                    body: Some(body.to_string()),
                    content_type: Some(JSON_CONTENT_TYPE.to_string()),
                }),
            )
            .await?;

        let result = ensure_success(response)
            .await?
            .json::<LlmInteractionResult>()
            .await
            .map_err(|err| ThemisError::Serde(err.to_string()))?;

        Ok(result)
    }

    pub async fn get_llm_interaction(&self, interaction_id: &str) -> Result<Option<LlmInteraction>> {
        let endpoint = &self.config.endpoints[0];
        let url = format!("{}/llm/interaction/{}", endpoint.trim_end_matches('/'), interaction_id);
        let response = self.request(Method::GET, url, None).await?;

        if response.status() == 404 {
            return Ok(None);
        }

        let interaction = ensure_success(response)
            .await?
            .json::<LlmInteraction>()
            .await
            .map_err(|err| ThemisError::Serde(err.to_string()))?;

        Ok(Some(interaction))
    }

    pub async fn list_llm_interactions(
        &self,
        options: Option<ListLlmInteractionsOptions>,
    ) -> Result<Vec<LlmInteraction>> {
        let endpoint = &self.config.endpoints[0];
        let mut url = format!("{}/llm/interaction", endpoint.trim_end_matches('/'));

        if let Some(opts) = options {
            let mut params = Vec::new();
            if let Some(model) = opts.model {
                params.push(format!("model={}", model));
            }
            if let Some(limit) = opts.limit {
                params.push(format!("limit={}", limit));
            }
            if let Some(offset) = opts.offset {
                params.push(format!("offset={}", offset));
            }
            if !params.is_empty() {
                url = format!("{}?{}", url, params.join("&"));
            }
        }

        let response = self.request(Method::GET, url, None).await?;

        #[derive(Deserialize)]
        struct ListResponse {
            interactions: Vec<LlmInteraction>,
        }

        let list_response = ensure_success(response)
            .await?
            .json::<ListResponse>()
            .await
            .map_err(|err| ThemisError::Serde(err.to_string()))?;

        Ok(list_response.interactions)
    }

    // ==================== Graph API ====================

    pub async fn graph_traverse(
        &self,
        start_node: &str,
        max_depth: Option<u32>,
        edge_type: Option<&str>,
    ) -> Result<Value> {
        let mut payload = Map::new();
        payload.insert("start".into(), Value::String(start_node.to_string()));
        payload.insert("max_depth".into(), Value::from(max_depth.unwrap_or(3)));
        if let Some(edge_type) = edge_type {
            payload.insert("edge_type".into(), Value::String(edge_type.to_string()));
        }

        let endpoint = self
            .config
            .endpoints
            .first()
            .cloned()
            .ok_or_else(|| ThemisError::InvalidConfig("endpoints must not be empty".into()))?;

        let url = format!("{endpoint}/graph/traverse");
        let response = self
            .request(
                Method::POST,
                url,
                Some(RequestBody {
                    body: Some(Value::Object(payload).to_string()),
                    content_type: Some(JSON_CONTENT_TYPE.to_string()),
                }),
            )
            .await?;
        ensure_success(response)
            .await?
            .json::<Value>()
            .await
            .map_err(|err| ThemisError::Serde(err.to_string()))
    }

    pub async fn shortest_path(
        &self,
        from: &str,
        to: &str,
        edge_type: Option<&str>,
    ) -> Result<Vec<String>> {
        let mut payload = Map::new();
        payload.insert("from".into(), Value::String(from.to_string()));
        payload.insert("to".into(), Value::String(to.to_string()));
        if let Some(edge_type) = edge_type {
            payload.insert("edge_type".into(), Value::String(edge_type.to_string()));
        }

        let endpoint = self
            .config
            .endpoints
            .first()
            .cloned()
            .ok_or_else(|| ThemisError::InvalidConfig("endpoints must not be empty".into()))?;

        let url = format!("{endpoint}/graph/shortest-path");
        let response = self
            .request(
                Method::POST,
                url,
                Some(RequestBody {
                    body: Some(Value::Object(payload).to_string()),
                    content_type: Some(JSON_CONTENT_TYPE.to_string()),
                }),
            )
            .await?;

        let result = ensure_success(response)
            .await?
            .json::<Value>()
            .await
            .map_err(|err| ThemisError::Serde(err.to_string()))?;

        let path = result
            .get("path")
            .and_then(|v| v.as_array())
            .map(|arr| arr.iter().filter_map(|v| v.as_str().map(String::from)).collect())
            .unwrap_or_default();

        Ok(path)
    }

    pub async fn neighbors(
        &self,
        node_id: &str,
        direction: Option<&str>,
        edge_type: Option<&str>,
        limit: Option<u32>,
    ) -> Result<Vec<String>> {
        let mut payload = Map::new();
        payload.insert("node".into(), Value::String(node_id.to_string()));
        if let Some(direction) = direction {
            payload.insert("direction".into(), Value::String(direction.to_string()));
        }
        if let Some(edge_type) = edge_type {
            payload.insert("edge_type".into(), Value::String(edge_type.to_string()));
        }
        if let Some(limit) = limit {
            payload.insert("limit".into(), Value::from(limit));
        }

        let endpoint = self
            .config
            .endpoints
            .first()
            .cloned()
            .ok_or_else(|| ThemisError::InvalidConfig("endpoints must not be empty".into()))?;

        let url = format!("{endpoint}/graph/neighbors");
        let response = self
            .request(
                Method::POST,
                url,
                Some(RequestBody {
                    body: Some(Value::Object(payload).to_string()),
                    content_type: Some(JSON_CONTENT_TYPE.to_string()),
                }),
            )
            .await?;

        let result = ensure_success(response)
            .await?
            .json::<Value>()
            .await
            .map_err(|err| ThemisError::Serde(err.to_string()))?;

        let neighbors = result
            .get("neighbors")
            .and_then(|v| v.as_array())
            .map(|arr| arr.iter().filter_map(|v| v.as_str().map(String::from)).collect())
            .unwrap_or_default();

        Ok(neighbors)
    }

    async fn current_endpoints(&self) -> Result<Vec<String>> {
        self.ensure_topology().await?;
        let topo = self.topology.read().await;
        Ok(topo
            .clone()
            .unwrap_or_else(|| self.config.endpoints.clone()))
    }

    async fn resolve_endpoint(&self, model: &str, collection: &str, uuid: &str) -> Result<String> {
        let endpoints = self.current_endpoints().await?;
        if endpoints.is_empty() {
            return Err(ThemisError::Topology("no endpoints available".into()));
        }
        let index = stable_hash(&build_urn(model, &self.config.namespace, collection, uuid))
            % endpoints.len();
        Ok(endpoints[index].clone())
    }

    async fn resolve_query_endpoint(&self, aql: &str) -> Result<String> {
        let endpoints = self.current_endpoints().await?;
        if endpoints.is_empty() {
            return Err(ThemisError::Topology("no endpoints available".into()));
        }
        let index = stable_hash(aql) % endpoints.len();
        Ok(endpoints[index].clone())
    }

    async fn ensure_topology(&self) -> Result<()> {
        {
            let topo = self.topology.read().await;
            if topo.is_some() {
                return Ok(());
            }
        }
        let mut topo = self.topology.write().await;
        if topo.is_some() {
            return Ok(());
        }
        match self.fetch_topology().await {
            Ok(shards) if !shards.is_empty() => {
                *topo = Some(shards);
                Ok(())
            }
            Ok(_) => Err(ThemisError::Topology(
                "topology response missing shards".into(),
            )),
            Err(err) => {
                *topo = Some(self.config.endpoints.clone());
                Err(err)
            }
        }
    }

    async fn fetch_topology(&self) -> Result<Vec<String>> {
        let url = self.metadata_url()?;
        let response = self.request(Method::GET, url, None).await?;
        let payload = ensure_success(response)
            .await?
            .json::<Value>()
            .await
            .map_err(|err| ThemisError::Serde(err.to_string()))?;
        Ok(extract_endpoints(&payload))
    }

    fn metadata_url(&self) -> Result<String> {
        if let Some(endpoint) = &self.config.metadata_endpoint {
            if endpoint.starts_with("http") {
                return Ok(endpoint.clone());
            }
            let base = self.config.endpoints.first().unwrap();
            return Ok(format!("{base}{endpoint}"));
        }
        let base = self.config.endpoints.first().unwrap();
        Ok(format!("{base}/_admin/cluster/topology"))
    }

    pub async fn begin_transaction(&self, options: TransactionOptions) -> Result<Transaction> {
        let endpoint = self
            .config
            .endpoints
            .first()
            .cloned()
            .ok_or_else(|| ThemisError::InvalidConfig("endpoints must not be empty".into()))?;
        
        let mut payload = Map::new();
        payload.insert(
            "isolation_level".into(),
            serde_json::to_value(options.isolation_level)
                .map_err(|e| ThemisError::Serde(e.to_string()))?,
        );
        if let Some(timeout) = options.timeout_ms {
            payload.insert("timeout".into(), Value::Number(timeout.into()));
        }

        let url = format!("{endpoint}/transaction/begin");
        let response = self
            .request(
                Method::POST,
                url,
                Some(RequestBody {
                    body: Some(Value::Object(payload).to_string()),
                    content_type: Some(JSON_CONTENT_TYPE.to_string()),
                }),
            )
            .await?;

        let result = ensure_success(response)
            .await?
            .json::<Value>()
            .await
            .map_err(|err| ThemisError::Serde(err.to_string()))?;

        let transaction_id = result
            .get("transaction_id")
            .and_then(|v| v.as_str())
            .ok_or_else(|| ThemisError::Transaction("missing transaction_id in response".into()))?
            .to_string();

        Ok(Transaction {
            client: self.clone(),
            transaction_id,
            is_active: true,
        })
    }

    async fn request(
        &self,
        method: Method,
        url: String,
        body: Option<RequestBody>,
    ) -> Result<Response> {
        self.request_with_headers(method, url, body, None).await
    }

    async fn request_with_headers(
        &self,
        method: Method,
        url: String,
        body: Option<RequestBody>,
        headers: Option<HashMap<String, String>>,
    ) -> Result<Response> {
        // Check circuit breaker
        if let Some(cb) = &self.circuit_breaker {
            if !cb.can_execute().await {
                self.log("ERROR", &format!("Circuit breaker is OPEN for {}", url));
                return Err(ThemisError::InvalidConfig(
                    "Circuit breaker is OPEN".into(),
                ));
            }
        }

        // Log request
        if let Some(logging) = &self.config.logging {
            if logging.enabled && logging.log_requests {
                self.log("INFO", &format!("{} {}", method, url));
            }
        }

        let mut attempt = 0usize;
        let max_attempts = self.config.max_retries.max(1);
        let mut last_error = None;

        loop {
            let mut builder = self.http.request(method.clone(), &url);
            if let Some(body) = &body {
                if let Some(content_type) = &body.content_type {
                    builder = builder.header("Content-Type", content_type);
                }
                if let Some(payload) = &body.body {
                    builder = builder.body(payload.clone());
                }
            }
            if let Some(headers) = &headers {
                for (key, value) in headers {
                    builder = builder.header(key, value);
                }
            }

            let result = builder.send().await;
            match result {
                Ok(response) => {
                    let status = response.status();

                    // Log response
                    if let Some(logging) = &self.config.logging {
                        if logging.enabled && logging.log_responses {
                            self.log("INFO", &format!("{} {} -> {}", method, url, status));
                        }
                    }

                    if status.is_server_error() && attempt + 1 < max_attempts {
                        // Record failure in circuit breaker
                        if let Some(cb) = &self.circuit_breaker {
                            cb.record_failure().await;
                        }
                        self.log("WARN", &format!("Server error {}, retrying...", status));
                        attempt += 1;
                        sleep(backoff(attempt)).await;
                        continue;
                    }

                    // Record success in circuit breaker
                    if let Some(cb) = &self.circuit_breaker {
                        cb.record_success().await;
                    }

                    return Ok(response);
                }
                Err(err) => {
                    // Record failure in circuit breaker
                    if let Some(cb) = &self.circuit_breaker {
                        cb.record_failure().await;
                    }

                    self.log("ERROR", &format!("Request failed: {}", err));

                    if attempt + 1 >= max_attempts || !should_retry(&err) {
                        return Err(ThemisError::Transport(err));
                    }
                    attempt += 1;
                    last_error = Some(err);
                    sleep(backoff(attempt)).await;
                }
            }
        }
    }
}

pub struct Transaction {
    client: ThemisClient,
    transaction_id: String,
    is_active: bool,
}

impl Transaction {
    pub fn transaction_id(&self) -> &str {
        &self.transaction_id
    }

    pub fn is_active(&self) -> bool {
        self.is_active
    }

    fn ensure_active(&self) -> Result<()> {
        if !self.is_active {
            return Err(ThemisError::Transaction(
                "transaction is not active".into(),
            ));
        }
        Ok(())
    }

    fn transaction_headers(&self) -> HashMap<String, String> {
        let mut headers = HashMap::new();
        headers.insert("X-Transaction-Id".to_string(), self.transaction_id.clone());
        headers
    }

    pub async fn get<T>(&self, model: &str, collection: &str, uuid: &str) -> Result<Option<T>>
    where
        T: DeserializeOwned,
    {
        self.ensure_active()?;
        let endpoint = self.client.resolve_endpoint(model, collection, uuid).await?;
        let key = build_entity_key(model, &self.client.config.namespace, collection, uuid);
        let url = format!("{endpoint}/entities");
        let body = Some(RequestBody {
            content_type: Some(JSON_CONTENT_TYPE.to_string()),
            body: Some(json!({ "key": key }).to_string()),
        });
        let response = self
            .client
            .request_with_headers(Method::GET, url, body, Some(self.transaction_headers()))
            .await?;
        if response.status() == 404 {
            return Ok(None);
        }
        let payload = ensure_success(response)
            .await?
            .json::<Value>()
            .await
            .map_err(|err| ThemisError::Serde(err.to_string()))?;
        decode_entity::<T>(payload).map(Some)
    }

    pub async fn put<T>(&self, model: &str, collection: &str, uuid: &str, data: &T) -> Result<()>
    where
        T: Serialize,
    {
        self.ensure_active()?;
        let endpoint = self.client.resolve_endpoint(model, collection, uuid).await?;
        let key = build_entity_key(model, &self.client.config.namespace, collection, uuid);
        let url = format!("{endpoint}/entities/{key}");
        let body = json!({ "blob": encode_entity(data)? });
        let response = self
            .client
            .request_with_headers(
                Method::PUT,
                url,
                Some(RequestBody {
                    body: Some(body.to_string()),
                    content_type: Some(JSON_CONTENT_TYPE.to_string()),
                }),
                Some(self.transaction_headers()),
            )
            .await?;
        ensure_success(response).await.map(|_| ())
    }

    pub async fn delete(&self, model: &str, collection: &str, uuid: &str) -> Result<bool> {
        self.ensure_active()?;
        let endpoint = self.client.resolve_endpoint(model, collection, uuid).await?;
        let key = build_entity_key(model, &self.client.config.namespace, collection, uuid);
        let url = format!("{endpoint}/entities/{key}");
        let response = self
            .client
            .request_with_headers(Method::DELETE, url, None, Some(self.transaction_headers()))
            .await?;
        if response.status() == 404 {
            return Ok(false);
        }
        ensure_success(response).await.map(|_| true)
    }

    pub async fn query<T>(&self, aql: &str, options: QueryOptions) -> Result<QueryResult<T>>
    where
        T: DeserializeOwned,
    {
        self.ensure_active()?;
        let mut payload = Map::new();
        payload.insert("query".into(), Value::String(aql.to_string()));
        if let Some(params) = options.params {
            payload.insert("params".into(), Value::Object(params));
        }
        if options.use_cursor {
            payload.insert("use_cursor".into(), Value::Bool(true));
        }
        if let Some(cursor) = options.cursor {
            payload.insert("cursor".into(), Value::String(cursor));
        }
        if let Some(batch_size) = options.batch_size {
            payload.insert("batch_size".into(), Value::Number(batch_size.into()));
        }

        let endpoint = self
            .client
            .config
            .endpoints
            .first()
            .cloned()
            .ok_or_else(|| ThemisError::InvalidConfig("endpoints must not be empty".into()))?;

        let url = format!("{endpoint}/query/aql");
        let response = self
            .client
            .request_with_headers(
                Method::POST,
                url,
                Some(RequestBody {
                    body: Some(Value::Object(payload).to_string()),
                    content_type: Some(JSON_CONTENT_TYPE.to_string()),
                }),
                Some(self.transaction_headers()),
            )
            .await?;
        let payload = ensure_success(response)
            .await?
            .json::<Value>()
            .await
            .map_err(|err| ThemisError::Serde(err.to_string()))?;
        parse_query_result::<T>(payload)
    }

    pub async fn commit(mut self) -> Result<()> {
        self.ensure_active()?;
        let endpoint = self
            .client
            .config
            .endpoints
            .first()
            .cloned()
            .ok_or_else(|| ThemisError::InvalidConfig("endpoints must not be empty".into()))?;

        let url = format!("{endpoint}/transaction/commit");
        let payload = json!({ "transaction_id": self.transaction_id });
        let response = self
            .client
            .request(
                Method::POST,
                url,
                Some(RequestBody {
                    body: Some(payload.to_string()),
                    content_type: Some(JSON_CONTENT_TYPE.to_string()),
                }),
            )
            .await?;
        ensure_success(response).await?;
        self.is_active = false;
        Ok(())
    }

    pub async fn rollback(mut self) -> Result<()> {
        self.ensure_active()?;
        let endpoint = self
            .client
            .config
            .endpoints
            .first()
            .cloned()
            .ok_or_else(|| ThemisError::InvalidConfig("endpoints must not be empty".into()))?;

        let url = format!("{endpoint}/transaction/rollback");
        let payload = json!({ "transaction_id": self.transaction_id });
        let response = self
            .client
            .request(
                Method::POST,
                url,
                Some(RequestBody {
                    body: Some(payload.to_string()),
                    content_type: Some(JSON_CONTENT_TYPE.to_string()),
                }),
            )
            .await?;
        ensure_success(response).await?;
        self.is_active = false;
        Ok(())
    }
}

struct RequestBody {
    body: Option<String>,
    content_type: Option<String>,
}

async fn ensure_success(response: Response) -> Result<Response> {
    if response.status().is_success() {
        return Ok(response);
    }
    let status = response.status().as_u16();
    let body = response.text().await.unwrap_or_default();
    Err(ThemisError::Http { status, body })
}

fn extract_endpoints(payload: &Value) -> Vec<String> {
    let mut endpoints = Vec::new();
    if let Some(shards) = payload.get("shards").and_then(|v| v.as_array()) {
        for shard in shards {
            match shard {
                Value::String(endpoint) => endpoints.push(normalize(endpoint)),
                Value::Object(map) => {
                    if let Some(Value::String(endpoint)) = map.get("endpoint") {
                        endpoints.push(normalize(endpoint));
                    }
                    if let Some(Value::String(endpoint)) = map.get("http_endpoint") {
                        endpoints.push(normalize(endpoint));
                    }
                    if let Some(Value::Array(values)) = map.get("endpoints") {
                        for endpoint in values {
                            if let Value::String(endpoint) = endpoint {
                                endpoints.push(normalize(endpoint));
                            }
                        }
                    }
                }
                _ => {}
            }
        }
    }
    endpoints
}

fn normalize(endpoint: &str) -> String {
    endpoint.trim_end_matches('/').to_string()
}

fn build_urn(model: &str, namespace: &str, collection: &str, uuid: &str) -> String {
    format!("urn:themis:{model}:{namespace}:{collection}:{uuid}")
}

fn build_entity_key(model: &str, namespace: &str, collection: &str, uuid: &str) -> String {
    format!("{model}.{namespace}.{collection}:{uuid}")
}

fn encode_entity<T: Serialize>(value: &T) -> Result<String> {
    match serde_json::to_value(value) {
        Ok(Value::String(inner)) => Ok(inner),
        Ok(other) => {
            serde_json::to_string(&other).map_err(|err| ThemisError::Serde(err.to_string()))
        }
        Err(err) => Err(ThemisError::Serde(err.to_string())),
    }
}

fn decode_entity<T: DeserializeOwned>(payload: Value) -> Result<T> {
    if let Some(entity) = payload.get("entity") {
        serde_json::from_value(entity.clone()).map_err(|err| ThemisError::Serde(err.to_string()))
    } else if let Some(blob) = payload.get("blob").and_then(|v| v.as_str()) {
        serde_json::from_str(blob).map_err(|err| ThemisError::Serde(err.to_string()))
    } else {
        serde_json::from_value(payload).map_err(|err| ThemisError::Serde(err.to_string()))
    }
}

fn decode_entities<T: DeserializeOwned>(values: &[Value]) -> Result<Vec<T>> {
    values
        .iter()
        .map(|value| decode_entity::<T>(value.clone()))
        .collect()
}

fn parse_query_result<T: DeserializeOwned>(payload: Value) -> Result<QueryResult<T>> {
    if let Some(entities) = payload.get("entities").and_then(|v| v.as_array()) {
        let items = decode_entities::<T>(entities)?;
        return Ok(QueryResult {
            items,
            has_more: false,
            next_cursor: None,
            raw: payload,
        });
    }
    let items = payload
        .get("items")
        .and_then(|v| v.as_array())
        .map(|values| decode_entities::<T>(values))
        .transpose()?;
    let has_more = payload
        .get("has_more")
        .and_then(|v| v.as_bool())
        .unwrap_or(false);
    let next_cursor = payload
        .get("next_cursor")
        .and_then(|v| v.as_str())
        .map(|s| s.to_string());
    Ok(QueryResult {
        items: items.unwrap_or_default(),
        has_more,
        next_cursor,
        raw: payload,
    })
}

fn is_single_shard_query(aql: &str) -> bool {
    aql.to_lowercase().contains("urn:themis:")
}

fn stable_hash(input: &str) -> usize {
    let mut hash: u64 = 2166136261;
    for byte in input.as_bytes() {
        hash ^= *byte as u64;
        hash = hash.wrapping_mul(16777619);
    }
    (hash & 0x7FFF_FFFF) as usize
}

fn should_retry(err: &reqwest::Error) -> bool {
    err.is_timeout() || err.is_connect()
}

fn backoff(attempt: usize) -> Duration {
    let exponent = attempt.min(5) as u32;
    let millis = 50u64 * (1u64 << exponent);
    Duration::from_millis(millis.min(1_000))
}

fn score_value(value: &Value) -> f64 {
    if let Some(score) = value.get("score").and_then(|v| v.as_f64()) {
        return score;
    }
    if let Some(distance) = value.get("distance").and_then(|v| v.as_f64()) {
        return -distance;
    }
    0.0
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde::{Deserialize, Serialize};

    #[tokio::test]
    async fn stable_hash_is_deterministic() {
        let value = stable_hash("urn:themis:relational:default:users:1");
        assert_eq!(value, stable_hash("urn:themis:relational:default:users:1"));
        assert_ne!(value, stable_hash("urn:themis:relational:default:users:2"));
    }

    #[test]
    fn normalize_trims_slash() {
        assert_eq!(normalize("http://example.com/"), "http://example.com");
    }

    #[test]
    fn build_urn_formats_values() {
        let urn = build_urn("relational", "default", "users", "550e8400");
        assert_eq!(urn, "urn:themis:relational:default:users:550e8400");
    }

    #[test]
    fn build_entity_key_matches_format() {
        let key = build_entity_key("graph", "tenant", "nodes", "1234");
        assert_eq!(key, "graph.tenant.nodes:1234");
    }

    #[test]
    fn score_value_prefers_higher_score() {
        let higher = json!({ "score": 0.8 });
        let lower = json!({ "score": 0.1 });
        assert!(score_value(&higher) > score_value(&lower));
    }

    #[test]
    fn score_value_uses_distance_when_score_missing() {
        let nearer = json!({ "distance": 0.2 });
        let farther = json!({ "distance": 0.9 });
        assert!(score_value(&nearer) > score_value(&farther));
    }

    #[test]
    fn backoff_caps_at_one_second() {
        assert_eq!(backoff(1), Duration::from_millis(100));
        assert_eq!(backoff(5), Duration::from_millis(1_000));
        assert_eq!(backoff(10), Duration::from_millis(1_000));
    }

    #[derive(Debug, Serialize, Deserialize, PartialEq)]
    struct TestEntity {
        name: String,
    }

    #[test]
    fn encode_entity_handles_string_passthrough() {
        let value = String::from("raw-json-string");
        let encoded = encode_entity(&value).expect("encode succeeds");
        assert_eq!(encoded, "raw-json-string");
    }

    #[test]
    fn encode_entity_serializes_struct() {
        let entity = TestEntity {
            name: "Alice".into(),
        };
        let encoded = encode_entity(&entity).expect("encode succeeds");
        assert_eq!(encoded, r#"{"name":"Alice"}"#);
    }

    #[test]
    fn decode_entity_reads_entity_field() {
        let payload = json!({ "entity": { "name": "Alice" } });
        let entity: TestEntity = decode_entity(payload).expect("decode succeeds");
        assert_eq!(entity, TestEntity { name: "Alice".into() });
    }

    #[test]
    fn decode_entity_reads_blob_field() {
        let payload = json!({ "blob": "{\"name\":\"Bob\"}" });
        let entity: TestEntity = decode_entity(payload).expect("decode succeeds");
        assert_eq!(entity, TestEntity { name: "Bob".into() });
    }

    #[test]
    fn decode_entity_falls_back_to_root() {
        let payload = json!({ "name": "Clara" });
        let entity: TestEntity = decode_entity(payload).expect("decode succeeds");
        assert_eq!(entity, TestEntity { name: "Clara".into() });
    }
}
