# frozen_string_literal: true

require 'net/http'
require 'json'
require 'uri'
require 'digest'

module ThemisDB
  # Official Ruby client for ThemisDB
  #
  # A high-performance multi-model database client with topology-aware routing,
  # ACID transactions, and support for relational, graph, vector, and document models.
  #
  # @example Basic usage
  #   client = ThemisDB::Client.new(['http://localhost:8080'])
  #   client.put('relational', 'users', 'alice', { name: 'Alice', age: 30 })
  #   user = client.get('relational', 'users', 'alice')
  #   puts user
  class Client
    VERSION = '1.0.0'
    DEFAULT_METADATA_PATH = '/_admin/cluster/topology'
    HEALTH_PATH = '/health'

    attr_reader :endpoints, :namespace, :timeout, :max_retries

    # Circuit breaker states
    CIRCUIT_CLOSED = :closed
    CIRCUIT_OPEN = :open
    CIRCUIT_HALF_OPEN = :half_open

    # Create a new ThemisDB client
    #
    # @param endpoints [Array<String>] List of ThemisDB server endpoints
    # @param options [Hash] Configuration options
    # @option options [String] :namespace ('default') Namespace for entities
    # @option options [Integer] :timeout (30) Request timeout in seconds
    # @option options [Integer] :max_retries (3) Maximum retry attempts
    # @option options [String] :metadata_endpoint Custom metadata endpoint
    # @option options [String] :metadata_path Metadata path
    # @option options [Hash] :circuit_breaker Circuit breaker configuration
    # @option options [Hash] :logging Logging configuration
    def initialize(endpoints, **options)
      raise ArgumentError, 'endpoints must not be empty' if endpoints.empty?

      @endpoints = endpoints.map { |e| e.chomp('/') }
      @namespace = options[:namespace] || 'default'
      @timeout = options[:timeout] || 30
      @max_retries = [1, options[:max_retries] || 3].max
      @metadata_endpoint = options[:metadata_endpoint]
      @metadata_path = options[:metadata_path] || DEFAULT_METADATA_PATH
      @shard_endpoints = @endpoints.dup
      @topology_cache = nil
      
      # Circuit breaker configuration
      cb_config = options[:circuit_breaker] || {}
      if cb_config[:enabled]
        @circuit_breaker_enabled = true
        @circuit_breaker_failure_threshold = cb_config[:failure_threshold] || 5
        @circuit_breaker_reset_timeout = cb_config[:reset_timeout] || 60
        @circuit_breaker_half_open_max = cb_config[:half_open_max_requests] || 3
        @circuit_breaker_state = CIRCUIT_CLOSED
        @circuit_breaker_failure_count = 0
        @circuit_breaker_success_count = 0
        @circuit_breaker_next_attempt_time = nil
      else
        @circuit_breaker_enabled = false
      end
      
      # Logging configuration
      log_config = options[:logging] || {}
      @logging_enabled = log_config[:enabled] || false
      @log_requests = log_config[:log_requests] || false
      @log_responses = log_config[:log_responses] || false
    end
    
    # Get circuit breaker state
    #
    # @return [Symbol, nil] :closed, :open, :half_open, or nil if disabled
    def circuit_breaker_state
      @circuit_breaker_enabled ? @circuit_breaker_state : nil
    end

    # Check server health
    #
    # @param endpoint [String, nil] Specific endpoint to check
    # @return [Hash] Health status response
    def health(endpoint = nil)
      target = endpoint ? endpoint.chomp('/') : @endpoints[0]
      request(:get, "#{target}#{HEALTH_PATH}")
    end

    # Get an entity
    #
    # @param model [String] Model name
    # @param collection [String] Collection name
    # @param uuid [String] Entity UUID
    # @return [Object, nil] Entity data or nil if not found
    def get(model, collection, uuid)
      urn = build_urn(model, collection, uuid)
      key = build_entity_key(model, collection, uuid)
      endpoint = resolve_endpoint(urn)

      response = request(:get, "#{endpoint}/entities/#{key}")
      return response['entity'] if response.key?('entity')
      return decode_blob(response['blob']) if response.key?('blob')

      response
    rescue NotFoundError
      nil
    end

    # Create or update an entity
    #
    # @param model [String] Model name
    # @param collection [String] Collection name
    # @param uuid [String] Entity UUID
    # @param data [Object] Entity data
    # @return [Boolean] true on success
    def put(model, collection, uuid, data)
      urn = build_urn(model, collection, uuid)
      key = build_entity_key(model, collection, uuid)
      endpoint = resolve_endpoint(urn)

      request(:put, "#{endpoint}/entities/#{key}", blob: encode_blob(data))
      true
    end

    # Delete an entity
    #
    # @param model [String] Model name
    # @param collection [String] Collection name
    # @param uuid [String] Entity UUID
    # @return [Boolean] true if deleted, false if not found
    def delete(model, collection, uuid)
      urn = build_urn(model, collection, uuid)
      key = build_entity_key(model, collection, uuid)
      endpoint = resolve_endpoint(urn)

      request(:delete, "#{endpoint}/entities/#{key}")
      true
    rescue NotFoundError
      false
    end

    # Batch get multiple entities
    #
    # @param model [String] Model name
    # @param collection [String] Collection name
    # @param uuids [Array<String>] UUIDs to fetch
    # @return [Hash] Result with :found, :missing, :errors keys
    def batch_get(model, collection, uuids)
      result = { found: {}, missing: [], errors: {} }
      return result if uuids.empty?

      uuids.each do |uuid|
        entity = get(model, collection, uuid)
        if entity.nil?
          result[:missing] << uuid
        else
          result[:found][uuid] = entity
        end
      rescue StandardError => e
        result[:errors][uuid] = e.message
      end

      result
    end

    # Batch put multiple entities
    #
    # @param model [String] Model name
    # @param collection [String] Collection name
    # @param items [Hash] Hash of uuid => data
    # @return [Hash] Result with :succeeded, :failed keys
    def batch_put(model, collection, items)
      result = { succeeded: [], failed: {} }
      return result if items.empty?

      items.each do |uuid, data|
        put(model, collection, uuid, data)
        result[:succeeded] << uuid
      rescue StandardError => e
        result[:failed][uuid] = e.message
      end

      result
    end

    # Batch delete multiple entities
    #
    # @param model [String] Model name
    # @param collection [String] Collection name
    # @param uuids [Array<String>] UUIDs to delete
    # @return [Hash] Result with :succeeded, :failed keys
    def batch_delete(model, collection, uuids)
      result = { succeeded: [], failed: {} }
      return result if uuids.empty?

      uuids.each do |uuid|
        delete(model, collection, uuid)
        result[:succeeded] << uuid
      rescue StandardError => e
        result[:failed][uuid] = e.message
      end

      result
    end

    # Execute an AQL query
    #
    # @param aql [String] AQL query string
    # @param options [Hash] Query options
    # @option options [Hash] :params Query parameters
    # @option options [Boolean] :use_cursor Enable cursor pagination
    # @option options [String] :cursor Pagination cursor
    # @option options [Integer] :batch_size Batch size
    # @return [Hash] Query result with :items, :has_more, :next_cursor, :raw
    def query(aql, **options)
      payload = { query: aql }
      payload[:params] = options[:params] if options[:params]
      payload[:use_cursor] = true if options[:use_cursor]
      payload[:cursor] = options[:cursor] if options[:cursor]
      payload[:batch_size] = options[:batch_size] if options[:batch_size]

      endpoints = single_shard_query?(aql) ? [resolve_query_endpoint(aql)] : current_endpoints
      partials = endpoints.map do |endpoint|
        response = request(:post, "#{endpoint}/query/aql", payload)
        parse_query_payload(response)
      end

      return { items: [], has_more: false, next_cursor: nil, raw: {} } if partials.empty?
      return partials[0] if partials.size == 1

      # Merge results
      merged_items = partials.flat_map { |p| p[:items] }
      any_has_more = partials.any? { |p| p[:has_more] }

      {
        items: merged_items,
        has_more: any_has_more,
        next_cursor: nil,
        raw: { partials: partials.map { |p| p[:raw] } }
      }
    end

    # Traverse a graph
    #
    # @param start_node [String] Starting node
    # @param max_depth [Integer] Maximum traversal depth
    # @param edge_type [String, nil] Optional edge type filter
    # @return [Array<String>] Visited nodes
    def graph_traverse(start_node, max_depth: 3, edge_type: nil)
      endpoint = resolve_endpoint(start_node)
      payload = { start: start_node, max_depth: max_depth }
      payload[:edge_type] = edge_type if edge_type

      response = request(:post, "#{endpoint}/graph/traverse", payload)
      response['nodes'] || response['visited'] || []
    end

    # Find shortest path between nodes
    #
    # @param start_node [String] Starting node
    # @param end_node [String] End node
    # @param edge_type [String, nil] Optional edge type filter
    # @return [Array<String>, nil] Path or nil if no path found
    def graph_shortest_path(start_node, end_node, edge_type: nil)
      endpoint = resolve_endpoint(start_node)
      payload = { start: start_node, end: end_node }
      payload[:edge_type] = edge_type if edge_type

      response = request(:post, "#{endpoint}/graph/shortest-path", payload)
      response['path']
    end

    # Get neighbors of a node
    #
    # @param node [String] Node URN or key
    # @param edge_type [String, nil] Optional edge type filter
    # @param direction [String] Direction: 'in', 'out', or 'both'
    # @return [Array<String>] Neighbor nodes
    def graph_neighbors(node, edge_type: nil, direction: 'both')
      endpoint = resolve_endpoint(node)
      payload = { node: node, direction: direction }
      payload[:edge_type] = edge_type if edge_type

      response = request(:post, "#{endpoint}/graph/neighbors", payload)
      response['neighbors'] || []
    end

    # Search for similar vectors
    #
    # @param embedding [Array<Float>] Query vector
    # @param top_k [Integer] Number of results
    # @param metadata_filter [Hash, nil] Optional metadata filter
    # @param options [Hash] Additional options
    # @return [Hash] Search results
    def vector_search(embedding, top_k: 10, metadata_filter: nil, **options)
      payload = { vector: embedding, k: top_k }
      payload[:filter] = metadata_filter if metadata_filter
      payload[:use_cursor] = true if options[:use_cursor]
      payload[:cursor] = options[:cursor] if options[:cursor]

      responses = current_endpoints.filter_map do |endpoint|
        request(:post, "#{endpoint}/vector/search", payload)
      rescue StandardError
        nil
      end

      return { results: [] } if responses.empty?
      return responses[0] if responses.size == 1

      # Merge and sort results
      merged = responses.flat_map { |r| r['results'] || r['items'] || [] }
      merged.sort_by! { |item| -(item['score'] || -item['distance'] || 0) }

      { results: merged.first(top_k), partials: responses }
    end

    # Upsert a vector
    #
    # @param id [String] Vector ID
    # @param embedding [Array<Float>] Vector embedding
    # @param metadata [Hash, nil] Optional metadata
    # @return [Boolean] true on success
    def vector_upsert(id, embedding, metadata: nil)
      endpoint = resolve_endpoint(id)
      payload = { id: id, vector: embedding }
      payload[:metadata] = metadata if metadata

      request(:post, "#{endpoint}/vector/upsert", payload)
      true
    end

    # Delete a vector
    #
    # @param id [String] Vector ID
    # @return [Boolean] true if deleted, false if not found
    def vector_delete(id)
      endpoint = resolve_endpoint(id)
      request(:delete, "#{endpoint}/vector/#{id}")
      true
    rescue NotFoundError
      false
    end

    # Begin a transaction
    #
    # @param options [Hash] Transaction options
    # @option options [String] :isolation_level ('READ_COMMITTED') Isolation level
    # @option options [Integer] :timeout Transaction timeout
    # @return [Transaction] Transaction object
    def begin_transaction(**options)
      endpoint = @endpoints[0]
      isolation = options[:isolation_level] || 'READ_COMMITTED'

      body = {}
      body[:isolation] = isolation == 'SNAPSHOT' ? 'snapshot' : 'read_committed'
      body[:timeout] = options[:timeout] if options[:timeout]

      response = request(:post, "#{endpoint}/transaction/begin", body)
      raise TransactionError, 'Server did not return transaction_id' unless response['transaction_id']

      Transaction.new(self, response['transaction_id'])
    end

    # @api private
    def request(method, url, body = nil, headers = {})
      # Check circuit breaker
      if @circuit_breaker_enabled && !can_execute_request?
        log('ERROR', "Circuit breaker is OPEN for #{url}")
        raise StandardError, 'Circuit breaker is OPEN'
      end
      
      # Log request
      log('INFO', "#{method.upcase} #{url}") if @log_requests
      
      uri = URI(url)
      http = Net::HTTP.new(uri.host, uri.port)
      http.read_timeout = @timeout
      http.use_ssl = uri.scheme == 'https'

      req_class = case method
                  when :get then Net::HTTP::Get
                  when :post then Net::HTTP::Post
                  when :put then Net::HTTP::Put
                  when :delete then Net::HTTP::Delete
                  else raise ArgumentError, "Unsupported method: #{method}"
                  end

      request = req_class.new(uri.request_uri)
      request['Content-Type'] = 'application/json'
      request['User-Agent'] = "themisdb-ruby-sdk/#{VERSION}"
      request['Accept'] = 'application/json'
      headers.each { |k, v| request[k] = v }

      request.body = JSON.generate(body) if body

      last_error = nil
      @max_retries.times do |attempt|
        response = http.request(request)
        
        # Log response
        log('INFO', "#{method.upcase} #{url} -> #{response.code}") if @log_responses
        
        case response.code.to_i
        when 200..299
          record_circuit_breaker_success
          return response.body.empty? ? {} : JSON.parse(response.body)
        when 404
          record_circuit_breaker_failure
          raise NotFoundError, "Resource not found: #{url}"
        when 500..599
          record_circuit_breaker_failure
          log('WARN', "Server error #{response.code}, retrying...")
          raise StandardError, "HTTP #{response.code}: #{response.body}" if attempt == @max_retries - 1
          last_error = StandardError.new("HTTP #{response.code}: #{response.body}")
          sleep(backoff_delay(attempt))
        else
          record_circuit_breaker_failure
          raise StandardError, "HTTP #{response.code}: #{response.body}"
        end
      rescue StandardError => e
        record_circuit_breaker_failure
        log('ERROR', "Request failed: #{e.message}")
        raise if attempt == @max_retries - 1
        last_error = e
        sleep(backoff_delay(attempt))
      end

      raise last_error if last_error
      raise StandardError, 'Request failed without specific error'
    end

    private

    def log(level, message)
      return unless @logging_enabled
      puts "[ThemisDB] [#{level}] #{message}"
    end
    
    def backoff_delay(attempt)
      (2 ** attempt) * 0.1  # Exponential backoff: 0.1s, 0.2s, 0.4s, etc.
    end
    
    def can_execute_request?
      return true unless @circuit_breaker_enabled
      
      case @circuit_breaker_state
      when CIRCUIT_CLOSED
        true
      when CIRCUIT_OPEN
        if Time.now >= @circuit_breaker_next_attempt_time
          @circuit_breaker_state = CIRCUIT_HALF_OPEN
          @circuit_breaker_success_count = 0
          true
        else
          false
        end
      when CIRCUIT_HALF_OPEN
        @circuit_breaker_success_count < @circuit_breaker_half_open_max
      end
    end
    
    def record_circuit_breaker_success
      return unless @circuit_breaker_enabled
      
      @circuit_breaker_failure_count = 0
      
      if @circuit_breaker_state == CIRCUIT_HALF_OPEN
        @circuit_breaker_success_count += 1
        if @circuit_breaker_success_count >= @circuit_breaker_half_open_max
          @circuit_breaker_state = CIRCUIT_CLOSED
        end
      end
    end
    
    def record_circuit_breaker_failure
      return unless @circuit_breaker_enabled
      
      @circuit_breaker_failure_count += 1
      @circuit_breaker_success_count = 0
      
      if @circuit_breaker_failure_count >= @circuit_breaker_failure_threshold
        @circuit_breaker_state = CIRCUIT_OPEN
        @circuit_breaker_next_attempt_time = Time.now + @circuit_breaker_reset_timeout
      end
    end

    def build_urn(model, collection, uuid)
      "urn:themis:#{model}:#{@namespace}:#{collection}:#{uuid}"
    end

    def build_entity_key(model, collection, uuid)
      table = "#{model}.#{@namespace}.#{collection}"
      "#{table}:#{uuid}"
    end

    def resolve_endpoint(urn)
      ensure_topology
      endpoints = current_endpoints
      raise TopologyError, 'No endpoints available' if endpoints.empty?

      index = stable_hash(urn) % endpoints.size
      endpoints[index]
    end

    def resolve_query_endpoint(aql)
      ensure_topology
      endpoints = current_endpoints
      raise TopologyError, 'No endpoints available' if endpoints.empty?

      index = stable_hash(aql) % endpoints.size
      endpoints[index]
    end

    def current_endpoints
      @shard_endpoints.empty? ? @endpoints : @shard_endpoints
    end

    def ensure_topology
      return if @topology_cache

      refresh_topology
    rescue TopologyError
      @shard_endpoints = @endpoints.dup
    end

    def refresh_topology
      url = metadata_url
      payload = request(:get, url)

      endpoints = extract_endpoints(payload)
      raise TopologyError, 'No shard endpoints found' if endpoints.empty?

      @topology_cache = payload
      @shard_endpoints = endpoints
    rescue StandardError => e
      raise TopologyError, "Failed to fetch topology: #{e.message}"
    end

    def metadata_url
      return @metadata_endpoint if @metadata_endpoint&.start_with?('http')

      "#{@endpoints[0]}#{@metadata_endpoint || @metadata_path}"
    end

    def extract_endpoints(payload)
      return [] unless payload['shards'].is_a?(Array)

      payload['shards'].flat_map do |shard|
        if shard.is_a?(String)
          shard.chomp('/')
        elsif shard.is_a?(Hash)
          [shard['endpoint'], shard['http_endpoint'], *shard['endpoints']].compact.map { |e| e.chomp('/') }
        end
      end.compact.uniq
    end

    def stable_hash(value)
      # Use custom FNV implementation defined below
      Digest::FNV.fnv1a_32(value)
    end

    def single_shard_query?(aql)
      aql.downcase.include?('urn:themis:')
    end

    def parse_query_payload(payload)
      if payload.key?('entities')
        items = payload['entities'].map { |e| decode_blob(e) }
        return {
          items: items,
          has_more: false,
          next_cursor: nil,
          raw: payload,
          count: payload['count'],
          table: payload['table']
        }
      end

      if payload.key?('items')
        items = payload['items'].map { |e| decode_blob(e) }
        return {
          items: items,
          has_more: payload['has_more'] || false,
          next_cursor: payload['next_cursor'],
          raw: payload,
          table: payload['table']
        }
      end

      { items: [], has_more: false, next_cursor: nil, raw: payload }
    end

    def decode_blob(blob)
      return blob unless blob.is_a?(String)

      JSON.parse(blob)
    rescue JSON::ParserError
      blob
    end

    def encode_blob(data)
      data.is_a?(String) ? data : JSON.generate(data)
    end

    # Create an LLM interaction
    #
    # @param model [String] LLM model name (e.g., 'gpt-4o', 'llama-3.1')
    # @param messages [Array<Hash>] List of messages
    # @param reasoning_steps [Array<Hash>, nil] Optional reasoning steps
    # @param metadata [Hash, nil] Optional metadata
    # @return [Hash]
    def llm_interaction(model, messages, reasoning_steps: nil, metadata: nil)
      endpoint = @endpoints[0]
      
      body = {
        model: model,
        messages: messages
      }
      
      body[:reasoning_steps] = reasoning_steps if reasoning_steps
      body[:metadata] = metadata if metadata
      
      request(:post, "#{endpoint}/llm/interaction", body)
    end

    # Get a specific LLM interaction by ID
    #
    # @param interaction_id [String] The interaction ID
    # @return [Hash, nil]
    def get_llm_interaction(interaction_id)
      endpoint = @endpoints[0]
      
      request(:get, "#{endpoint}/llm/interaction/#{interaction_id}")
    rescue NotFoundError
      nil
    end

    # List LLM interactions with optional filtering
    #
    # @param model [String, nil] Optional model name filter
    # @param limit [Integer, nil] Maximum number of results
    # @param offset [Integer, nil] Result offset for pagination
    # @return [Array<Hash>]
    def list_llm_interactions(model: nil, limit: nil, offset: nil)
      endpoint = @endpoints[0]
      params = []
      
      params << "model=#{URI.encode_www_form_component(model)}" if model
      params << "limit=#{limit}" if limit
      params << "offset=#{offset}" if offset
      
      url = "#{endpoint}/llm/interaction"
      url += "?#{params.join('&')}" unless params.empty?
      
      response = request(:get, url)
      response['interactions'] || []
    end
  end

  # Custom exceptions
  class NotFoundError < StandardError; end
  class TopologyError < StandardError; end
  class TransactionError < StandardError; end

  # Transaction class for ACID operations
  class Transaction
    attr_reader :transaction_id

    def initialize(client, transaction_id)
      @client = client
      @transaction_id = transaction_id
      @committed = false
      @rolled_back = false
    end

    def active?
      !@committed && !@rolled_back
    end

    def get(model, collection, uuid)
      ensure_active
      urn = @client.send(:build_urn, model, collection, uuid)
      key = @client.send(:build_entity_key, model, collection, uuid)
      endpoint = @client.send(:resolve_endpoint, urn)

      response = tx_request(:get, "#{endpoint}/entities/#{key}")
      return response['entity'] if response.key?('entity')
      return @client.send(:decode_blob, response['blob']) if response.key?('blob')

      response
    rescue NotFoundError
      nil
    end

    def put(model, collection, uuid, data)
      ensure_active
      urn = @client.send(:build_urn, model, collection, uuid)
      key = @client.send(:build_entity_key, model, collection, uuid)
      endpoint = @client.send(:resolve_endpoint, urn)

      tx_request(:put, "#{endpoint}/entities/#{key}", blob: @client.send(:encode_blob, data))
      true
    end

    def delete(model, collection, uuid)
      ensure_active
      urn = @client.send(:build_urn, model, collection, uuid)
      key = @client.send(:build_entity_key, model, collection, uuid)
      endpoint = @client.send(:resolve_endpoint, urn)

      tx_request(:delete, "#{endpoint}/entities/#{key}")
      true
    rescue NotFoundError
      false
    end

    def query(aql, **options)
      ensure_active
      # Similar implementation to client.query but with transaction context
      payload = { query: aql }
      payload[:params] = options[:params] if options[:params]

      endpoint = @client.send(:current_endpoints).first
      response = tx_request(:post, "#{endpoint}/query/aql", payload)
      @client.send(:parse_query_payload, response)
    end

    def commit
      ensure_active
      endpoint = @client.endpoints[0]
      tx_request(:post, "#{endpoint}/transaction/commit", transaction_id: @transaction_id)
      @committed = true
    end

    def rollback
      ensure_active
      endpoint = @client.endpoints[0]
      tx_request(:post, "#{endpoint}/transaction/rollback", transaction_id: @transaction_id)
      @rolled_back = true
    end

    private

    def ensure_active
      raise TransactionError, 'Transaction already committed' if @committed
      raise TransactionError, 'Transaction already rolled back' if @rolled_back
    end

    def tx_request(method, url, body = nil)
      headers = { 'X-Transaction-Id' => @transaction_id }
      @client.request(method, url, body, headers)
    end
  end
end

# FNV hash implementation (fallback)
module Digest
  module FNV
    FNV1_32_INIT = 0x811c9dc5
    FNV_32_PRIME = 0x01000193

    def self.fnv1a_32(data)
      hash = FNV1_32_INIT
      data.each_byte do |byte|
        hash ^= byte
        hash = (hash * FNV_32_PRIME) & 0xffffffff
      end
      hash
    end
  end
end
