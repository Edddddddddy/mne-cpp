//=============================================================================================================
/**
 * @file     test_adaptive_denoising_plugin.cpp
 * @brief    Public-interface tracer for adaptive denoising channel mapping.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <adaptivedenoising/adaptivedenoisingprocessor.h>

#include <fiff/fiff_constants.h>

#include <Eigen/Dense>

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <limits>
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

} // namespace

//=============================================================================================================

class TestAdaptiveDenoisingPlugin : public QObject
{
    Q_OBJECT

private slots:
    void mapsRowsTrainsAndAppliesOnly();
    void invalidConfigurationDisarmsLearnedModel();
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

QTEST_APPLESS_MAIN(TestAdaptiveDenoisingPlugin)

#include "test_adaptive_denoising_plugin.moc"
