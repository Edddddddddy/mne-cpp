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
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestCausalReferenceDenoiser)
#include "test_causal_reference_denoiser.moc"
