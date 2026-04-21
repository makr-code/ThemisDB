"use strict";
/**
 * Ollama HTTP Client
 *
 * Wraps the Ollama local REST API (`/api/generate` and `/api/tags`).
 * Supports streaming responses so tokens are pushed to the VS Code chat
 * stream incrementally.
 *
 * API reference: https://github.com/ollama/ollama/blob/main/docs/api.md
 */
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
exports.OllamaClient = void 0;
const https = __importStar(require("https"));
const http = __importStar(require("http"));
const url_1 = require("url");
class OllamaClient {
    endpoint;
    constructor(endpoint) {
        this.endpoint = endpoint;
    }
    /**
     * Streams a completion from Ollama and calls `onToken` for each chunk.
     * Resolves with the full concatenated response text.
     */
    async generate(options) {
        const { model, prompt, onToken, signal, timeoutMs = 60_000 } = options;
        const body = JSON.stringify({
            model,
            prompt,
            stream: true,
        });
        const url = new url_1.URL("/api/generate", this.endpoint);
        const isHttps = url.protocol === "https:";
        const transport = isHttps ? https : http;
        return new Promise((resolve, reject) => {
            const req = transport.request({
                hostname: url.hostname,
                port: url.port || (isHttps ? 443 : 80),
                path: url.pathname,
                method: "POST",
                headers: {
                    "Content-Type": "application/json",
                    "Content-Length": Buffer.byteLength(body),
                },
            }, (res) => {
                if (res.statusCode !== 200) {
                    reject(new Error(`Ollama API returned HTTP ${res.statusCode ?? "unknown"}`));
                    return;
                }
                let fullText = "";
                res.setEncoding("utf8");
                res.on("data", (chunk) => {
                    // Ollama streams newline-delimited JSON objects
                    for (const line of chunk.split("\n")) {
                        const trimmed = line.trim();
                        if (!trimmed) {
                            continue;
                        }
                        try {
                            const parsed = JSON.parse(trimmed);
                            if (parsed.response) {
                                fullText += parsed.response;
                                onToken(parsed.response);
                            }
                        }
                        catch {
                            // Partial JSON line — ignore and wait for the next chunk
                        }
                    }
                });
                res.on("end", () => resolve(fullText));
                res.on("error", reject);
            });
            req.on("error", reject);
            // Honour AbortSignal
            if (signal) {
                signal.addEventListener("abort", () => {
                    req.destroy();
                    reject(new Error("Request aborted by caller."));
                });
            }
            // Honour timeout
            req.setTimeout(timeoutMs, () => {
                req.destroy();
                reject(new Error(`Ollama request timed out after ${timeoutMs} ms.`));
            });
            req.write(body);
            req.end();
        });
    }
    /**
     * Checks whether the Ollama server is reachable and returns the list of
     * locally available model names.
     */
    async health() {
        try {
            const raw = await this.getJson("/api/tags");
            const parsed = raw;
            const models = (parsed.models ?? []).map((m) => m.name);
            return { ok: true, models };
        }
        catch (err) {
            return {
                ok: false,
                models: [],
                error: err instanceof Error ? err.message : String(err),
            };
        }
    }
    // ---------------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------------
    getJson(path) {
        const url = new url_1.URL(path, this.endpoint);
        const isHttps = url.protocol === "https:";
        const transport = isHttps ? https : http;
        return new Promise((resolve, reject) => {
            const req = transport.get({
                hostname: url.hostname,
                port: url.port || (isHttps ? 443 : 80),
                path: url.pathname + url.search,
            }, (res) => {
                let data = "";
                res.setEncoding("utf8");
                res.on("data", (chunk) => (data += chunk));
                res.on("end", () => {
                    try {
                        resolve(JSON.parse(data));
                    }
                    catch {
                        reject(new Error("Failed to parse Ollama JSON response."));
                    }
                });
                res.on("error", reject);
            });
            req.on("error", reject);
        });
    }
}
exports.OllamaClient = OllamaClient;
//# sourceMappingURL=ollamaClient.js.map