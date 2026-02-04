# ThemisDB Java SDK

## Overview

Official Java client library for ThemisDB. This SDK provides a robust, enterprise-ready Java interface for interacting with ThemisDB's REST API.

## Status

🚧 **Under Development** - This SDK is currently in active development. Basic structure and placeholder functionality are in place.

## Features (Planned)

- ✅ Bearer Token (JWT) authentication
- ✅ Type-safe CRUD operations
- ✅ AQL (Advanced Query Language) query execution
- ✅ LLM inference and RAG operations
- ✅ Async and sync API variants
- ✅ Real-time token streaming
- ✅ Model and LoRA management
- ✅ Comprehensive error handling
- ✅ Connection pooling
- ✅ Java 11+ support

## Installation

Once published, add the SDK to your Maven `pom.xml`:

```xml
<dependency>
    <groupId>com.themisdb</groupId>
    <artifactId>themisdb-client</artifactId>
    <version>0.1.0</version>
</dependency>
```

Or if using Gradle:

```gradle
implementation 'com.themisdb:themisdb-client:0.1.0'
```

## Quick Start

```java
import com.themisdb.client.ThemisDBClient;
import com.themisdb.client.model.QueryResult;
import com.themisdb.client.model.InferRequest;
import com.themisdb.client.model.InferResponse;

public class Example {
    public static void main(String[] args) {
        // Initialize client with authentication
        ThemisDBClient client = ThemisDBClient.builder()
            .baseUrl("http://localhost:8080")
            .bearerToken("your-jwt-token")
            .build();

        // Execute AQL query
        QueryResult result = client.query()
            .aql("FOR doc IN myCollection RETURN doc")
            .execute();
        
        System.out.println(result);

        // LLM inference
        InferResponse response = client.llm()
            .infer()
            .prompt("What is ThemisDB?")
            .model("mistral-7b")
            .execute();
        
        System.out.println("Response: " + response.getText());

        // Close client when done
        client.close();
    }
}
```

## API Reference

### Client Initialization

```java
// Builder pattern
ThemisDBClient client = ThemisDBClient.builder()
    .baseUrl("http://localhost:8080")
    .bearerToken("token")
    .timeout(Duration.ofSeconds(30))
    .build();

// Or with configuration object
ClientConfig config = new ClientConfig.Builder()
    .baseUrl("http://localhost:8080")
    .bearerToken("token")
    .build();
ThemisDBClient client = new ThemisDBClient(config);
```

### Data Operations

```java
// Query execution
QueryResult result = client.query()
    .aql("FOR doc IN myCollection FILTER doc.age > @age RETURN doc")
    .bindVar("age", 25)
    .execute();

// Collection operations
List<Collection> collections = client.collections().list();
client.collections().create("newCollection");
client.collections().drop("oldCollection");

// Document operations
Document doc = client.collection("users")
    .document("user123")
    .get();

client.collection("users")
    .document("user123")
    .update(updatedDoc);

client.collection("users")
    .document("user123")
    .delete();
```

### LLM Operations

```java
// Inference
InferResponse response = client.llm()
    .infer()
    .prompt("Explain quantum computing")
    .model("mistral-7b")
    .maxTokens(100)
    .execute();

// Streaming inference
Stream<TokenChunk> stream = client.llm()
    .infer()
    .prompt("Tell me a story")
    .stream();

stream.forEach(chunk -> System.out.print(chunk.getToken()));

// RAG (Retrieval-Augmented Generation)
RAGResponse response = client.llm()
    .rag()
    .query("What are the key features?")
    .collection("documentation")
    .execute();

// Embeddings
double[] embeddings = client.llm()
    .embed("Sample text for embedding")
    .execute();

// Model management
List<Model> models = client.llm().listModels();
client.llm().loadModel("mistral-7b");
client.llm().unloadModel("old-model");
```

### Administrative Functions

```java
// Health check
HealthStatus health = client.admin().health();
System.out.println("Status: " + health.getStatus());

// Statistics
Statistics stats = client.admin().stats();
System.out.println("Documents: " + stats.getDocumentCount());

// Cache management
client.admin().clearCache();
CacheStats cacheStats = client.admin().cacheStats();
```

### Error Handling

```java
try {
    QueryResult result = client.query()
        .aql("INVALID AQL")
        .execute();
} catch (ThemisDBApiException e) {
    System.err.println("API error " + e.getCode() + ": " + e.getMessage());
} catch (ThemisDBNetworkException e) {
    System.err.println("Network error: " + e.getMessage());
} catch (ThemisDBException e) {
    System.err.println("Error: " + e.getMessage());
}
```

### Async Operations

```java
// Using CompletableFuture
CompletableFuture<QueryResult> future = client.query()
    .aql("FOR doc IN myCollection RETURN doc")
    .executeAsync();

future.thenAccept(result -> {
    System.out.println("Query completed: " + result);
});

// LLM async inference
CompletableFuture<InferResponse> inferFuture = client.llm()
    .infer()
    .prompt("What is ThemisDB?")
    .executeAsync();
```

## Development

### Prerequisites

- Java 11 or later
- Maven 3.6+ or Gradle 7+

### Setup

```bash
# Clone the repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB/sdks/java

# Build the SDK
mvn clean install

# Run tests
mvn test

# Run examples
mvn exec:java -Dexec.mainClass="com.themisdb.examples.BasicExample"
```

### Project Structure

```
java/
├── src/
│   ├── main/
│   │   ├── java/
│   │   │   └── com/
│   │   │       └── themisdb/
│   │   │           └── client/
│   │   │               ├── ThemisDBClient.java
│   │   │               ├── api/           # API interfaces
│   │   │               ├── model/         # Data models
│   │   │               ├── exception/     # Exception classes
│   │   │               └── util/          # Utilities
│   │   └── resources/
│   └── test/
│       └── java/
│           └── com/
│               └── themisdb/
│                   └── client/
│                       └── ClientTest.java
├── examples/
│   └── BasicExample.java
├── pom.xml                    # Maven configuration
└── README.md                  # This file
```

## Testing

Run the test suite:

```bash
mvn test
```

Run with coverage:

```bash
mvn test jacoco:report
```

Run integration tests:

```bash
mvn verify -P integration-tests
```

## Examples

See the [examples](./examples) directory for usage examples:

- [BasicExample.java](./examples/BasicExample.java) - Basic operations
- [StreamingExample.java](./examples/StreamingExample.java) - Token streaming
- [RAGExample.java](./examples/RAGExample.java) - RAG operations

## Contributing

Contributions are welcome! Please see the [CONTRIBUTING.md](../../CONTRIBUTING.md) guide for details.

## License

Apache 2.0 - See [LICENSE](../../LICENSE) for details.

## Support

- Documentation: [docs.themisdb.org](https://docs.themisdb.org)
- GitHub Issues: [github.com/makr-code/ThemisDB/issues](https://github.com/makr-code/ThemisDB/issues)
- Community: [ThemisDB Discussions](https://github.com/makr-code/ThemisDB/discussions)
