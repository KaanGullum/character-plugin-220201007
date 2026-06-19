#pragma once

#include <plugin/IPlugin.h>

#include <array>
#include <string>

namespace arkheon::astsim {
class ModelFactoryRegistry;
}

namespace student220201007 {

class StudentMotionPlugin final : public arkheon::astlib::IPlugin {
public:
    StudentMotionPlugin() = default;
    ~StudentMotionPlugin() override = default;

    [[nodiscard]] int getInterfaceVersion() const override;
    [[nodiscard]] arkheon::astlib::PluginMetadata getMetadata() const override;

    void initialize(arkheon::astlib::PluginContext& context) override;
    void tick(double dt) override;
    void shutdown() override;

private:
    bool initialized_ = false;
    bool shutdown_ = false;
    double elapsedSeconds_ = 0.0;
    std::array<bool, 8> animationRegistered_ {};
    std::string modelType_ = "animationModelNathanHuman";
    arkheon::astsim::ModelFactoryRegistry* modelFactoryRegistry_ = nullptr;
};

} // namespace student220201007

extern "C" {
ARKHEON_ASTLIB_API arkheon::astlib::IPlugin* create_plugin();
ARKHEON_ASTLIB_API void destroy_plugin(arkheon::astlib::IPlugin* plugin);
ARKHEON_ASTLIB_API const char* get_plugin_signature();
}
