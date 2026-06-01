/**
 * @file lora_package.cc
 * @brief LoRAPackage and PortableAdapterProduct repository stub.
 *
 * Skeleton: factory and minimal concrete class.  Replace with production
 * storage, serialisation, and compilation logic in sub-issue #5416.
 */

#include "retrieval/include/lora_package.h"

#include <stdexcept>

namespace themis::retrieval {

namespace {

class LoRARepositoryImpl final : public ILoRARepository {
public:
    explicit LoRARepositoryImpl(std::string storage_path)
        : storage_path_(std::move(storage_path)) {}

    std::string store(LoRAPackage /*pkg*/) override {
        // TODO(#5416): Persist package to storage_path_.
        throw std::runtime_error("LoRARepository::store not implemented yet");
    }

    std::optional<LoRAPackage> load(const std::string& /*id*/,
                                     bool /*lazy*/) override {
        // TODO(#5416): Load package from storage_path_.
        return std::nullopt;
    }

    PortableAdapterProduct compile(const std::string& /*package_id*/,
                                   const std::string& /*target_arch*/) override {
        // TODO(#5416): Transpile weights to target architecture format.
        throw std::runtime_error("LoRARepository::compile not implemented yet");
    }

    std::vector<std::string> listPackageIds(std::size_t /*limit*/) override {
        // TODO(#5416): Enumerate packages in storage_path_.
        return {};
    }

    bool purge(const std::string& /*id*/) override {
        // TODO(#5416): Delete package and its products.
        return false;
    }

private:
    std::string storage_path_;
};

} // namespace

std::unique_ptr<ILoRARepository> makeLoRARepository(const std::string& storage_path) {
    return std::make_unique<LoRARepositoryImpl>(storage_path);
}

} // namespace themis::retrieval
