---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Embedding Boundary for the Network Module

## Purpose and conclusion

This page clarifies where embedding work belongs when integrating an embedding
producer with ThemisDB. The `network` module is a transport and protocol
boundary: its source does not tokenize text, call an embedding model, or
transform content into vectors. There are no network-owned embedding models,
embedding dimensions, or vectorization quality metrics to tune here.

Here, “embedding” means machine-learning/vector embedding. Embedding this
network library into a host process is a separate integration topic covered
by [`IMPLEMENTATION.md`](IMPLEMENTATION.md).

## What the module does

The wire-protocol session dispatch contains a `VECTOR_SEARCH` handler and the
server can be constructed with a `VectorIndexManager`
([`wire_protocol_server.h`](../../include/network/wire_protocol_server.h)).
This is a request-transport/integration path to vector search, not an
embedding-generation API. The caller is responsible for sending the request
payload expected by the protocol and for ensuring any vector or embedding data
has already been produced in the appropriate application layer.

The network module does not define the model, text preprocessing, dimension,
normalization, distance metric, or version compatibility of those vectors.
Those are contracts of the embedding and vector-index components, not of
`src/network/`.

## Integration constraints

- Do not infer a vector format or model contract from the existence of the
  `handleVectorSearch()` dispatch entry.
- Treat the payload as untrusted network input and preserve the wire server’s
  frame and request validation before dispatch.
- A vector-search transport success is not evidence that a suitable index,
  model, or embedding version is configured.
- The wire server’s startup dependencies and operation availability are
  configuration-dependent; consult
  [`IMPLEMENTATION.md`](IMPLEMENTATION.md) and the vector-index API before
  claiming an end-to-end embedding workflow.

## Validation

For a change that only transports vector-search input, test framing, auth,
dispatch, and error behavior in the network focused suites. For embedding
generation or vector semantics, validate in the owning embedding/index module
as well; a network test cannot establish semantic quality or vector
compatibility.

See [`API.md`](API.md) for the network-facing handler boundary and
[`RESEARCH.md`](RESEARCH.md) for performance-evidence limits.
