# Sign Safety Guide - ThemisDB

## Overview

This document describes best practices for safely handling signed/unsigned integer types in ThemisDB to avoid common pitfalls and improve code quality.

## Why is Sign Safety Important?

Signed/unsigned mismatches can lead to serious bugs:

- **Buffer Overflows**: Negative indices are interpreted as large unsigned values
- **Infinite Loops**: Incorrect loop conditions due to implicit conversion
- **Arithmetic Errors**: Overflow/underflow in mixed operations
- **Security Vulnerabilities**: Unexpected behavior in critical operations

## Safe Arithmetic Utilities

ThemisDB provides helper functions in `include/utils/safe_arithmetic.h` for safe signed/unsigned operations.

See the full documentation in:
- German: `docs/de/guides/SIGN_SAFETY_GUIDE.md`
- Tests: `tests/test_safe_arithmetic.cpp`

## Quick Reference

Use `size_t` for sizes and indices. Use safe arithmetic utilities when mixing signed/unsigned types.
