## 🔍 Tiefenanalyse-Update (2026-04-07): Falscher Root Cause — AVX2/SIMD existiert bereits

### Was das Issue falsch beschreibt

Das Issue behauptet: *"Scalar loop in Gorilla decode — No explicit AVX2/AVX-512"*

Das ist **unzutreffend**. `src/timeseries/gorilla_simd.cpp` enthält:

- **`prefix_sum_i64()`** (Z. 164–236): vollständiger AVX2-Pfad (4×int64 pro Iteration, Kogge-Stone Prefix-Scan) + NEON-Fallback für ARM
- **`prefix_xor_u64()`** (Z. 247–300): vollständiger AVX2-Pfad für Double-Bit-Patterns + NEON-Fallback
- **`GorillaSIMDDecoder::decodeAll()`** (Z. 307–384): 2-Phasen-Architektur (Phase 1: scalar parse → Phase 2: SIMD bulk)

Die Phase-2 SIMD-Implementierung ist korrekt und **nicht** der Engpass.

### Tatsächlicher Bottleneck: Phase 1 (`parse_gorilla_chunk`, Z. 102–153)

**Problem 1: `BitReader::readBits(N)` — Bit-für-Bit-Schleife**

```cpp
uint64_t BitReader::readBits(int bits) {
    uint64_t v = 0;
    for (int i = 0; i < bits; ++i) {       // 64 Iterationen für readBits(64)!
        if (readBit()) v |= (1ULL << i);   // pro Bit: Bounds-Check + Bit-Extract
    }
    return v;
}
```

Für `readBits(64)` (ein `first_vbits`-Read) sind das 64 Schleifeniterationen mit je einem Bounds-Check. Fix: Byte-orientierter `BitReader` (liest N Bytes direkt via `buf_[byte_idx]` + Shift).

**Problem 2: `readVarUInt()` ruft `readBits(8)` statt direktem Byte-Read auf**

Jedes LEB128-Byte kostet 8 Bit-Iterationen statt eines einzigen `uint8_t buf_[byte_idx++]`.

**Problem 3: Kein `reserve()` auf `dods`/`xorvals` (Z. 98–99)**

```cpp
struct GorillaParsed {
    std::vector<int64_t>  dods;    // kein reserve() vor push_back-Schleife
    std::vector<uint64_t> xorvals; // kein reserve() vor push_back-Schleife
};
```

Für einen typischen 128-Punkte-Chunk: ~7 Reallokationen (2→4→8→16→32→64→128).

**Problem 4: Unnötige `payload`-Kopie (Z. 330)**

```cpp
const std::vector<uint8_t> payload(payload_ptr, payload_ptr + payload_size);
```

Kopiert die gesamten Chunk-Daten nur um den 3-Byte-Header zu überspringen. Fix: `BitReader` mit `payload_ptr`-Offset direkt konstruieren.

### Korrigierte Akzeptanzkriterien

- ✅ Phase 2 (AVX2 prefix-sum/XOR) — kein Änderungsbedarf
- 🔧 Phase 1: Byte-orientierter `BitReader` (kein Bit-für-Bit-Loop)
- 🔧 `reserve(expected_chunk_size)` vor der `push_back`-Schleife in `parse_gorilla_chunk()`
- 🔧 Header-Strip via Offset statt `std::vector`-Kopie (Z. 330)
- Ziel >2 GB/s bleibt bestehen und ist durch Phase-1-Optimierung allein erreichbar
- Bit-identische Ausgabe gegenüber bisheriger Implementierung (kein functionales Risiko)
