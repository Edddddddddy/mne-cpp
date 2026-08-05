//=============================================================================================================
/**
 * @file     test_adaptive_denoising_plugin.cpp
 * @brief    Public-interface tracer for adaptive denoising channel mapping.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <adaptivedenoising/adaptivedenoisingprocessor.h>
#include <adaptivedenoising/adaptivedenoisingblockqueue.h>

#include <fiff/fiff_constants.h>

#include <Eigen/Dense>

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <memory>
#include <type_traits>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ADAPTIVEDENOISINGPLUGIN;
using namespace Eigen;
using namespace RTPROCESSINGLIB;

//=============================================================================================================

static_assert(
    std::is_nothrow_default_constructible<AdaptiveDenoisingProcessor>::value,
    "AdaptiveDenoisingProcessor must be nothrow default constructible");
static_assert(
    !std::is_copy_constructible<AdaptiveDenoisingProcessor>::value,
    "AdaptiveDenoisingProcessor must not be copy constructible");
static_assert(
    !std::is_copy_assignable<AdaptiveDenoisingProcessor>::value,
    "AdaptiveDenoisingProcessor must not be copy assignable");
static_assert(
    !std::is_move_constructible<AdaptiveDenoisingProcessor>::value,
    "AdaptiveDenoisingProcessor must not be move constructible");
static_assert(
    !std::is_move_assignable<AdaptiveDenoisingProcessor>::value,
    "AdaptiveDenoisingProcessor must not be move assignable");

//=============================================================================================================

namespace
{

constexpr Index kChannelCount = 6;
constexpr Index kBlockSamples = 16;
constexpr Index kTargetRow = 2;

struct InvalidConfigureCase
{
    const char*                         name;
    AdaptiveDenoisingConfigureStatus   expectedStatus;
    AdaptiveDenoisingStreamDescriptor  stream;
    Index                              maxBlockSamples;
    AdaptiveDenoisingSettings          settings;
};

AdaptiveDenoisingStreamDescriptor goodTwoChannelStream()
{
    return AdaptiveDenoisingStreamDescriptor{
        1000.0,
        {{FIFFV_REF_MEG_CH, false}, {FIFFV_MEG_CH, false}}};
}

AdaptiveDenoisingSettings goodSettings()
{
    AdaptiveDenoisingSettings settings;
    settings.tapCount = 1;
    settings.adaptationIntervalSamples = kBlockSamples;
    settings.memoryTimeSeconds = 30.0;
    settings.regularization = 1e-8;
    return settings;
}

AdaptiveDenoisingStreamDescriptor streamWithChannels(
    std::initializer_list<AdaptiveDenoisingChannelDescriptor> channels)
{
    return AdaptiveDenoisingStreamDescriptor{1000.0, channels};
}

bool rowEquals(const MatrixXd& actual, const MatrixXd& expected, Index row) noexcept
{
    for (Index column = 0; column < expected.cols(); ++column) {
        if (actual(row, column) != expected(row, column)) {
            return false;
        }
    }

    return true;
}

bool matrixEquals(const MatrixXd& actual, const MatrixXd& expected) noexcept
{
    if (actual.rows() != expected.rows() || actual.cols() != expected.cols()) {
        return false;
    }

    for (Index row = 0; row < expected.rows(); ++row) {
        if (!rowEquals(actual, expected, row)) {
            return false;
        }
    }

    return true;
}

bool sameOwner(
    const std::shared_ptr<const FIFFLIB::FiffInfo>& first,
    const std::shared_ptr<const FIFFLIB::FiffInfo>& second) noexcept
{
    return !first.owner_before(second) && !second.owner_before(first);
}

} // namespace

//=============================================================================================================

class TestAdaptiveDenoisingPlugin : public QObject
{
    Q_OBJECT

private slots:
    void mapsRowsTrainsAndAppliesOnly();
    void invalidConfigurationDisarmsLearnedModel();
    void validReconfigureResetsAndRelearnsModel();
    void queuePreservesFifoDropNewestAndMetadata();
};

//=============================================================================================================

void TestAdaptiveDenoisingPlugin::mapsRowsTrainsAndAppliesOnly()
{
    AdaptiveDenoisingStreamDescriptor stream;
    stream.samplingFrequencyHz = 1000.0;
    stream.channels = {
        {FIFFV_REF_MEG_CH, false},
        {FIFFV_REF_MEG_CH, true},
        {FIFFV_MEG_CH, false},
        {FIFFV_MEG_CH, true},
        {FIFFV_STIM_CH, false},
        {FIFFV_MISC_CH, false}
    };

    AdaptiveDenoisingSettings settings;
    settings.tapCount = 1;
    settings.adaptationIntervalSamples = kBlockSamples;
    settings.memoryTimeSeconds = 30.0;
    settings.regularization = 1e-8;

    AdaptiveDenoisingProcessor processor;
    const AdaptiveDenoisingConfigureResult configureResult =
        processor.configure(stream, kBlockSamples, settings);

    QVERIFY(configureResult.status == AdaptiveDenoisingConfigureStatus::Ready);
    QCOMPARE(configureResult.referenceCount, Index(1));
    QCOMPARE(configureResult.targetCount, Index(1));
    QCOMPARE(configureResult.featureCount, Index(1));

    MatrixXd train(kChannelCount, kBlockSamples);
    for (Index column = 0; column < kBlockSamples; ++column) {
        train(0, column) = static_cast<double>(column + 1);
        train(2, column) = 2.0 * train(0, column);
        train(1, column) = 1001.0 + static_cast<double>(column);
        train(3, column) = 2001.0 + static_cast<double>(column);
        train(4, column) = 3001.0 + static_cast<double>(column);
        train(5, column) = 4001.0 + static_cast<double>(column);
    }
    const MatrixXd expectedTrain = train;

    const DenoiserProcessResult trainResult =
        processor.process(train, DenoisingMode::ApplyAndLearn);

    QVERIFY(trainResult.status == DenoiserProcessStatus::Processed);
    QCOMPARE(trainResult.diagnostics.referenceRowCount, Index(1));
    QCOMPARE(trainResult.diagnostics.targetRowCount, Index(1));
    QCOMPARE(trainResult.diagnostics.featureCount, Index(1));
    QCOMPARE(trainResult.diagnostics.modelGeneration, std::uint64_t(1));
    QCOMPARE(trainResult.diagnostics.modelUpdatesAccepted, std::uint64_t(1));
    QCOMPARE(trainResult.diagnostics.modelUpdatesRejected, std::uint64_t(0));

    for (Index row = 0; row < kChannelCount; ++row) {
        if (row != kTargetRow) {
            QVERIFY(rowEquals(train, expectedTrain, row));
        }
    }

    MatrixXd probe(kChannelCount, kBlockSamples);
    for (Index column = 0; column < kBlockSamples; ++column) {
        probe(0, column) = 17.0;
        probe(2, column) = 34.0;
        probe(1, column) = 5001.0 + static_cast<double>(column);
        probe(3, column) = 6001.0 + static_cast<double>(column);
        probe(4, column) = 7001.0 + static_cast<double>(column);
        probe(5, column) = 8001.0 + static_cast<double>(column);
    }
    const MatrixXd expectedProbe = probe;

    const DenoiserProcessResult probeResult =
        processor.process(probe, DenoisingMode::ApplyOnly);

    QVERIFY(probeResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(probe.array().isFinite().all());

    for (Index row = 0; row < kChannelCount; ++row) {
        if (row != kTargetRow) {
            QVERIFY(rowEquals(probe, expectedProbe, row));
        }
    }

    for (Index column = 0; column < kBlockSamples; ++column) {
        QVERIFY(std::abs(probe(kTargetRow, column)) <= 1e-5);
    }
}

//=============================================================================================================

void TestAdaptiveDenoisingPlugin::invalidConfigurationDisarmsLearnedModel()
{
    const double quietNaN = std::numeric_limits<double>::quiet_NaN();
    std::vector<InvalidConfigureCase> cases;

    {
        AdaptiveDenoisingStreamDescriptor stream = goodTwoChannelStream();
        stream.samplingFrequencyHz = 0.0;
        cases.push_back(InvalidConfigureCase{
            "InvalidMetadata/fs-zero",
            AdaptiveDenoisingConfigureStatus::InvalidMetadata,
            stream,
            kBlockSamples,
            goodSettings()});
    }

    {
        AdaptiveDenoisingStreamDescriptor stream = goodTwoChannelStream();
        stream.samplingFrequencyHz = quietNaN;
        cases.push_back(InvalidConfigureCase{
            "InvalidMetadata/fs-NaN",
            AdaptiveDenoisingConfigureStatus::InvalidMetadata,
            stream,
            kBlockSamples,
            goodSettings()});
    }

    cases.push_back(InvalidConfigureCase{
        "InvalidMetadata/empty-channels",
        AdaptiveDenoisingConfigureStatus::InvalidMetadata,
        streamWithChannels({}),
        kBlockSamples,
        goodSettings()});

    cases.push_back(InvalidConfigureCase{
        "InvalidMetadata/max-block-zero",
        AdaptiveDenoisingConfigureStatus::InvalidMetadata,
        goodTwoChannelStream(),
        0,
        goodSettings()});

    cases.push_back(InvalidConfigureCase{
        "MissingReferences/bad-ref-only",
        AdaptiveDenoisingConfigureStatus::MissingReferences,
        streamWithChannels({{FIFFV_REF_MEG_CH, true}, {FIFFV_MEG_CH, false}}),
        kBlockSamples,
        goodSettings()});

    cases.push_back(InvalidConfigureCase{
        "MissingReferences/no-ref-kind",
        AdaptiveDenoisingConfigureStatus::MissingReferences,
        streamWithChannels({{FIFFV_MEG_CH, false}}),
        kBlockSamples,
        goodSettings()});

    cases.push_back(InvalidConfigureCase{
        "MissingTargets/bad-meg-only",
        AdaptiveDenoisingConfigureStatus::MissingTargets,
        streamWithChannels({{FIFFV_REF_MEG_CH, false}, {FIFFV_MEG_CH, true}}),
        kBlockSamples,
        goodSettings()});

    cases.push_back(InvalidConfigureCase{
        "MissingTargets/no-meg-kind",
        AdaptiveDenoisingConfigureStatus::MissingTargets,
        streamWithChannels({{FIFFV_REF_MEG_CH, false}}),
        kBlockSamples,
        goodSettings()});

    {
        AdaptiveDenoisingSettings settings = goodSettings();
        settings.tapCount = 0;
        cases.push_back(InvalidConfigureCase{
            "InvalidSettings/tap-zero",
            AdaptiveDenoisingConfigureStatus::InvalidSettings,
            goodTwoChannelStream(),
            kBlockSamples,
            settings});
    }

    {
        AdaptiveDenoisingSettings settings = goodSettings();
        settings.tapCount = 33;
        cases.push_back(InvalidConfigureCase{
            "InvalidSettings/tap-33",
            AdaptiveDenoisingConfigureStatus::InvalidSettings,
            goodTwoChannelStream(),
            kBlockSamples,
            settings});
    }

    {
        AdaptiveDenoisingSettings settings = goodSettings();
        settings.adaptationIntervalSamples = 15;
        cases.push_back(InvalidConfigureCase{
            "InvalidSettings/interval-15",
            AdaptiveDenoisingConfigureStatus::InvalidSettings,
            goodTwoChannelStream(),
            kBlockSamples,
            settings});
    }

    {
        AdaptiveDenoisingSettings settings = goodSettings();
        settings.adaptationIntervalSamples = 2049;
        cases.push_back(InvalidConfigureCase{
            "InvalidSettings/interval-2049",
            AdaptiveDenoisingConfigureStatus::InvalidSettings,
            goodTwoChannelStream(),
            kBlockSamples,
            settings});
    }

    {
        AdaptiveDenoisingSettings settings = goodSettings();
        settings.memoryTimeSeconds = 0.5;
        cases.push_back(InvalidConfigureCase{
            "InvalidSettings/memory-below-min",
            AdaptiveDenoisingConfigureStatus::InvalidSettings,
            goodTwoChannelStream(),
            kBlockSamples,
            settings});
    }

    {
        AdaptiveDenoisingSettings settings = goodSettings();
        settings.memoryTimeSeconds = 301.0;
        cases.push_back(InvalidConfigureCase{
            "InvalidSettings/memory-301",
            AdaptiveDenoisingConfigureStatus::InvalidSettings,
            goodTwoChannelStream(),
            kBlockSamples,
            settings});
    }

    {
        AdaptiveDenoisingSettings settings = goodSettings();
        settings.memoryTimeSeconds = quietNaN;
        cases.push_back(InvalidConfigureCase{
            "InvalidSettings/memory-NaN",
            AdaptiveDenoisingConfigureStatus::InvalidSettings,
            goodTwoChannelStream(),
            kBlockSamples,
            settings});
    }

    {
        AdaptiveDenoisingSettings settings = goodSettings();
        settings.regularization = 1e-9;
        cases.push_back(InvalidConfigureCase{
            "InvalidSettings/regularization-below-min",
            AdaptiveDenoisingConfigureStatus::InvalidSettings,
            goodTwoChannelStream(),
            kBlockSamples,
            settings});
    }

    {
        AdaptiveDenoisingSettings settings = goodSettings();
        settings.regularization = 1.0000001;
        cases.push_back(InvalidConfigureCase{
            "InvalidSettings/regularization-above-max",
            AdaptiveDenoisingConfigureStatus::InvalidSettings,
            goodTwoChannelStream(),
            kBlockSamples,
            settings});
    }

    {
        AdaptiveDenoisingSettings settings = goodSettings();
        settings.regularization = quietNaN;
        cases.push_back(InvalidConfigureCase{
            "InvalidSettings/regularization-NaN",
            AdaptiveDenoisingConfigureStatus::InvalidSettings,
            goodTwoChannelStream(),
            kBlockSamples,
            settings});
    }

    {
        AdaptiveDenoisingSettings settings = goodSettings();
        settings.tapCount = 32;
        cases.push_back(InvalidConfigureCase{
            "InvalidSettings/feature-cap-9x32",
            AdaptiveDenoisingConfigureStatus::InvalidSettings,
            streamWithChannels({
                {FIFFV_REF_MEG_CH, false},
                {FIFFV_REF_MEG_CH, false},
                {FIFFV_REF_MEG_CH, false},
                {FIFFV_REF_MEG_CH, false},
                {FIFFV_REF_MEG_CH, false},
                {FIFFV_REF_MEG_CH, false},
                {FIFFV_REF_MEG_CH, false},
                {FIFFV_REF_MEG_CH, false},
                {FIFFV_REF_MEG_CH, false},
                {FIFFV_MEG_CH, false}}),
            kBlockSamples,
            settings});
    }

    for (const InvalidConfigureCase& testCase : cases) {
        AdaptiveDenoisingProcessor processor;

        const AdaptiveDenoisingConfigureResult readyResult =
            processor.configure(goodTwoChannelStream(), kBlockSamples, goodSettings());
        QVERIFY2(readyResult.status == AdaptiveDenoisingConfigureStatus::Ready, testCase.name);
        QCOMPARE(readyResult.referenceCount, Index(1));
        QCOMPARE(readyResult.targetCount, Index(1));
        QCOMPARE(readyResult.featureCount, Index(1));

        MatrixXd trainingBlock(2, kBlockSamples);
        for (Index column = 0; column < kBlockSamples; ++column) {
            trainingBlock(0, column) = static_cast<double>(column + 1);
            trainingBlock(1, column) = 2.0 * trainingBlock(0, column);
        }

        const DenoiserProcessResult trainingResult =
            processor.process(trainingBlock, DenoisingMode::ApplyAndLearn);
        QVERIFY2(trainingResult.status == DenoiserProcessStatus::Processed, testCase.name);
        QCOMPARE(trainingResult.diagnostics.modelGeneration, std::uint64_t(1));
        QCOMPARE(trainingResult.diagnostics.modelUpdatesAccepted, std::uint64_t(1));
        QCOMPARE(trainingResult.diagnostics.modelUpdatesRejected, std::uint64_t(0));

        const AdaptiveDenoisingConfigureResult invalidResult =
            processor.configure(testCase.stream, testCase.maxBlockSamples, testCase.settings);
        QVERIFY2(invalidResult.status == testCase.expectedStatus, testCase.name);
        QCOMPARE(invalidResult.referenceCount, Index(0));
        QCOMPARE(invalidResult.targetCount, Index(0));
        QCOMPARE(invalidResult.featureCount, Index(0));

        const AdaptiveDenoisingConfigureResult snapshot = processor.configuration();
        QVERIFY2(snapshot.status == testCase.expectedStatus, testCase.name);
        QCOMPARE(snapshot.referenceCount, Index(0));
        QCOMPARE(snapshot.targetCount, Index(0));
        QCOMPARE(snapshot.featureCount, Index(0));

        MatrixXd probe(2, kBlockSamples);
        for (Index column = 0; column < kBlockSamples; ++column) {
            probe(0, column) = 17.0;
            probe(1, column) = 34.0;
        }
        const MatrixXd expectedProbe = probe;

        const DenoiserProcessResult probeResult =
            processor.process(probe, DenoisingMode::ApplyOnly);
        QVERIFY2(probeResult.status == DenoiserProcessStatus::NotConfigured, testCase.name);
        QVERIFY2(matrixEquals(probe, expectedProbe), testCase.name);

        const DenoiserProcessDiagnostics& diagnostics = probeResult.diagnostics;
        QCOMPARE(diagnostics.referenceRowCount, Index(0));
        QCOMPARE(diagnostics.targetRowCount, Index(0));
        QCOMPARE(diagnostics.featureCount, Index(0));
        QCOMPARE(diagnostics.warmupSamplesRemaining, Index(0));
        QCOMPARE(diagnostics.modelGeneration, std::uint64_t(0));
        QCOMPARE(diagnostics.modelUpdatesAccepted, std::uint64_t(0));
        QCOMPARE(diagnostics.modelUpdatesRejected, std::uint64_t(0));
        QVERIFY(std::isnan(diagnostics.inputRms));
        QVERIFY(std::isnan(diagnostics.outputRms));
        QVERIFY(std::isnan(diagnostics.estimatedNoiseRms));
    }
}

//=============================================================================================================

void TestAdaptiveDenoisingPlugin::validReconfigureResetsAndRelearnsModel()
{
    AdaptiveDenoisingProcessor processor;

    const AdaptiveDenoisingConfigureResult initialResult =
        processor.configure(goodTwoChannelStream(), kBlockSamples, goodSettings());
    QVERIFY(initialResult.status == AdaptiveDenoisingConfigureStatus::Ready);
    QCOMPARE(initialResult.referenceCount, Index(1));
    QCOMPARE(initialResult.targetCount, Index(1));
    QCOMPARE(initialResult.featureCount, Index(1));

    MatrixXd initialTraining(2, kBlockSamples);
    for (Index column = 0; column < kBlockSamples; ++column) {
        initialTraining(0, column) = static_cast<double>(column + 1);
        initialTraining(1, column) = 2.0 * initialTraining(0, column);
    }

    const DenoiserProcessResult initialTrainingResult =
        processor.process(initialTraining, DenoisingMode::ApplyAndLearn);
    QVERIFY(initialTrainingResult.status == DenoiserProcessStatus::Processed);
    QCOMPARE(initialTrainingResult.diagnostics.modelGeneration, std::uint64_t(1));
    QCOMPARE(initialTrainingResult.diagnostics.modelUpdatesAccepted, std::uint64_t(1));
    QCOMPARE(initialTrainingResult.diagnostics.modelUpdatesRejected, std::uint64_t(0));

    const AdaptiveDenoisingStreamDescriptor reconfiguredStream = streamWithChannels({
        {FIFFV_MISC_CH, false},
        {FIFFV_REF_MEG_CH, false},
        {FIFFV_MEG_CH, false}});
    const AdaptiveDenoisingConfigureResult reconfigureResult =
        processor.configure(reconfiguredStream, kBlockSamples, goodSettings());

    QVERIFY(reconfigureResult.status == AdaptiveDenoisingConfigureStatus::Ready);
    QCOMPARE(reconfigureResult.referenceCount, Index(1));
    QCOMPARE(reconfigureResult.targetCount, Index(1));
    QCOMPARE(reconfigureResult.featureCount, Index(1));

    const AdaptiveDenoisingConfigureResult reconfiguredSnapshot = processor.configuration();
    QVERIFY(reconfiguredSnapshot.status == AdaptiveDenoisingConfigureStatus::Ready);
    QCOMPARE(reconfiguredSnapshot.referenceCount, Index(1));
    QCOMPARE(reconfiguredSnapshot.targetCount, Index(1));
    QCOMPARE(reconfiguredSnapshot.featureCount, Index(1));

    MatrixXd resetProbe(3, kBlockSamples);
    for (Index column = 0; column < kBlockSamples; ++column) {
        resetProbe(0, column) = 9001.0 + static_cast<double>(column);
        resetProbe(1, column) = 17.0;
        resetProbe(2, column) = 34.0;
    }
    const MatrixXd expectedResetProbe = resetProbe;

    const DenoiserProcessResult resetProbeResult =
        processor.process(resetProbe, DenoisingMode::ApplyOnly);
    QVERIFY(resetProbeResult.status == DenoiserProcessStatus::Processed);
    QCOMPARE(resetProbeResult.diagnostics.modelGeneration, std::uint64_t(0));
    QCOMPARE(resetProbeResult.diagnostics.modelUpdatesAccepted, std::uint64_t(0));
    QCOMPARE(resetProbeResult.diagnostics.modelUpdatesRejected, std::uint64_t(0));
    QVERIFY(matrixEquals(resetProbe, expectedResetProbe));

    MatrixXd reconfiguredTraining(3, kBlockSamples);
    for (Index column = 0; column < kBlockSamples; ++column) {
        reconfiguredTraining(0, column) = 10001.0 + static_cast<double>(column);
        reconfiguredTraining(1, column) = static_cast<double>(column + 1);
        reconfiguredTraining(2, column) = 3.0 * reconfiguredTraining(1, column);
    }
    const MatrixXd expectedReconfiguredTraining = reconfiguredTraining;

    const DenoiserProcessResult reconfiguredTrainingResult =
        processor.process(reconfiguredTraining, DenoisingMode::ApplyAndLearn);
    QVERIFY(reconfiguredTrainingResult.status == DenoiserProcessStatus::Processed);
    QCOMPARE(reconfiguredTrainingResult.diagnostics.modelGeneration, std::uint64_t(1));
    QCOMPARE(reconfiguredTrainingResult.diagnostics.modelUpdatesAccepted, std::uint64_t(1));
    QCOMPARE(reconfiguredTrainingResult.diagnostics.modelUpdatesRejected, std::uint64_t(0));
    QVERIFY(rowEquals(reconfiguredTraining, expectedReconfiguredTraining, 0));
    QVERIFY(rowEquals(reconfiguredTraining, expectedReconfiguredTraining, 1));

    MatrixXd futureProbe(3, kBlockSamples);
    for (Index column = 0; column < kBlockSamples; ++column) {
        futureProbe(0, column) = 11001.0 + static_cast<double>(column);
        futureProbe(1, column) = 17.0;
        futureProbe(2, column) = 51.0;
    }
    const MatrixXd expectedFutureProbe = futureProbe;

    const DenoiserProcessResult futureProbeResult =
        processor.process(futureProbe, DenoisingMode::ApplyOnly);
    QVERIFY(futureProbeResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(futureProbe.array().isFinite().all());
    QVERIFY(rowEquals(futureProbe, expectedFutureProbe, 0));
    QVERIFY(rowEquals(futureProbe, expectedFutureProbe, 1));

    for (Index column = 0; column < kBlockSamples; ++column) {
        QVERIFY(std::abs(futureProbe(2, column)) <= 1e-5);
    }
}

//=============================================================================================================

void TestAdaptiveDenoisingPlugin::queuePreservesFifoDropNewestAndMetadata()
{
    AdaptiveDenoisingBlockQueue queue;
    AdaptiveDenoisingBlockQueueConfig config;
    config.channelCount = 2;
    config.maxBlockSamples = 4;
    config.capacity = 2;

    QVERIFY(queue.configure(config) == AdaptiveDenoisingQueueConfigureStatus::Ready);

    MatrixXd blockA(2, 3);
    blockA(0, 0) = 1.0;
    blockA(0, 1) = 2.0;
    blockA(0, 2) = 3.0;
    blockA(1, 0) = 11.0;
    blockA(1, 1) = 12.0;
    blockA(1, 2) = 13.0;
    const MatrixXd originalA = blockA;

    MatrixXd blockB(2, 4);
    blockB(0, 0) = 21.0;
    blockB(0, 1) = 22.0;
    blockB(0, 2) = 23.0;
    blockB(0, 3) = 24.0;
    blockB(1, 0) = 31.0;
    blockB(1, 1) = 32.0;
    blockB(1, 2) = 33.0;
    blockB(1, 3) = 34.0;
    const MatrixXd originalB = blockB;

    MatrixXd blockC(2, 2);
    blockC(0, 0) = 41.0;
    blockC(0, 1) = 42.0;
    blockC(1, 0) = 51.0;
    blockC(1, 1) = 52.0;
    const MatrixXd originalC = blockC;

    const std::shared_ptr<int> ownerA = std::make_shared<int>(1);
    const std::shared_ptr<int> ownerB = std::make_shared<int>(2);
    const std::shared_ptr<int> ownerC = std::make_shared<int>(3);
    const std::shared_ptr<const FIFFLIB::FiffInfo> metadataA(ownerA, nullptr);
    const std::shared_ptr<const FIFFLIB::FiffInfo> metadataB(ownerB, nullptr);
    const std::shared_ptr<const FIFFLIB::FiffInfo> metadataC(ownerC, nullptr);

    QVERIFY(!sameOwner(metadataA, metadataB));
    QVERIFY(!sameOwner(metadataA, metadataC));
    QVERIFY(!sameOwner(metadataB, metadataC));

    QVERIFY(queue.tryPush(blockA, metadataA) == AdaptiveDenoisingQueuePushStatus::Pushed);
    QVERIFY(queue.tryPush(blockB, metadataB) == AdaptiveDenoisingQueuePushStatus::Pushed);

    blockA.array() += 1000.0;
    blockB.array() += 2000.0;

    QVERIFY(queue.tryPush(blockC, metadataC) == AdaptiveDenoisingQueuePushStatus::Full);

    AdaptiveDenoisingQueuedBlock destination;
    destination.data.resize(2, 4);

    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Popped);
    QCOMPARE(destination.sampleCount, Index(3));
    QVERIFY(sameOwner(destination.fiffInfo, metadataA));
    for (Index row = 0; row < originalA.rows(); ++row) {
        QVERIFY(rowEquals(destination.data, originalA, row));
    }

    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Popped);
    QCOMPARE(destination.sampleCount, Index(4));
    QVERIFY(sameOwner(destination.fiffInfo, metadataB));
    for (Index row = 0; row < originalB.rows(); ++row) {
        QVERIFY(rowEquals(destination.data, originalB, row));
    }

    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Timeout);

    QVERIFY(queue.tryPush(blockC, metadataC) == AdaptiveDenoisingQueuePushStatus::Pushed);
    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Popped);
    QCOMPARE(destination.sampleCount, Index(2));
    QVERIFY(sameOwner(destination.fiffInfo, metadataC));
    for (Index row = 0; row < originalC.rows(); ++row) {
        QVERIFY(rowEquals(destination.data, originalC, row));
    }
}

//=============================================================================================================

QTEST_APPLESS_MAIN(TestAdaptiveDenoisingPlugin)

#include "test_adaptive_denoising_plugin.moc"
