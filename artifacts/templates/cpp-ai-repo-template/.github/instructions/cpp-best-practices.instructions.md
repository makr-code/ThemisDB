---
description: Modern C++ safety and readability requirements.
applyTo: "**/*.{c,cc,cpp,cxx,h,hh,hpp,hxx,ipp,tpp}"
---

- Prefer RAII-based ownership and deterministic cleanup.
- Prefer `std::unique_ptr` and `std::shared_ptr` over raw ownership.
- Prefer const-correct APIs.
- Prefer `std::string_view` and `std::span` for non-owning inputs.
- Keep critical sections short and use `std::lock_guard`.
- Avoid global mutable state.
- Keep code simple and maintainable.
