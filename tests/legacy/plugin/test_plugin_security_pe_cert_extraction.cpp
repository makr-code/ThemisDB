#include <gtest/gtest.h>
#include "acceleration/plugin_security.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace themis::acceleration;

// ============================================================================
// Helper: build a minimal PE32 binary in memory
// ============================================================================

// Layout:
//   0x00  DOS header (MZ magic + e_lfanew at 0x3C pointing to PE_OFFSET)
//   PE_OFFSET  PE signature "PE\0\0"
//   PE_OFFSET+4   COFF file header (20 bytes)
//   PE_OFFSET+24  Optional header PE32 (magic 0x010B, 224 bytes total)
//     optional-header + 96 bytes = DataDirectory start
//     DataDirectory[4] = Security dir (file-offset + size)
//   CERT_TABLE_OFFSET  WIN_CERTIFICATE records

namespace {

constexpr uint32_t PE_OFFSET         = 0x80u;
constexpr uint32_t OPT_HDR_OFFSET    = PE_OFFSET + 24u;   // +4 sig +20 coff
constexpr uint32_t DATA_DIR_OFFSET   = OPT_HDR_OFFSET + 96u;  // PE32 layout
constexpr uint32_t SECURITY_DIR_OFF  = DATA_DIR_OFFSET + 4u * 8u;  // index 4
constexpr uint32_t CERT_TABLE_OFFSET = 0x200u;

// Write a 32-bit LE value into a byte vector at the given position.
static void writeLE32(std::vector<uint8_t>& v, size_t pos, uint32_t val) {
    v[pos + 0] = static_cast<uint8_t>(val & 0xFFu);
    v[pos + 1] = static_cast<uint8_t>((val >> 8)  & 0xFFu);
    v[pos + 2] = static_cast<uint8_t>((val >> 16) & 0xFFu);
    v[pos + 3] = static_cast<uint8_t>((val >> 24) & 0xFFu);
}

static void writeLE16(std::vector<uint8_t>& v, size_t pos, uint16_t val) {
    v[pos + 0] = static_cast<uint8_t>(val & 0xFFu);
    v[pos + 1] = static_cast<uint8_t>((val >> 8) & 0xFFu);
}

// Build a PE32 binary containing one or more WIN_CERTIFICATE records.
// Each blob in |blobs| becomes one WIN_CERT_TYPE_PKCS_SIGNED_DATA record.
// If |non_pkcs7_first| is true, a record with wCertificateType=0x0001 (X.509)
// is prepended to exercise the type-skip logic.
static std::vector<uint8_t> buildMinimalPE32(
    const std::vector<std::vector<uint8_t>>& blobs,
    bool non_pkcs7_first = false)
{
    // Build the certificate table
    std::vector<uint8_t> cert_table;

    auto appendRecord = [&](uint16_t type, const std::vector<uint8_t>& data) {
        uint32_t rec_len = 8u + static_cast<uint32_t>(data.size());
        size_t before = cert_table.size();
        cert_table.resize(cert_table.size() + rec_len, 0);
        writeLE32(cert_table, before + 0, rec_len);
        writeLE16(cert_table, before + 4, 0x0200u);  // wRevision
        writeLE16(cert_table, before + 6, type);
        std::copy(data.begin(), data.end(), cert_table.begin() + before + 8);
        // Pad to 8-byte boundary
        while (cert_table.size() % 8u != 0) {
          cert_table.push_back(0u);
        }
    };

    if (non_pkcs7_first) {
        // Prepend a non-PKCS#7 record (type 0x0001, X.509 DER)
        appendRecord(0x0001u, {0x30u, 0x00u});
    }

    for (const auto& blob : blobs) {
        appendRecord(0x0002u, blob);  // WIN_CERT_TYPE_PKCS_SIGNED_DATA
    }

    uint32_t cert_table_size = static_cast<uint32_t>(cert_table.size());
    uint32_t file_size = CERT_TABLE_OFFSET + cert_table_size;

    std::vector<uint8_t> pe(file_size, 0u);

    // DOS header
    pe[0] = 'M';  pe[1] = 'Z';
    writeLE32(pe, 0x3Cu, PE_OFFSET);

    // PE signature
    pe[PE_OFFSET + 0] = 'P';
    pe[PE_OFFSET + 1] = 'E';
    pe[PE_OFFSET + 2] = 0;
    pe[PE_OFFSET + 3] = 0;

    // COFF file header: Machine=0x014C (x86), SizeOfOptionalHeader=224
    writeLE16(pe, PE_OFFSET + 4,  0x014Cu);
    writeLE16(pe, PE_OFFSET + 16, 0x00E0u);

    // Optional header: Magic=0x010B (PE32), NumberOfRvaAndSizes=16
    writeLE16(pe, OPT_HDR_OFFSET + 0,  0x010Bu);
    writeLE32(pe, OPT_HDR_OFFSET + 92, 16u);

    // Security DataDirectory: VirtualAddress = file offset (not RVA for this dir)
    writeLE32(pe, SECURITY_DIR_OFF + 0, CERT_TABLE_OFFSET);
    writeLE32(pe, SECURITY_DIR_OFF + 4, cert_table_size);

    // Certificate table
    std::copy(cert_table.begin(), cert_table.end(),
              pe.begin() + CERT_TABLE_OFFSET);

    return pe;
}

// Write bytes to a temp file and return its path.
static std::string writeTempFile(
    const std::filesystem::path& dir,
    const std::string& name,
    const std::vector<uint8_t>& data)
{
    std::filesystem::path p = dir / name;
    std::ofstream f(p, std::ios::binary);
    f.write(reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size()));
    return p.string();
}

}  // namespace

// ============================================================================
// Test fixture
// ============================================================================

class PECertExtractionTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = std::filesystem::temp_directory_path() /
                    "themis_pe_cert_extraction_test";
        std::filesystem::create_directories(test_dir_);
        policy_.allowUnsigned = true;
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir_);
    }

    EnhancedPluginSecurityVerifier makeVerifier() const {
        return EnhancedPluginSecurityVerifier(policy_);
    }

    std::filesystem::path test_dir_;
    PluginSecurityPolicy  policy_;
};

// ============================================================================
// PE certificate-table extraction tests
// ============================================================================

// Single WIN_CERT_TYPE_PKCS_SIGNED_DATA record → blob is returned unchanged.
TEST_F(PECertExtractionTest, PE_SinglePKCS7_ExtractedCorrectly) {
    const std::vector<uint8_t> expected = {0x30u, 0x82u, 0x01u, 0x00u,
                                           0xAAu, 0xBBu, 0xCCu, 0xDDu};
    auto pe = buildMinimalPE32({expected});
    const std::string path = writeTempFile(test_dir_, "single.dll", pe);

    auto verifier = makeVerifier();
    auto result   = verifier.extractSigningCertificateForTesting(path);

    ASSERT_TRUE(result.has_value()) << "Expected a PKCS#7 blob to be extracted";
    EXPECT_EQ(*result, expected);
}

// Table with two PKCS#7 records → first blob is returned.
TEST_F(PECertExtractionTest, PE_MultiplePKCS7_ReturnsFirst) {
    const std::vector<uint8_t> first  = {0x01u, 0x02u, 0x03u};
    const std::vector<uint8_t> second = {0x04u, 0x05u, 0x06u};
    auto pe = buildMinimalPE32({first, second});
    const std::string path = writeTempFile(test_dir_, "multi.dll", pe);

    auto verifier = makeVerifier();
    auto result   = verifier.extractSigningCertificateForTesting(path);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, first) << "Should return the first PKCS#7 blob";
}

// Table starts with a non-PKCS#7 record (type 0x0001) followed by a PKCS#7
// record → the PKCS#7 record should be found and returned.
TEST_F(PECertExtractionTest, PE_SkipsNonPKCS7_FindsNextRecord) {
    const std::vector<uint8_t> pkcs7 = {0xAAu, 0xBBu};
    // non_pkcs7_first=true: record type 0x0001 is prepended
    auto pe = buildMinimalPE32({pkcs7}, /*non_pkcs7_first=*/true);
    const std::string path = writeTempFile(test_dir_, "skip.dll", pe);

    auto verifier = makeVerifier();
    auto result   = verifier.extractSigningCertificateForTesting(path);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, pkcs7);
}

// No security directory (VA=0) → nullopt returned.
TEST_F(PECertExtractionTest, PE_NoSecurityDir_ReturnsNullopt) {
    // Build a PE32 with all DataDirectory entries zeroed.
    constexpr uint32_t pe_off = 0x80u;
    constexpr uint32_t opt_off = pe_off + 24u;
    constexpr uint32_t total = opt_off + 224u;

    std::vector<uint8_t> pe(total, 0u);
    pe[0] = 'M';  pe[1] = 'Z';
    writeLE32(pe, 0x3Cu, pe_off);
    pe[pe_off + 0] = 'P';  pe[pe_off + 1] = 'E';
    writeLE16(pe, pe_off + 16, 0x00E0u);  // SizeOfOptionalHeader=224
    writeLE16(pe, opt_off + 0, 0x010Bu);  // Magic PE32
    writeLE32(pe, opt_off + 92, 16u);     // NumberOfRvaAndSizes=16
    // Security dir VA+size remain zero

    const std::string path = writeTempFile(test_dir_, "nosig.dll", pe);

    auto verifier = makeVerifier();
    auto result   = verifier.extractSigningCertificateForTesting(path);

    EXPECT_FALSE(result.has_value());
}

// Not a PE (not MZ magic) → nullopt returned.
TEST_F(PECertExtractionTest, PE_InvalidMagic_ReturnsNullopt) {
    const std::vector<uint8_t> data = {0x00u, 0x00u, 0x00u, 0x00u};
    const std::string path = writeTempFile(test_dir_, "invalid.bin", data);

    auto verifier = makeVerifier();
    EXPECT_FALSE(verifier.extractSigningCertificateForTesting(path).has_value());
}

// PE32+ (64-bit) optional header → DataDirectory is at offset +112.
TEST_F(PECertExtractionTest, PE_PE32Plus_ExtractedCorrectly) {
    // For PE32+, data dirs start at opt_hdr + 112 (not +96).
    constexpr uint32_t pe_off64       = 0x80u;
    constexpr uint32_t opt_off64      = pe_off64 + 24u;
    constexpr uint32_t data_dir64     = opt_off64 + 112u;
    constexpr uint32_t sec_dir64      = data_dir64 + 4u * 8u;
    constexpr uint32_t cert_tbl64     = 0x300u;

    const std::vector<uint8_t> blob = {0xDEu, 0xADu, 0xBEu, 0xEFu};
    // WIN_CERTIFICATE record
    std::vector<uint8_t> cert_rec(8u + blob.size(), 0u);
    writeLE32(cert_rec, 0, static_cast<uint32_t>(cert_rec.size()));
    writeLE16(cert_rec, 4, 0x0200u);
    writeLE16(cert_rec, 6, 0x0002u);
    std::copy(blob.begin(), blob.end(), cert_rec.begin() + 8);
    // Pad to 8-byte boundary
    while (cert_rec.size() % 8u) {
      cert_rec.push_back(0u);
    }

    uint32_t file_size = cert_tbl64 + static_cast<uint32_t>(cert_rec.size());
    std::vector<uint8_t> pe(file_size, 0u);

    pe[0] = 'M';  pe[1] = 'Z';
    writeLE32(pe, 0x3Cu, pe_off64);
    pe[pe_off64 + 0] = 'P';  pe[pe_off64 + 1] = 'E';
    writeLE16(pe, pe_off64 + 16, 0x00F0u);   // SizeOfOptionalHeader=240
    writeLE16(pe, opt_off64 + 0, 0x020Bu);   // Magic PE32+
    writeLE32(pe, opt_off64 + 108, 16u);     // NumberOfRvaAndSizes (offset 108 in PE32+ optional hdr)
    writeLE32(pe, sec_dir64 + 0, cert_tbl64);
    writeLE32(pe, sec_dir64 + 4, static_cast<uint32_t>(cert_rec.size()));
    std::copy(cert_rec.begin(), cert_rec.end(), pe.begin() + cert_tbl64);

    const std::string path = writeTempFile(test_dir_, "pe64.dll", pe);

    auto verifier = makeVerifier();
    auto result   = verifier.extractSigningCertificateForTesting(path);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, blob);
}

// ============================================================================
// ELF sidecar extraction tests
// ============================================================================

// ELF .so with a matching sidecar <plugin>.so.sig → sidecar data returned.
TEST_F(PECertExtractionTest, ELF_SidecarSigFile_Extracted) {
    // Minimal ELF64 LE header (64 bytes, no valid section table)
    std::vector<uint8_t> elf(64, 0u);
    elf[0] = 0x7F; elf[1] = 'E'; elf[2] = 'L'; elf[3] = 'F';
    elf[4] = 2u;   // ELFCLASS64
    elf[5] = 1u;   // ELFDATA2LSB

    const std::string so_path = writeTempFile(
        test_dir_, "plugin.so", elf);

    // Sidecar file: plugin.so.sig
    const std::vector<uint8_t> sig_data = {0x30u, 0x82u, 0x00u, 0x10u,
                                            0x01u, 0x02u, 0x03u, 0x04u};
    writeTempFile(test_dir_, "plugin.so.sig", sig_data);

    auto verifier = makeVerifier();
    auto result   = verifier.extractSigningCertificateForTesting(so_path);

    ASSERT_TRUE(result.has_value())
        << "Expected sidecar .sig data to be returned";
    EXPECT_EQ(*result, sig_data);
}

// ELF .so without a sidecar file (and no .note.gnu.signature section) →
// nullopt returned.
TEST_F(PECertExtractionTest, ELF_NoSidecar_ReturnsNullopt) {
    std::vector<uint8_t> elf(64, 0u);
    elf[0] = 0x7F; elf[1] = 'E'; elf[2] = 'L'; elf[3] = 'F';
    elf[4] = 2u;   // ELFCLASS64
    elf[5] = 1u;   // ELFDATA2LSB

    const std::string so_path = writeTempFile(
        test_dir_, "unsigned_plugin.so", elf);
    // No sidecar written

    auto verifier = makeVerifier();
    auto result   = verifier.extractSigningCertificateForTesting(so_path);

    EXPECT_FALSE(result.has_value())
        << "Unsigned ELF without sidecar should return nullopt";
}

// Minimal ELF64 with a .note.gnu.signature section → section data returned.
TEST_F(PECertExtractionTest, ELF_NoteGnuSignatureSection_Extracted) {
    // We construct a minimal ELF64 binary that contains:
    //   - A section header for SHT_NULL (index 0, required)
    //   - A section header for .note.gnu.signature
    //   - A section header for .shstrtab (section name string table)
    // Section data is appended after the headers.

    // Layout:
    //   0x00   ELF64 header  (64 bytes)
    //   0x40   .note.gnu.signature data (N bytes, aligned)
    //   0x40+N .shstrtab data: "\0.note.gnu.signature\0"
    //   shoff  Section header table (3 × 64 bytes)

    const std::vector<uint8_t> sig_payload = {
        0x30u, 0x82u, 0x00u, 0x08u,
        0xCAu, 0xFEu, 0xBAu, 0xBEu};

    constexpr uint64_t sig_offset  = 0x40u;
    uint64_t sig_size = sig_payload.size();

    // Build .shstrtab: "\0.note.gnu.signature\0"
    std::string shstrtab_str;
    shstrtab_str.push_back('\0');            // index 0: empty name for SHT_NULL
    uint32_t sig_name_idx = 1u;              // index of ".note.gnu.signature"
    shstrtab_str += ".note.gnu.signature";
    shstrtab_str.push_back('\0');
    uint32_t shstrtab_name_idx =
        static_cast<uint32_t>(shstrtab_str.size());
    shstrtab_str += ".shstrtab";
    shstrtab_str.push_back('\0');

    uint64_t shstrtab_offset = sig_offset + sig_size;
    // Align to 8 bytes
    while (shstrtab_offset % 8u) {
      ++shstrtab_offset;
    }
    uint64_t shstrtab_size = shstrtab_str.size();

    uint64_t shoff = shstrtab_offset + shstrtab_size;
    while (shoff % 8u) {
      ++shoff;
    }

    // 3 section headers (SHT_NULL, .note.gnu.signature, .shstrtab)
    constexpr uint16_t shnum     = 3u;
    constexpr uint16_t shstrndx  = 2u;  // .shstrtab is section 2
    constexpr uint16_t shentsize = 64u;

    uint64_t file_size = shoff + shnum * shentsize;
    std::vector<uint8_t> elf(static_cast<size_t>(file_size), 0u);

    // Helper to write LE values into the ELF buffer
    auto wLE16 = [&](size_t off, uint16_t v) {
        elf[off]     = static_cast<uint8_t>(v & 0xFFu);
        elf[off + 1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
    };
    auto wLE32 = [&](size_t off, uint32_t v) {
        elf[off]     = static_cast<uint8_t>(v & 0xFFu);
        elf[off + 1] = static_cast<uint8_t>((v >> 8)  & 0xFFu);
        elf[off + 2] = static_cast<uint8_t>((v >> 16) & 0xFFu);
        elf[off + 3] = static_cast<uint8_t>((v >> 24) & 0xFFu);
    };
    auto wLE64 = [&](size_t off, uint64_t v) {
        for (int i = 0; i < 8; ++i)
            elf[off + i] = static_cast<uint8_t>((v >> (8 * i)) & 0xFFu);
    };

    // ELF header
    elf[0] = 0x7F; elf[1] = 'E'; elf[2] = 'L'; elf[3] = 'F';
    elf[4] = 2u;   // ELFCLASS64
    elf[5] = 1u;   // ELFDATA2LSB
    elf[6] = 1u;   // EV_CURRENT
    wLE16(16, 3u);    // ET_DYN
    wLE64(40, shoff);
    wLE16(58, shentsize);
    wLE16(60, shnum);
    wLE16(62, shstrndx);

    // .note.gnu.signature data
    std::copy(sig_payload.begin(), sig_payload.end(),
              elf.begin() + sig_offset);

    // .shstrtab data
    std::copy(shstrtab_str.begin(), shstrtab_str.end(),
              elf.begin() + shstrtab_offset);

    // Section headers
    // [0] SHT_NULL — all zeros, already zeroed

    // [1] .note.gnu.signature
    size_t sh1 = static_cast<size_t>(shoff + 1u * shentsize);
    wLE32(sh1 + 0,  sig_name_idx);   // sh_name
    wLE32(sh1 + 4,  7u);             // sh_type = SHT_NOTE
    wLE64(sh1 + 24, sig_offset);     // sh_offset
    wLE64(sh1 + 32, sig_size);       // sh_size

    // [2] .shstrtab
    size_t sh2 = static_cast<size_t>(shoff + 2u * shentsize);
    wLE32(sh2 + 0,  shstrtab_name_idx);
    wLE32(sh2 + 4,  3u);               // sh_type = SHT_STRTAB
    wLE64(sh2 + 24, shstrtab_offset);
    wLE64(sh2 + 32, shstrtab_size);

    const std::string so_path = writeTempFile(
        test_dir_, "signed.so", elf);

    auto verifier = makeVerifier();
    auto result   = verifier.extractSigningCertificateForTesting(so_path);

    ASSERT_TRUE(result.has_value())
        << ".note.gnu.signature section data should be extracted";
    EXPECT_EQ(*result, sig_payload);
}

// ELF32 .so with a sidecar .sig file → sidecar data returned.
TEST_F(PECertExtractionTest, ELF32_SidecarSigFile_Extracted) {
    // Minimal ELF32 LE header (52 bytes)
    std::vector<uint8_t> elf(52, 0u);
    elf[0] = 0x7F; elf[1] = 'E'; elf[2] = 'L'; elf[3] = 'F';
    elf[4] = 1u;   // ELFCLASS32
    elf[5] = 1u;   // ELFDATA2LSB

    const std::string so_path = writeTempFile(
        test_dir_, "plugin32.so", elf);

    const std::vector<uint8_t> sig_data = {0xAAu, 0xBBu, 0xCCu};
    writeTempFile(test_dir_, "plugin32.so.sig", sig_data);

    auto verifier = makeVerifier();
    auto result   = verifier.extractSigningCertificateForTesting(so_path);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, sig_data);
}

// ============================================================================
// Unknown file format
// ============================================================================

TEST_F(PECertExtractionTest, UnknownFormat_ReturnsNullopt) {
    // File that starts with a random pattern (not MZ, not ELF, not Mach-O)
    const std::vector<uint8_t> data(64, 0xAAu);
    const std::string path = writeTempFile(test_dir_, "unknown.bin", data);

    auto verifier = makeVerifier();
    EXPECT_FALSE(verifier.extractSigningCertificateForTesting(path).has_value());
}

TEST_F(PECertExtractionTest, NonExistentFile_ReturnsNullopt) {
    auto verifier = makeVerifier();
    EXPECT_FALSE(verifier.extractSigningCertificateForTesting(
        "/nonexistent/path/plugin.dll").has_value());
}
