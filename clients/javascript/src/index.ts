const DEFAULT_METADATA_PATH = "/_admin/cluster/topology";
const HEALTH_PATH = "/health";

export interface ThemisClientConfig {
  endpoints: string[];
  namespace?: string;
  timeoutMs?: number;
  metadataEndpoint?: string;
  maxRetries?: number;
  // Connection pooling options
  pooling?: {
    maxConnections?: number;
    keepAlive?: boolean;
    keepAliveTimeout?: number;
  };
  // Circuit breaker options
  circuitBreaker?: {
    enabled?: boolean;
    failureThreshold?: number;
    resetTimeout?: number;
    halfOpenMaxRequests?: number;
  };
  // Logging options
  logging?: {
    enabled?: boolean;
    logRequests?: boolean;
    logResponses?: boolean;
    logger?: (message: string, level: "info" | "warn" | "error") => void;
  };
}

export interface BatchGetResult<T = unknown> {
  found: Record<string, T>;
  missing: string[];
  errors: Record<string, string>;
}

export interface QueryOptions {
  params?: Record<string, unknown>;
  useCursor?: boolean;
  cursor?: string;
  batchSize?: number;
}

export interface QueryResult<T = unknown> {
  items: T[];
  hasMore: boolean;
  nextCursor?: string | null;
  raw: Record<string, unknown>;
}

export interface LlmMessage {
  role: string;
  content: string;
  image_url?: string;
}

export interface ReasoningStep {
  type: string;
  content: string[];
}

export interface LlmInteraction {
  id: string;
  created_at: string;
  model: string;
  messages: LlmMessage[];
  reasoning_steps?: ReasoningStep[];
  metadata?: Record<string, unknown>;
}

export interface LlmInteractionResult {
  id: string;
  success: boolean;
}

export interface LlmInteractionOptions {
  reasoning_steps?: ReasoningStep[];
  metadata?: Record<string, unknown>;
}

export interface ListLlmInteractionsOptions {
  model?: string;
  limit?: number;
  offset?: number;
}

export class TopologyError extends Error {}

enum CircuitBreakerState {
  CLOSED = "CLOSED",
  OPEN = "OPEN",
  HALF_OPEN = "HALF_OPEN",
}

class CircuitBreaker {
  private state: CircuitBreakerState = CircuitBreakerState.CLOSED;
  private failureCount = 0;
  private successCount = 0;
  private nextAttemptTime = 0;
  
  constructor(
    private readonly failureThreshold: number,
    private readonly resetTimeoutMs: number,
    private readonly halfOpenMaxRequests: number,
  ) {}

  canExecute(): boolean {
    if (this.state === CircuitBreakerState.CLOSED) {
      return true;
    }
    if (this.state === CircuitBreakerState.OPEN) {
      if (Date.now() >= this.nextAttemptTime) {
        this.state = CircuitBreakerState.HALF_OPEN;
        this.successCount = 0;
        return true;
      }
      return false;
    }
    // HALF_OPEN state
    return this.successCount < this.halfOpenMaxRequests;
  }

  recordSuccess(): void {
    if (this.state === CircuitBreakerState.HALF_OPEN) {
      this.successCount++;
      if (this.successCount >= this.halfOpenMaxRequests) {
        this.state = CircuitBreakerState.CLOSED;
        this.failureCount = 0;
      }
    } else if (this.state === CircuitBreakerState.CLOSED) {
      this.failureCount = 0;
    }
  }

  recordFailure(): void {
    this.failureCount++;
    if (this.failureCount >= this.failureThreshold) {
      this.state = CircuitBreakerState.OPEN;
      this.nextAttemptTime = Date.now() + this.resetTimeoutMs;
    }
  }

  getState(): string {
    return this.state;
  }
}

export class ThemisClient {
  private readonly endpoints: string[];
  private readonly namespace: string;
  private readonly timeoutMs: number;
  private readonly metadataEndpoint?: string;
  private readonly maxRetries: number;
  private topologyCache: { shards: string[] } | null = null;
  private readonly circuitBreaker?: CircuitBreaker;
  private readonly loggingEnabled: boolean;
  private readonly logRequests: boolean;
  private readonly logResponses: boolean;
  private readonly logger: (message: string, level: "info" | "warn" | "error") => void;

  constructor(config: ThemisClientConfig) {
    if (!config.endpoints || config.endpoints.length === 0) {
      throw new Error("endpoints must not be empty");
    }
    this.endpoints = config.endpoints.map((e) => e.replace(/\/$/, ""));
    this.namespace = config.namespace ?? "default";
    this.timeoutMs = config.timeoutMs ?? 30_000;
    this.metadataEndpoint = config.metadataEndpoint;
    this.maxRetries = Math.max(1, config.maxRetries ?? 3);
    
    // Initialize circuit breaker if enabled
    if (config.circuitBreaker?.enabled) {
      this.circuitBreaker = new CircuitBreaker(
        config.circuitBreaker.failureThreshold ?? 5,
        config.circuitBreaker.resetTimeout ?? 60_000,
        config.circuitBreaker.halfOpenMaxRequests ?? 3,
      );
    }
    
    // Initialize logging
    this.loggingEnabled = config.logging?.enabled ?? false;
    this.logRequests = config.logging?.logRequests ?? false;
    this.logResponses = config.logging?.logResponses ?? false;
    this.logger = config.logging?.logger ?? ((msg, level) => {
      if (level === "error") console.error(`[ThemisDB] ${msg}`);
      else if (level === "warn") console.warn(`[ThemisDB] ${msg}`);
      else console.log(`[ThemisDB] ${msg}`);
    });
  }

  async health(endpoint?: string): Promise<unknown> {
    const target = normalizeEndpoint(endpoint ?? this.endpoints[0]);
    const response = await this.request("GET", `${target}${HEALTH_PATH}`);
    return response.json();
  }

  async get<T = unknown>(model: string, collection: string, uuid: string): Promise<T | null> {
    const urn = this.buildUrn(model, collection, uuid);
    const key = this.buildEntityKey(model, collection, uuid);
    const endpoint = await this.resolveEndpoint(urn);
    const response = await this.request("GET", `${endpoint}/entities/${key}`);
    if (response.status === 404) {
      return null;
    }
    if (!response.ok) {
      throw await toHttpError(response, "failed to load entity");
    }
    const payload = (await response.json()) as Record<string, unknown>;
    return decodeEntity<T>(payload);
  }

  async put(model: string, collection: string, uuid: string, data: unknown): Promise<boolean> {
    const urn = this.buildUrn(model, collection, uuid);
    const key = this.buildEntityKey(model, collection, uuid);
    const endpoint = await this.resolveEndpoint(urn);
    const response = await this.request("PUT", `${endpoint}/entities/${key}`, {
      body: JSON.stringify({ blob: encodeEntity(data) }),
      headers: { "Content-Type": "application/json" },
    });
    if (!response.ok) {
      throw await toHttpError(response, "failed to upsert entity");
    }
    return true;
  }

  async delete(model: string, collection: string, uuid: string): Promise<boolean> {
    const urn = this.buildUrn(model, collection, uuid);
    const key = this.buildEntityKey(model, collection, uuid);
    const endpoint = await this.resolveEndpoint(urn);
    const response = await this.request("DELETE", `${endpoint}/entities/${key}`);
    if (response.status === 404) {
      return false;
    }
    if (!response.ok) {
      throw await toHttpError(response, "failed to delete entity");
    }
    return true;
  }

  async batchGet<T = unknown>(model: string, collection: string, uuids: string[]): Promise<BatchGetResult<T>> {
    const result: BatchGetResult<T> = { found: {}, missing: [], errors: {} };
    for (const uuid of uuids) {
      try {
        const entity = await this.get<T>(model, collection, uuid);
        if (entity === null) {
          result.missing.push(uuid);
        } else {
          result.found[uuid] = entity;
        }
      } catch (error) {
        result.errors[uuid] = error instanceof Error ? error.message : String(error);
      }
    }
    return result;
  }

  async query<T = unknown>(aql: string, options: QueryOptions = {}): Promise<QueryResult<T>> {
    const payload: Record<string, unknown> = { query: aql };
    if (options.params) payload.params = options.params;
    if (options.useCursor) payload.use_cursor = true;
    if (options.cursor) payload.cursor = options.cursor;
    if (options.batchSize !== undefined) payload.batch_size = options.batchSize;

    const endpoints = await this.queryEndpoints(aql);
    const responses: QueryResult<T>[] = [];
    for (const endpoint of endpoints) {
      const resp = await this.request("POST", `${endpoint}/query/aql`, {
        body: JSON.stringify(payload),
        headers: { "Content-Type": "application/json" },
      });
      if (!resp.ok) {
        throw await toHttpError(resp, "query execution failed");
      }
    const data = (await resp.json()) as Record<string, unknown>;
    responses.push(parseQueryResult<T>(data));
    }

    if (responses.length === 0) {
      return { items: [], hasMore: false, nextCursor: null, raw: {} };
    }
    if (responses.length === 1) {
      return responses[0];
    }
    const items: T[] = [];
    let hasMore = false;
    for (const part of responses) {
      items.push(...part.items);
      hasMore = hasMore || part.hasMore;
    }
    return { items, hasMore, nextCursor: null, raw: { partials: responses.map((r) => r.raw) } };
  }

  // ==================== Graph API ====================

  async graphTraverse(
    startNode: string,
    options: { maxDepth?: number; edgeType?: string } = {},
  ): Promise<{ nodes: string[]; visited: string[]; edges?: Record<string, unknown>[] }> {
    const body: Record<string, unknown> = {
      start: startNode,
      max_depth: options.maxDepth ?? 3,
    };
    if (options.edgeType) body.edge_type = options.edgeType;

    const endpoint = await this.resolveEndpoint(startNode);
    const response = await this.request("POST", `${endpoint}/graph/traverse`, {
      body: JSON.stringify(body),
      headers: { "Content-Type": "application/json" },
    });

    if (!response.ok) {
      throw await toHttpError(response, "graph traversal failed");
    }

    const payload = (await response.json()) as { nodes?: string[]; visited?: string[]; edges?: Record<string, unknown>[] };
    return {
      nodes: payload.nodes ?? [],
      visited: payload.visited ?? [],
      edges: payload.edges,
    };
  }

  async shortestPath(
    from: string,
    to: string,
    options: { edgeType?: string; maxDepth?: number } = {},
  ): Promise<string[]> {
    const body: Record<string, unknown> = { from, to };
    if (options.edgeType) body.edge_type = options.edgeType;
    if (options.maxDepth) body.max_depth = options.maxDepth;

    const endpoint = this.endpoints[0];
    const response = await this.request("POST", `${endpoint}/graph/shortest-path`, {
      body: JSON.stringify(body),
      headers: { "Content-Type": "application/json" },
    });

    if (!response.ok) {
      throw await toHttpError(response, "shortest path query failed");
    }

    const payload = (await response.json()) as { path: string[] };
    return payload.path ?? [];
  }

  async neighbors(
    nodeId: string,
    options: { direction?: "in" | "out" | "both"; edgeType?: string; limit?: number } = {},
  ): Promise<string[]> {
    const body: Record<string, unknown> = { node: nodeId };
    if (options.direction) body.direction = options.direction;
    if (options.edgeType) body.edge_type = options.edgeType;
    if (options.limit) body.limit = options.limit;

    const endpoint = this.endpoints[0];
    const response = await this.request("POST", `${endpoint}/graph/neighbors`, {
      body: JSON.stringify(body),
      headers: { "Content-Type": "application/json" },
    });

    if (!response.ok) {
      throw await toHttpError(response, "neighbors query failed");
    }

    const payload = (await response.json()) as { neighbors: string[] };
    return payload.neighbors ?? [];
  }

  // ==================== Vector API ====================

  async vectorSearch(
    embedding: number[],
    options: { topK?: number; filter?: Record<string, unknown>; cursor?: string; useCursor?: boolean } = {},
  ): Promise<Record<string, unknown>> {
    const body: Record<string, unknown> = {
      vector: embedding,
      k: options.topK ?? 10,
    };
    if (options.filter) body.filter = options.filter;
    if (options.cursor) body.cursor = options.cursor;
    if (options.useCursor) body.use_cursor = true;

    const endpoints = await this.currentEndpoints();
    const payloads: Record<string, unknown>[] = [];
    for (const endpoint of endpoints) {
      const resp = await this.request("POST", `${endpoint}/vector/search`, {
        body: JSON.stringify(body),
        headers: { "Content-Type": "application/json" },
      });
      if (resp.ok) {
        payloads.push((await resp.json()) as Record<string, unknown>);
      }
    }
    if (payloads.length === 0) {
      return { results: [] };
    }
    if (payloads.length === 1) {
      return payloads[0];
    }
    const hits: Record<string, unknown>[] = [];
    for (const payload of payloads) {
      const items = (payload.results ?? payload.items) as Record<string, unknown>[] | undefined;
      if (items) hits.push(...items);
    }
    hits.sort((a, b) => {
      const scoreA = typeof a.score === "number" ? a.score : typeof a.distance === "number" ? -a.distance : 0;
      const scoreB = typeof b.score === "number" ? b.score : typeof b.distance === "number" ? -b.distance : 0;
      return scoreB - scoreA;
    });
    return { results: hits.slice(0, options.topK ?? 10), partials: payloads };
  }

  // ==================== LLM API ====================

  async llmInteraction(
    model: string,
    messages: LlmMessage[],
    options: LlmInteractionOptions = {},
  ): Promise<LlmInteractionResult> {
    const body: Record<string, unknown> = {
      model,
      messages,
    };
    if (options.reasoning_steps) {
      body.reasoning_steps = options.reasoning_steps;
    }
    if (options.metadata) {
      body.metadata = options.metadata;
    }

    const endpoint = this.endpoints[0];
    const response = await this.request("POST", `${endpoint}/llm/interaction`, {
      body: JSON.stringify(body),
      headers: { "Content-Type": "application/json" },
    });

    if (!response.ok) {
      throw await toHttpError(response, "LLM interaction failed");
    }

    const payload = (await response.json()) as LlmInteractionResult;
    return payload;
  }

  async getLlmInteraction(interactionId: string): Promise<LlmInteraction | null> {
    const endpoint = this.endpoints[0];
    const response = await this.request("GET", `${endpoint}/llm/interaction/${interactionId}`);

    if (response.status === 404) {
      return null;
    }

    if (!response.ok) {
      throw await toHttpError(response, "failed to get LLM interaction");
    }

    const payload = (await response.json()) as LlmInteraction;
    return payload;
  }

  async listLlmInteractions(
    options: ListLlmInteractionsOptions = {},
  ): Promise<LlmInteraction[]> {
    const params = new URLSearchParams();
    if (options.model) params.set("model", options.model);
    if (options.limit !== undefined) params.set("limit", options.limit.toString());
    if (options.offset !== undefined) params.set("offset", options.offset.toString());

    const endpoint = this.endpoints[0];
    const url = `${endpoint}/llm/interaction${params.toString() ? `?${params}` : ""}`;
    const response = await this.request("GET", url);

    if (!response.ok) {
      throw await toHttpError(response, "failed to list LLM interactions");
    }

    const payload = (await response.json()) as { interactions: LlmInteraction[] };
    return payload.interactions ?? [];
  }

  // ==================== Private Methods ====================

  private async currentEndpoints(): Promise<string[]> {
    await this.ensureTopology();
    return this.topologyCache?.shards ?? this.endpoints;
  }

  private async queryEndpoints(aql: string): Promise<string[]> {
    if (isSingleShardQuery(aql)) {
      const endpoint = await this.resolveQueryEndpoint(aql);
      return [endpoint];
    }
    return this.currentEndpoints();
  }

  private async resolveEndpoint(urn: string): Promise<string> {
    const endpoints = await this.currentEndpoints();
    if (endpoints.length === 0) {
      throw new TopologyError("no endpoints available");
    }
    const index = stableHash(urn) % endpoints.length;
    return endpoints[index];
  }

  private async resolveQueryEndpoint(aql: string): Promise<string> {
    const endpoints = await this.currentEndpoints();
    if (endpoints.length === 0) {
      throw new TopologyError("no endpoints available for query");
    }
    const index = stableHash(aql) % endpoints.length;
    return endpoints[index];
  }

  private async ensureTopology(): Promise<void> {
    if (this.topologyCache) {
      return;
    }
    try {
      this.topologyCache = await this.fetchTopology();
    } catch (error) {
      this.topologyCache = { shards: [...this.endpoints] };
      if (error instanceof Error) {
        throw new TopologyError(`failed to fetch topology: ${error.message}`);
      }
      throw new TopologyError("failed to fetch topology");
    }
  }

  private async fetchTopology(): Promise<{ shards: string[] }> {
    const url = this.metadataUrl();
    const response = await this.request("GET", url);
    if (!response.ok) {
      throw await toHttpError(response, "topology request failed");
    }
    const payload = (await response.json()) as Record<string, unknown>;
    const shards = extractEndpoints(payload);
    if (shards.length === 0) {
      throw new TopologyError("topology response missing shards");
    }
    return { shards };
  }

  private metadataUrl(): string {
    if (this.metadataEndpoint) {
      if (this.metadataEndpoint.startsWith("http")) {
        return this.metadataEndpoint;
      }
      return `${this.endpoints[0]}${this.metadataEndpoint}`;
    }
    return `${this.endpoints[0]}${DEFAULT_METADATA_PATH}`;
  }

  private buildUrn(model: string, collection: string, uuid: string): string {
    return `urn:themis:${model}:${this.namespace}:${collection}:${uuid}`;
  }

  private buildEntityKey(model: string, collection: string, uuid: string): string {
    return `${model}.${this.namespace}.${collection}:${uuid}`;
  }

  private async request(method: string, url: string, init: RequestInit = {}): Promise<Response> {
    // Check circuit breaker
    if (this.circuitBreaker && !this.circuitBreaker.canExecute()) {
      const error = new Error(`Circuit breaker is OPEN for endpoint: ${url}`);
      if (this.loggingEnabled) {
        this.logger(`Circuit breaker blocked request to ${url}`, "warn");
      }
      throw error;
    }

    let attempt = 0;
    let lastError: unknown;
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), this.timeoutMs);
    
    // Log request if enabled
    if (this.loggingEnabled && this.logRequests) {
      this.logger(`Request: ${method} ${url}`, "info");
    }
    
    try {
      while (attempt < this.maxRetries) {
        try {
          const resp = await getFetch()(url, {
            ...init,
            method,
            signal: controller.signal,
          });
          
          // Log response if enabled
          if (this.loggingEnabled && this.logResponses) {
            this.logger(`Response: ${method} ${url} - Status: ${resp.status}`, "info");
          }
          
          if (resp.status >= 500 && attempt + 1 < this.maxRetries) {
            attempt += 1;
            if (this.loggingEnabled) {
              this.logger(`Retry attempt ${attempt} for ${url} after 5xx error`, "warn");
            }
            await delay(2 ** attempt * 50);
            continue;
          }
          
          // Record success/failure for circuit breaker
          if (this.circuitBreaker) {
            resp.ok ? this.circuitBreaker.recordSuccess() : this.circuitBreaker.recordFailure();
          }
          
          return resp;
        } catch (error) {
          lastError = error;
          
          // Log error if enabled
          if (this.loggingEnabled) {
            this.logger(
              `Request error: ${method} ${url} - ${error instanceof Error ? error.message : String(error)}`,
              "error",
            );
          }
          
          if (!shouldRetry(error) || attempt + 1 >= this.maxRetries) {
            // Record failure for circuit breaker
            if (this.circuitBreaker) {
              this.circuitBreaker.recordFailure();
            }
            throw error;
          }
          attempt += 1;
          await delay(2 ** attempt * 50);
        }
      }
      
      // Record failure for circuit breaker
      if (this.circuitBreaker) {
        this.circuitBreaker.recordFailure();
      }
      
      throw lastError ?? new Error("request failed");
    } finally {
      clearTimeout(timeout);
    }
  }

  async beginTransaction(options?: TransactionOptions): Promise<Transaction> {
    const endpoint = this.endpoints[0];
    const body: Record<string, unknown> = {};
    if (options?.isolationLevel) {
      if (options.isolationLevel === "SNAPSHOT") {
        body.isolation = "snapshot";
      } else if (options.isolationLevel === "SERIALIZABLE") {
        body.isolation = "serializable";
      } else {
        body.isolation = "read_committed";
      }
    }
    
    const response = await this.request("POST", `${endpoint}/transaction/begin`, {
      body: JSON.stringify(body),
      headers: { "Content-Type": "application/json" },
    });
    
    if (!response.ok) {
      throw await toHttpError(response, "failed to begin transaction");
    }
    
    const payload = (await response.json()) as { transaction_id: string; isolation: string; status: string };
    return new Transaction(this, payload.transaction_id);
  }

  // Internal method for Transaction class to access private methods
  async _txRequest(method: string, url: string, txId: string, init: RequestInit = {}): Promise<Response> {
    const headers = { ...(init.headers as Record<string, string> || {}), "X-Transaction-Id": txId };
    return this.request(method, url, { ...init, headers });
  }

  _getEndpoints(): string[] {
    return this.endpoints;
  }

  async _resolveEndpoint(urn: string): Promise<string> {
    return this.resolveEndpoint(urn);
  }

  async _queryEndpoints(aql: string): Promise<string[]> {
    return this.queryEndpoints(aql);
  }

  _buildUrn(model: string, collection: string, uuid: string): string {
    return this.buildUrn(model, collection, uuid);
  }

  _buildEntityKey(model: string, collection: string, uuid: string): string {
    return this.buildEntityKey(model, collection, uuid);
  }
  
  /**
   * Get the current circuit breaker state if enabled
   * @returns Circuit breaker state or null if disabled
   */
  getCircuitBreakerState(): string | null {
    return this.circuitBreaker ? this.circuitBreaker.getState() : null;
  }
}

export interface TransactionOptions {
  /**
   * Isolation level for the transaction.
   *
   * - `"READ_COMMITTED"` (default) – only committed values are visible.
   *   Non-repeatable reads and phantom reads are possible.
   * - `"SNAPSHOT"` – the transaction sees a consistent snapshot of the database
   *   as of its start time.
   *
   *   **Warning:** Write-skew and phantom-read anomalies are possible at
   *   SNAPSHOT isolation. Two concurrent SNAPSHOT transactions that each read
   *   the same data and write disjoint keys can both commit even when their
   *   combined effect violates an application invariant (e.g. double-booking,
   *   over-withdrawal). Use `"SERIALIZABLE"` when strict correctness matters.
   *
   * - `"SERIALIZABLE"` – full serializability via SSI / predicate locking.
   *   Prevents write skew and phantom reads. May abort more transactions and
   *   has higher latency than SNAPSHOT.
   */
  isolationLevel?: "READ_COMMITTED" | "SNAPSHOT" | "SERIALIZABLE";
  timeout?: number;
}

export class TransactionError extends Error {
  constructor(message: string) {
    super(message);
    this.name = "TransactionError";
  }
}

export class Transaction {
  private readonly client: ThemisClient;
  private readonly txId: string;
  private committed = false;
  private rolledBack = false;

  constructor(client: ThemisClient, txId: string) {
    this.client = client;
    this.txId = txId;
  }

  get transactionId(): string {
    return this.txId;
  }

  get isActive(): boolean {
    return !this.committed && !this.rolledBack;
  }

  private ensureActive(): void {
    if (this.committed) {
      throw new TransactionError("Transaction already committed");
    }
    if (this.rolledBack) {
      throw new TransactionError("Transaction already rolled back");
    }
  }

  async get<T = unknown>(model: string, collection: string, uuid: string): Promise<T | null> {
    this.ensureActive();
    const urn = this.client._buildUrn(model, collection, uuid);
    const key = this.client._buildEntityKey(model, collection, uuid);
    const endpoint = await this.client._resolveEndpoint(urn);
    
    const response = await this.client._txRequest("GET", `${endpoint}/entities/${key}`, this.txId);
    
    if (response.status === 404) {
      return null;
    }
    if (!response.ok) {
      throw await toHttpError(response, "failed to load entity in transaction");
    }
    const payload = (await response.json()) as Record<string, unknown>;
    return decodeEntity<T>(payload);
  }

  async put(model: string, collection: string, uuid: string, data: unknown): Promise<boolean> {
    this.ensureActive();
    const urn = this.client._buildUrn(model, collection, uuid);
    const key = this.client._buildEntityKey(model, collection, uuid);
    const endpoint = await this.client._resolveEndpoint(urn);
    
    const response = await this.client._txRequest("PUT", `${endpoint}/entities/${key}`, this.txId, {
      body: JSON.stringify({ blob: encodeEntity(data) }),
      headers: { "Content-Type": "application/json" },
    });
    
    if (!response.ok) {
      throw await toHttpError(response, "failed to upsert entity in transaction");
    }
    return true;
  }

  async delete(model: string, collection: string, uuid: string): Promise<boolean> {
    this.ensureActive();
    const urn = this.client._buildUrn(model, collection, uuid);
    const key = this.client._buildEntityKey(model, collection, uuid);
    const endpoint = await this.client._resolveEndpoint(urn);
    
    const response = await this.client._txRequest("DELETE", `${endpoint}/entities/${key}`, this.txId);
    
    if (response.status === 404) {
      return false;
    }
    if (!response.ok) {
      throw await toHttpError(response, "failed to delete entity in transaction");
    }
    return true;
  }

  async query<T = unknown>(aql: string, options: QueryOptions = {}): Promise<QueryResult<T>> {
    this.ensureActive();
    const payload: Record<string, unknown> = { query: aql };
    if (options.params) payload.params = options.params;
    if (options.useCursor) payload.use_cursor = true;
    if (options.cursor) payload.cursor = options.cursor;
    if (options.batchSize !== undefined) payload.batch_size = options.batchSize;

    const endpoints = await this.client._queryEndpoints(aql);
    const responses: QueryResult<T>[] = [];
    
    for (const endpoint of endpoints) {
      const resp = await this.client._txRequest("POST", `${endpoint}/query/aql`, this.txId, {
        body: JSON.stringify(payload),
        headers: { "Content-Type": "application/json" },
      });
      
      if (!resp.ok) {
        throw await toHttpError(resp, "query execution failed in transaction");
      }
      
      const data = (await resp.json()) as Record<string, unknown>;
      responses.push(parseQueryResult<T>(data));
    }

    if (responses.length === 0) {
      return { items: [], hasMore: false, nextCursor: null, raw: {} };
    }
    if (responses.length === 1) {
      return responses[0];
    }
    
    const items: T[] = [];
    let hasMore = false;
    for (const part of responses) {
      items.push(...part.items);
      hasMore = hasMore || part.hasMore;
    }
    return { items, hasMore, nextCursor: null, raw: { partials: responses.map((r) => r.raw) } };
  }

  async commit(): Promise<void> {
    this.ensureActive();
    const endpoint = this.client._getEndpoints()[0];
    
    const response = await this.client._txRequest("POST", `${endpoint}/transaction/commit`, this.txId, {
      body: JSON.stringify({ transaction_id: this.txId }),
      headers: { "Content-Type": "application/json" },
    });
    
    if (!response.ok) {
      throw await toHttpError(response, "failed to commit transaction");
    }
    
    this.committed = true;
  }

  async rollback(): Promise<void> {
    this.ensureActive();
    const endpoint = this.client._getEndpoints()[0];
    
    const response = await this.client._txRequest("POST", `${endpoint}/transaction/rollback`, this.txId, {
      body: JSON.stringify({ transaction_id: this.txId }),
      headers: { "Content-Type": "application/json" },
    });
    
    if (!response.ok) {
      throw await toHttpError(response, "failed to rollback transaction");
    }
    
    this.rolledBack = true;
  }
}

export const version = "0.0.0-alpha.0";

function decodeEntity<T>(payload: Record<string, unknown>): T {
  if ("entity" in payload) {
    return payload.entity as T;
  }
  if (typeof payload.blob === "string") {
    try {
      return JSON.parse(payload.blob) as T;
    } catch {
      return payload.blob as unknown as T;
    }
  }
  return payload as unknown as T;
}

function encodeEntity(data: unknown): string {
  if (typeof data === "string") {
    return data;
  }
  return JSON.stringify(data);
}

function extractEndpoints(payload: Record<string, unknown>): string[] {
  const shards = payload.shards;
  if (!Array.isArray(shards)) {
    return [];
  }
  const endpoints = new Set<string>();
  for (const shard of shards) {
    if (typeof shard === "string") {
      endpoints.add(normalizeEndpoint(shard));
      continue;
    }
    if (typeof shard === "object" && shard !== null) {
      const maybeEndpoint = (shard as { endpoint?: string; http_endpoint?: string; endpoints?: string[] }).endpoint;
      const httpEndpoint = (shard as { endpoint?: string; http_endpoint?: string; endpoints?: string[] }).http_endpoint;
      if (typeof maybeEndpoint === "string") endpoints.add(normalizeEndpoint(maybeEndpoint));
      if (typeof httpEndpoint === "string") endpoints.add(normalizeEndpoint(httpEndpoint));
      if (Array.isArray((shard as { endpoints?: string[] }).endpoints)) {
        for (const value of (shard as { endpoints?: string[] }).endpoints ?? []) {
          if (typeof value === "string") endpoints.add(normalizeEndpoint(value));
        }
      }
    }
  }
  return Array.from(endpoints);
}

function parseQueryResult<T>(payload: Record<string, unknown>): QueryResult<T> {
  if (Array.isArray(payload.entities)) {
    return {
      items: decodeEntities<T>(payload.entities as unknown[]),
      hasMore: false,
      nextCursor: null,
      raw: payload,
    };
  }
  if (Array.isArray(payload.items)) {
    return {
      items: decodeEntities<T>(payload.items as unknown[]),
      hasMore: Boolean(payload.has_more),
      nextCursor: (payload.next_cursor as string) ?? null,
      raw: payload,
    };
  }
  return { items: [], hasMore: false, nextCursor: null, raw: payload };
}

function decodeEntities<T>(values: unknown[]): T[] {
  return values.map((value) => {
    if (typeof value === "string") {
      try {
        return JSON.parse(value) as T;
      } catch {
        return value as unknown as T;
      }
    }
    return value as T;
  });
}

function normalizeEndpoint(endpoint: string): string {
  return endpoint.replace(/\/$/, "");
}

function stableHash(value: string): number {
  let hash = 2166136261;
  for (let i = 0; i < value.length; i += 1) {
    hash ^= value.charCodeAt(i);
    hash = Math.imul(hash, 16777619);
  }
  return Math.abs(hash);
}

function isSingleShardQuery(aql: string): boolean {
  return aql.toLowerCase().includes("urn:themis:");
}

async function toHttpError(response: Response, message: string): Promise<Error> {
  let body: unknown;
  try {
    body = await response.text();
  } catch {
    body = undefined;
  }
  return new Error(`${message}: ${response.status} ${response.statusText} ${(body as string) || ""}`.trim());
}

function shouldRetry(error: unknown): boolean {
  if (error instanceof DOMException && error.name === "AbortError") {
    return true;
  }
  return error instanceof TypeError;
}

function delay(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function getFetch(): typeof globalThis.fetch {
  if (typeof globalThis.fetch === "function") {
    return globalThis.fetch.bind(globalThis);
  }
  throw new Error("global fetch API not available; please provide a polyfill such as cross-fetch");
}
