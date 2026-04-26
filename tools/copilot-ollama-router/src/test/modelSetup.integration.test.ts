/**
 * Integration-style tests for ModelSetupManager HTTP behavior.
 *
 * Uses a real in-process HTTP server to simulate Ollama endpoints.
 */

import assert from "node:assert/strict";
import http from "node:http";
import type { AddressInfo } from "node:net";

// eslint-disable-next-line @typescript-eslint/no-require-imports
const { ModelSetupManager } = require("../modelSetup.js") as typeof import("../modelSetup.js");

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

async function stopServer(server: http.Server): Promise<void> {
  return new Promise((resolve, reject) => {
    server.close((err) => (err ? reject(err) : resolve()));
  });
}

describe("ModelSetupManager integration — /api/tags", () => {
  let server: http.Server;
  let baseUrl: string;

  before(async () => {
    ({ server, url: baseUrl } = await startMockServer((req, res) => {
      if (req.method === "GET" && req.url === "/api/tags") {
        res.writeHead(200, { "Content-Type": "application/json" });
        res.end(JSON.stringify({ models: [{ name: "codellama:13b" }, { name: "qwen2.5-coder:7b" }] }));
        return;
      }
      res.writeHead(404);
      res.end();
    }));
  });

  after(async () => stopServer(server));

  it("queryInstalledModels returns parsed model tags", async () => {
    const mgr = new ModelSetupManager(baseUrl);
    const tags = await mgr.queryInstalledModels();
    assert.deepEqual(tags, ["codellama:13b", "qwen2.5-coder:7b"]);
  });

  it("queryInstalledModels returns [] on invalid JSON", async () => {
    await stopServer(server);
    ({ server, url: baseUrl } = await startMockServer((req, res) => {
      if (req.method === "GET" && req.url === "/api/tags") {
        res.writeHead(200, { "Content-Type": "application/json" });
        res.end("{not-json");
        return;
      }
      res.writeHead(404);
      res.end();
    }));

    const mgr = new ModelSetupManager(baseUrl);
    const tags = await mgr.queryInstalledModels();
    assert.deepEqual(tags, []);
  });
});

describe("ModelSetupManager integration — /api/pull", () => {
  let server: http.Server;
  let baseUrl: string;

  before(async () => {
    ({ server, url: baseUrl } = await startMockServer((req, res) => {
      if (req.method === "POST" && req.url === "/api/pull") {
        res.writeHead(200, { "Content-Type": "application/x-ndjson" });
        res.write(JSON.stringify({ status: "downloading", completed: 5, total: 10 }) + "\n");
        res.write(JSON.stringify({ status: "downloading", completed: 10, total: 10 }) + "\n");
        res.write(JSON.stringify({ status: "success" }) + "\n");
        res.end();
        return;
      }
      res.writeHead(404);
      res.end();
    }));
  });

  after(async () => stopServer(server));

  it("pullModelForPanel emits progress including percentage", async () => {
    const mgr = new ModelSetupManager(baseUrl);
    const progress: Array<{ message: string; percent: number | null }> = [];

    await mgr.pullModelForPanel("codellama:13b", (message, percent) => {
      progress.push({ message, percent });
    });

    assert.ok(progress.length >= 2, "expected at least 2 progress events");
    assert.ok(progress.some((p) => p.percent === 50));
    assert.ok(progress.some((p) => p.percent === 100));
    assert.ok(progress.some((p) => p.message === "success"));
  });
});

describe("ModelSetupManager integration — /api/delete", () => {
  it("deleteModel resolves for 404 (already gone)", async () => {
    const { server, url } = await startMockServer((req, res) => {
      if (req.method === "DELETE" && req.url === "/api/delete") {
        res.writeHead(404, { "Content-Type": "application/json" });
        res.end(JSON.stringify({ error: "not found" }));
        return;
      }
      res.writeHead(500);
      res.end();
    });

    try {
      const mgr = new ModelSetupManager(url);
      await mgr.deleteModel("missing:model");
    } finally {
      await stopServer(server);
    }
  });

  it("deleteModel rejects for non-200/404 status", async () => {
    const { server, url } = await startMockServer((req, res) => {
      if (req.method === "DELETE" && req.url === "/api/delete") {
        res.writeHead(500, { "Content-Type": "application/json" });
        res.end(JSON.stringify({ error: "boom" }));
        return;
      }
      res.writeHead(404);
      res.end();
    });

    try {
      const mgr = new ModelSetupManager(url);
      await assert.rejects(
        () => mgr.deleteModel("broken:model"),
        (err: Error) => {
          assert.match(err.message, /HTTP 500/i);
          return true;
        }
      );
    } finally {
      await stopServer(server);
    }
  });
});
