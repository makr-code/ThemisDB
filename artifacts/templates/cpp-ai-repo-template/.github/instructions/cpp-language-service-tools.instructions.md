---
description: Enforce semantic symbol tools for C/C++ impact analysis.
applyTo: "**/*.{c,cc,cpp,cxx,h,hh,hpp,hxx,ipp,tpp}"
---

- Use semantic symbol tools for rename, signature changes, and impact checks.
- Do not rely on text-only search for final C/C++ symbol analysis.
- For behavior-affecting changes, inspect references and call hierarchy first.
