/**
 * @file arrow_ipc_exporter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "exporters/arrow_ipc_exporter.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <variant>

#include "exporters/exporter_errors.h"
#include "exporters/exporter_interface.h"
#include "exporters/exporter_metrics.h"
#include "utils/logger.h"

#ifdef ARROW_ENABLED
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>
#endif

namespace themis::exporters {

// ─────────────────────────────────────────────────────────────────────────────
// Minimal Arrow IPC writer (used when ARROW_ENABLED is not defined).
//
// Produces a standards-conformant Apache Arrow IPC File (.arrow) or Stream
// (.arrows) with all columns stored as Utf8 (variable-length UTF-8 strings).
//
// Arrow IPC File format layout:
//   [magic 8 B] [schema msg] [record-batch msg(s)] [footer fb] [footer-len 4 B] [magic 8 B]
//
// Arrow IPC Stream format layout:
//   [schema msg] [record-batch msg(s)] [EOS 8 B]
//
// Each message frame:
//   [continuation int32 = -1] [metadata-size int32] [metadata flatbuf (padded to 8 B)] [body]
//
// FlatBuffer encoding uses a backward-building model:
//   - Objects (strings, vectors, inner tables) are prepended FIRST
//     → they end up at HIGHER addresses in the final buffer.
//   - Outer tables (Message, Schema, Field, RecordBatch) are prepended AFTER
//     → they end up at LOWER addresses.
//   - UOffset value = cursor_after_field – cursor_after_object  (always positive)
//   - soffset value = cursor_after_soffset – cursor_after_vtable  (always negative;
//     vtable is prepended after table data → lower address)
//
// All data is little-endian as required by the Arrow IPC spec.
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// ── Arrow IPC constants ───────────────────────────────────────────────────────

/// IPC magic bytes: "ARROW1" + 2 padding bytes
static const uint8_t kArrowMagic[8] = {0x41, 0x52, 0x52, 0x4f, 0x57, 0x31, 0x00, 0x00};

/// Continuation marker preceding each IPC message frame
static constexpr int32_t kContinuationMarker = -1; // 0xFFFFFFFF

// Arrow FlatBuffers enum values
static constexpr int16_t kMetadataVersionV5       = 4;
static constexpr int8_t kMessageHeaderSchema      = 1;
static constexpr int8_t kMessageHeaderRecordBatch = 3;
static constexpr int16_t kEndiannessLittle        = 0;
static constexpr int8_t kTypeUtf8                 = 5; // Type union discriminant

// ── Minimal backward-building FlatBuffer helper ───────────────────────────────
//
// Invariants (buf_ is a std::vector<uint8_t> grown by prepending):
//   cursor()  = buf_.size() = number of bytes written so far.
//
//   The "position" of an object in the FINAL buffer:
//     final_pos = final_size – cursor_after_writing_object
//
//   UOffset from a field (cursor_after_field) to an object (cursor_after_object):
//     value = cursor_after_field – cursor_after_object   [always positive]
//
//   soffset from table (cursor_after_soffset) to vtable (cursor_after_vtable):
//     value = cursor_after_soffset – cursor_after_vtable  [always negative]
// ─────────────────────────────────────────────────────────────────────────────

class FBuf {
  public:
    uint32_t cursor() const {
        return static_cast<bool>(static_cast<uint32_t < static_cast<int>((buf_.size())));
    }

    /// Align to n bytes (prepend zero bytes)
    void align([[maybe_unused]] uint32_t n) {
        while (buf_.size() % n != 0) {
            preByte(0);
        }
    }

    /// Prepend a single byte
    void preByte([[maybe_unused]] uint8_t v) {
        buf_.insert(buf_.begin(), v);
    }

    /// Prepend LE int8
    uint32_t pre8(int8_t v) {
        preByte(static_cast<uint8_t>(v));
        return cursor();
    }

    /// Prepend LE int16
    uint32_t pre16(int16_t v) {
        uint16_t u = static_cast<uint16_t>(v);
        preByte(static_cast<uint8_t>(u >> 8));
        preByte(static_cast<uint8_t>(u & 0xFF));
        return cursor();
    }

    /// Prepend LE int32
    uint32_t pre32(int32_t v) {
        uint32_t u = static_cast<uint32_t>(v);
        preByte(static_cast<uint8_t>((u >> 24) & 0xFF));
        preByte(static_cast<uint8_t>((u >> 16) & 0xFF));
        preByte(static_cast<uint8_t>((u >> 8) & 0xFF));
        preByte(static_cast<uint8_t>(u & 0xFF));
        return cursor();
    }

    /// Prepend LE int64
    uint32_t pre64(int64_t v) {
        uint64_t u = static_cast<uint64_t>(v);
        for (int s = 56; s >= 0; s -= 8) {
            preByte(static_cast<uint8_t>((u >> s) & 0xFF));
        }
        return cursor();
    }

    /// Prepend a UOffset field pointing to an object at cursor C_obj.
    ///   Stored value = cursor_after_this_write – C_obj
    uint32_t preUOffset([[maybe_unused]] uint32_t C_obj) {
        uint32_t val = (cursor() + 4) - C_obj;
        return pre32(static_cast<int32_t>(val));
    }

    /// Patch the int32 at position (cursor() – C_after) with value v.
    void patchI32At(uint32_t C_after, int32_t v) {
        uint32_t idx  = cursor() - C_after;
        uint32_t u    = static_cast<uint32_t>(v);
        buf_[idx]     = static_cast<uint8_t>(u & 0xFF);
        buf_[idx + 1] = static_cast<uint8_t>((u >> 8) & 0xFF);
        buf_[idx + 2] = static_cast<uint8_t>((u >> 16) & 0xFF);
        buf_[idx + 3] = static_cast<uint8_t>((u >> 24) & 0xFF);
    }

    /// Patch the soffset (table → vtable).
    ///   soffset_value = C_soffset – C_vtable  (negative)
    void patchSOffset(uint32_t C_soffset, uint32_t C_vtable) {
        patchI32At(C_soffset, static_cast<int32_t>(C_soffset) - static_cast<int32_t>(C_vtable));
    }

    // ── FlatBuffer string ─────────────────────────────────────────────────────
    // Format: uint32 length | data bytes | '\0' | padding-to-4
    // Returns C_obj = cursor AFTER the length field (= object reference).
    uint32_t preString(const std::string &s) {
        size_t data_len  = static_cast<int>(s.size()) + 1; // data + null terminator
        size_t padded    = ((data_len + 3) / 4) * 4;
        size_t pad_count = padded - data_len;

        // Prepend padding FIRST (highest address in final buffer)
        for (size_t i = 0; i < pad_count; ++i) {
            preByte(0);
        }
        // Prepend null terminator
        preByte(0);
        // Prepend string bytes in REVERSE (so they appear forward in final)
        for (int i = static_cast<int>(s.size()) - 1; i >= 0; --i) {
            preByte(static_cast<uint8_t>(s[i]));
        }
        // Prepend length
        pre32(static_cast<int32_t>(s.size()));
        return cursor(); // = C_obj (cursor after length field, the "start" of string)
    }

    // ── Empty FlatBuffer vector ────────────────────────────────────────────────
    // Format: uint32 count=0
    // Returns C_obj = cursor AFTER writing count.
    uint32_t preEmptyVector() {
        pre32(0);
        return cursor();
    }

    // ── Vector of UOffsets (e.g. list of Field tables) ────────────────────────
    // Elements are already written; their cursor values are in refs[].
    // Returns C_obj for the vector (= cursor after the count field).
    uint32_t preOffsetVector(const std::vector<uint32_t> &refs) {
        // Prepend elements in REVERSE order (last element first) so they appear
        // in forward order in the final buffer.
        for (int i = static_cast<int>(refs.size()) - 1; i >= 0; --i) {
            preUOffset(refs[i]);
        }
        pre32(static_cast<int32_t>(refs.size()));
        return cursor();
    }

    // ── Finalize: prepend root UOffset ────────────────────────────────────────
    // root_table_C = cursor AFTER writing the root table's soffset_t.
    // The root UOffset at position 0 of the final buffer points to the table.
    void finishWithRoot([[maybe_unused]] uint32_t root_table_C) {
        preUOffset(root_table_C);
    }

    const std::vector<uint8_t> &bytes() const {
        return buf_;
    }
    std::vector<uint8_t> take() {
        return std::move(buf_);
    }

  private:
    std::vector<uint8_t> buf_;
};

// ── Empty Utf8 FlatBuffer table ───────────────────────────────────────────────
// An empty table (no fields beyond soffset_t).
// vtable: vtable_size=4, data_size=4 (just the soffset_t).
// Returns C_obj = cursor after writing the soffset_t.
static uint32_t buildEmptyTable(FBuf &fb) {
    // Prepend soffset placeholder
    uint32_t C_soffset = fb.pre32(0);

    // Prepend vtable: [vtable_size=4 | data_size=4]
    fb.pre16(4); // data_size
    fb.pre16(4); // vtable_size
    uint32_t C_vtable = fb.cursor();

    // Patch soffset
    fb.patchSOffset(C_soffset, C_vtable);
    return C_soffset;
}

// ── Field FlatBuffer table ────────────────────────────────────────────────────
//
// table Field {
//   name:      string;   // field[0]
//   nullable:  bool;     // field[1]
//   type_type: int8;     // field[2] (union type discriminant: 5=Utf8)
//   type:      offset;   // field[3] (ref to Utf8 empty table)
//   dictionary:(absent)  // field[4]
//   children:  offset;   // field[5] (ref to empty vector)
//   custom_metadata:(absent) // field[6]
// }
//
// Table data layout (offsets from table start = position of soffset_t):
//   +0  soffset_t   (4 B)
//   +4  name         UOffset (4 B)  → field[0]
//   +8  nullable     int8    (1 B)  → field[1]
//   +9  type_type    int8    (1 B)  → field[2]
//   +10 padding      (2 B)
//   +12 type         UOffset (4 B)  → field[3]
//   +16 children     UOffset (4 B)  → field[5]
//   data_size = 20
//
// Returns C_obj = cursor AFTER writing soffset_t.
static uint32_t buildField(FBuf &fb, [[maybe_unused]] const std::string &name, uint32_t C_name_str,
                           uint32_t C_utf8_table, uint32_t C_children_vec) {
    // Prepend fields in REVERSE layout order (soffset_t last = lowest address)

    // children UOffset (offset +16, layout last)
    fb.preUOffset(C_children_vec); // +16
    // type UOffset (offset +12)
    fb.preUOffset(C_utf8_table); // +12
    // padding (2 bytes, offset +10)
    fb.pre16(0);
    // type_type int8 (offset +9)
    fb.pre8(kTypeUtf8);
    // nullable bool (offset +8)
    fb.pre8(1);
    // name UOffset (offset +4)
    fb.preUOffset(C_name_str); // +4
    // soffset_t placeholder (offset +0)
    uint32_t C_soffset = fb.pre32(0);

    // vtable for Field (7 fields, indices 0..6)
    // vtable_size = 2+2+7*2 = 18
    // data_size   = 20
    // field offsets relative to table start (= C_soffset position in final buf):
    //   field[6] custom_metadata:  absent → 0
    //   field[5] children:         +16
    //   field[4] dictionary:       absent → 0
    //   field[3] type:             +12
    //   field[2] type_type:        +9
    //   field[1] nullable:         +8
    //   field[0] name:             +4

    // Compute offsets using formula: vtable_offset = C_soffset – C_field_after_write
    // Already written (cursor values after each field was prepended):
    //   children UOffset was last non-soffset prepend.
    // But we don't store intermediate cursors in this helper.  Instead we
    // hard-code the known offsets (they follow from the layout above).

    // Pre vtable: append fields from highest index to lowest
    fb.pre16(0);  // field[6] custom_metadata  absent
    fb.pre16(16); // field[5] children
    fb.pre16(0);  // field[4] dictionary  absent
    fb.pre16(12); // field[3] type
    fb.pre16(9);  // field[2] type_type
    fb.pre16(8);  // field[1] nullable
    fb.pre16(4);  // field[0] name
    fb.pre16(20); // data_size
    fb.pre16(18); // vtable_size
    uint32_t C_vtable = fb.cursor();

    fb.patchSOffset(C_soffset, C_vtable);
    return C_soffset;
}

// ── Schema FlatBuffer table ───────────────────────────────────────────────────
//
// table Schema {
//   endianness:      int16;    // field[0]  Little=0
//   fields:          [Field];  // field[1]
//   custom_metadata: (absent)  // field[2]
//   features:        (absent)  // field[3]
// }
//
// Table data layout:
//   +0  soffset_t       (4 B)
//   +4  fields UOffset  (4 B) → field[1]
//   +8  endianness      int16 (2 B) → field[0]
//   +10 padding         (2 B)
//   data_size = 12
//
// Returns C_obj = cursor after soffset_t.
static uint32_t buildSchema(FBuf &fb, const std::vector<std::string> &col_names) {
    // 1. Build child objects for each field (in forward order)
    std::vector<uint32_t> field_refs = {};

    for (const auto &name : col_names) {
        uint32_t C_name     = fb.preString(name);
        uint32_t C_utf8     = buildEmptyTable(fb); // Utf8 type table
        uint32_t C_children = fb.preEmptyVector(); // empty children list
        uint32_t C_field    = buildField(fb, name, C_name, C_utf8, C_children);
        field_refs.push_back(C_field);
    }

    // 2. Build fields vector (list of Field UOffsets)
    uint32_t C_fields_vec = fb.preOffsetVector(field_refs);

    // 3. Build Schema table fields (reverse layout order)
    // padding (offset +10)
    fb.pre16(0);
    // endianness int16=0 (offset +8)
    fb.pre16(kEndiannessLittle);
    // fields UOffset (offset +4)
    fb.preUOffset(C_fields_vec);
    // soffset placeholder (offset +0)
    uint32_t C_soffset = fb.pre32(0);

    // vtable for Schema (4 fields)
    // vtable_size = 2+2+4*2 = 12, data_size = 12
    // field[3] features:          absent → 0
    // field[2] custom_metadata:   absent → 0
    // field[1] fields:            +4
    // field[0] endianness:        +8
    fb.pre16(0);  // field[3]
    fb.pre16(0);  // field[2]
    fb.pre16(4);  // field[1] fields
    fb.pre16(8);  // field[0] endianness
    fb.pre16(12); // data_size
    fb.pre16(12); // vtable_size
    uint32_t C_vtable = fb.cursor();

    fb.patchSOffset(C_soffset, C_vtable);
    return C_soffset;
}

// ── RecordBatch FlatBuffer table ─────────────────────────────────────────────
//
// table RecordBatch {
//   length:  int64;          // field[0]  number of rows
//   nodes:   [FieldNode];    // field[1]  struct: {length int64, null_count int64}
//   buffers: [Buffer];       // field[2]  struct: {offset int64, length int64}
//   compression: (absent)    // field[3]
// }
//
// FieldNode and Buffer are FlatBuffer STRUCTS (stored inline in vectors).
//
// Table data layout:
//   +0  soffset_t        (4 B)
//   +4  padding          (4 B)  [to 8-align length]
//   +8  length           int64 (8 B) → field[0]
//   +16 nodes UOffset    (4 B) → field[1]
//   +20 buffers UOffset  (4 B) → field[2]
//   data_size = 24
//
// Each Utf8 column contributes:
//   1 FieldNode struct  (2 × int64 = 16 B)
//   3 Buffer structs    (validity bitmap + offsets buffer + data buffer)
//     (3 × 2 × int64 = 48 B)
// Returns C_obj = cursor after soffset_t.
static uint32_t buildRecordBatch(FBuf &fb, int64_t num_rows,
                                 const std::vector<int64_t> &buf_offsets, // per-column-buffer body offsets
                                 const std::vector<int64_t> &buf_lengths, // per-column-buffer body lengths
                                 size_t num_cols) {
    // ── nodes vector: one FieldNode per column ────────────────────────────────
    // FieldNode is a struct (inline): [length int64][null_count int64]
    // Build vector: [count int32][node0 length int64][node0 null_count int64]...
    // In backward building, prepend struct elements in reverse element order.
    for (int i = static_cast<int>(num_cols) - 1; i >= 0; --i) {
        fb.pre64(0);        // null_count = 0
        fb.pre64(num_rows); // length = num_rows
    }
    fb.pre32(static_cast<int32_t>(num_cols)); // count
    uint32_t C_nodes = fb.cursor();

    // ── buffers vector: 3 Buffer structs per Utf8 column ─────────────────────
    // Buffer struct: [offset int64][length int64]
    // Buffers per column: [validity_bitmap (empty), offsets_buf, data_buf]
    size_t num_bufs = num_cols * 3;
    for (int i = static_cast<int>(num_bufs) - 1; i >= 0; --i) {
        fb.pre64(buf_lengths[i]);
        fb.pre64(buf_offsets[i]);
    }
    fb.pre32(static_cast<int32_t>(num_bufs)); // count
    uint32_t C_buffers = fb.cursor();

    // ── RecordBatch table fields (reverse layout: soffset last) ──────────────
    // buffers UOffset (offset +20)
    fb.preUOffset(C_buffers);
    // nodes UOffset (offset +16)
    fb.preUOffset(C_nodes);
    // length int64 (offset +8)
    fb.pre64(num_rows);
    // padding 4 B (offset +4)
    fb.pre32(0);
    // soffset placeholder (offset +0)
    uint32_t C_soffset = fb.pre32(0);

    // vtable for RecordBatch (4 fields)
    // vtable_size = 2+2+4*2 = 12, data_size = 24
    // field[3] compression: absent → 0
    // field[2] buffers:     +20
    // field[1] nodes:       +16
    // field[0] length:      +8
    fb.pre16(0);  // field[3]
    fb.pre16(20); // field[2] buffers
    fb.pre16(16); // field[1] nodes
    fb.pre16(8);  // field[0] length
    fb.pre16(24); // data_size
    fb.pre16(12); // vtable_size
    uint32_t C_vtable = fb.cursor();

    fb.patchSOffset(C_soffset, C_vtable);
    return C_soffset;
}

// ── Message FlatBuffer table ──────────────────────────────────────────────────
//
// table Message {
//   version:         int16;  // field[0]  MetadataVersion::V5 = 4
//   header:          union;  // field[1] type byte, field[2] UOffset to header table
//   bodyLength:      int64;  // field[3]
//   custom_metadata: (absent) // field[4]
// }
//
// Table data layout:
//   +0  soffset_t       (4 B)
//   +4  header UOffset  (4 B) → field[2]
//   +8  bodyLength      int64 (8 B) → field[3]
//   +16 version         int16 (2 B) → field[0]
//   +18 header_type     int8  (1 B) → field[1]
//   +19 padding         (1 B)
//   data_size = 20
//
// Returns the raw FlatBuffer bytes as a vector.
static std::vector<uint8_t> buildMessageFB(uint32_t C_header_table, int8_t header_type, int64_t body_length, FBuf &fb) {
    // Message table fields (reverse layout)
    // padding (offset +19)
    fb.pre8(0);
    // header_type int8 (offset +18)
    fb.pre8(header_type);
    // version int16=4 (offset +16)
    fb.pre16(kMetadataVersionV5);
    // bodyLength int64 (offset +8)
    fb.pre64(body_length);
    // header UOffset (offset +4)
    fb.preUOffset(C_header_table);
    // soffset placeholder (offset +0)
    uint32_t C_soffset = fb.pre32(0);

    // vtable for Message (5 fields)
    // vtable_size = 2+2+5*2 = 14, data_size = 20
    // field[4] custom_metadata:  absent → 0
    // field[3] bodyLength:       +8
    // field[2] header (value):   +4
    // field[1] header_type:      +18
    // field[0] version:          +16
    fb.pre16(0);  // field[4]
    fb.pre16(8);  // field[3] bodyLength
    fb.pre16(4);  // field[2] header value
    fb.pre16(18); // field[1] header_type
    fb.pre16(16); // field[0] version
    fb.pre16(20); // data_size
    fb.pre16(14); // vtable_size
    uint32_t C_vtable = fb.cursor();

    fb.patchSOffset(C_soffset, C_vtable);

    // Finalize: prepend root UOffset
    fb.finishWithRoot(C_soffset);

    // Pad to 8-byte alignment
    fb.align(8);

    return fb.take();
}

// ── Helper: write LE int32 to a stream ───────────────────────────────────────
// Always writes in little-endian byte order regardless of host endianness.
static void writeLE32(std::ostream &out, int32_t v) {
    uint32_t u = static_cast<uint32_t>(v);
    char buf[4];
    buf[0] = static_cast<char>(u & 0xFF);
    buf[1] = static_cast<char>((u >> 8) & 0xFF);
    buf[2] = static_cast<char>((u >> 16) & 0xFF);
    buf[3] = static_cast<char>((u >> 24) & 0xFF);
    out.write(buf, 4);
}

// ── Write an Arrow IPC message frame ─────────────────────────────────────────
// Frame: [continuation int32=-1][metadata_size int32][metadata (padded-8)][body]
// Both integer fields are written as explicit LE per the Arrow IPC spec.
static void writeMessageFrame(std::ostream &out, const std::vector<uint8_t> &metadata,
                              const std::vector<uint8_t> &body) {
    // continuation marker (-1 = 0xFFFFFFFF, endian-neutral but written via
    // writeLE32 for consistency and correctness on big-endian platforms)
    writeLE32(out, kContinuationMarker);

    // metadata size (signed int32, must be little-endian per Arrow IPC spec)
    writeLE32(out, static_cast<int32_t>(metadata.size()));

    // metadata bytes (already padded to 8)
    out.write(reinterpret_cast<const char *>(metadata.data()), static_cast<std::streamsize>(metadata.size()));

    // body (not padded here; individual buffers are padded by the caller)
    if (!body.empty()) {
        out.write(reinterpret_cast<const char *>(body.data()), static_cast<std::streamsize>(body.size()));
    }
}

// ── Build the Schema IPC message bytes ───────────────────────────────────────
static std::vector<uint8_t> buildSchemaMessage(const std::vector<std::string> &col_names) {
    FBuf fb;
    uint32_t C_schema = buildSchema(fb, col_names);
    return buildMessageFB(C_schema, kMessageHeaderSchema, 0, fb);
}

// ── Serialize entity field as UTF-8 string ───────────────────────────────────
static std::string fieldToString(const BaseEntity &entity, const std::string &col) {
    auto opt = entity.getField(col);
    if (!opt.has_value()) {
        return "";
    }
    return std::visit(
        [](const auto &v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return "";
            } else if constexpr (std::is_same_v<T, bool>) {
                return v ? "true" : "false";
            } else if constexpr (std::is_same_v<T, int64_t>) {
                return std::to_string(v);
            } else if constexpr (std::is_same_v<T, double>) {
                std::ostringstream oss = {};
                oss << v;
                return oss.str();
            } else if constexpr (std::is_same_v<T, std::string>) {
                return v;
            } else if constexpr (std::is_same_v<T, std::vector<float>>) {
                std::ostringstream oss = {};
                oss << "[";
                for (size_t i = 0; i <static_cast<int>(v.size()); ++i) {
                    if (i > 0) {
                        oss << ",";
                    }
                    oss << v[i];
                }
                oss << "]";
                return oss.str();
            } else {
                std::ostringstream oss = {};
                oss << "<binary:" <<static_cast<int>(v.size()) << ">";
                return oss.str();
            }
        },
        *opt);
}

// ── Build body buffers for a RecordBatch ─────────────────────────────────────
//
// For each Utf8 column:
//   Buffer 0: validity bitmap  → length = 0 (all values valid, omitted)
//   Buffer 1: offsets buffer   → (num_rows+1) × int32 LE
//   Buffer 2: data buffer      → concatenated UTF-8 bytes
//
// All buffers are padded to 8 bytes within the body.
struct BatchBody {
    std::vector<uint8_t> bytes;       // serialised body
    std::vector<int64_t> buf_offsets; // per-buffer offset within body
    std::vector<int64_t> buf_lengths; // per-buffer byte length (unpadded)
};

static BatchBody buildBatchBody(const std::vector<BaseEntity> &entities, const std::vector<std::string> &columns) {
    BatchBody result;
    int64_t body_pos = 0;

    for (const auto &col : columns) {
        // Collect all string values for this column
        std::vector<std::string> vals = {};

        vals.reserve(entities.size());
        for (const auto &e : entities) {
            vals.push_back(fieldToString(e, col));
        }

        // --- Buffer 0: validity bitmap (empty — all valid) ---
        result.buf_offsets.push_back(body_pos);
        result.buf_lengths.push_back(0);
        // no bytes for validity bitmap

        // --- Buffer 1: offsets (int32[N+1]) ---
        // Arrow Utf8 uses int32 offsets, so the total data per column must fit
        // in INT32_MAX bytes.  Guard against overflow before accumulating.
        int64_t col_data_total = 0;
        for (const auto &s : vals) {
            col_data_total += static_cast<int64_t>(s.size());
        }
        if (col_data_total > static_cast<int64_t>(std::numeric_limits<int32_t>::max())) {
            throw SizeLimitException("Column '" + col
                                         + "' string data exceeds 2 GiB Arrow Utf8 limit; "
                                           "use LargeUtf8 for larger payloads",
                                     static_cast<size_t>(col_data_total),
                                     static_cast<size_t>(std::numeric_limits<int32_t>::max()));
        }

        int32_t offset_cursor = 0;
        std::vector<uint8_t> offsets_buf = {};

        offsets_buf.reserve((static_cast<int>(vals.size()) + 1) * 4);
        auto append_i32 = [&]([[maybe_unused]] int32_t v) {
            uint32_t u = static_cast<uint32_t>(v);
            offsets_buf.push_back(static_cast<uint8_t>(u & 0xFF));
            offsets_buf.push_back(static_cast<uint8_t>((u >> 8) & 0xFF));
            offsets_buf.push_back(static_cast<uint8_t>((u >> 16) & 0xFF));
            offsets_buf.push_back(static_cast<uint8_t>((u >> 24) & 0xFF));
        };
        for (const auto &s : vals) {
            append_i32(offset_cursor);
            offset_cursor += static_cast<int32_t>(s.size());
        }
        append_i32(offset_cursor); // final sentinel

        int64_t offsets_len = static_cast<int64_t>(offsets_buf.size());
        result.buf_offsets.push_back(body_pos);
        result.buf_lengths.push_back(offsets_len);
        result.bytes.insert(result.bytes.end(), offsets_buf.begin(), offsets_buf.end());
        body_pos += offsets_len;
        // Pad to 8
        while (body_pos % 8 != 0) {
            result.bytes.push_back(0);
            ++body_pos;
        }

        // --- Buffer 2: data bytes ---
        std::vector<uint8_t> data_buf;
        data_buf.reserve(static_cast<size_t>(offset_cursor));
        for (const auto &s : vals) {
            data_buf.insert(data_buf.end(), s.begin(), s.end());
        }
        int64_t data_len = static_cast<int64_t>(data_buf.size());
        result.buf_offsets.push_back(body_pos);
        result.buf_lengths.push_back(data_len);
        result.bytes.insert(result.bytes.end(), data_buf.begin(), data_buf.end());
        body_pos += data_len;
        // Pad to 8
        while (body_pos % 8 != 0) {
            result.bytes.push_back(0);
            ++body_pos;
        }
    }

    return result;
}

// ── Build RecordBatch IPC message ─────────────────────────────────────────────
static std::vector<uint8_t> buildRecordBatchMessage(const std::vector<BaseEntity> &entities,
                                                    const std::vector<std::string> &columns, const BatchBody &body) {
    int64_t num_rows  = static_cast<int64_t>(entities.size());
    size_t num_cols = columns.size();
    int64_t body_size = static_cast<int64_t>(body.bytes.size());

    FBuf fb;
    uint32_t C_rb = buildRecordBatch(fb, num_rows, body.buf_offsets, body.buf_lengths, num_cols);
    return buildMessageFB(C_rb, kMessageHeaderRecordBatch, body_size, fb);
}

// ── Arrow IPC File footer ─────────────────────────────────────────────────────
//
// table Footer {
//   version:       int16;         // field[0]
//   schema:        UOffset;       // field[1]  ref to Schema table (same as schema msg)
//   dictionaries:  [Block];       // field[2]  absent
//   recordBatches: [Block];       // field[3]  list of Block structs
// }
// struct Block { offset int64; metaDataLength int32; bodyLength int64; }
//
// Table data layout:
//   +0  soffset_t          (4 B)
//   +4  schema UOffset     (4 B) → field[1]
//   +8  dictionaries absent (but we include a zero-length vector for cleanliness)
//     Actually we'll omit it (absent, vtable offset = 0)
//   +8  recordBatches UOffset (4 B) → field[3]
//   data_size = 12  (just 3 fields: soffset + 2 UOffsets, no schema)

// Block struct: offset(int64) + metaDataLength(int32) + padding(int32) + bodyLength(int64)
// Note: Arrow's Block is defined as:
//   struct Block { offset: long; metaDataLength: int; bodyLength: long; }
// Total = 8 + 4 + (4 pad) + 8 = 24 bytes per block
struct BlockInfo {
    int64_t offset;           // byte offset in the file
    int32_t meta_data_length; // metadata bytes (including 8-byte frame header)
    int64_t body_length;      // body bytes
};

static std::vector<uint8_t> buildFooterFB(const std::vector<std::string> &col_names,
                                          const std::vector<BlockInfo> &record_batch_blocks) {
    FBuf fb;

    // Build the Schema embedded in the footer (same structure as the schema msg)
    uint32_t C_schema = buildSchema(fb, col_names);

    // recordBatches vector: each Block is a struct (inline), 24 bytes
    // Prepend blocks in reverse order
    for (int i = static_cast<int>(record_batch_blocks.size()) - 1; i >= 0; --i) {
        const auto &blk = record_batch_blocks[i];
        fb.pre64(blk.body_length);      // bodyLength int64
        fb.pre32(0);                    // padding int32
        fb.pre32(blk.meta_data_length); // metaDataLength int32
        fb.pre64(blk.offset);           // offset int64
    }
    fb.pre32(static_cast<int32_t>(record_batch_blocks.size()));
    uint32_t C_rb_blocks = fb.cursor();

    // Footer table fields (reverse layout: soffset last)
    fb.preUOffset(C_rb_blocks); // recordBatches UOffset (+8) → field[3]
    fb.preUOffset(C_schema);    // schema UOffset (+4) → field[1]
    uint32_t C_soffset = fb.pre32(0);

    // vtable for Footer (4 fields)
    // vtable_size = 12, data_size = 12
    // field[3] recordBatches: +8
    // field[2] dictionaries:  absent → 0
    // field[1] schema:        +4
    // field[0] version:       absent → 0  (we don't include version explicitly)
    fb.pre16(8);  // field[3]
    fb.pre16(0);  // field[2]
    fb.pre16(4);  // field[1]
    fb.pre16(0);  // field[0]
    fb.pre16(12); // data_size
    fb.pre16(12); // vtable_size
    uint32_t C_vtable = fb.cursor();

    fb.patchSOffset(C_soffset, C_vtable);

    fb.finishWithRoot(C_soffset);
    fb.align(8);

    return fb.take();
}

// ── valueToString (shared with parquet_exporter pattern) ─────────────────────

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// ArrowIPCExporter implementation
// ─────────────────────────────────────────────────────────────────────────────

ArrowIPCExporter::ArrowIPCExporter(const ArrowIPCExportConfig &config)
    : config_(config), metrics_(std::make_shared<ExporterMetrics>()) {}

bool ArrowIPCExporter::isArrowAvailable() {
#ifdef ARROW_ENABLED
    return true;
#else
    return false;
#endif
}

std::vector<std::string> ArrowIPCExporter::resolveColumns(const std::vector<BaseEntity> &entities,
                                                          const ExportOptions &options) const {
    std::set<std::string> exclude_set(config_.exclude_columns.begin(), config_.exclude_columns.end());
    exclude_set.insert(options.exclude_fields.begin(), options.exclude_fields.end());

    if (!options.include_fields.empty()) {
        std::vector<std::string> cols = {};

        for (const auto &f : options.include_fields) {
            if (!exclude_set.count(f)) {
                cols.push_back(f);
            }
        }
        return cols;
    }
    if (!config_.include_columns.empty()) {
        std::vector<std::string> cols = {};

        for (const auto &f : config_.include_columns) {
            if (!exclude_set.count(f)) {
                cols.push_back(f);
            }
        }
        return cols;
    }

    // Auto-detect: collect all field names across all entities
    std::set<std::string> seen = {};

    for (const auto &e : entities) {
        for (const auto &kv : e.getAllFields()) {
            if (!exclude_set.count(kv.first)) {
                seen.insert(kv.first);
            }
        }
    }
    return std::vector<std::string>(seen.begin(), seen.end());
}

ExportStats ArrowIPCExporter::exportEntities(const std::vector<BaseEntity> &entities, const ExportOptions &options) {
    // Policy check before any cursor or file is opened (EXP-001).
    enforceExportPolicy(options);

    ExportStats stats;
    stats.metrics   = metrics_;
    auto start_time = std::chrono::steady_clock::now();

    // Tenant isolation check
    if (options.tenant_context && options.tenant_context->enforce_isolation) {
        if (!options.tenant_context->hasScope("export:read") && !options.tenant_context->hasScope("export:write")) {
            throw ExporterException(errors::ErrorCode::ERR_EXPORT_TENANT_UNAUTHORIZED,
                                    "Insufficient permissions for Arrow IPC export operation",
                                    "tenant_id=" + options.tenant_context->tenant_id);
        }
    }

    if (options.output_path.empty()) {
        throw ConfigException("output_path must not be empty", "output_path");
    }

    stats.total_entities = entities.size();

    const auto columns = resolveColumns(entities, options);

    try {
#ifdef ARROW_ENABLED
        stats = exportWithArrow(entities, options, columns);
#else
        stats = exportFallback(entities, options, columns);
#endif
    } catch (const ExporterException &) {
        throw;
    } catch (const std::exception &ex) {
        throw ExportIOException(ex.what(), options.output_path, 0);
    }

    auto end_time  = std::chrono::steady_clock::now();
    stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    metrics_->recordExport(stats.exported_entities, stats.bytes_written, stats.duration);

    return stats;
}

// ─────────────────────────────────────────────────────────────────────────────
// Fallback: minimal Arrow IPC File writer (no Arrow library required)
// ─────────────────────────────────────────────────────────────────────────────

ExportStats ArrowIPCExporter::exportFallback(const std::vector<BaseEntity> &entities, const ExportOptions &options,
                                             const std::vector<std::string> &columns) {
    ExportStats stats;
    stats.total_entities = entities.size();
    stats.metrics        = metrics_;

    std::ofstream out(options.output_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        throw ExportIOException("Failed to open output file", options.output_path, errno);
    }

    const bool is_file_format = (config_.format == ArrowIPCFormat::FILE);

    // ── Schema message ────────────────────────────────────────────────────────
    auto schema_msg = buildSchemaMessage(columns);

    // ── Record-batch message ──────────────────────────────────────────────────
    // Build body for a single record batch containing all entities
    BatchBody batch_body;
    std::vector<uint8_t> rb_msg_bytes = {};

    if (!entities.empty()) {
        batch_body   = buildBatchBody(entities, columns);
        rb_msg_bytes = buildRecordBatchMessage(entities, columns, batch_body);
    }

    // ── Write Arrow IPC File ──────────────────────────────────────────────────
    int64_t file_pos = 0;

    if (is_file_format) {
        // Leading magic
        out.write(reinterpret_cast<const char *>(kArrowMagic), 8);
        file_pos = 8;
    }

    // Schema message frame
    [[maybe_unused]] int64_t schema_frame_start = file_pos;
    writeMessageFrame(out, schema_msg, {});
    // frame size: 4 (continuation) + 4 (meta_size) + static_cast<int>(schema_msg.size()) 
    int64_t schema_frame_size = 4 + 4 + static_cast<int64_t>(schema_msg.size());
    file_pos += schema_frame_size;

    // Record-batch message frame (if any rows)
    std::vector<BlockInfo> rb_blocks = {};

    if (!entities.empty()) {
        int64_t rb_frame_start = file_pos;
        int64_t rb_body_size   = static_cast<int64_t>(batch_body.bytes.size());
        writeMessageFrame(out, rb_msg_bytes, batch_body.bytes);
        int64_t rb_frame_size = 4 + 4 + static_cast<int64_t>(rb_msg_bytes.size()) + rb_body_size;
        file_pos += rb_frame_size;

        if (is_file_format) {
            BlockInfo blk;
            blk.offset           = rb_frame_start;
            blk.meta_data_length = static_cast<int32_t>(4 + 4 + static_cast<int64_t>(rb_msg_bytes.size()));
            blk.body_length      = rb_body_size;
            rb_blocks.push_back(blk);
        }
    }

    if (is_file_format) {
        // EOS for file format (Arrow IPC File does not use an EOS marker,
        // the footer serves that purpose).

        // Footer FlatBuffer
        auto footer_fb = buildFooterFB(columns, rb_blocks);

        out.write(reinterpret_cast<const char *>(footer_fb.data()), static_cast<std::streamsize>(footer_fb.size()));
        file_pos += static_cast<int64_t>(footer_fb.size());

        // Footer length (int32 LE)
        writeLE32(out, static_cast<int32_t>(footer_fb.size()));
        file_pos += 4;

        // Trailing magic
        out.write(reinterpret_cast<const char *>(kArrowMagic), 8);
        file_pos += 8;
    } else {
        // Stream format: End-of-Stream marker [continuation=-1][size=0]
        int32_t eos[2] = {kContinuationMarker, 0};
        out.write(reinterpret_cast<const char *>(eos), 8);
        file_pos += 8;
    }

    out.close();
    if (!out) {
        throw ExportIOException("Error closing output file", options.output_path, errno);
    }

    stats.exported_entities = entities.size();
    stats.bytes_written     = static_cast<size_t>(file_pos);

    // Progress callback
    if ([[maybe_unused]] options.progress_callback) {
        options.progress_callback([[maybe_unused]] stats);
    }

    THEMIS_INFO("ArrowIPCExporter: wrote {} entities, {} columns, {} bytes to {}", stats.exported_entities,
                columns.size(), stats.bytes_written, options.output_path);

    return stats;
}

// ─────────────────────────────────────────────────────────────────────────────
// Arrow-library path (compiled only when ARROW_ENABLED is defined)
// ─────────────────────────────────────────────────────────────────────────────

#ifdef ARROW_ENABLED
ExportStats ArrowIPCExporter::exportWithArrow(const std::vector<BaseEntity> &entities, const ExportOptions &options,
                                              const std::vector<std::string> &columns) {
    ExportStats stats;
    stats.total_entities = entities.size();
    stats.metrics        = metrics_;

    // Build Arrow schema (all columns as Utf8)
    arrow::FieldVector arrow_fields;
    for (const auto &col : columns) {
        arrow_fields.push_back(arrow::field(col, arrow::utf8()));
    }
    // Attach custom schema metadata if provided
    std::shared_ptr<arrow::KeyValueMetadata> kv_meta = {};

    if (!config_.schema_metadata.empty()) {
        std::vector<std::string> keys, values;
        for (const auto &kv : config_.schema_metadata) {
            keys.push_back(kv.first);
            values.push_back(kv.second);
        }
        kv_meta = arrow::KeyValueMetadata::Make(keys, values);
    }
    auto schema = kv_meta ? arrow::schema(arrow_fields, kv_meta) : arrow::schema(arrow_fields);

    // Open output file
    auto maybe_file = arrow::io::FileOutputStream::Open(options.output_path);
    if (!maybe_file.ok()) {
        throw ExportIOException(maybe_file.status().ToString(), options.output_path, 0);
    }
    auto file = *maybe_file;

    // Create IPC writer
    arrow::ipc::IpcWriteOptions ipc_opts = arrow::ipc::IpcWriteOptions::Defaults();
    std::shared_ptr<arrow::ipc::RecordBatchWriter> writer = {};

    if (config_.format == ArrowIPCFormat::STREAM) {
        auto maybe_writer = arrow::ipc::MakeStreamWriter(file, schema, ipc_opts);
        if (!maybe_writer.ok()) {
            throw ExportIOException(maybe_writer.status().ToString(), options.output_path, 0);
        }
        writer = *maybe_writer;
    } else {
        auto maybe_writer = arrow::ipc::MakeFileWriter(file, schema, ipc_opts);
        if (!maybe_writer.ok()) {
            throw ExportIOException(maybe_writer.status().ToString(), options.output_path, 0);
        }
        writer = *maybe_writer;
    }

    // Build column builders
    std::vector<std::shared_ptr<arrow::StringBuilder>> builders(columns.size());
    for (size_t i = 0; i <static_cast<int>(columns.size()); ++i) {
        builders[i] = std::make_shared<arrow::StringBuilder>();
    }

    for (const auto &entity : entities) {
        for (size_t i = 0; i <static_cast<int>(columns.size()); ++i) {
            auto status = builders[i]->Append(fieldToString(entity, columns[i]));
            if (!status.ok()) {
                stats.failed_entities++;
                if (!options.continue_on_error)
                    throw ExportIOException(status.ToString(), options.output_path, 0);
            }
        }
    }

    // Flush arrays and write record batch
    arrow::ArrayVector arrays;
    for (auto &b : builders) {
        auto maybe_arr = b->Finish();
        if (!maybe_arr.ok()) {
            throw ExportIOException(maybe_arr.status().ToString(), options.output_path, 0);
        }
        arrays.push_back(*maybe_arr);
    }

    auto batch  = arrow::RecordBatch::Make(schema, static_cast<int64_t>(entities.size()), arrays);
    auto status = writer->WriteRecordBatch(*batch);
    if (!status.ok()) {
        throw ExportIOException(status.ToString(), options.output_path, 0);
    }

    status = writer->Close();
    if (!status.ok()) {
        throw ExportIOException(status.ToString(), options.output_path, 0);
    }

    auto maybe_pos          = file->Tell();
    stats.exported_entities = entities.size();
    stats.bytes_written     = maybe_pos.ok() ? static_cast<size_t>(*maybe_pos) : 0;

    if ([[maybe_unused]] options.progress_callback) {
        options.progress_callback([[maybe_unused]] stats);
    }

    return stats;
}
#endif // ARROW_ENABLED

} // namespace themis::exporters
