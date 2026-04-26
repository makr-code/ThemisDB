/**
 * Mocha setup file — patches Node's require so that `require('vscode')`
 * returns our lightweight mock instead of throwing "Cannot find module 'vscode'".
 *
 * Must be listed in .mocharc via --require before any test file.
 */

// eslint-disable-next-line @typescript-eslint/no-require-imports
const Module = require("module");

// Store original _resolveFilename
const _origResolveFilename = Module._resolveFilename.bind(Module);

Module._resolveFilename = function (
  request: string,
  parent: unknown,
  isMain: boolean,
  options: unknown
): string {
  if (request === "vscode") {
    // Resolve to our compiled mock module
    return require.resolve("./vscode-mock.js");
  }
  return _origResolveFilename(request, parent, isMain, options);
};
