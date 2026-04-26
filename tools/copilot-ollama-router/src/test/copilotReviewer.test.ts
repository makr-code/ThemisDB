/**
 * Unit tests for CopilotReviewer.review()
 */

import assert from "node:assert/strict";
import type { CancellationToken } from "vscode";

// eslint-disable-next-line @typescript-eslint/no-require-imports
const vscodeMock = require("./vscode-mock.js") as typeof import("./vscode-mock.js") & {
  LanguageModelError?: new (message: string) => Error;
};
// eslint-disable-next-line @typescript-eslint/no-require-imports
const { CopilotReviewer } = require("../copilotReviewer.js") as typeof import("../copilotReviewer.js");

function makeToken(): CancellationToken {
  return {
    isCancellationRequested: false,
    onCancellationRequested: () => ({
      dispose: (): void => undefined,
    }),
  } as unknown as CancellationToken;
}

describe("CopilotReviewer.review", () => {
  it("returns false when no Copilot model is available", async () => {
    vscodeMock.lm.selectChatModels = async (): Promise<unknown[]> => [];

    const reviewer = new CopilotReviewer();
    let collected = "";

    const ok = await reviewer.review(
      "const x = 1;",
      "check this",
      (fragment: string): void => {
        collected += fragment;
      },
      makeToken()
    );

    assert.equal(ok, false);
    assert.equal(collected, "");
  });

  it("streams fragments and returns true when Copilot responds", async () => {
    const fakeModel = {
      sendRequest: async (): Promise<{ text: AsyncIterable<string> }> => ({
        text: (async function* (): AsyncGenerator<string> {
          yield "- issue 1\n";
          yield "- issue 2\n";
        })(),
      }),
    };

    vscodeMock.lm.selectChatModels = async (): Promise<unknown[]> => [fakeModel];

    const reviewer = new CopilotReviewer();
    let collected = "";

    const ok = await reviewer.review(
      "int main() {}",
      "review this",
      (fragment: string): void => {
        collected += fragment;
      },
      makeToken()
    );

    assert.equal(ok, true);
    assert.equal(collected, "- issue 1\n- issue 2\n");
  });

  it("returns false and emits unavailable note on LanguageModelError", async () => {
    class MockLanguageModelError extends Error {}
    vscodeMock.LanguageModelError = MockLanguageModelError;

    const fakeModel = {
      sendRequest: async (): Promise<never> => {
        throw new MockLanguageModelError("no access");
      },
    };

    vscodeMock.lm.selectChatModels = async (): Promise<unknown[]> => [fakeModel];

    const reviewer = new CopilotReviewer();
    let collected = "";

    const ok = await reviewer.review(
      "int main() {}",
      "review this",
      (fragment: string): void => {
        collected += fragment;
      },
      makeToken()
    );

    assert.equal(ok, false);
    assert.match(collected, /Copilot review unavailable/i);
  });
});
