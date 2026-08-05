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

bool rowEquals(const MatrixXd& actual, const MatrixXd& expected, Index row) noexcept
{
    for (Index column = 0; column < expected.cols(); ++column) {
        if (actual(row, column) != expected(row, column)) {
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

QTEST_APPLESS_MAIN(TestAdaptiveDenoisingPlugin)

#include "test_adaptive_denoising_plugin.moc"
