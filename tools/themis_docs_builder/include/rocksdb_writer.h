/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rocksdb_writer.h                                   ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:58:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     45                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file rocksdb_writer.h
 * @brief RocksDB writer interface for namespaced document output.
 *
 * Writes records into the configured RocksDB instance using column-family
 * separation to keep namespaces isolated.
 */

#pragma once

#include <string>

namespace themis {
namespace tools {

class RocksDBWriter {
public:
    /**
     * @brief Rocks DBWriter.
     * @param[in] db_path Input parameter.
     * @return Return value.
     */
    explicit RocksDBWriter(const std::string& db_path);
    /**
     * @brief Write.
     * @param[in] key Input parameter.
     * @param[in] value Input parameter.
     * @param[in] cf Input parameter.
     */
    void write(const std::string& key, const std::string& value, const std::string& cf);
};

} // namespace tools
} // namespace themis
