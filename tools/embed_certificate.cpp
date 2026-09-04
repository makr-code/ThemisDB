/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            embed_certificate.cpp                              ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:58:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     278                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// embed_certificate.cpp
// Tool to embed ThemisDB certificates and signatures as C++ const arrays
// Usage: embed_certificate [options]

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <cstring>
#include <ctime>
#include <cstdint>
#include <cstddef>

void printUsage(const char* programName) {
    std::cout << "ThemisDB Certificate Embedding Tool\n";
    std::cout << "====================================\n\n";
    std::cout << "Usage: " << programName << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --cert <file>       Path to certificate file (DER or PEM format)\n";
    std::cout << "  --signature <file>  Path to signature file (binary)\n";
    std::cout << "  --output <file>     Output header file (default: plugin_embedded_cert.h)\n";
    std::cout << "  --namespace <ns>    C++ namespace (default: themis::plugins)\n";
    std::cout << "  --plugin-name <n>   Plugin name for array identifiers\n";
    std::cout << "  --help              Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " --cert ca.crt --output embedded_cert.h\n";
    std::cout << "  " << programName << " --cert signer.crt --signature plugin.sig --output plugin_cert.h\n";
}

std::vector<uint8_t> readBinaryFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error("Failed to read file: " + filePath);
    }
    
    return buffer;
}

void writeArrayToHeader(
    std::ofstream& out,
    const std::string& arrayName,
    const std::vector<uint8_t>& data,
    const std::string& comment
) {
    out << "// " << comment << "\n";
    out << "const unsigned char " << arrayName << "[] = {\n";
    
    for (size_t i = 0; i < data.size(); ++i) {
        if (i % 12 == 0) {
            out << "    ";
        }
        
        out << "0x" << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(data[i]);
        
        if (i < data.size() - 1) {
            out << ", ";
        }
        
        if ((i + 1) % 12 == 0 || i == data.size() - 1) {
            out << "\n";
        }
    }
    
    out << "};\n";
    out << "const size_t " << arrayName << "_LEN = " << std::dec << data.size() << ";\n\n";
}

std::string getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    char buf[100];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S UTC", std::gmtime(&now));
    return std::string(buf);
}

int main(int argc, char* argv[]) {
    try {
        std::string certPath;
        std::string signaturePath;
        std::string outputPath = "plugin_embedded_cert.h";
        std::string namespaceName = "themis::plugins";
        std::string pluginName;
        
        // Parse command line arguments
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "--help" || arg == "-h") {
                printUsage(argv[0]);
                return 0;
            } else if (arg == "--cert" && i + 1 < argc) {
                certPath = argv[++i];
            } else if (arg == "--signature" && i + 1 < argc) {
                signaturePath = argv[++i];
            } else if (arg == "--output" && i + 1 < argc) {
                outputPath = argv[++i];
            } else if (arg == "--namespace" && i + 1 < argc) {
                namespaceName = argv[++i];
            } else if (arg == "--plugin-name" && i + 1 < argc) {
                pluginName = argv[++i];
            } else {
                std::cerr << "Unknown option: " << arg << "\n";
                printUsage(argv[0]);
                return 1;
            }
        }
        
        // Validate required arguments
        if (certPath.empty()) {
            std::cerr << "Error: --cert option is required\n";
            printUsage(argv[0]);
            return 1;
        }
        
        // Read certificate file
        std::vector<uint8_t> certData = readBinaryFile(certPath);
        std::cout << "Read certificate: " << certPath << " (" << certData.size() << " bytes)\n";
        
        // Read signature file if provided
        std::vector<uint8_t> signatureData = {};

        if (!signaturePath.empty()) {
            signatureData = readBinaryFile(signaturePath);
            std::cout << "Read signature: " << signaturePath << " (" << signatureData.size() << " bytes)\n";
        }
        
        // Generate output header file
        std::ofstream out(outputPath);
        if (!out) {
            throw std::runtime_error("Failed to create output file: " + outputPath);
        }
        
        // Write header guard
        std::string guardName = "THEMIS_PLUGIN_EMBEDDED_CERT_H";
        if (!pluginName.empty()) {
            guardName = "THEMIS_" + pluginName + "_EMBEDDED_CERT_H";
            // Convert to uppercase and replace invalid characters
            for (char& c : guardName) {
                if (c >= 'a' && c <= 'z') {
                    c = c - 'a' + 'A';
                } else if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')) {
                    c = '_';
                }
            }
        }
        
        out << "#ifndef " << guardName << "\n";
        out << "#define " << guardName << "\n\n";
        
        // Write file header comment
        out << "// Auto-generated by ThemisDB Certificate Embedding Tool\n";
        out << "// Generated: " << getCurrentTimestamp() << "\n";
        out << "// Certificate: " << certPath << "\n";
        if (!signaturePath.empty()) {
            out << "// Signature: " << signaturePath << "\n";
        }
        out << "//\n";
        out << "// DO NOT EDIT THIS FILE MANUALLY!\n";
        out << "// Regenerate using: embed_certificate --cert " << certPath;
        if (!signaturePath.empty()) {
            out << " --signature " << signaturePath;
        }
        out << "\n\n";
        
        // Write includes
        out << "#include <cstddef>\n";
        out << "#include <cstdint>\n\n";
        
        // Parse namespace
        std::vector<std::string> namespaces;
        size_t pos = 0;
        std::string ns = namespaceName;
        while ((pos = ns.find("::")) != std::string::npos) {
            namespaces.push_back(ns.substr(0, pos));
            ns.erase(0, pos + 2);
        }
        if (!ns.empty()) {
            namespaces.push_back(ns);
        }
        
        // Open namespaces
        for (const auto& n : namespaces) {
            out << "namespace " << n << " {\n";
        }
        out << "\n";
        
        // Write certificate array
        std::string certArrayName = "THEMISDB_PLUGIN_CERT";
        if (!pluginName.empty()) {
            certArrayName = "THEMISDB_" + pluginName + "_CERT";
            for (char& c : certArrayName) {
                if (c >= 'a' && c <= 'z') {
                    c = c - 'a' + 'A';
                }
            }
        }
        
        writeArrayToHeader(
            out,
            certArrayName,
            certData,
            "ThemisDB.org Official Plugin Certificate (X.509 DER format)"
        );
        
        // Write signature array if provided
        if (!signatureData.empty()) {
            std::string sigArrayName = "THEMISDB_PLUGIN_SIGNATURE";
            if (!pluginName.empty()) {
                sigArrayName = "THEMISDB_" + pluginName + "_SIGNATURE";
                for (char& c : sigArrayName) {
                    if (c >= 'a' && c <= 'z') {
                        c = c - 'a' + 'A';
                    }
                }
            }
            
            writeArrayToHeader(
                out,
                sigArrayName,
                signatureData,
                "Digital signature of plugin binary (RSA-4096 + SHA-256)"
            );
        }
        
        // Close namespaces
        for (auto it = namespaces.rbegin(); it != namespaces.rend(); ++it) {
            out << "} // namespace " << *it << "\n";
        }
        out << "\n";
        
        // Close header guard
        out << "#endif // " << guardName << "\n";
        
        out.close();
        
        std::cout << "\n✅ Successfully generated: " << outputPath << "\n";
        std::cout << "   Certificate array: " << certArrayName << " (" << certData.size() << " bytes)\n";
        if (!signatureData.empty()) {
            std::cout << "   Signature array: THEMISDB_PLUGIN_SIGNATURE (" << signatureData.size() << " bytes)\n";
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
