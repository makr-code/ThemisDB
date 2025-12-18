# ThemisDB Ruby SDK

Official Ruby client for ThemisDB - A high-performance multi-model database with native LLM integration.

## Features

- ✅ **Full Type Safety** - Modern Ruby with strong conventions
- ✅ **Transaction Support** - BEGIN/COMMIT/ROLLBACK with isolation levels
- ✅ **Multi-Model** - Relational, Graph, Vector operations
- ✅ **Query Support** - AQL (Advanced Query Language)
- ✅ **Topology-Aware** - Automatic shard routing with consistent hashing
- ✅ **Batch Operations** - Efficient bulk operations
- ✅ **Vector Search** - Similarity search for LLM/AI applications
- ✅ **Graph Operations** - Traverse, shortest path, neighbors
- ✅ **Retry Logic** - Automatic retries for failed requests
- ✅ **Idiomatic Ruby** - Follows Ruby conventions and best practices

## Requirements

- Ruby >= 2.7

## Installation

Add this line to your application's Gemfile:

```ruby
gem 'themisdb'
```

And then execute:

```bash
bundle install
```

Or install it yourself as:

```bash
gem install themisdb
```

## Quick Start

```ruby
require 'themisdb'

# Create client
client = ThemisDB::Client.new(['http://localhost:8080'])

# Basic CRUD
client.put('relational', 'users', 'alice', { name: 'Alice', age: 30 })
user = client.get('relational', 'users', 'alice')
puts user

# Delete
client.delete('relational', 'users', 'alice')
```

## Transaction Support

```ruby
require 'themisdb'

client = ThemisDB::Client.new(['http://localhost:8080'])

# Begin a transaction
tx = client.begin_transaction(isolation_level: 'SNAPSHOT')

begin
  # Perform operations within the transaction
  tx.put('relational', 'accounts', 'acc1', { balance: 1000 })
  tx.put('relational', 'accounts', 'acc2', { balance: 500 })
  
  # Read within transaction
  acc1 = tx.get('relational', 'accounts', 'acc1')
  puts "Account 1 balance: #{acc1['balance']}"
  
  # Commit the transaction
  tx.commit
rescue => e
  # Rollback on error
  tx.rollback
  raise
end
```

## Batch Operations

```ruby
require 'themisdb'

client = ThemisDB::Client.new(['http://localhost:8080'])

# Batch Put
items = {
  'user1' => { name: 'Alice', age: 30 },
  'user2' => { name: 'Bob', age: 25 },
  'user3' => { name: 'Charlie', age: 35 }
}

result = client.batch_put('relational', 'users', items)
puts "Succeeded: #{result[:succeeded].size}"
puts "Failed: #{result[:failed].size}"

# Batch Get
uuids = ['user1', 'user2', 'user3']
result = client.batch_get('relational', 'users', uuids)

result[:found].each do |uuid, user|
  puts "#{user['name']} (#{uuid})"
end

# Batch Delete
result = client.batch_delete('relational', 'users', ['user1', 'user2'])
```

## AQL Queries

```ruby
require 'themisdb'

client = ThemisDB::Client.new(['http://localhost:8080'])

# Simple query
result = client.query('FOR user IN users FILTER user.age > 25 RETURN user')

result[:items].each do |user|
  puts "#{user['name']} is #{user['age']} years old"
end

# Parameterized query
result = client.query(
  'FOR user IN users FILTER user.city == @city RETURN user',
  params: { city: 'Berlin' }
)

# Cursor-based pagination
result = client.query(
  'FOR user IN users RETURN user',
  use_cursor: true,
  batch_size: 100
)

if result[:has_more]
  next_page = client.query(
    'FOR user IN users RETURN user',
    use_cursor: true,
    cursor: result[:next_cursor],
    batch_size: 100
  )
end
```

## Graph Operations

```ruby
require 'themisdb'

client = ThemisDB::Client.new(['http://localhost:8080'])

# Traverse graph from a starting node
nodes = client.graph_traverse('user:alice', max_depth: 3)
puts nodes

# Find shortest path between two nodes
path = client.graph_shortest_path('user:alice', 'user:charlie')
puts "Path: #{path.join(' -> ')}" if path

# Get neighbors of a node
neighbors = client.graph_neighbors('user:alice', direction: 'both')
puts neighbors

# Filter by edge type
friends = client.graph_neighbors('user:alice', edge_type: 'FRIEND', direction: 'out')
puts friends
```

## Vector Operations

Perfect for LLM and AI applications:

```ruby
require 'themisdb'

client = ThemisDB::Client.new(['http://localhost:8080'])

# Upsert a vector
embedding = Array.new(768, 0.1)  # 768-dimensional embedding
client.vector_upsert('doc1', embedding, metadata: { title: 'AI Research Paper' })

# Search for similar vectors
query_embedding = Array.new(768, 0.15)
results = client.vector_search(query_embedding, top_k: 10)

results[:results].each do |result|
  score = result['score'] || result['distance'] || 0
  puts "Document: #{result['id']}, Score: #{score.round(4)}"
end

# Search with metadata filter
results = client.vector_search(
  query_embedding,
  top_k: 10,
  metadata_filter: { category: 'research' }
)

# Delete a vector
client.vector_delete('doc1')
```

## Rails Integration Example

```ruby
# config/initializers/themisdb.rb
require 'themisdb'

THEMIS_CLIENT = ThemisDB::Client.new(
  [ENV.fetch('THEMISDB_URL', 'http://localhost:8080')],
  namespace: Rails.env,
  timeout: 30,
  max_retries: 3
)

# app/models/concerns/themisdb_persistable.rb
module ThemisdbPersistable
  extend ActiveSupport::Concern

  included do
    after_save :sync_to_themisdb
    after_destroy :remove_from_themisdb
  end

  private

  def sync_to_themisdb
    THEMIS_CLIENT.put(
      'relational',
      self.class.table_name,
      id.to_s,
      attributes
    )
  end

  def remove_from_themisdb
    THEMIS_CLIENT.delete('relational', self.class.table_name, id.to_s)
  end
end

# app/models/user.rb
class User < ApplicationRecord
  include ThemisdbPersistable
end
```

## Configuration Options

```ruby
client = ThemisDB::Client.new(
  ['http://localhost:8080', 'http://localhost:8081'],  # Multiple endpoints
  namespace: 'production',           # Default: 'default'
  timeout: 30,                       # Request timeout in seconds
  max_retries: 3,                    # Maximum retry attempts
  metadata_endpoint: nil,            # Custom metadata endpoint
  metadata_path: '/_admin/cluster/topology'  # Topology path
)
```

## Error Handling

```ruby
require 'themisdb'

client = ThemisDB::Client.new(['http://localhost:8080'])

begin
  user = client.get('relational', 'users', 'nonexistent')
  # user will be nil for not found
  
  if user.nil?
    puts 'User not found'
  end
rescue ThemisDB::TopologyError => e
  puts "Topology error: #{e.message}"
rescue ThemisDB::TransactionError => e
  puts "Transaction error: #{e.message}"
rescue => e
  puts "Request failed: #{e.message}"
end
```

## Development

```bash
# Install dependencies
bundle install

# Run tests
bundle exec rspec

# Run linter
bundle exec rubocop

# Build gem
gem build themisdb.gemspec
```

## API Reference

### ThemisDB::Client

#### Constructor

```ruby
ThemisDB::Client.new(endpoints, **options)
```

**Options:**
- `namespace` (String) - Namespace for entities (default: 'default')
- `timeout` (Integer) - Request timeout in seconds (default: 30)
- `max_retries` (Integer) - Maximum retry attempts (default: 3)
- `metadata_endpoint` (String, nil) - Custom metadata endpoint
- `metadata_path` (String) - Metadata path

#### Methods

- `get(model, collection, uuid)` - Retrieve an entity
- `put(model, collection, uuid, data)` - Create/update an entity
- `delete(model, collection, uuid)` - Delete an entity
- `batch_get(model, collection, uuids)` - Batch retrieve
- `batch_put(model, collection, items)` - Batch create/update
- `batch_delete(model, collection, uuids)` - Batch delete
- `query(aql, **options)` - Execute AQL query
- `graph_traverse(start_node, max_depth:, edge_type:)` - Graph traversal
- `graph_shortest_path(start_node, end_node, edge_type:)` - Shortest path
- `graph_neighbors(node, edge_type:, direction:)` - Get neighbors
- `vector_search(embedding, top_k:, metadata_filter:, **options)` - Vector search
- `vector_upsert(id, embedding, metadata:)` - Upsert vector
- `vector_delete(id)` - Delete vector
- `begin_transaction(**options)` - Start transaction
- `health(endpoint)` - Health check

### ThemisDB::Transaction

#### Properties

- `transaction_id` - Get transaction ID
- `active?` - Check if transaction is active

#### Methods

- `get(model, collection, uuid)` - Retrieve within transaction
- `put(model, collection, uuid, data)` - Update within transaction
- `delete(model, collection, uuid)` - Delete within transaction
- `query(aql, **options)` - Query within transaction
- `commit` - Commit the transaction
- `rollback` - Rollback the transaction

## License

MIT

## Support

- **Documentation**: [https://makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)
- **GitHub Issues**: [Report bugs or request features](https://github.com/makr-code/ThemisDB/issues)
- **Discussions**: [Community discussions](https://github.com/makr-code/ThemisDB/discussions)
