/**
 * Ollama HTTP Client
 *
 * Wraps the Ollama local REST API (`/api/generate` and `/api/tags`).
 * Supports streaming responses so tokens are pushed to the VS Code chat
 * stream incrementally.
 *
 * API reference: https://github.com/ollama/ollama/blob/main/docs/api.md
 */

import * as https from "https";
import * as http from "http";
import { URL } from "url";

export interface OllamaGenerateOptions {
  model: string;
  prompt: string;
  /** Called for each streamed token chunk. */
  onToken: (token: string) => void;
  /** AbortSignal to cancel an in-flight request. */
  signal?: AbortSignal;
  /** Maximum wall-clock time to wait for a response (ms). */
  timeoutMs?: number;
}

export interface OllamaHealthResult {
  ok: boolean;
  models: string[];
  error?: string;
}

export class OllamaClient {
  constructor(private readonly endpoint: string) {}

  /**
   * Streams a completion from Ollama and calls `onToken` for each chunk.
   * Resolves with the full concatenated response text.
   */
  async generate(options: OllamaGenerateOptions): Promise<string> {
    const { model, prompt, onToken, signal, timeoutMs = 60_000 } = options;

    const body = JSON.stringify({
      model,
      prompt,
      stream: true,
    });

    const url = new URL("/api/generate", this.endpoint);
    const isHttps = url.protocol === "https:";
    const transport = isHttps ? https : http;

    return new Promise<string>((resolve, reject) => {
      const req = transport.request(
        {
          hostname: url.hostname,
          port: url.port || (isHttps ? 443 : 80),
          path: url.pathname,
          method: "POST",
          headers: {
            "Content-Type": "application/json",
            "Content-Length": Buffer.byteLength(body),
          },
        },
        (res) => {
          if (res.statusCode !== 200) {
            reject(
              new Error(
                `Ollama API returned HTTP ${res.statusCode ?? "unknown"}`
              )
            );
            return;
          }

          let fullText = "";
          res.setEncoding("utf8");

          res.on("data", (chunk: string) => {
            // Ollama streams newline-delimited JSON objects
            for (const line of chunk.split("\n")) {
              const trimmed = line.trim();
              if (!trimmed) {
                continue;
              }
              try {
                const parsed = JSON.parse(trimmed) as {
                  response?: string;
                  done?: boolean;
                };
                if (parsed.response) {
                  fullText += parsed.response;
                  onToken(parsed.response);
                }
              } catch {
                // Partial JSON line — ignore and wait for the next chunk
              }
            }
          });

          res.on("end", () => resolve(fullText));
          res.on("error", reject);
        }
      );

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
  async health(): Promise<OllamaHealthResult> {
    try {
      const raw = await this.getJson("/api/tags");
      const parsed = raw as { models?: Array<{ name: string }> };
      const models = (parsed.models ?? []).map((m) => m.name);
      return { ok: true, models };
    } catch (err) {
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

  private getJson(path: string): Promise<unknown> {
    const url = new URL(path, this.endpoint);
    const isHttps = url.protocol === "https:";
    const transport = isHttps ? https : http;

    return new Promise<unknown>((resolve, reject) => {
      const req = transport.get(
        {
          hostname: url.hostname,
          port: url.port || (isHttps ? 443 : 80),
          path: url.pathname + url.search,
        },
        (res) => {
          let data = "";
          res.setEncoding("utf8");
          res.on("data", (chunk: string) => (data += chunk));
          res.on("end", () => {
            try {
              resolve(JSON.parse(data));
            } catch {
              reject(new Error("Failed to parse Ollama JSON response."));
            }
          });
          res.on("error", reject);
        }
      );
      req.on("error", reject);
    });
  }
}
