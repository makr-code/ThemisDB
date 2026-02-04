# ThemisDB JavaScript/TypeScript SDK

## Overview

Official JavaScript/TypeScript client library for ThemisDB. This SDK provides a complete interface for interacting with ThemisDB's REST API, including data operations, LLM inference, and administrative functions.

## Status

🚧 **Under Development** - This SDK is currently in active development. Basic structure and placeholder functionality are in place.

## Features (Planned)

- ✅ Bearer Token (JWT) authentication
- ✅ CRUD operations for collections and documents
- ✅ AQL (Advanced Query Language) query execution
- ✅ LLM inference and RAG operations
- ✅ Real-time token streaming
- ✅ Model and LoRA management
- ✅ Comprehensive error handling
- ✅ TypeScript type definitions

## Installation

Once published, the SDK will be installable via npm:

```bash
npm install @themisdb/client
# or
yarn add @themisdb/client
```

## Quick Start

```typescript
import { ThemisDBClient } from '@themisdb/client';

// Initialize client with authentication
const client = new ThemisDBClient({
  baseUrl: 'http://localhost:8080',
  bearerToken: 'your-jwt-token'
});

// Execute AQL query
const result = await client.query('FOR doc IN myCollection RETURN doc');
console.log(result);

// LLM inference
const response = await client.llm.infer({
  prompt: 'What is ThemisDB?',
  model: 'mistral-7b'
});
console.log(response.text);
```

## API Reference

### Client Initialization

```typescript
const client = new ThemisDBClient({
  baseUrl: string,        // ThemisDB server URL
  bearerToken?: string,   // Optional JWT token
  timeout?: number        // Request timeout in ms (default: 30000)
});
```

### Data Operations

```typescript
// Query execution
await client.query(aql: string, bindVars?: object): Promise<QueryResult>

// Collection operations
await client.collections.list(): Promise<Collection[]>
await client.collections.create(name: string, options?: object): Promise<Collection>
await client.collections.drop(name: string): Promise<void>

// Document operations
await client.documents.create(collection: string, doc: object): Promise<Document>
await client.documents.get(collection: string, key: string): Promise<Document>
await client.documents.update(collection: string, key: string, doc: object): Promise<Document>
await client.documents.delete(collection: string, key: string): Promise<void>
```

### LLM Operations

```typescript
// Inference
await client.llm.infer(options: InferOptions): Promise<InferResponse>
await client.llm.stream(options: InferOptions): AsyncIterator<TokenChunk>

// RAG (Retrieval-Augmented Generation)
await client.llm.rag(options: RAGOptions): Promise<RAGResponse>

// Embeddings
await client.llm.embed(text: string, model?: string): Promise<number[]>

// Model management
await client.llm.listModels(): Promise<Model[]>
await client.llm.loadModel(modelId: string): Promise<void>
await client.llm.unloadModel(modelId: string): Promise<void>
```

### Administrative Functions

```typescript
// Health and statistics
await client.admin.health(): Promise<HealthStatus>
await client.admin.stats(): Promise<Statistics>

// Cache management
await client.admin.clearCache(): Promise<void>
await client.admin.getCacheStats(): Promise<CacheStats>
```

## Development

### Prerequisites

- Node.js 16+ or 18+
- npm or yarn

### Setup

```bash
# Clone the repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB/sdks/javascript

# Install dependencies
npm install

# Run tests
npm test

# Build the SDK
npm run build
```

### Project Structure

```
javascript/
├── src/                    # Source code
│   ├── client.ts          # Main client class
│   ├── api/               # API modules
│   ├── models/            # Type definitions
│   └── utils/             # Utility functions
├── tests/                 # Test files
│   └── client.test.ts     # Example tests
├── examples/              # Usage examples
│   └── basic.ts           # Basic usage example
├── package.json           # Package configuration
├── tsconfig.json          # TypeScript configuration
└── README.md              # This file
```

## Testing

Run the test suite:

```bash
npm test
```

Run with coverage:

```bash
npm run test:coverage
```

## Examples

See the [examples](./examples) directory for usage examples:

- [basic.ts](./examples/basic.ts) - Basic operations
- [streaming.ts](./examples/streaming.ts) - Real-time token streaming
- [rag.ts](./examples/rag.ts) - RAG operations

## Contributing

Contributions are welcome! Please see the [CONTRIBUTING.md](../../CONTRIBUTING.md) guide for details.

## License

Apache 2.0 - See [LICENSE](../../LICENSE) for details.

## Support

- Documentation: [docs.themisdb.org](https://docs.themisdb.org)
- GitHub Issues: [github.com/makr-code/ThemisDB/issues](https://github.com/makr-code/ThemisDB/issues)
- Community: [ThemisDB Discussions](https://github.com/makr-code/ThemisDB/discussions)
