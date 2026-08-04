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
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestCausalReferenceDenoiser)
#include "test_causal_reference_denoiser.moc"
