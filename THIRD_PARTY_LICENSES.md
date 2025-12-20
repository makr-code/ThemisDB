# Third-Party Licenses

ThemisDB v1.3.0 uses the following third-party libraries and components. This document provides the complete list of dependencies and their respective licenses.

---

## Core Dependencies

### 1. RocksDB
- **License:** Apache License 2.0 (with optional GPL 2.0 components not used)
- **Copyright:** Facebook, Inc. and its affiliates
- **Website:** https://rocksdb.org/
- **Usage:** Primary key-value storage engine

### 2. OpenSSL
- **License:** Apache License 2.0 (v3.x)
- **Copyright:** The OpenSSL Project
- **Website:** https://www.openssl.org/
- **Usage:** Cryptography and TLS/SSL

### 3. simdjson
- **License:** Apache License 2.0
- **Copyright:** Daniel Lemire and contributors
- **Website:** https://github.com/simdjson/simdjson
- **Usage:** High-performance JSON parsing

### 4. Intel TBB (Threading Building Blocks)
- **License:** Apache License 2.0
- **Copyright:** Intel Corporation
- **Website:** https://github.com/oneapi-src/oneTBB
- **Usage:** Parallel programming and task scheduling

### 5. Apache Arrow
- **License:** Apache License 2.0
- **Copyright:** The Apache Software Foundation
- **Website:** https://arrow.apache.org/
- **Usage:** Columnar data format and analytics

### 6. HNSWlib
- **License:** Apache License 2.0
- **Copyright:** Yu. A. Malkov and D. A. Yashunin
- **Website:** https://github.com/nmslib/hnswlib
- **Usage:** Vector similarity search (HNSW algorithm)

### 7. Boost (Asio, Beast)
- **License:** Boost Software License 1.0
- **Copyright:** Boost.org contributors
- **Website:** https://www.boost.org/
- **Usage:** Asynchronous I/O and HTTP/WebSocket support

### 8. spdlog
- **License:** MIT License
- **Copyright:** Gabi Melman and contributors
- **Website:** https://github.com/gabime/spdlog
- **Usage:** Fast C++ logging library

### 9. nlohmann-json
- **License:** MIT License
- **Copyright:** Niels Lohmann
- **Website:** https://github.com/nlohmann/json
- **Usage:** JSON for Modern C++

### 10. OpenTelemetry C++
- **License:** Apache License 2.0
- **Copyright:** The OpenTelemetry Authors
- **Website:** https://opentelemetry.io/
- **Usage:** Observability and distributed tracing

### 11. cURL
- **License:** curl License (MIT-style)
- **Copyright:** Daniel Stenberg and contributors
- **Website:** https://curl.se/
- **Usage:** HTTP client library

### 12. yaml-cpp
- **License:** MIT License
- **Copyright:** Jesse Beder
- **Website:** https://github.com/jbeder/yaml-cpp
- **Usage:** YAML parser and emitter

### 13. zstd (Zstandard)
- **License:** BSD 3-Clause License (library), GPL 2.0 (CLI - not used)
- **Copyright:** Facebook, Inc. and its affiliates
- **Website:** https://facebook.github.io/zstd/
- **Usage:** Fast lossless compression

### 14. mimalloc
- **License:** MIT License
- **Copyright:** Microsoft Corporation
- **Website:** https://github.com/microsoft/mimalloc
- **Usage:** High-performance memory allocator

### 15. Google Test
- **License:** BSD 3-Clause License
- **Copyright:** Google Inc.
- **Website:** https://github.com/google/googletest
- **Usage:** C++ testing framework

### 16. Google Benchmark
- **License:** Apache License 2.0
- **Copyright:** Google Inc.
- **Website:** https://github.com/google/benchmark
- **Usage:** Microbenchmark support library

### 17. fmt
- **License:** MIT License
- **Copyright:** Victor Zverovich
- **Website:** https://fmt.dev/
- **Usage:** Modern formatting library

---

## LLM Dependencies (v1.3.0)

### 18. llama.cpp
- **License:** MIT License
- **Copyright:** Georgi Gerganov and contributors
- **Website:** https://github.com/ggerganov/llama.cpp
- **Usage:** LLM inference engine for GGUF models

---

## RPC Dependencies (v1.3.0)

### 19. gRPC
- **License:** Apache License 2.0
- **Copyright:** The gRPC Authors
- **Website:** https://grpc.io/
- **Usage:** High-performance RPC framework

### 20. Protocol Buffers (Protobuf)
- **License:** BSD 3-Clause License
- **Copyright:** Google Inc.
- **Website:** https://protobuf.dev/
- **Usage:** Data serialization

---

## GPU Dependencies (Optional)

### 21. FAISS
- **License:** MIT License
- **Copyright:** Facebook AI Research
- **Website:** https://github.com/facebookresearch/faiss
- **Usage:** GPU-accelerated vector similarity search

### 22. NVIDIA CUDA Toolkit
- **License:** NVIDIA CUDA EULA (Proprietary)
- **Copyright:** NVIDIA Corporation
- **Website:** https://developer.nvidia.com/cuda-toolkit
- **Usage:** GPU computing platform (not redistributed, user-installed)
- **Note:** ThemisDB does not redistribute CUDA; users must install it separately

---

## License Texts

### MIT License

```
MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### Apache License 2.0

Full text available at: https://www.apache.org/licenses/LICENSE-2.0

### BSD 3-Clause License

```
BSD 3-Clause License

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

### Boost Software License 1.0

Full text available at: https://www.boost.org/LICENSE_1_0.txt

---

## License Summary

| License Type | Count | Libraries |
|--------------|-------|-----------|
| **MIT** | 7 | spdlog, nlohmann-json, yaml-cpp, mimalloc, fmt, llama.cpp, FAISS |
| **Apache 2.0** | 11 | RocksDB, OpenSSL, simdjson, TBB, Arrow, HNSWlib, OpenTelemetry, Benchmark, gRPC |
| **BSD 3-Clause** | 3 | Google Test, Protobuf, zstd (library) |
| **Boost License** | 1 | Boost (Asio, Beast) |
| **curl License** | 1 | cURL |
| **Proprietary** | 1 | CUDA Toolkit (optional, user-installed) |

**Total:** 22 dependencies + 2 optional

---

## Attribution Notice

ThemisDB includes software developed by:
- The Apache Software Foundation (http://www.apache.org/)
- Facebook, Inc. and its affiliates
- Google Inc.
- Microsoft Corporation
- Intel Corporation
- The OpenSSL Project
- Boost.org
- The OpenTelemetry Authors
- And many other open-source contributors

We are grateful to all the open-source projects and their contributors that make ThemisDB possible.

---

## Compliance

All third-party licenses are compatible with ThemisDB's MIT License with Government Clause. For detailed license compatibility analysis, see `docs/legal/LICENSE_COMPATIBILITY_ANALYSIS.md`.

---

**ThemisDB License:** MIT License with Government Clause  
**Document Version:** 1.0  
**Last Updated:** 17. Dezember 2025
