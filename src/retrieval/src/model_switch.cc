/**
 * @file model_switch.cc
 * @brief Model-Switch Controller implementation stub.
 *
 * Skeleton: factory and minimal concrete class.  Replace with production
 * hot-swap orchestration in sub-issue #5419.
 */

#include "retrieval/include/model_switch.h"

namespace themis::retrieval {

namespace {

class ModelSwitchControllerImpl final : public IModelSwitchController {
public:
    explicit ModelSwitchControllerImpl(ModelVersion initial,
                                       CompatibilityPolicy policy)
        : active_(std::move(initial)), policy_(std::move(policy)) {}

    SwitchResult execute(const SwitchRequest& req) override {
        // TODO(#5419): Implement zero-downtime model and adapter swap.
        SwitchResult result{
            .outcome          = SwitchOutcome::Success,
            .previous_model_id = active_.id,
            .active_model_id   = req.target_model.id,
        };
        previous_ = active_;
        active_   = req.target_model;
        if (observer_) observer_(result);
        return result;
    }

    SwitchResult rollback() override {
        // TODO(#5419): Restore previous model safely.
        return execute(SwitchRequest{.target_model = previous_});
    }

    ModelVersion active() const override { return active_; }

    bool checkCompatibility(const ModelVersion& target,
                             const CompatibilityPolicy& policy) const override {
        if (policy.require_exact_arch_match &&
            target.architecture != active_.architecture) {
            return false;
        }
        return true;
    }

    void onSwitch(SwitchObserver obs) override {
        observer_ = std::move(obs);
    }

private:
    ModelVersion       active_;
    ModelVersion       previous_;
    CompatibilityPolicy policy_;
    SwitchObserver     observer_;
};

} // namespace

std::unique_ptr<IModelSwitchController> makeModelSwitchController(
    const ModelVersion& initial, const CompatibilityPolicy& policy) {
    return std::make_unique<ModelSwitchControllerImpl>(initial, policy);
}

} // namespace themis::retrieval
