//=============================================================================================================
/**
 * @file     main.cpp
 * @brief    Deterministic public-interface example for causal reference denoising.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <rtprocessing/causalreferencedenoiser.h>

#include <Eigen/Dense>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace RTPROCESSINGLIB;

//=============================================================================================================

namespace
{

constexpr double kPi = 3.1415926535897932384626433832795;
constexpr double kSamplingFrequencyHz = 1000.0;
constexpr Index kChannelCount = 4;
constexpr Index kMaxBlockSamples = 128;
constexpr Index kTapCount = 4;
constexpr Index kAdaptationIntervalSamples = 128;
constexpr double kMemoryTimeSeconds = 30.0;
constexpr double kRegularization = 1e-3;
constexpr Index kReferenceCount = 2;
constexpr Index kTargetRow = 2;

// The coefficients are tap-major: [r0(t), r1(t), r0(t-1), r1(t-1), ...].
constexpr double kTapMajorWeights[kReferenceCount * kTapCount] = {
    1.40, -1.10,
    0.80,  0.65,
   -0.55,  0.45,
    0.35, -0.25};

double referenceValue(Index reference, std::int64_t sampleIndex) noexcept
{
    // Negative indices are the explicit zero causal prehistory.
    if (sampleIndex < 0) {
        return 0.0;
    }

    const double timeSeconds = static_cast<double>(sampleIndex) / kSamplingFrequencyHz;
    if (reference == 0) {
        return 0.80 * std::sin(2.0 * kPi * 13.0 * timeSeconds)
               + 0.35 * std::cos(2.0 * kPi * 31.0 * timeSeconds)
               + 0.20 * std::sin(2.0 * kPi * 71.0 * timeSeconds + 0.40);
    }

    return 0.60 * std::cos(2.0 * kPi * 19.0 * timeSeconds + 0.20)
           - 0.45 * std::sin(2.0 * kPi * 53.0 * timeSeconds)
           + 0.25 * std::cos(2.0 * kPi * 127.0 * timeSeconds + 0.70);
}

double environmentalNoise(std::int64_t sampleIndex) noexcept
{
    double noise = 0.0;
    for (Index lag = 0; lag < kTapCount; ++lag) {
        for (Index reference = 0; reference < kReferenceCount; ++reference) {
            const Index weightIndex = lag * kReferenceCount + reference;
            noise += kTapMajorWeights[weightIndex]
                     * referenceValue(reference, sampleIndex - lag);
        }
    }
    return noise;
}

double cleanSignal(std::int64_t sampleIndex) noexcept
{
    const double timeSeconds = static_cast<double>(sampleIndex) / kSamplingFrequencyHz;
    return 0.45 * std::sin(2.0 * kPi * 7.0 * timeSeconds + 0.25);
}

double preservedValue(std::int64_t sampleIndex) noexcept
{
    const double timeSeconds = static_cast<double>(sampleIndex) / kSamplingFrequencyHz;
    return 100.0 + 0.01 * static_cast<double>(sampleIndex)
           + 0.30 * std::cos(2.0 * kPi * 3.0 * timeSeconds);
}

// Rows are channels and columns are chronological samples: 4 x sampleCount.
MatrixXd makeBlock(std::int64_t firstSample, Index sampleCount)
{
    MatrixXd block(kChannelCount, sampleCount);
    for (Index sample = 0; sample < sampleCount; ++sample) {
        const std::int64_t sampleIndex = firstSample + static_cast<std::int64_t>(sample);
        block(0, sample) = referenceValue(0, sampleIndex);
        block(1, sample) = referenceValue(1, sampleIndex);
        block(kTargetRow, sample) = environmentalNoise(sampleIndex)
                                     + cleanSignal(sampleIndex);
        block(3, sample) = preservedValue(sampleIndex);
    }
    return block;
}

bool exactNonTargetRows(const MatrixXd& actual, const MatrixXd& expected) noexcept
{
    if (actual.rows() != expected.rows() || actual.cols() != expected.cols()) {
        return false;
    }

    for (Index row = 0; row < actual.rows(); ++row) {
        if (row == kTargetRow) {
            continue;
        }
        for (Index sample = 0; sample < actual.cols(); ++sample) {
            if (actual(row, sample) != expected(row, sample)) {
                return false;
            }
        }
    }
    return true;
}

bool finiteTarget(const MatrixXd& block) noexcept
{
    for (Index sample = 0; sample < block.cols(); ++sample) {
        if (!std::isfinite(block(kTargetRow, sample))) {
            return false;
        }
    }
    return true;
}

bool exactBlock(const MatrixXd& actual, const MatrixXd& expected) noexcept
{
    if (actual.rows() != expected.rows() || actual.cols() != expected.cols()) {
        return false;
    }

    for (Index row = 0; row < actual.rows(); ++row) {
        for (Index sample = 0; sample < actual.cols(); ++sample) {
            if (actual(row, sample) != expected(row, sample)) {
                return false;
            }
        }
    }
    return true;
}

const char* statusName(DenoiserProcessStatus status) noexcept
{
    switch (status) {
    case DenoiserProcessStatus::NotConfigured:
        return "NotConfigured";
    case DenoiserProcessStatus::InvalidShape:
        return "InvalidShape";
    case DenoiserProcessStatus::NonFiniteInput:
        return "NonFiniteInput";
    case DenoiserProcessStatus::Bypassed:
        return "Bypassed";
    case DenoiserProcessStatus::Processed:
        return "Processed";
    }
    return "Unknown";
}

void printStage(const char* stage, const DenoiserProcessResult& result)
{
    const DenoiserProcessDiagnostics& diagnostics = result.diagnostics;
    std::cout << std::setprecision(10)
              << stage
              << " status=" << statusName(result.status)
              << " warmup=" << diagnostics.warmupSamplesRemaining
              << " generation=" << diagnostics.modelGeneration
              << " accepted=" << diagnostics.modelUpdatesAccepted
              << " rejected=" << diagnostics.modelUpdatesRejected
              << " inputRms=" << diagnostics.inputRms
              << " outputRms=" << diagnostics.outputRms
              << " estimatedNoiseRms=" << diagnostics.estimatedNoiseRms
              << '\n';
}

bool verifyCommonStage(const char* stage,
                       const MatrixXd& before,
                       const MatrixXd& after,
                       const DenoiserProcessResult& result,
                       bool bypass) noexcept
{
    if (result.status != (bypass ? DenoiserProcessStatus::Bypassed
                                 : DenoiserProcessStatus::Processed)) {
        std::cerr << "FAIL " << stage << ": unexpected process status\n";
        return false;
    }
    if (result.diagnostics.referenceRowCount != kReferenceCount
        || result.diagnostics.targetRowCount != 1
        || result.diagnostics.featureCount != kReferenceCount * kTapCount) {
        std::cerr << "FAIL " << stage << ": unexpected R/M/P diagnostics\n";
        return false;
    }
    if (!exactNonTargetRows(after, before)) {
        std::cerr << "FAIL " << stage << ": reference/preserved row changed\n";
        return false;
    }
    if (!finiteTarget(after)) {
        std::cerr << "FAIL " << stage << ": target output is nonfinite\n";
        return false;
    }
    if (bypass && !exactBlock(after, before)) {
        std::cerr << "FAIL " << stage << ": bypass changed the block\n";
        return false;
    }
    return true;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main()
{
    CausalReferenceDenoiserConfig config;
    config.samplingFrequencyHz = kSamplingFrequencyHz;
    config.channelCount = kChannelCount;
    config.maxBlockSamples = kMaxBlockSamples;
    config.referenceRows = VectorXi(kReferenceCount);
    config.referenceRows << 0, 1;
    config.targetRows = VectorXi(1);
    config.targetRows << kTargetRow;
    config.tapCount = kTapCount;
    config.adaptationIntervalSamples = kAdaptationIntervalSamples;
    config.memoryTimeSeconds = kMemoryTimeSeconds;
    config.regularization = kRegularization;

    CausalReferenceDenoiser denoiser;
    if (denoiser.configure(config) != DenoiserStatus::Configured) {
        std::cerr << "FAIL configure: requested configuration was rejected\n";
        return 1;
    }

    const Index featureCount = config.referenceRows.size() * config.tapCount;
    std::cout << "configured R=" << config.referenceRows.size()
              << " M=" << config.targetRows.size()
              << " P=" << featureCount
              << " fs=" << config.samplingFrequencyHz
              << " maxBlock=" << config.maxBlockSamples
              << " taps=" << config.tapCount
              << " interval=" << config.adaptationIntervalSamples
              << " memorySeconds=" << config.memoryTimeSeconds
              << " regularization=" << config.regularization << '\n';
    std::cout << "tap-major weights=[1.40,-1.10,0.80,0.65,-0.55,0.45,0.35,-0.25]\n";

    // A global sample index keeps the reference stream causal and continuous across blocks.
    std::int64_t nextSample = 0;

    MatrixXd bypassWarmup = makeBlock(nextSample, 2);
    const MatrixXd bypassWarmupInput = bypassWarmup;
    const DenoiserProcessResult bypassWarmupResult =
        denoiser.process(bypassWarmup, DenoisingMode::BypassTrackHistory);
    printStage("bypass-warmup", bypassWarmupResult);
    if (!verifyCommonStage("bypass-warmup",
                           bypassWarmupInput,
                           bypassWarmup,
                           bypassWarmupResult,
                           true)
        || bypassWarmupResult.diagnostics.warmupSamplesRemaining
               != kTapCount - 1 - bypassWarmup.cols()
        || bypassWarmupResult.diagnostics.modelGeneration != 0
        || bypassWarmupResult.diagnostics.modelUpdatesAccepted != 0
        || bypassWarmupResult.diagnostics.modelUpdatesRejected != 0) {
        std::cerr << "FAIL bypass-warmup: unexpected history diagnostics\n";
        return 1;
    }
    nextSample += bypassWarmup.cols();

    std::uint64_t learnedGeneration = 0;
    for (int blockIndex = 0; blockIndex < 5; ++blockIndex) {
        MatrixXd learningBlock = makeBlock(nextSample, kMaxBlockSamples);
        const MatrixXd learningInput = learningBlock;
        const DenoiserProcessResult learningResult =
            denoiser.process(learningBlock, DenoisingMode::ApplyAndLearn);
        const char* stage = blockIndex == 0 ? "learn-1"
                                            : blockIndex == 1 ? "learn-2"
                                                              : blockIndex == 2 ? "learn-3"
                                                                                : blockIndex == 3 ? "learn-4"
                                                                                                  : "learn-5";
        printStage(stage, learningResult);
        if (!verifyCommonStage(stage,
                               learningInput,
                               learningBlock,
                               learningResult,
                               false)) {
            return 1;
        }
        learnedGeneration = learningResult.diagnostics.modelGeneration;
        nextSample += learningBlock.cols();
    }

    if (learnedGeneration == 0) {
        std::cerr << "FAIL learning: model generation never advanced\n";
        return 1;
    }

    MatrixXd frozenBlock = makeBlock(nextSample, kMaxBlockSamples);
    const MatrixXd frozenInput = frozenBlock;
    const std::uint64_t generationBeforeFreeze = learnedGeneration;
    const DenoiserProcessResult frozenResult =
        denoiser.process(frozenBlock, DenoisingMode::ApplyOnly);
    printStage("apply-only-freeze", frozenResult);
    if (!verifyCommonStage("apply-only-freeze",
                           frozenInput,
                           frozenBlock,
                           frozenResult,
                           false)
        || frozenResult.diagnostics.modelGeneration != generationBeforeFreeze
        || frozenResult.diagnostics.modelUpdatesAccepted != 0
        || frozenResult.diagnostics.modelUpdatesRejected != 0) {
        std::cerr << "FAIL apply-only-freeze: model changed or emitted update events\n";
        return 1;
    }
    nextSample += frozenBlock.cols();

    denoiser.reset();

    MatrixXd resetBypass = makeBlock(nextSample, 1);
    const MatrixXd resetBypassInput = resetBypass;
    const DenoiserProcessResult resetBypassResult =
        denoiser.process(resetBypass, DenoisingMode::BypassTrackHistory);
    printStage("reset-then-bypass", resetBypassResult);
    if (!verifyCommonStage("reset-then-bypass",
                           resetBypassInput,
                           resetBypass,
                           resetBypassResult,
                           true)
        || resetBypassResult.diagnostics.modelGeneration != 0
        || resetBypassResult.diagnostics.warmupSamplesRemaining != kTapCount - 2
        || resetBypassResult.diagnostics.modelUpdatesAccepted != 0
        || resetBypassResult.diagnostics.modelUpdatesRejected != 0) {
        std::cerr << "FAIL reset-then-bypass: reset did not restore generation/warmup\n";
        return 1;
    }

    std::cout << "example invariants: PASS\n";
    return 0;
}
