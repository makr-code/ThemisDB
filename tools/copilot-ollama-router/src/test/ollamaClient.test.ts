/**
 * Unit tests for OllamaClient
 *
 * Spins up a real in-process HTTP server that simulates Ollama's
 * /api/generate (streaming NDJSON) and /api/tags endpoints.
 * No external process required.
 */

import assert from "node:assert/strict";
import http from "node:http";
import type { AddressInfo } from "node:net";
// eslint-disable-next-line @typescript-eslint/no-require-imports
const { OllamaClient } = require("../ollamaClient.js") as typeof import("../ollamaClient.js");

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/** Start a minimal Ollama mock server. Returns the server + its base URL. */
async function startMockServer(
  handler: (req: http.IncomingMessage, res: http.ServerResponse) => void
): Promise<{ server: http.Server; url: string }> {
  return new Promise((resolve, reject) => {
    const server = http.createServer(handler);
    server.on("error", reject);
    server.listen(0, "127.0.0.1", () => {
      const { port } = server.address() as AddressInfo;
      resolve({ server, url: `http://127.0.0.1:${port}` });
    });
  });
}

/** Stop the server and wait for it to close. */
async function stopServer(server: http.Server): Promise<void> {
  return new Promise((resolve, reject) => {
    server.close((err) => (err ? reject(err) : resolve()));
  });
}

/** Build an NDJSON streaming response body for Ollama /api/generate. */
function makeStreamBody(tokens: string[]): string {
  const lines = tokens.map((t, i) =>
    JSON.stringify({ response: t, done: i === tokens.length - 1 })
  );
  return lines.join("\n") + "\n";
}

// ---------------------------------------------------------------------------
// generate() — happy path
// ---------------------------------------------------------------------------

describe("OllamaClient.generate — happy path", () => {
  let server: http.Server;
  let baseUrl: string;

  before(async () => {
    ({ server, url: baseUrl } = await startMockServer((req, res) => {
      if (req.method === "POST" && req.url === "/api/generate") {
        res.writeHead(200, { "Content-Type": "application/x-ndjson" });
        res.end(makeStreamBody(["Hello", " ", "world", "!"]));
      } else {
        res.writeHead(404);
        res.end();
      }
    }));
  });

  after(async () => stopServer(server));

  it("resolves with concatenated token text", async () => {
    const client = new OllamaClient(baseUrl);
    const tokens: string[] = [];
    const result = await client.generate({
      model: "codellama:13b",
      prompt: "Say hello",
      onToken: (t) => tokens.push(t),
    });
    assert.equal(result, "Hello world!");
  });

  it("calls onToken for each token", async () => {
    const client = new OllamaClient(baseUrl);
    const tokens: string[] = [];
    await client.generate({
      model: "codellama:13b",
      prompt: "Say hello",
      onToken: (t) => tokens.push(t),
    });
    assert.deepEqual(tokens, ["Hello", " ", "world", "!"]);
  });
});

// ---------------------------------------------------------------------------
// generate() — HTTP error status
// ---------------------------------------------------------------------------

describe("OllamaClient.generate — HTTP error", () => {
  let server: http.Server;
  let baseUrl: string;

  before(async () => {
    ({ server, url: baseUrl } = await startMockServer((_req, res) => {
      res.writeHead(503);
      res.end("Service Unavailable");
    }));
  });

  after(async () => stopServer(server));

  it("rejects with an error mentioning the HTTP status code", async () => {
    const client = new OllamaClient(baseUrl);
    await assert.rejects(
      () =>
        client.generate({
          model: "codellama:13b",
          prompt: "test",
          onToken: () => undefined,
        }),
      (err: Error) => {
        assert.ok(err.message.includes("503"), `Expected 503 in error: ${err.message}`);
        return true;
      }
    );
  });
});

// ---------------------------------------------------------------------------
// generate() — AbortSignal
// ---------------------------------------------------------------------------

describe("OllamaClient.generate — AbortSignal", () => {
  let server: http.Server;
  let baseUrl: string;

  before(async () => {
    ({ server, url: baseUrl } = await startMockServer((_req, res) => {
      // Slow server: writes one token then waits forever
      res.writeHead(200, { "Content-Type": "application/x-ndjson" });
      res.write(JSON.stringify({ response: "tok1", done: false }) + "\n");
      // Never calls res.end() → forces client to wait
    }));
  });

  after(async () => stopServer(server));

  it("rejects with abort error when signal fires", async () => {
    const client = new OllamaClient(baseUrl);
    const controller = new AbortController();

    const promise = client.generate({
      model: "codellama:13b",
      prompt: "test",
      onToken: () => undefined,
      signal: controller.signal,
      timeoutMs: 5_000,
    });

    // Abort almost immediately
    setImmediate(() => controller.abort());

    await assert.rejects(
      () => promise,
      (err: Error & { code?: string }) => {
        assert.ok(
          err.message.toLowerCase().includes("abort") ||
            err.message.toLowerCase().includes("destroy") ||
            err.code === "ECONNRESET",
          `Expected abort/destroy error, got: ${err.message}`
        );
        return true;
      }
    );
  });
});

// ---------------------------------------------------------------------------
// generate() — timeout
// ---------------------------------------------------------------------------

describe("OllamaClient.generate — timeout", () => {
  let server: http.Server;
  let baseUrl: string;

  before(async () => {
    ({ server, url: baseUrl } = await startMockServer((_req, res) => {
      res.writeHead(200, { "Content-Type": "application/x-ndjson" });
      // Never sends data
    }));
  });

  after(async () => stopServer(server));

  it("rejects with a timeout error after timeoutMs", async () => {
    const client = new OllamaClient(baseUrl);

    await assert.rejects(
      () =>
        client.generate({
          model: "codellama:13b",
          prompt: "test",
          onToken: () => undefined,
          timeoutMs: 50, // very short
        }),
      (err: Error) => {
        assert.ok(
          err.message.toLowerCase().includes("timeout") ||
            err.message.toLowerCase().includes("timed out"),
          `Expected timeout error, got: ${err.message}`
        );
        return true;
      }
    );
  });
});

// ---------------------------------------------------------------------------
// health() — server reachable
// ---------------------------------------------------------------------------

describe("OllamaClient.health — reachable", () => {
  let server: http.Server;
  let baseUrl: string;

  before(async () => {
    ({ server, url: baseUrl } = await startMockServer((req, res) => {
      if (req.url === "/api/tags") {
        res.writeHead(200, { "Content-Type": "application/json" });
        res.end(
          JSON.stringify({
            models: [{ name: "codellama:13b" }, { name: "qwen2.5-coder:7b" }],
          })
        );
      } else {
        res.writeHead(404);
        res.end();
      }
    }));
  });

  after(async () => stopServer(server));

  it("returns ok=true", async () => {
    const client = new OllamaClient(baseUrl);
    const result = await client.health();
    assert.equal(result.ok, true);
  });

  it("returns model list", async () => {
    const client = new OllamaClient(baseUrl);
    const result = await client.health();
    assert.ok(result.models.includes("codellama:13b"));
    assert.ok(result.models.includes("qwen2.5-coder:7b"));
  });
});

// ---------------------------------------------------------------------------
// health() — server unreachable
// ---------------------------------------------------------------------------

describe("OllamaClient.health — unreachable", () => {
  it("returns ok=false with error message", async () => {
    // Port 1 is almost certainly closed
    const client = new OllamaClient("http://127.0.0.1:1");
    const result = await client.health();
    assert.equal(result.ok, false);
    assert.ok(result.error, "Expected error field to be set");
  });
});

// ---------------------------------------------------------------------------
// generate() — partial / malformed NDJSON lines
// ---------------------------------------------------------------------------

describe("OllamaClient.generate — malformed NDJSON", () => {
  let server: http.Server;
  let baseUrl: string;

  before(async () => {
    ({ server, url: baseUrl } = await startMockServer((_req, res) => {
      res.writeHead(200, { "Content-Type": "application/x-ndjson" });
      // Mix of valid, empty, and malformed lines
      res.end(
        [
          JSON.stringify({ response: "good1", done: false }),
          "", // empty line
          "INVALID_JSON",
          JSON.stringify({ response: "good2", done: true }),
        ].join("\n") + "\n"
      );
    }));
  });

  after(async () => stopServer(server));

  it("ignores malformed lines and returns valid tokens concatenated", async () => {
    const client = new OllamaClient(baseUrl);
    const result = await client.generate({
      model: "codellama:13b",
      prompt: "test",
      onToken: () => undefined,
    });
    assert.equal(result, "good1good2");
  });
});
