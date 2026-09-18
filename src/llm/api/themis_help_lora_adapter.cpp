#include "themis/llm/llm_factory.h"
#include "llm/applications/themis_help_lora.h"

namespace themis {
namespace llm {
namespace applications {

// Define PImpl for ThemisHelpLoRA to match header's forward declaration.
class ThemisHelpLoRA::Impl {
public:
    Impl() { impl = createThemisHelpLoRA(); }
    std::shared_ptr<IThemisHelpLoRA> impl;
    bool isTrained() const { return impl ? impl->isTrained() : false; }
    /**
     * @brief Query.
     * @param[in] prompt Input parameter.
     * @return Return value.
     * @details Calls: std::string().
     */
    std::string query(const std::string& prompt) { return impl ? impl->query(prompt) : std::string(); }
    PerformanceMetrics getMetrics() const { return impl ? impl->getMetrics() : PerformanceMetrics(); }
    FeedbackStats getFeedbackStats() const { return impl ? impl->getFeedbackStats() : FeedbackStats(); }
    std::string getVersion() const { return impl ? impl->getVersion() : std::string(); }
};

ThemisHelpLoRA::ThemisHelpLoRA(const ThemisHelpLoRA::Config& cfg) : impl_(std::make_unique<Impl>()) {
    (void)cfg;
}
ThemisHelpLoRA::ThemisHelpLoRA() : impl_(std::make_unique<Impl>()) {}
ThemisHelpLoRA::~ThemisHelpLoRA() = default;

bool ThemisHelpLoRA::isTrained() const { return impl_->isTrained(); }
/**
 * @brief Query.
 * @param[in] question Input parameter.
 * @param[in] user_id Identifier of the user.
 * @return Return value.
 * @details Implements query without additional internal calls.
 */
std::string ThemisHelpLoRA::query(const std::string& question, const std::string& user_id) { (void)user_id; return impl_->query(question); }

PerformanceMetrics ThemisHelpLoRA::getMetrics() const { return impl_->getMetrics(); }
FeedbackStats ThemisHelpLoRA::getFeedbackStats() const { return impl_->getFeedbackStats(); }
std::string ThemisHelpLoRA::getVersion() const { return impl_->getVersion(); }

} // namespace applications
} // namespace llm
} // namespace themis
