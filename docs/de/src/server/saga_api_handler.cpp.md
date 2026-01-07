# saga_api_handler.cpp

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Src

---


Path: `src/server/saga_api_handler.cpp`

Purpose: Handlers to orchestrate SAGA/transactional workflows exposed via API.

Public functions / symbols:
- `static std::string base64_encode_local(const std::vector<uint8_t>& data) {`
- `for (const auto& step : steps) {`
- `if (!saga_logger_) {`
- ``
- `for (auto& byte : j["ciphertext_hash"]) {`
- `std::ifstream ifs("data/logs/saga_signatures.jsonl");`

