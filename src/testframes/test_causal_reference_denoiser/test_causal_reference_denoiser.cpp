//=============================================================================================================
/**
 * @file     test_causal_reference_denoiser.cpp
 * @brief    Public-interface tracer bullet for causal reference denoising.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <rtprocessing/causalreferencedenoiser.h>

#include <Eigen/Dense>

#include <cmath>
#include <limits>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace RTPROCESSINGLIB;

//=============================================================================================================

class TestCausalReferenceDenoiser : public QObject
{
    Q_OBJECT

private slots:
    void configureAndBypassTrackHistory();
    void rejectedConfigurationPreservesCommittedShape();
    void rejectsInvalidConfiguration_data();
    void rejectsInvalidConfiguration();
    void appliesAndLearnsCausallyAcrossEpochs();
};

//=============================================================================================================

void TestCausalReferenceDenoiser::configureAndBypassTrackHistory()
{
    CausalReferenceDenoiserConfig config;
    config.samplingFrequencyHz = 1000.0;
    config.channelCount = 3;
    config.maxBlockSamples = 4;
    config.referenceRows = VectorXi(1);
    config.referenceRows << 0;
    config.targetRows = VectorXi(2);
    config.targetRows << 1, 2;
    config.tapCount = 4;
    config.adaptationIntervalSamples = 128;
    config.memoryTimeSeconds = 30.0;
    config.regularization = 1e-3;

    CausalReferenceDenoiser denoiser;
    QVERIFY(denoiser.configure(config) == DenoiserStatus::Configured);

    MatrixXd block(3, 4);
    block << 10.0, 11.0, 12.0, 13.0,
              20.0, 21.0, 22.0, 23.0,
              30.0, 31.0, 32.0, 33.0;
    const MatrixXd expected = block;

    const DenoiserProcessResult result =
        denoiser.process(block, DenoisingMode::BypassTrackHistory);

    QVERIFY(result.status == DenoiserProcessStatus::Bypassed);
    QCOMPARE(block.rows(), expected.rows());
    QCOMPARE(block.cols(), expected.cols());
    QVERIFY((block.array() == expected.array()).all());
}

//=============================================================================================================

void TestCausalReferenceDenoiser::rejectedConfigurationPreservesCommittedShape()
{
    CausalReferenceDenoiserConfig config;
    config.samplingFrequencyHz = 1000.0;
    config.channelCount = 3;
    config.maxBlockSamples = 4;
    config.referenceRows = VectorXi(1);
    config.referenceRows << 0;
    config.targetRows = VectorXi(2);
    config.targetRows << 1, 2;
    config.tapCount = 4;
    config.adaptationIntervalSamples = 128;
    config.memoryTimeSeconds = 30.0;
    config.regularization = 1e-3;

    CausalReferenceDenoiser denoiser;
    QVERIFY(denoiser.configure(config) == DenoiserStatus::Configured);

    CausalReferenceDenoiserConfig rejected = config;
    rejected.channelCount = 4;
    rejected.maxBlockSamples = 0;
    QVERIFY(denoiser.configure(rejected) == DenoiserStatus::InvalidConfiguration);

    MatrixXd oversizedBlock(4, 4);
    oversizedBlock << 10.0, 11.0, 12.0, 13.0,
                      20.0, 21.0, 22.0, 23.0,
                      30.0, 31.0, 32.0, 33.0,
                      40.0, 41.0, 42.0, 43.0;
    const MatrixXd expectedOversizedBlock = oversizedBlock;

    const DenoiserProcessResult oversizedResult =
        denoiser.process(oversizedBlock, DenoisingMode::BypassTrackHistory);

    QVERIFY(oversizedResult.status == DenoiserProcessStatus::InvalidShape);
    QCOMPARE(oversizedBlock.rows(), expectedOversizedBlock.rows());
    QCOMPARE(oversizedBlock.cols(), expectedOversizedBlock.cols());
    QVERIFY((oversizedBlock.array() == expectedOversizedBlock.array()).all());

    MatrixXd committedBlock(3, 4);
    committedBlock << 50.0, 51.0, 52.0, 53.0,
                      60.0, 61.0, 62.0, 63.0,
                      70.0, 71.0, 72.0, 73.0;
    const MatrixXd expectedCommittedBlock = committedBlock;

    const DenoiserProcessResult committedResult =
        denoiser.process(committedBlock, DenoisingMode::BypassTrackHistory);

    QVERIFY(committedResult.status == DenoiserProcessStatus::Bypassed);
    QCOMPARE(committedBlock.rows(), expectedCommittedBlock.rows());
    QCOMPARE(committedBlock.cols(), expectedCommittedBlock.cols());
    QVERIFY((committedBlock.array() == expectedCommittedBlock.array()).all());
}

//=============================================================================================================

void TestCausalReferenceDenoiser::rejectsInvalidConfiguration_data()
{
    QTest::addColumn<QString>("mutation");

    QTest::newRow("samplingFrequencyHz_zero") << QStringLiteral("samplingFrequencyHz_zero");
    QTest::newRow("samplingFrequencyHz_negative") << QStringLiteral("samplingFrequencyHz_negative");
    QTest::newRow("samplingFrequencyHz_nan") << QStringLiteral("samplingFrequencyHz_nan");
    QTest::newRow("samplingFrequencyHz_positiveInfinity") << QStringLiteral("samplingFrequencyHz_positiveInfinity");
    QTest::newRow("tapCount_zero") << QStringLiteral("tapCount_zero");
    QTest::newRow("tapCount_negative") << QStringLiteral("tapCount_negative");
    QTest::newRow("adaptationIntervalSamples_zero") << QStringLiteral("adaptationIntervalSamples_zero");
    QTest::newRow("adaptationIntervalSamples_negative") << QStringLiteral("adaptationIntervalSamples_negative");
    QTest::newRow("memoryTimeSeconds_zero") << QStringLiteral("memoryTimeSeconds_zero");
    QTest::newRow("memoryTimeSeconds_negative") << QStringLiteral("memoryTimeSeconds_negative");
    QTest::newRow("memoryTimeSeconds_nan") << QStringLiteral("memoryTimeSeconds_nan");
    QTest::newRow("memoryTimeSeconds_positiveInfinity") << QStringLiteral("memoryTimeSeconds_positiveInfinity");
    QTest::newRow("regularization_belowMinimum") << QStringLiteral("regularization_belowMinimum");
    QTest::newRow("regularization_aboveMaximum") << QStringLiteral("regularization_aboveMaximum");
    QTest::newRow("regularization_nan") << QStringLiteral("regularization_nan");
    QTest::newRow("regularization_positiveInfinity") << QStringLiteral("regularization_positiveInfinity");
    QTest::newRow("referenceRows_empty") << QStringLiteral("referenceRows_empty");
    QTest::newRow("targetRows_empty") << QStringLiteral("targetRows_empty");
    QTest::newRow("referenceRows_duplicate") << QStringLiteral("referenceRows_duplicate");
    QTest::newRow("targetRows_duplicate") << QStringLiteral("targetRows_duplicate");
    QTest::newRow("referenceRows_negative") << QStringLiteral("referenceRows_negative");
    QTest::newRow("targetRows_negative") << QStringLiteral("targetRows_negative");
    QTest::newRow("referenceRows_equalChannelCount") << QStringLiteral("referenceRows_equalChannelCount");
    QTest::newRow("targetRows_equalChannelCount") << QStringLiteral("targetRows_equalChannelCount");
    QTest::newRow("referenceRows_targetRows_overlap") << QStringLiteral("referenceRows_targetRows_overlap");
    QTest::newRow("referenceRows_tapCount_featureLimit") << QStringLiteral("referenceRows_tapCount_featureLimit");
}

//=============================================================================================================

void TestCausalReferenceDenoiser::rejectsInvalidConfiguration()
{
    QFETCH(QString, mutation);

    CausalReferenceDenoiserConfig candidate;
    candidate.samplingFrequencyHz = 1000.0;
    candidate.channelCount = 3;
    candidate.maxBlockSamples = 4;
    candidate.referenceRows = VectorXi(1);
    candidate.referenceRows << 0;
    candidate.targetRows = VectorXi(2);
    candidate.targetRows << 1, 2;
    candidate.tapCount = 4;
    candidate.adaptationIntervalSamples = 128;
    candidate.memoryTimeSeconds = 30.0;
    candidate.regularization = 1e-3;

    const double quietNaN = std::numeric_limits<double>::quiet_NaN();
    const double positiveInfinity = std::numeric_limits<double>::infinity();

    if (mutation == QStringLiteral("samplingFrequencyHz_zero")) {
        candidate.samplingFrequencyHz = 0.0;
    } else if (mutation == QStringLiteral("samplingFrequencyHz_negative")) {
        candidate.samplingFrequencyHz = -1.0;
    } else if (mutation == QStringLiteral("samplingFrequencyHz_nan")) {
        candidate.samplingFrequencyHz = quietNaN;
    } else if (mutation == QStringLiteral("samplingFrequencyHz_positiveInfinity")) {
        candidate.samplingFrequencyHz = positiveInfinity;
    } else if (mutation == QStringLiteral("tapCount_zero")) {
        candidate.tapCount = 0;
    } else if (mutation == QStringLiteral("tapCount_negative")) {
        candidate.tapCount = -1;
    } else if (mutation == QStringLiteral("adaptationIntervalSamples_zero")) {
        candidate.adaptationIntervalSamples = 0;
    } else if (mutation == QStringLiteral("adaptationIntervalSamples_negative")) {
        candidate.adaptationIntervalSamples = -1;
    } else if (mutation == QStringLiteral("memoryTimeSeconds_zero")) {
        candidate.memoryTimeSeconds = 0.0;
    } else if (mutation == QStringLiteral("memoryTimeSeconds_negative")) {
        candidate.memoryTimeSeconds = -1.0;
    } else if (mutation == QStringLiteral("memoryTimeSeconds_nan")) {
        candidate.memoryTimeSeconds = quietNaN;
    } else if (mutation == QStringLiteral("memoryTimeSeconds_positiveInfinity")) {
        candidate.memoryTimeSeconds = positiveInfinity;
    } else if (mutation == QStringLiteral("regularization_belowMinimum")) {
        candidate.regularization = 0.0;
    } else if (mutation == QStringLiteral("regularization_aboveMaximum")) {
        candidate.regularization = 1.1;
    } else if (mutation == QStringLiteral("regularization_nan")) {
        candidate.regularization = quietNaN;
    } else if (mutation == QStringLiteral("regularization_positiveInfinity")) {
        candidate.regularization = positiveInfinity;
    } else if (mutation == QStringLiteral("referenceRows_empty")) {
        candidate.referenceRows = VectorXi(0);
    } else if (mutation == QStringLiteral("targetRows_empty")) {
        candidate.targetRows = VectorXi(0);
    } else if (mutation == QStringLiteral("referenceRows_duplicate")) {
        candidate.referenceRows = VectorXi(2);
        candidate.referenceRows << 0, 0;
    } else if (mutation == QStringLiteral("targetRows_duplicate")) {
        candidate.targetRows = VectorXi(3);
        candidate.targetRows << 1, 2, 2;
    } else if (mutation == QStringLiteral("referenceRows_negative")) {
        candidate.referenceRows = VectorXi(1);
        candidate.referenceRows << -1;
    } else if (mutation == QStringLiteral("targetRows_negative")) {
        candidate.targetRows = VectorXi(2);
        candidate.targetRows << -1, 2;
    } else if (mutation == QStringLiteral("referenceRows_equalChannelCount")) {
        candidate.referenceRows = VectorXi(1);
        candidate.referenceRows << candidate.channelCount;
    } else if (mutation == QStringLiteral("targetRows_equalChannelCount")) {
        candidate.targetRows = VectorXi(2);
        candidate.targetRows << 1, candidate.channelCount;
    } else if (mutation == QStringLiteral("referenceRows_targetRows_overlap")) {
        candidate.targetRows = VectorXi(2);
        candidate.targetRows << 0, 2;
    } else if (mutation == QStringLiteral("referenceRows_tapCount_featureLimit")) {
        candidate.tapCount = 257;
    } else {
        QFAIL("Unknown invalid configuration mutation");
    }

    CausalReferenceDenoiser denoiser;
    QVERIFY(denoiser.configure(candidate) == DenoiserStatus::InvalidConfiguration);
}

//=============================================================================================================

void TestCausalReferenceDenoiser::appliesAndLearnsCausallyAcrossEpochs()
{
    CausalReferenceDenoiserConfig config;
    config.samplingFrequencyHz = 1000.0;
    config.channelCount = 3;
    config.maxBlockSamples = 3;
    config.referenceRows = VectorXi(1);
    config.referenceRows << 0;
    config.targetRows = VectorXi(1);
    config.targetRows << 1;
    config.tapCount = 2;
    config.adaptationIntervalSamples = 2;
    config.memoryTimeSeconds = 300.0;
    config.regularization = 1e-8;

    CausalReferenceDenoiser denoiser;
    QVERIFY(denoiser.configure(config) == DenoiserStatus::Configured);

    MatrixXd firstBlock(3, 3);
    firstBlock << 1.0, 2.0, 3.0,
                  5.0, 7.0, 12.0,
                  100.0, 101.0, 102.0;
    const MatrixXd expectedFirstBlock = firstBlock;

    const DenoiserProcessResult firstResult =
        denoiser.process(firstBlock, DenoisingMode::ApplyAndLearn);

    QVERIFY(firstResult.status == DenoiserProcessStatus::Processed);
    QVERIFY((firstBlock.array() == expectedFirstBlock.array()).all());

    MatrixXd postBoundaryBlock(3, 1);
    postBoundaryBlock << 5.0,
                         19.0,
                         103.0;

    const DenoiserProcessResult postBoundaryResult =
        denoiser.process(postBoundaryBlock, DenoisingMode::ApplyOnly);

    QVERIFY(postBoundaryResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(postBoundaryBlock(0, 0) == 5.0);
    QVERIFY(postBoundaryBlock(2, 0) == 103.0);
    QVERIFY(std::abs(postBoundaryBlock(1, 0)) <= 1e-4);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestCausalReferenceDenoiser)
#include "test_causal_reference_denoiser.moc"
