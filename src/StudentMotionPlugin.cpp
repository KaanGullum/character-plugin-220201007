#include "StudentMotionPlugin.h"

#include <core/logging/GlobalLogger.h>
#include <model/AnimationModel.h>
#include <model/ModelFactoryRegistry.h>
#include <plugin/IModelPluginService.h>
#include <plugin/IPluginServices.h>
#include <plugin/PluginContext.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <string>
#include <string_view>
#include <unordered_set>

#include <Windows.h>

namespace student220201007 {
namespace {

constexpr std::array<std::string_view, 8> kAnimationCodes {
    "Student Walk",
    "Student Run",
    "Student Jump",
    "Student Crouch",
    "Idle Neutral",
    "Idle Breathing",
    "Idle Shake",
    "Idle Stopped"
};

constexpr double kPi = 3.14159265358979323846;
constexpr std::string_view kLogCategory = "student-220201007-motion-plugin";

// 0: Walk, 1: Run, 2: Jump, 3: Crouch. -1 follows the active scenario script.
std::atomic<int> gManualMotionState {-1};

[[nodiscard]] double clamp(double value, double low, double high) {
    return std::max(low, std::min(value, high));
}

[[nodiscard]] bool hasJoint(
    const std::unordered_set<std::string>& availableJointIds,
    const char* jointId) {
    if (!jointId || *jointId == '\0') {
        return false;
    }
    if (availableJointIds.empty()) {
        return true;
    }
    return availableJointIds.find(jointId) != availableJointIds.end();
}

void pushJoint(
    const std::unordered_set<std::string>& availableJointIds,
    arkheon::astsim::AnimationModelOutput& output,
    const char* jointId,
    double xRad,
    double yRad,
    double zRad) {
    if (!hasJoint(availableJointIds, jointId)) {
        return;
    }
    constexpr double kMaxAbsRad = 2.4;
    output.jointOverrides.push_back({
        jointId,
        clamp(xRad, -kMaxAbsRad, kMaxAbsRad),
        clamp(yRad, -kMaxAbsRad, kMaxAbsRad),
        clamp(zRad, -kMaxAbsRad, kMaxAbsRad)
    });
}

[[nodiscard]] std::unordered_set<std::string> collectJoints(
    const arkheon::astsim::AnimationModelInput& input) {
    std::unordered_set<std::string> availableJointIds;
    availableJointIds.reserve(input.entity.joints.size());
    for (const auto& joint : input.entity.joints) {
        availableJointIds.insert(joint.jointId);
    }
    return availableJointIds;
}

void prepareOutput(arkheon::astsim::AnimationModelOutput& output) {
    output.clearExistingJointOverrides = true;
    output.jointOverrides.clear();
    output.jointOverrides.reserve(10);
}

// Global blending state
double g_lastSimulationTime = -1.0;
double g_blendWeights[4] = {1.0, 0.0, 0.0, 0.0};
double g_startupTime = -1.0;

constexpr const char* kJointNames[10] = {
    "leftShoulder", "rightShoulder", "leftElbow", "rightElbow",
    "leftHip", "rightHip", "leftKnee", "rightKnee", "leftAnkle", "rightAnkle"
};

struct Pose {
    double values[10][3];
    Pose() {
        for(int i=0; i<10; ++i) { values[i][0]=0; values[i][1]=0; values[i][2]=0; }
    }
};

void applyBaseArms(Pose& p, double elbowBendZ) {
    p.values[0][0] = 1.57; p.values[0][1] = 1.57; p.values[0][2] = 1.57;
    p.values[1][0] = 1.57; p.values[1][1] = 1.57; p.values[1][2] = 1.57;
    p.values[2][0] = elbowBendZ;
    p.values[3][0] = elbowBendZ;
}

Pose computeState0_Walk(double t) {
    Pose p;
    applyBaseArms(p, 0.05);
    const double leftHip    =  std::sin(5.0 * t) * 0.45;
    const double rightHip   = -leftHip;
    const double leftKnee   = (leftHip  > 0.0) ? -leftHip  * 0.75 : -0.05;
    const double rightKnee  = (rightHip > 0.0) ? -rightHip * 0.75 : -0.05;
    const double leftAnkle  = -leftKnee  * 0.30;
    const double rightAnkle = -rightKnee * 0.30;
    p.values[4][2] = leftHip;
    p.values[5][2] = rightHip;
    p.values[6][2] = leftKnee;
    p.values[7][2] = rightKnee;
    p.values[8][2] = leftAnkle;
    p.values[9][2] = rightAnkle;
    const double armPhase = std::sin(5.0 * t);
    const double leftForwardY = 0.04 + (std::max(0.0, -armPhase) * 0.18);
    const double rightForwardY = 0.04 + (std::max(0.0, armPhase) * 0.18);
    p.values[0][1] += leftForwardY;
    p.values[1][1] += rightForwardY;
    p.values[2][0] = 0.25 + (std::max(0.0, -armPhase) * 0.30);
    p.values[3][0] = 0.25 + (std::max(0.0, armPhase) * 0.30);
    return p;
}

Pose computeState1_Run(double t) {
    Pose p;
    applyBaseArms(p, 0.20);
    const double leftHip    =  std::sin(10.0 * t) * 0.70;
    const double rightHip   = -leftHip;
    const double leftKnee   = (leftHip  > 0.0) ? -leftHip  * 1.15 : -0.10;
    const double rightKnee  = (rightHip > 0.0) ? -rightHip * 1.15 : -0.10;
    const double leftAnkle  = -leftKnee  * 0.40;
    const double rightAnkle = -rightKnee * 0.40;
    p.values[4][2] = leftHip;
    p.values[5][2] = rightHip;
    p.values[6][2] = leftKnee;
    p.values[7][2] = rightKnee;
    p.values[8][2] = leftAnkle;
    p.values[9][2] = rightAnkle;
    const double leftSwing  =  std::sin(10.0 * t) * 0.30;
    const double rightSwing = -leftSwing;
    p.values[0][0] += leftSwing;
    p.values[1][0] += rightSwing;
    const double armPhase = std::sin(10.0 * t);
    const double leftForwardY = 0.08 + (std::max(0.0, -armPhase) * 0.34);
    const double rightForwardY = 0.08 + (std::max(0.0, armPhase) * 0.34);
    p.values[0][1] += leftForwardY;
    p.values[1][1] += rightForwardY;
    p.values[2][0] = 0.90 + (std::max(0.0, -armPhase) * 0.20);
    p.values[3][0] = 0.90 + (std::max(0.0, armPhase) * 0.20);
    return p;
}

Pose computeState2_Jump(double t) {
    Pose p;
    const double cycle       =  std::sin((4.2 * t) - 1.57079632679);
    const double compression =  std::max(0.0, -cycle);
    const double extension   =  std::max(0.0,  cycle);
    const double hip         =  0.78 * compression - 0.06 * extension;
    const double knee        = -1.25 * compression + 0.02 * extension;
    const double ankle       = -0.45 * compression + 0.18 * extension;
    const double armTakeoff  =  extension;
    const double armForwardY =  0.45 * armTakeoff;
    const double elbow       =  0.10 + (0.25 * compression) + (0.15 * armTakeoff);
    applyBaseArms(p, elbow);
    p.values[4][2] = hip;
    p.values[5][2] = hip;
    p.values[6][2] = knee;
    p.values[7][2] = knee;
    p.values[8][2] = ankle;
    p.values[9][2] = ankle;
    p.values[0][1] += armForwardY;
    p.values[1][1] += armForwardY;
    return p;
}

Pose computeState3_Crouch(double t) {
    Pose p;
    applyBaseArms(p, 0.05);
    double pulse = std::sin(t * 1.5) * 0.10;
    double elbowPulse = 0.5 + (0.5 * std::sin((t * 1.5) + kPi));
    p.values[4][2] = 1.45 + pulse;
    p.values[5][2] = 1.45 + pulse;
    p.values[6][2] = -1.45 - pulse;
    p.values[7][2] = -1.45 - pulse;
    p.values[8][2] = -0.15;
    p.values[9][2] = -0.15;
    p.values[2][0] = 0.16 + (elbowPulse * 0.12);
    p.values[3][0] = 0.16 + (elbowPulse * 0.12);
    return p;
}

void updateManualMotionStateFromKeyboard() {
    if ((GetAsyncKeyState('0') & 0x8000) != 0) {
        gManualMotionState.store(-1, std::memory_order_relaxed);
        return;
    }
    if ((GetAsyncKeyState('1') & 0x8000) != 0) {
        gManualMotionState.store(0, std::memory_order_relaxed);
    } else if ((GetAsyncKeyState('2') & 0x8000) != 0) {
        gManualMotionState.store(1, std::memory_order_relaxed);
    } else if ((GetAsyncKeyState('3') & 0x8000) != 0) {
        gManualMotionState.store(2, std::memory_order_relaxed);
    } else if ((GetAsyncKeyState('4') & 0x8000) != 0) {
        gManualMotionState.store(3, std::memory_order_relaxed);
    }
}

[[nodiscard]] bool evaluateMotionState(
    const arkheon::astsim::AnimationModelInput& input,
    arkheon::astsim::AnimationModelOutput& output,
    int fallbackState) {
    updateManualMotionStateFromKeyboard();
    const int manualState = gManualMotionState.load(std::memory_order_relaxed);
    const int targetState = manualState >= 0 ? manualState : fallbackState;

    double t = input.simulationTimeSeconds;
    
    // Initialize timing
    if (g_startupTime < 0.0) {
        g_startupTime = t;
        g_lastSimulationTime = t;
        for(int i=0; i<4; ++i) g_blendWeights[i] = (i == targetState) ? 1.0 : 0.0;
    }

    double dt = t - g_lastSimulationTime;
    if (dt < 0.0 || dt > 0.2) dt = 0.016; // protect against large pauses
    g_lastSimulationTime = t;

    // Transition blend speed (e.g. 2.0 = 0.5 seconds to fully blend)
    const double blendSpeed = 2.0; 
    double sum = 0.0;
    for (int i = 0; i < 4; ++i) {
        if (i == targetState) {
            g_blendWeights[i] += blendSpeed * dt;
        } else {
            g_blendWeights[i] -= blendSpeed * dt;
        }
        g_blendWeights[i] = clamp(g_blendWeights[i], 0.0, 1.0);
        sum += g_blendWeights[i];
    }
    // Normalize weights
    if (sum > 0.0) {
        for (int i = 0; i < 4; ++i) {
            g_blendWeights[i] /= sum;
        }
    }

    // Compute all 4 states for this exact time
    Pose poses[4] = {
        computeState0_Walk(t),
        computeState1_Run(t),
        computeState2_Jump(t),
        computeState3_Crouch(t)
    };

    // Calculate startup trick for "Calib first".
    // Keep offsets exactly 0 for the first 0.2s so the GLB calibrates to true T-pose,
    // then blend smoothly to the active pose over the next 1.0s.
    double elapsed = t - g_startupTime;
    double startupBlend = clamp((elapsed - 0.2) * 1.0, 0.0, 1.0);

    // Final blended pose
    Pose finalPose;
    for (int j = 0; j < 10; ++j) {
        for (int axis = 0; axis < 3; ++axis) {
            double val = 0.0;
            for (int s = 0; s < 4; ++s) {
                val += poses[s].values[j][axis] * g_blendWeights[s];
            }
            finalPose.values[j][axis] = val * startupBlend;
        }
    }

    prepareOutput(output);
    auto joints = collectJoints(input);
    for (int j = 0; j < 10; ++j) {
        pushJoint(joints, output, kJointNames[j], 
            finalPose.values[j][0], 
            finalPose.values[j][1], 
            finalPose.values[j][2]);
    }
    return output.jointOverrides.size() == 10;
}

// Evaluator mapping for the four submitted states.
[[nodiscard]] bool evaluateStateWalk(
    const arkheon::astsim::AnimationModelInput& input,
    arkheon::astsim::AnimationModelOutput& output) {
    return evaluateMotionState(input, output, 0);
}
[[nodiscard]] bool evaluateStateRun(
    const arkheon::astsim::AnimationModelInput& input,
    arkheon::astsim::AnimationModelOutput& output) {
    return evaluateMotionState(input, output, 1);
}
[[nodiscard]] bool evaluateStateJump(
    const arkheon::astsim::AnimationModelInput& input,
    arkheon::astsim::AnimationModelOutput& output) {
    return evaluateMotionState(input, output, 2);
}
[[nodiscard]] bool evaluateStateCrouch(
    const arkheon::astsim::AnimationModelInput& input,
    arkheon::astsim::AnimationModelOutput& output) {
    return evaluateMotionState(input, output, 3);
}

[[nodiscard]] arkheon::astsim::IAnimationModel::AnimationEvaluationFunction evaluatorForIndex(std::size_t index) {
    switch (index) {
    case 0: case 4: return evaluateStateWalk;
    case 1: case 5: return evaluateStateRun;
    case 2: case 6: return evaluateStateJump;
    case 3: case 7: return evaluateStateCrouch;
    default: return {};
    }
}

} // namespace

int StudentMotionPlugin::getInterfaceVersion() const { return 1; }
arkheon::astlib::PluginMetadata StudentMotionPlugin::getMetadata() const {
    arkheon::astlib::PluginMetadata metadata;
    metadata.setPluginId("student-220201007-motion-plugin");
    metadata.setVersion("1.0.0");
    metadata.setAuthor("220201007");
    return metadata;
}

void StudentMotionPlugin::initialize(arkheon::astlib::PluginContext& context) {
    initialized_ = true; shutdown_ = false;
    elapsedSeconds_ = 0.0;
    animationRegistered_.fill(false);
    modelFactoryRegistry_ = nullptr;

    if (context.services) {
        auto* rawService = context.services->getService(arkheon::astsim::IModelPluginService::kPluginServiceId);
        auto* service = static_cast<arkheon::astsim::IModelPluginService*>(rawService);
        modelFactoryRegistry_ = service ? &service->modelFactoryRegistry() : nullptr;
    }
    AST_LOG_INFO("initialize: modelService=" + std::string(modelFactoryRegistry_ ? "yes" : "no"), kLogCategory);
    
    if (!modelFactoryRegistry_) return;
    auto* prototypeBase = modelFactoryRegistry_->getRegisteredPrototype(modelType_);
    auto* prototypeAnimationModel = dynamic_cast<arkheon::astsim::IAnimationModel*>(prototypeBase);
    if (!prototypeAnimationModel) return;

    for (std::size_t i = 0; i < kAnimationCodes.size(); ++i) {
        animationRegistered_[i] = prototypeAnimationModel->registerAnimation(kAnimationCodes[i], evaluatorForIndex(i));
    }
}

void StudentMotionPlugin::tick(double dt) {
    if (!initialized_ || shutdown_) return;
    elapsedSeconds_ += clamp(dt, 0.0, 0.1);
}

void StudentMotionPlugin::shutdown() {
    if (modelFactoryRegistry_) {
        auto* prototypeBase = modelFactoryRegistry_->getRegisteredPrototype(modelType_);
        auto* prototypeAnimationModel = dynamic_cast<arkheon::astsim::IAnimationModel*>(prototypeBase);
        if (prototypeAnimationModel) {
            for (std::size_t i = 0; i < kAnimationCodes.size(); ++i) {
                if (animationRegistered_[i]) {
                    static_cast<void>(prototypeAnimationModel->registerAnimation(
                        kAnimationCodes[i], arkheon::astsim::IAnimationModel::AnimationEvaluationFunction {}));
                }
            }
        }
    }
    animationRegistered_.fill(false);
    shutdown_ = true;
    modelFactoryRegistry_ = nullptr;
}

} // namespace student220201007

extern "C" {
ARKHEON_ASTLIB_API arkheon::astlib::IPlugin* create_plugin() { return new student220201007::StudentMotionPlugin(); }
ARKHEON_ASTLIB_API void destroy_plugin(arkheon::astlib::IPlugin* plugin) { delete plugin; }
ARKHEON_ASTLIB_API const char* get_plugin_signature() { return "ARKHEON_PLUGIN_V1"; }
} // extern "C"
