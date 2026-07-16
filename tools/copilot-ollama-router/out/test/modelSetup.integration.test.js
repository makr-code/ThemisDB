"use strict";
/**
 * Integration-style tests for ModelSetupManager HTTP behavior.
 *
 * Uses a real in-process HTTP server to simulate Ollama endpoints.
 */
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const strict_1 = __importDefault(require("node:assert/strict"));
const node_http_1 = __importDefault(require("node:http"));
// eslint-disable-next-line @typescript-eslint/no-require-imports
const { ModelSetupManager } = require("../modelSetup.js");
async function startMockServer(handler) {
    return new Promise((resolve, reject) => {
        const server = node_http_1.default.createServer(handler);
        server.on("error", reject);
        server.listen(0, "127.0.0.1", () => {
            const { port } = server.address();
            resolve({ server, url: `http://127.0.0.1:${port}` });
        });
    });
}
async function stopServer(server) {
    return new Promise((resolve, reject) => {
        server.close((err) => (err ? reject(err) : resolve()));
    });
}
describe("ModelSetupManager integration — /api/tags", () => {
    let server;
    let baseUrl;
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
        strict_1.default.deepEqual(tags, ["codellama:13b", "qwen2.5-coder:7b"]);
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
        strict_1.default.deepEqual(tags, []);
    });
});
describe("ModelSetupManager integration — /api/pull", () => {
    let server;
    let baseUrl;
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
        const progress = [];
        await mgr.pullModelForPanel("codellama:13b", (message, percent) => {
            progress.push({ message, percent });
        });
        strict_1.default.ok(progress.length >= 2, "expected at least 2 progress events");
        strict_1.default.ok(progress.some((p) => p.percent === 50));
        strict_1.default.ok(progress.some((p) => p.percent === 100));
        strict_1.default.ok(progress.some((p) => p.message === "success"));
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
        }
        finally {
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
            await strict_1.default.rejects(() => mgr.deleteModel("broken:model"), (err) => {
                strict_1.default.match(err.message, /HTTP 500/i);
                return true;
            });
        }
        finally {
            await stopServer(server);
        }
    });
});
//# sourceMappingURL=modelSetup.integration.test.js.map