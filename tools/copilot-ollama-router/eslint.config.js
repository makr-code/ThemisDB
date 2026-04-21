// @ts-check
const tseslint = require("typescript-eslint");

module.exports = tseslint.config(
  ...tseslint.configs.recommended,
  {
    rules: {
      // Strict safety rules
      "@typescript-eslint/no-explicit-any": "error",
      "@typescript-eslint/no-unused-vars": ["error", { argsIgnorePattern: "^_" }],
      "@typescript-eslint/explicit-function-return-type": ["error", { allowExpressions: true }],
      "@typescript-eslint/no-non-null-assertion": "error",
      "@typescript-eslint/strict-boolean-expressions": "off", // too noisy for VS Code extension patterns
      "no-console": "error",
      "no-debugger": "error",
      "eqeqeq": ["error", "always", { null: "ignore" }],
    },
  },
  {
    ignores: ["out/**", "node_modules/**"],
  }
);
