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

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace RTPROCESSINGLIB;

//=============================================================================================================

static_assert(
    std::is_nothrow_default_constructible<CausalReferenceDenoiser>::value,
    "CausalReferenceDenoiser must be nothrow default constructible");
static_assert(
    !std::is_copy_constructible<CausalReferenceDenoiser>::value,
    "CausalReferenceDenoiser must not be copy constructible");
static_assert(
    !std::is_copy_assignable<CausalReferenceDenoiser>::value,
    "CausalReferenceDenoiser must not be copy assignable");
static_assert(
    !std::is_move_constructible<CausalReferenceDenoiser>::value,
    "CausalReferenceDenoiser must not be move constructible");
static_assert(
    !std::is_move_assignable<CausalReferenceDenoiser>::value,
    "CausalReferenceDenoiser must not be move assignable");
static_assert(
    noexcept(std::declval<CausalReferenceDenoiser&>().reset()),
    "CausalReferenceDenoiser::reset must be noexcept");
using MutableBlockRef = Eigen::Ref<Eigen::MatrixXd>;
static_assert(
    std::is_convertible<Eigen::MatrixXd&, MutableBlockRef>::value,
    "MatrixXd lvalue must convert to Eigen::Ref<MatrixXd>");
// The public-header member declaration/body contract is explicitly noexcept;
// Eigen::Ref argument-wrapper construction is outside that body and is not
// asserted noexcept in C++14.
static_assert(
    std::is_same<
        decltype(std::declval<CausalReferenceDenoiser&>().process(
            std::declval<Eigen::MatrixXd&>(),
            DenoisingMode::BypassTrackHistory)),
        DenoiserProcessResult>::value,
    "process(MatrixXd lvalue, mode) must return DenoiserProcessResult");

namespace
{

bool matricesEqualIncludingNonFinite(const MatrixXd& actual,
                                     const MatrixXd& expected) noexcept
{
    if (actual.rows() != expected.rows() || actual.cols() != expected.cols()) {
        return false;
    }

    for (Index row = 0; row < actual.rows(); ++row) {
        for (Index column = 0; column < actual.cols(); ++column) {
            const double actualValue = actual(row, column);
            const double expectedValue = expected(row, column);

            if (std::isnan(expectedValue)) {
                if (!std::isnan(actualValue)) {
                    return false;
                }
            } else if (std::isinf(expectedValue)) {
                if (actualValue != expectedValue) {
                    return false;
                }
            } else if (actualValue != expectedValue) {
                return false;
            }
        }
    }

    return true;
}

} // namespace

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
    void appliesAnalyticForgettingAcrossEpochs();
    void rejectsSelectedNonFiniteAtomically_data();
    void rejectsSelectedNonFiniteAtomically();
    void preservesChunkBoundaryEquivalence();
    void resetRestoresFreshConfiguredState();
    void nonLearningModesPreserveModelAndAdvanceHistory_data();
    void nonLearningModesPreserveModelAndAdvanceHistory();
    void reportsDiagnosticsAcrossLifecycle();
    void rejectsPoisonedEpochsAndRecoversWithinBlock();
    void acceptsLoadedRankDeficientEpoch();
    void reportsStableRmsForLargeBypassAndAnalyticApplyOnly();
    void fallsBackAtomicallyWhenPredictionOverflows();
    void meetsQuantitativeSyntheticAcceptance();
    void processDoesNotAllocateAfterConfigure();
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

void TestCausalReferenceDenoiser::appliesAnalyticForgettingAcrossEpochs()
{
    CausalReferenceDenoiserConfig config;
    config.samplingFrequencyHz = 1.0;
    config.channelCount = 2;
    config.maxBlockSamples = 2;
    config.referenceRows = VectorXi(1);
    config.referenceRows << 0;
    config.targetRows = VectorXi(1);
    config.targetRows << 1;
    config.tapCount = 1;
    config.adaptationIntervalSamples = 2;
    config.memoryTimeSeconds = 1.0 / std::log(2.0);
    config.regularization = 1e-3;

    CausalReferenceDenoiser denoiser;
    QVERIFY(denoiser.configure(config) == DenoiserStatus::Configured);

    MatrixXd firstBlock(2, 2);
    firstBlock << 1.0, 1.0,
                  0.0, 1.0;
    const MatrixXd originalFirstBlock = firstBlock;

    const DenoiserProcessResult firstResult =
        denoiser.process(firstBlock, DenoisingMode::ApplyAndLearn);

    QVERIFY(firstResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(firstBlock.allFinite());
    QVERIFY((firstBlock.row(0).array() == originalFirstBlock.row(0).array()).all());
    QVERIFY(firstResult.diagnostics.modelUpdatesAccepted == 1);
    QVERIFY(firstResult.diagnostics.modelUpdatesRejected == 0);
    QVERIFY(firstResult.diagnostics.modelGeneration == 1);

    MatrixXd secondBlock(2, 2);
    secondBlock << 1.0, 1.0,
                   0.0, 0.0;
    const MatrixXd originalSecondBlock = secondBlock;

    const DenoiserProcessResult secondResult =
        denoiser.process(secondBlock, DenoisingMode::ApplyAndLearn);

    QVERIFY(secondResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(secondBlock.allFinite());
    QVERIFY((secondBlock.row(0).array() == originalSecondBlock.row(0).array()).all());
    QVERIFY(secondResult.diagnostics.modelUpdatesAccepted == 1);
    QVERIFY(secondResult.diagnostics.modelUpdatesRejected == 0);
    QVERIFY(secondResult.diagnostics.modelGeneration == 2);

    // Independently, lambda=0.5. Epoch one accumulates
    // G=0.5*1+1=1.5 and H=0.5*0+1=1. Epoch two has pending
    // G=0.5*1+1=1.5 and H=0.5*0+0=0, then composes the committed
    // statistics aged by lambda^2=0.25: G=1.5*0.25+1.5=1.875 and
    // H=1*0.25+0=0.25. Scalar relative loading gives G*(1+regularization).
    const double expectedGram = 1.875;
    const double expectedCross = 0.25;
    const double expectedWeight = expectedCross
                                  / (expectedGram * (1.0 + config.regularization));
    const double expectedTarget = -expectedWeight;
    const double tolerance = 1e-12;

    // Sensitivity: lambda=1 gives W=1/(4*(1+regularization)); omitting
    // within-epoch decay gives W=0.25/(2.5*(1+regularization)); omitting
    // committed-stat aging gives W=1/(3*(1+regularization)). Each is
    // materially different from 0.25/(1.875*(1+regularization)).
    MatrixXd probeBlock(2, 1);
    probeBlock << 1.0,
                  0.0;
    const MatrixXd originalProbeBlock = probeBlock;

    const DenoiserProcessResult probeResult =
        denoiser.process(probeBlock, DenoisingMode::ApplyOnly);

    qInfo() << "analytic forgetting expected target" << expectedTarget
            << "observed target" << probeBlock(1, 0)
            << "expected weight" << expectedWeight;
    QVERIFY(probeResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(probeBlock.allFinite());
    QVERIFY(probeBlock(0, 0) == originalProbeBlock(0, 0));
    QVERIFY(probeResult.diagnostics.modelUpdatesAccepted == 0);
    QVERIFY(probeResult.diagnostics.modelUpdatesRejected == 0);
    QVERIFY(probeResult.diagnostics.modelGeneration == 2);
    QVERIFY(std::abs(probeBlock(1, 0) - expectedTarget) <= tolerance);
}

//=============================================================================================================

void TestCausalReferenceDenoiser::rejectsSelectedNonFiniteAtomically_data()
{
    QTest::addColumn<QString>("mutation");

    QTest::newRow("selected_reference_nan") << QStringLiteral("selected_reference_nan");
    QTest::newRow("selected_target_positive_infinity")
        << QStringLiteral("selected_target_positive_infinity");
}

//=============================================================================================================

void TestCausalReferenceDenoiser::rejectsSelectedNonFiniteAtomically()
{
    QFETCH(QString, mutation);

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

    CausalReferenceDenoiser control;
    CausalReferenceDenoiser subject;
    QVERIFY(control.configure(config) == DenoiserStatus::Configured);
    QVERIFY(subject.configure(config) == DenoiserStatus::Configured);

    MatrixXd controlPrime(3, 2);
    controlPrime << 1.0, 2.0,
                    5.0, 7.0,
                    100.0, 101.0;
    MatrixXd subjectPrime = controlPrime;

    const DenoiserProcessResult controlPrimeResult =
        control.process(controlPrime, DenoisingMode::ApplyAndLearn);
    const DenoiserProcessResult subjectPrimeResult =
        subject.process(subjectPrime, DenoisingMode::ApplyAndLearn);

    QVERIFY(controlPrimeResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(subjectPrimeResult.status == DenoiserProcessStatus::Processed);
    QVERIFY((controlPrime.array() == subjectPrime.array()).all());
    QVERIFY(controlPrime(0, 0) == 1.0);
    QVERIFY(controlPrime(0, 1) == 2.0);
    QVERIFY(controlPrime(2, 0) == 100.0);
    QVERIFY(controlPrime(2, 1) == 101.0);

    MatrixXd badBlock(3, 1);
    badBlock << 3.0,
                12.0,
                102.0;
    if (mutation == QStringLiteral("selected_reference_nan")) {
        badBlock(0, 0) = std::numeric_limits<double>::quiet_NaN();
    } else if (mutation == QStringLiteral("selected_target_positive_infinity")) {
        badBlock(1, 0) = std::numeric_limits<double>::infinity();
    } else {
        QFAIL("Unknown selected non-finite mutation");
    }
    const MatrixXd expectedBadBlock = badBlock;

    const DenoiserProcessResult badResult =
        subject.process(badBlock, DenoisingMode::ApplyAndLearn);

    QVERIFY(badResult.status == DenoiserProcessStatus::NonFiniteInput);
    QVERIFY(matricesEqualIncludingNonFinite(badBlock, expectedBadBlock));

    MatrixXd controlContinuation(3, 1);
    controlContinuation << 3.0,
                           12.0,
                           102.0;
    MatrixXd subjectContinuation = controlContinuation;

    const DenoiserProcessResult controlContinuationResult =
        control.process(controlContinuation, DenoisingMode::ApplyAndLearn);
    const DenoiserProcessResult subjectContinuationResult =
        subject.process(subjectContinuation, DenoisingMode::ApplyAndLearn);

    QVERIFY(controlContinuationResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(subjectContinuationResult.status == DenoiserProcessStatus::Processed);
    QVERIFY((controlContinuation - subjectContinuation).array().abs().maxCoeff() <= 1e-12);
    QVERIFY(controlContinuation(0, 0) == 3.0);
    QVERIFY(subjectContinuation(0, 0) == 3.0);
    QVERIFY(controlContinuation(2, 0) == 102.0);
    QVERIFY(subjectContinuation(2, 0) == 102.0);

    MatrixXd controlProbe(3, 1);
    controlProbe << 5.0,
                    19.0,
                    103.0;
    MatrixXd subjectProbe = controlProbe;

    const DenoiserProcessResult controlProbeResult =
        control.process(controlProbe, DenoisingMode::ApplyOnly);
    const DenoiserProcessResult subjectProbeResult =
        subject.process(subjectProbe, DenoisingMode::ApplyOnly);

    QVERIFY(controlProbeResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(subjectProbeResult.status == DenoiserProcessStatus::Processed);
    QVERIFY((controlProbe - subjectProbe).array().abs().maxCoeff() <= 1e-12);
    QVERIFY(controlProbe(0, 0) == 5.0);
    QVERIFY(subjectProbe(0, 0) == 5.0);
    QVERIFY(controlProbe(2, 0) == 103.0);
    QVERIFY(subjectProbe(2, 0) == 103.0);
    QVERIFY(std::abs(controlProbe(1, 0)) <= 1e-4);
    QVERIFY(std::abs(subjectProbe(1, 0)) <= 1e-4);
}

//=============================================================================================================

void TestCausalReferenceDenoiser::preservesChunkBoundaryEquivalence()
{
    CausalReferenceDenoiserConfig config;
    config.samplingFrequencyHz = 1000.0;
    config.channelCount = 5;
    config.maxBlockSamples = 32;
    config.referenceRows = VectorXi(2);
    config.referenceRows << 0, 1;
    config.targetRows = VectorXi(2);
    config.targetRows << 2, 3;
    config.tapCount = 3;
    config.adaptationIntervalSamples = 4;
    config.memoryTimeSeconds = 30.0;
    config.regularization = 1e-6;

    CausalReferenceDenoiser completeDenoiser;
    CausalReferenceDenoiser chunkedDenoiser;
    QVERIFY(completeDenoiser.configure(config) == DenoiserStatus::Configured);
    QVERIFY(chunkedDenoiser.configure(config) == DenoiserStatus::Configured);

    constexpr Eigen::Index streamLength = 21;
    constexpr Eigen::Index probeLength = 5;
    const auto reference0 = [](int sample) {
        const double t = static_cast<double>(sample);
        return 0.75 * std::sin(0.29 * t) + 0.20 * std::cos(0.07 * t);
    };
    const auto reference1 = [](int sample) {
        const double t = static_cast<double>(sample);
        return 0.55 * std::cos(0.23 * t) - 0.30 * std::sin(0.13 * t);
    };
    const auto clean2 = [](int sample) {
        const double t = static_cast<double>(sample);
        return 0.25 * std::sin(0.17 * t) + 0.05 * std::cos(0.41 * t);
    };
    const auto clean3 = [](int sample) {
        const double t = static_cast<double>(sample);
        return -0.18 * std::cos(0.19 * t) + 0.07 * std::sin(0.37 * t);
    };
    const auto makeStream = [&](int firstSample, Eigen::Index sampleCount) {
        MatrixXd stream(5, sampleCount);
        for (Eigen::Index column = 0; column < sampleCount; ++column) {
            const int sample = firstSample + static_cast<int>(column);
            const double currentReference0 = reference0(sample);
            const double currentReference1 = reference1(sample);
            const double laggedReference0 = reference0(sample - 1);
            const double laggedReference1 = reference1(sample - 1);
            const double twiceLaggedReference0 = reference0(sample - 2);
            const double twiceLaggedReference1 = reference1(sample - 2);

            stream(0, column) = currentReference0;
            stream(1, column) = currentReference1;
            stream(2, column) = 1.7 * currentReference0 - 0.8 * currentReference1
                                + 0.6 * laggedReference0 + 1.1 * laggedReference1
                                - 0.35 * twiceLaggedReference0 + 0.45 * twiceLaggedReference1
                                + clean2(sample);
            stream(3, column) = -0.9 * currentReference0 + 1.3 * currentReference1
                                + 0.85 * laggedReference0 - 0.55 * laggedReference1
                                + 0.25 * twiceLaggedReference0 + 0.70 * twiceLaggedReference1
                                + clean3(sample);
            stream(4, column) = 100.0 + 0.5 * static_cast<double>(sample)
                                + 0.1 * std::cos(0.20 * static_cast<double>(sample));
        }
        return stream;
    };

    const MatrixXd originalStream = makeStream(0, streamLength);
    MatrixXd completeOutput = originalStream;
    const DenoiserProcessResult completeResult =
        completeDenoiser.process(completeOutput, DenoisingMode::ApplyAndLearn);
    QVERIFY(completeResult.status == DenoiserProcessStatus::Processed);

    MatrixXd chunkedOutput(5, streamLength);
    const Eigen::Index chunkSizes[] = {1, 3, 2, 5, 4, 6};
    Eigen::Index chunkStart = 0;
    for (const Eigen::Index chunkSize : chunkSizes) {
        MatrixXd chunk = originalStream.middleCols(chunkStart, chunkSize);
        const DenoiserProcessResult chunkResult =
            chunkedDenoiser.process(chunk, DenoisingMode::ApplyAndLearn);
        QVERIFY(chunkResult.status == DenoiserProcessStatus::Processed);
        chunkedOutput.middleCols(chunkStart, chunkSize) = chunk;
        chunkStart += chunkSize;
    }
    QCOMPARE(chunkStart, streamLength);

    const auto relativeDifference = [](const MatrixXd& first, const MatrixXd& second) {
        const double denominator = std::max(first.norm(), 1.0);
        return (first - second).norm() / denominator;
    };

    const double streamRelativeDifference =
        relativeDifference(completeOutput, chunkedOutput);
    qInfo() << "stream relative difference" << streamRelativeDifference;
    QVERIFY(streamRelativeDifference <= 1e-10);

    const Eigen::Index exactRows[] = {0, 1, 4};
    for (const Eigen::Index row : exactRows) {
        for (Eigen::Index column = 0; column < streamLength; ++column) {
            QVERIFY(completeOutput(row, column) == originalStream(row, column));
            QVERIFY(chunkedOutput(row, column) == originalStream(row, column));
        }
    }

    const MatrixXd originalProbe = makeStream(static_cast<int>(streamLength), probeLength);
    MatrixXd completeProbe = originalProbe;
    MatrixXd chunkedProbe = originalProbe;
    const DenoiserProcessResult completeProbeResult =
        completeDenoiser.process(completeProbe, DenoisingMode::ApplyOnly);
    const DenoiserProcessResult chunkedProbeResult =
        chunkedDenoiser.process(chunkedProbe, DenoisingMode::ApplyOnly);
    QVERIFY(completeProbeResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(chunkedProbeResult.status == DenoiserProcessStatus::Processed);

    const double probeRelativeDifference =
        relativeDifference(completeProbe, chunkedProbe);
    qInfo() << "probe relative difference" << probeRelativeDifference;
    QVERIFY(probeRelativeDifference <= 1e-10);
    for (const Eigen::Index row : exactRows) {
        for (Eigen::Index column = 0; column < probeLength; ++column) {
            QVERIFY(completeProbe(row, column) == originalProbe(row, column));
            QVERIFY(chunkedProbe(row, column) == originalProbe(row, column));
        }
    }
}

//=============================================================================================================

void TestCausalReferenceDenoiser::resetRestoresFreshConfiguredState()
{
    CausalReferenceDenoiserConfig config;
    config.samplingFrequencyHz = 1000.0;
    config.channelCount = 4;
    config.maxBlockSamples = 6;
    config.referenceRows = VectorXi(1);
    config.referenceRows << 0;
    config.targetRows = VectorXi(1);
    config.targetRows << 1;
    config.tapCount = 3;
    config.adaptationIntervalSamples = 3;
    config.memoryTimeSeconds = 300.0;
    config.regularization = 1e-8;

    CausalReferenceDenoiser subject;
    QVERIFY(subject.configure(config) == DenoiserStatus::Configured);

    // The first two samples warm causal history. The next three independent
    // features commit weights [2, -3, 4], and the final sample both observes
    // that model and starts the next adaptation epoch.
    MatrixXd subjectPrime(4, 6);
    subjectPrime << 1.0, 0.0, 0.0, 1.0, 0.0, 2.0,
                    10.0, -5.0, 4.0, 2.0, -3.0, 8.0,
                    100.0, 101.0, 102.0, 103.0, 104.0, 105.0,
                    -50.0, -49.0, -48.0, -47.0, -46.0, -45.0;
    const MatrixXd originalSubjectPrime = subjectPrime;

    const DenoiserProcessResult primeResult =
        subject.process(subjectPrime, DenoisingMode::ApplyAndLearn);

    QVERIFY(primeResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(std::abs(subjectPrime(1, 5)) <= 1e-5);
    const Eigen::Index exactRows[] = {0, 2, 3};
    for (const Eigen::Index row : exactRows) {
        for (Eigen::Index column = 0; column < subjectPrime.cols(); ++column) {
            QVERIFY(subjectPrime(row, column) == originalSubjectPrime(row, column));
        }
    }

    subject.reset();

    CausalReferenceDenoiser control;
    QVERIFY(control.configure(config) == DenoiserStatus::Configured);

    // Train a deliberately different model after reset. A retained model or
    // history changes this output immediately; retained statistics or a
    // partial-epoch count changes the model observed by the later probe.
    MatrixXd originalTraining(4, 5);
    originalTraining << 1.0, 0.0, 0.0, 1.0, 0.0,
                        7.0, -6.0, -2.0, -4.0, 1.5,
                        200.0, 201.0, 202.0, 203.0, 204.0,
                        -100.0, -99.0, -98.0, -97.0, -96.0;
    MatrixXd subjectTraining = originalTraining;
    MatrixXd controlTraining = originalTraining;

    const DenoiserProcessResult subjectTrainingResult =
        subject.process(subjectTraining, DenoisingMode::ApplyAndLearn);
    const DenoiserProcessResult controlTrainingResult =
        control.process(controlTraining, DenoisingMode::ApplyAndLearn);

    QVERIFY(subjectTrainingResult.status == controlTrainingResult.status);
    QVERIFY(controlTrainingResult.status == DenoiserProcessStatus::Processed);

    const auto relativeDifference = [](const MatrixXd& first, const MatrixXd& second) {
        const double denominator = std::max(second.norm(), 1.0);
        return (first - second).norm() / denominator;
    };

    const double trainingRelativeDifference =
        relativeDifference(subjectTraining, controlTraining);
    qInfo() << "reset training relative difference" << trainingRelativeDifference;
    QVERIFY(trainingRelativeDifference <= 1e-12);
    for (const Eigen::Index row : exactRows) {
        for (Eigen::Index column = 0; column < originalTraining.cols(); ++column) {
            QVERIFY(subjectTraining(row, column) == originalTraining(row, column));
            QVERIFY(controlTraining(row, column) == originalTraining(row, column));
        }
    }

    MatrixXd originalProbe(4, 2);
    originalProbe << 2.0, -1.0,
                     -10.0, 7.0,
                     205.0, 206.0,
                     -95.0, -94.0;
    MatrixXd subjectProbe = originalProbe;
    MatrixXd controlProbe = originalProbe;

    const DenoiserProcessResult subjectProbeResult =
        subject.process(subjectProbe, DenoisingMode::ApplyOnly);
    const DenoiserProcessResult controlProbeResult =
        control.process(controlProbe, DenoisingMode::ApplyOnly);

    QVERIFY(subjectProbeResult.status == controlProbeResult.status);
    QVERIFY(controlProbeResult.status == DenoiserProcessStatus::Processed);
    const double probeRelativeDifference = relativeDifference(subjectProbe, controlProbe);
    qInfo() << "reset probe relative difference" << probeRelativeDifference;
    QVERIFY(probeRelativeDifference <= 1e-12);
    QVERIFY(controlProbe.row(1).norm() <= 1e-5);
    for (const Eigen::Index row : exactRows) {
        for (Eigen::Index column = 0; column < originalProbe.cols(); ++column) {
            QVERIFY(subjectProbe(row, column) == originalProbe(row, column));
            QVERIFY(controlProbe(row, column) == originalProbe(row, column));
        }
    }
}

//=============================================================================================================

void TestCausalReferenceDenoiser::nonLearningModesPreserveModelAndAdvanceHistory_data()
{
    QTest::addColumn<bool>("applyModel");

    QTest::newRow("ApplyOnly") << true;
    QTest::newRow("BypassTrackHistory") << false;
}

//=============================================================================================================

void TestCausalReferenceDenoiser::nonLearningModesPreserveModelAndAdvanceHistory()
{
    QFETCH(bool, applyModel);

    constexpr double currentWeight = 2.0;
    constexpr double lagWeight = 3.0;
    constexpr double residualTolerance = 1e-5;

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

    // After the zero-reference warmup, the independent features [1, 0] and
    // [0, 1] identify the analytic target model 2 * r(t) + 3 * r(t - 1).
    MatrixXd trainingBlock(3, 3);
    trainingBlock << 0.0, 1.0, 0.0,
                     0.0, currentWeight * 1.0 + lagWeight * 0.0,
                     currentWeight * 0.0 + lagWeight * 1.0,
                     100.0, 101.0, 102.0;
    const MatrixXd originalTrainingBlock = trainingBlock;

    const DenoiserProcessResult trainingResult =
        denoiser.process(trainingBlock, DenoisingMode::ApplyAndLearn);

    QVERIFY(trainingResult.status == DenoiserProcessStatus::Processed);
    QVERIFY((trainingBlock.array() == originalTrainingBlock.array()).all());

    // These incompatible raw targets would replace the committed model if a
    // nominally non-learning mode accumulated one full adaptation interval.
    MatrixXd adversarialBlock(3, 2);
    adversarialBlock << 10.0, -4.0,
                        500.0, -400.0,
                        200.0, 201.0;
    const MatrixXd originalAdversarialBlock = adversarialBlock;
    const Eigen::Index exactRows[] = {0, 2};
    const DenoisingMode mode = applyModel
                                   ? DenoisingMode::ApplyOnly
                                   : DenoisingMode::BypassTrackHistory;

    const DenoiserProcessResult adversarialResult = denoiser.process(adversarialBlock, mode);

    if (applyModel) {
        QVERIFY(adversarialResult.status == DenoiserProcessStatus::Processed);
        const double expectedFirstResidual =
            500.0 - (currentWeight * 10.0 + lagWeight * 0.0);
        const double expectedSecondResidual =
            -400.0 - (currentWeight * -4.0 + lagWeight * 10.0);
        QVERIFY(std::abs(adversarialBlock(1, 0) - expectedFirstResidual)
                <= residualTolerance);
        QVERIFY(std::abs(adversarialBlock(1, 1) - expectedSecondResidual)
                <= residualTolerance);
        for (const Eigen::Index row : exactRows) {
            for (Eigen::Index column = 0; column < adversarialBlock.cols(); ++column) {
                QVERIFY(adversarialBlock(row, column)
                        == originalAdversarialBlock(row, column));
            }
        }
    } else {
        QVERIFY(adversarialResult.status == DenoiserProcessStatus::Bypassed);
        QVERIFY((adversarialBlock.array() == originalAdversarialBlock.array()).all());
    }

    // The first feature [7, -4] depends on the final adversarial reference.
    // Both targets are generated solely from the original analytic model.
    MatrixXd probeBlock(3, 2);
    probeBlock << 7.0, -6.0,
                  currentWeight * 7.0 + lagWeight * -4.0,
                  currentWeight * -6.0 + lagWeight * 7.0,
                  300.0, 301.0;
    const MatrixXd originalProbeBlock = probeBlock;

    const DenoiserProcessResult probeResult =
        denoiser.process(probeBlock, DenoisingMode::ApplyOnly);

    QVERIFY(probeResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(std::abs(probeBlock(1, 0)) <= residualTolerance);
    QVERIFY(std::abs(probeBlock(1, 1)) <= residualTolerance);
    for (const Eigen::Index row : exactRows) {
        for (Eigen::Index column = 0; column < probeBlock.cols(); ++column) {
            QVERIFY(probeBlock(row, column) == originalProbeBlock(row, column));
        }
    }
}

//=============================================================================================================

void TestCausalReferenceDenoiser::reportsDiagnosticsAcrossLifecycle()
{
    CausalReferenceDenoiser denoiser;

    MatrixXd unconfiguredBlock(3, 2);
    unconfiguredBlock << 1.0, 2.0,
                         3.0, 4.0,
                         5.0, 6.0;
    const MatrixXd originalUnconfiguredBlock = unconfiguredBlock;

    const DenoiserProcessResult unconfiguredResult =
        denoiser.process(unconfiguredBlock, DenoisingMode::ApplyAndLearn);
    const DenoiserProcessDiagnostics& unconfiguredDiagnostics =
        unconfiguredResult.diagnostics;

    QVERIFY(unconfiguredResult.status == DenoiserProcessStatus::NotConfigured);
    QVERIFY((unconfiguredBlock.array() == originalUnconfiguredBlock.array()).all());
    QVERIFY(unconfiguredDiagnostics.referenceRowCount == 0);
    QVERIFY(unconfiguredDiagnostics.targetRowCount == 0);
    QVERIFY(unconfiguredDiagnostics.featureCount == 0);
    QVERIFY(unconfiguredDiagnostics.warmupSamplesRemaining == 0);
    QVERIFY(unconfiguredDiagnostics.modelGeneration == 0);
    QVERIFY(unconfiguredDiagnostics.modelUpdatesAccepted == 0);
    QVERIFY(unconfiguredDiagnostics.modelUpdatesRejected == 0);
    QVERIFY(std::isnan(unconfiguredDiagnostics.inputRms));
    QVERIFY(std::isnan(unconfiguredDiagnostics.outputRms));
    QVERIFY(std::isnan(unconfiguredDiagnostics.estimatedNoiseRms));

    CausalReferenceDenoiserConfig config;
    config.samplingFrequencyHz = 1000.0;
    config.channelCount = 3;
    config.maxBlockSamples = 2;
    config.referenceRows = VectorXi(1);
    config.referenceRows << 0;
    config.targetRows = VectorXi(1);
    config.targetRows << 1;
    config.tapCount = 3;
    config.adaptationIntervalSamples = 2;
    config.memoryTimeSeconds = 30.0;
    config.regularization = 1e-3;
    QVERIFY(denoiser.configure(config) == DenoiserStatus::Configured);

    MatrixXd bypassWarmupBlock(3, 1);
    bypassWarmupBlock << 1.0,
                         3.0,
                         5.0;
    const MatrixXd originalBypassWarmupBlock = bypassWarmupBlock;
    const DenoiserProcessResult bypassWarmupResult =
        denoiser.process(bypassWarmupBlock, DenoisingMode::BypassTrackHistory);

    QVERIFY(bypassWarmupResult.status == DenoiserProcessStatus::Bypassed);
    QVERIFY((bypassWarmupBlock.array() == originalBypassWarmupBlock.array()).all());
    QVERIFY(bypassWarmupResult.diagnostics.referenceRowCount == 1);
    QVERIFY(bypassWarmupResult.diagnostics.targetRowCount == 1);
    QVERIFY(bypassWarmupResult.diagnostics.featureCount == 3);
    QVERIFY(bypassWarmupResult.diagnostics.warmupSamplesRemaining == 1);
    QVERIFY(bypassWarmupResult.diagnostics.modelGeneration == 0);
    QVERIFY(bypassWarmupResult.diagnostics.modelUpdatesAccepted == 0);
    QVERIFY(bypassWarmupResult.diagnostics.modelUpdatesRejected == 0);
    QVERIFY(bypassWarmupResult.diagnostics.inputRms == 3.0);
    QVERIFY(bypassWarmupResult.diagnostics.outputRms == 3.0);
    QVERIFY(bypassWarmupResult.diagnostics.estimatedNoiseRms == 0.0);

    MatrixXd applyOnlyWarmupBlock(3, 1);
    applyOnlyWarmupBlock << 2.0,
                           -4.0,
                           6.0;
    const MatrixXd originalApplyOnlyWarmupBlock = applyOnlyWarmupBlock;
    const DenoiserProcessResult applyOnlyWarmupResult =
        denoiser.process(applyOnlyWarmupBlock, DenoisingMode::ApplyOnly);

    QVERIFY(applyOnlyWarmupResult.status == DenoiserProcessStatus::Processed);
    QVERIFY((applyOnlyWarmupBlock.array() == originalApplyOnlyWarmupBlock.array()).all());
    QVERIFY(applyOnlyWarmupResult.diagnostics.referenceRowCount == 1);
    QVERIFY(applyOnlyWarmupResult.diagnostics.targetRowCount == 1);
    QVERIFY(applyOnlyWarmupResult.diagnostics.featureCount == 3);
    QVERIFY(applyOnlyWarmupResult.diagnostics.warmupSamplesRemaining == 0);
    QVERIFY(applyOnlyWarmupResult.diagnostics.modelGeneration == 0);
    QVERIFY(applyOnlyWarmupResult.diagnostics.modelUpdatesAccepted == 0);
    QVERIFY(applyOnlyWarmupResult.diagnostics.modelUpdatesRejected == 0);
    QVERIFY(applyOnlyWarmupResult.diagnostics.inputRms == 4.0);
    QVERIFY(applyOnlyWarmupResult.diagnostics.outputRms == 4.0);
    QVERIFY(applyOnlyWarmupResult.diagnostics.estimatedNoiseRms == 0.0);

    MatrixXd learningBlock(3, 2);
    learningBlock << 3.0, 4.0,
                     5.0, -12.0,
                     7.0, 8.0;
    const MatrixXd originalLearningBlock = learningBlock;
    const DenoiserProcessResult learningResult =
        denoiser.process(learningBlock, DenoisingMode::ApplyAndLearn);
    const double expectedLearningRms = std::sqrt(84.5);

    QVERIFY(learningResult.status == DenoiserProcessStatus::Processed);
    QVERIFY((learningBlock.array() == originalLearningBlock.array()).all());
    QVERIFY(learningResult.diagnostics.referenceRowCount == 1);
    QVERIFY(learningResult.diagnostics.targetRowCount == 1);
    QVERIFY(learningResult.diagnostics.featureCount == 3);
    QVERIFY(learningResult.diagnostics.warmupSamplesRemaining == 0);
    QVERIFY(learningResult.diagnostics.modelGeneration == 1);
    QVERIFY(learningResult.diagnostics.modelUpdatesAccepted == 1);
    QVERIFY(learningResult.diagnostics.modelUpdatesRejected == 0);
    QVERIFY(std::abs(learningResult.diagnostics.inputRms - expectedLearningRms) <= 1e-12);
    QVERIFY(std::abs(learningResult.diagnostics.outputRms - expectedLearningRms) <= 1e-12);
    QVERIFY(learningResult.diagnostics.estimatedNoiseRms == 0.0);

    denoiser.reset();

    MatrixXd resetBypassBlock(3, 1);
    resetBypassBlock << -2.0,
                         9.0,
                        10.0;
    const MatrixXd originalResetBypassBlock = resetBypassBlock;
    const DenoiserProcessResult resetBypassResult =
        denoiser.process(resetBypassBlock, DenoisingMode::BypassTrackHistory);

    QVERIFY(resetBypassResult.status == DenoiserProcessStatus::Bypassed);
    QVERIFY((resetBypassBlock.array() == originalResetBypassBlock.array()).all());
    QVERIFY(resetBypassResult.diagnostics.referenceRowCount == 1);
    QVERIFY(resetBypassResult.diagnostics.targetRowCount == 1);
    QVERIFY(resetBypassResult.diagnostics.featureCount == 3);
    QVERIFY(resetBypassResult.diagnostics.warmupSamplesRemaining == 1);
    QVERIFY(resetBypassResult.diagnostics.modelGeneration == 0);
    QVERIFY(resetBypassResult.diagnostics.modelUpdatesAccepted == 0);
    QVERIFY(resetBypassResult.diagnostics.modelUpdatesRejected == 0);
    QVERIFY(resetBypassResult.diagnostics.inputRms == 9.0);
    QVERIFY(resetBypassResult.diagnostics.outputRms == 9.0);
    QVERIFY(resetBypassResult.diagnostics.estimatedNoiseRms == 0.0);
}

//=============================================================================================================

void TestCausalReferenceDenoiser::rejectsPoisonedEpochsAndRecoversWithinBlock()
{
    CausalReferenceDenoiserConfig config;
    config.samplingFrequencyHz = 1000.0;
    config.channelCount = 2;
    config.maxBlockSamples = 8;
    config.referenceRows = VectorXi(1);
    config.referenceRows << 0;
    config.targetRows = VectorXi(1);
    config.targetRows << 1;
    config.tapCount = 1;
    config.adaptationIntervalSamples = 2;
    config.memoryTimeSeconds = 300.0;
    config.regularization = 1e-8;

    CausalReferenceDenoiser denoiser;
    QVERIFY(denoiser.configure(config) == DenoiserStatus::Configured);

    const double extremeReference = std::numeric_limits<double>::max() / 2.0;
    MatrixXd learningBlock(2, 8);
    learningBlock << 0.0, 0.0,
                     extremeReference, extremeReference,
                     extremeReference, extremeReference,
                     1.0, 2.0,
                     0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2.0, 4.0;
    const RowVectorXd originalReferences = learningBlock.row(0);
    const RowVectorXd originalTargets = learningBlock.row(1);

    const DenoiserProcessResult learningResult =
        denoiser.process(learningBlock, DenoisingMode::ApplyAndLearn);
    const double expectedLearningRms = std::sqrt(2.5);

    QVERIFY(learningResult.status == DenoiserProcessStatus::Processed);
    QVERIFY((learningBlock.row(0).array() == originalReferences.array()).all());
    QVERIFY(learningBlock.row(1).allFinite());
    QVERIFY((learningBlock.row(1).array() == originalTargets.array()).all());
    QVERIFY(learningResult.diagnostics.modelUpdatesAccepted == 2);
    QVERIFY(learningResult.diagnostics.modelUpdatesRejected == 2);
    QVERIFY(learningResult.diagnostics.modelGeneration == 2);
    QVERIFY(std::abs(learningResult.diagnostics.inputRms - expectedLearningRms)
            <= 1e-12);
    QVERIFY(std::abs(learningResult.diagnostics.outputRms - expectedLearningRms)
            <= 1e-12);
    QVERIFY(learningResult.diagnostics.estimatedNoiseRms == 0.0);

    MatrixXd probeBlock(2, 1);
    probeBlock << 3.0,
                  6.0;
    const double originalProbeReference = probeBlock(0, 0);

    const DenoiserProcessResult probeResult =
        denoiser.process(probeBlock, DenoisingMode::ApplyOnly);

    QVERIFY(probeResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(probeBlock(0, 0) == originalProbeReference);
    QVERIFY(std::isfinite(probeBlock(1, 0)));
    QVERIFY(std::abs(probeBlock(1, 0)) <= 1e-5);
    QVERIFY(probeResult.diagnostics.modelGeneration == 2);
    QVERIFY(probeResult.diagnostics.modelUpdatesAccepted == 0);
    QVERIFY(probeResult.diagnostics.modelUpdatesRejected == 0);
    QVERIFY(std::isfinite(probeResult.diagnostics.inputRms));
    QVERIFY(std::isfinite(probeResult.diagnostics.outputRms));
    QVERIFY(std::isfinite(probeResult.diagnostics.estimatedNoiseRms));
}

//=============================================================================================================

void TestCausalReferenceDenoiser::acceptsLoadedRankDeficientEpoch()
{
    CausalReferenceDenoiserConfig config;
    config.samplingFrequencyHz = 1000.0;
    config.channelCount = 3;
    config.maxBlockSamples = 5;
    config.referenceRows = VectorXi(2);
    config.referenceRows << 0, 1;
    config.targetRows = VectorXi(1);
    config.targetRows << 2;
    config.tapCount = 2;
    config.adaptationIntervalSamples = 4;
    config.memoryTimeSeconds = 300.0;
    config.regularization = 1e-8;

    CausalReferenceDenoiser denoiser;
    QVERIFY(denoiser.configure(config) == DenoiserStatus::Configured);

    // After one warmup sample, the P=4 eligible features are
    // v, 2v, 4v, and 8v for v=[2,4,1,2]^T, hence they span rank one.
    // Every target follows the same relationship y(t)=3*r0(t).
    MatrixXd learningBlock(3, 5);
    learningBlock << 1.0, 2.0, 4.0, 8.0, 16.0,
                     2.0, 4.0, 8.0, 16.0, 32.0,
                     3.0, 6.0, 12.0, 24.0, 48.0;
    const MatrixXd originalLearningBlock = learningBlock;

    const DenoiserProcessResult learningResult =
        denoiser.process(learningBlock, DenoisingMode::ApplyAndLearn);

    QVERIFY(learningResult.status == DenoiserProcessStatus::Processed);
    QVERIFY((learningBlock.array() == originalLearningBlock.array()).all());
    QVERIFY(learningResult.diagnostics.featureCount == 4);
    QVERIFY(learningResult.diagnostics.modelUpdatesAccepted == 1);
    QVERIFY(learningResult.diagnostics.modelUpdatesRejected == 0);
    QVERIFY(learningResult.diagnostics.modelGeneration == 1);
    QVERIFY((learningBlock.row(0).array()
             == originalLearningBlock.row(0).array()).all());
    QVERIFY((learningBlock.row(1).array()
             == originalLearningBlock.row(1).array()).all());

    // Continuing the geometric references gives feature 16v and analytic
    // target 3*32=96 under the same rank-one relationship.
    MatrixXd probeBlock(3, 1);
    probeBlock << 32.0,
                  64.0,
                  96.0;
    const MatrixXd originalProbeBlock = probeBlock;

    const DenoiserProcessResult probeResult =
        denoiser.process(probeBlock, DenoisingMode::ApplyOnly);

    QVERIFY(probeResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(probeBlock.allFinite());
    QVERIFY(std::abs(probeBlock(2, 0)) <= 1e-5);
    QVERIFY(probeResult.diagnostics.modelGeneration == 1);
    QVERIFY(probeResult.diagnostics.modelUpdatesAccepted == 0);
    QVERIFY(probeResult.diagnostics.modelUpdatesRejected == 0);
    QVERIFY(probeBlock(0, 0) == originalProbeBlock(0, 0));
    QVERIFY(probeBlock(1, 0) == originalProbeBlock(1, 0));
}

//=============================================================================================================

void TestCausalReferenceDenoiser::reportsStableRmsForLargeBypassAndAnalyticApplyOnly()
{
    {
        CausalReferenceDenoiserConfig config;
        config.samplingFrequencyHz = 1000.0;
        config.channelCount = 2;
        config.maxBlockSamples = 4;
        config.referenceRows = VectorXi(1);
        config.referenceRows << 0;
        config.targetRows = VectorXi(1);
        config.targetRows << 1;
        config.tapCount = 1;
        config.adaptationIntervalSamples = 2;
        config.memoryTimeSeconds = 300.0;
        config.regularization = 1e-8;

        CausalReferenceDenoiser denoiser;
        QVERIFY(denoiser.configure(config) == DenoiserStatus::Configured);

        const double largeFiniteTarget = std::numeric_limits<double>::max() / 4.0;
        MatrixXd bypassBlock(2, 4);
        bypassBlock << 1.0, -2.0, 3.0, -4.0,
                       largeFiniteTarget, -largeFiniteTarget,
                       largeFiniteTarget, -largeFiniteTarget;
        const MatrixXd originalBypassBlock = bypassBlock;

        const DenoiserProcessResult bypassResult =
            denoiser.process(bypassBlock, DenoisingMode::BypassTrackHistory);
        const double largeRmsRelativeTolerance =
            8.0 * std::numeric_limits<double>::epsilon();

        QVERIFY(bypassResult.status == DenoiserProcessStatus::Bypassed);
        QVERIFY(originalBypassBlock.row(1).allFinite());
        QVERIFY((bypassBlock.array() == originalBypassBlock.array()).all());
        QVERIFY(std::isfinite(bypassResult.diagnostics.inputRms));
        QVERIFY(std::isfinite(bypassResult.diagnostics.outputRms));
        QVERIFY(std::abs(bypassResult.diagnostics.inputRms / largeFiniteTarget - 1.0)
                <= largeRmsRelativeTolerance);
        QVERIFY(std::abs(bypassResult.diagnostics.outputRms / largeFiniteTarget - 1.0)
                <= largeRmsRelativeTolerance);
        QVERIFY(bypassResult.diagnostics.estimatedNoiseRms == 0.0);
    }

    {
        CausalReferenceDenoiserConfig config;
        config.samplingFrequencyHz = 1000.0;
        config.channelCount = 2;
        config.maxBlockSamples = 2;
        config.referenceRows = VectorXi(1);
        config.referenceRows << 0;
        config.targetRows = VectorXi(1);
        config.targetRows << 1;
        config.tapCount = 1;
        config.adaptationIntervalSamples = 2;
        config.memoryTimeSeconds = 300.0;
        config.regularization = 1e-8;

        CausalReferenceDenoiser denoiser;
        QVERIFY(denoiser.configure(config) == DenoiserStatus::Configured);

        MatrixXd trainingBlock(2, 2);
        trainingBlock << 1.0, 2.0,
                         2.0, 4.0;
        const DenoiserProcessResult trainingResult =
            denoiser.process(trainingBlock, DenoisingMode::ApplyAndLearn);

        QVERIFY(trainingResult.status == DenoiserProcessStatus::Processed);
        QVERIFY(trainingResult.diagnostics.modelGeneration == 1);
        QVERIFY(trainingResult.diagnostics.modelUpdatesAccepted == 1);
        QVERIFY(trainingResult.diagnostics.modelUpdatesRejected == 0);

        MatrixXd probeBlock(2, 2);
        probeBlock << 3.0, 4.0,
                      10.0, 14.0;
        const MatrixXd originalProbeBlock = probeBlock;

        const DenoiserProcessResult probeResult =
            denoiser.process(probeBlock, DenoisingMode::ApplyOnly);

        const double weight = 2.0 / (1.0 + config.regularization);
        const double expectedFirstResidual = 10.0 - 3.0 * weight;
        const double expectedSecondResidual = 14.0 - 4.0 * weight;
        const double expectedInputRms = std::sqrt(148.0);
        const double expectedOutputRms =
            std::sqrt((expectedFirstResidual * expectedFirstResidual
                       + expectedSecondResidual * expectedSecondResidual) / 2.0);
        const double expectedNoiseRms = weight * std::sqrt(12.5);
        const double relativeTolerance = 1e-12;
        const double absoluteTolerance = 1e-12;
        const auto withinTolerance = [relativeTolerance, absoluteTolerance](double actual,
                                                                            double expected) {
            return std::abs(actual - expected)
                   <= absoluteTolerance + relativeTolerance * std::abs(expected);
        };

        QVERIFY(probeResult.status == DenoiserProcessStatus::Processed);
        QVERIFY(probeBlock.allFinite());
        QVERIFY((probeBlock.row(0).array() == originalProbeBlock.row(0).array()).all());
        QVERIFY(probeResult.diagnostics.modelGeneration == 1);
        QVERIFY(probeResult.diagnostics.modelUpdatesAccepted == 0);
        QVERIFY(probeResult.diagnostics.modelUpdatesRejected == 0);
        QVERIFY(std::isfinite(probeResult.diagnostics.inputRms));
        QVERIFY(std::isfinite(probeResult.diagnostics.outputRms));
        QVERIFY(std::isfinite(probeResult.diagnostics.estimatedNoiseRms));
        QVERIFY(withinTolerance(probeResult.diagnostics.inputRms, expectedInputRms));
        QVERIFY(withinTolerance(probeResult.diagnostics.outputRms, expectedOutputRms));
        QVERIFY(withinTolerance(probeResult.diagnostics.estimatedNoiseRms,
                                expectedNoiseRms));
        QVERIFY(withinTolerance(probeBlock(1, 0), expectedFirstResidual));
        QVERIFY(withinTolerance(probeBlock(1, 1), expectedSecondResidual));
    }
}

//=============================================================================================================

void TestCausalReferenceDenoiser::fallsBackAtomicallyWhenPredictionOverflows()
{
    CausalReferenceDenoiserConfig config;
    config.samplingFrequencyHz = 1000.0;
    config.channelCount = 4;
    config.maxBlockSamples = 2;
    config.referenceRows = VectorXi(1);
    config.referenceRows << 0;
    config.targetRows = VectorXi(2);
    config.targetRows << 1, 2;
    config.tapCount = 1;
    config.adaptationIntervalSamples = 2;
    config.memoryTimeSeconds = 300.0;
    config.regularization = 1e-8;

    CausalReferenceDenoiser denoiser;
    QVERIFY(denoiser.configure(config) == DenoiserStatus::Configured);

    MatrixXd trainingBlock(4, 2);
    trainingBlock << 1.0, 2.0,
                     2.0, 4.0,
                     0.5, 1.0,
                     100.0, 101.0;
    const MatrixXd originalTrainingBlock = trainingBlock;

    const DenoiserProcessResult trainingResult =
        denoiser.process(trainingBlock, DenoisingMode::ApplyAndLearn);

    QVERIFY(trainingResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(trainingBlock.allFinite());
    QVERIFY((trainingBlock.array() == originalTrainingBlock.array()).all());
    QVERIFY(trainingResult.diagnostics.modelGeneration == 1);
    QVERIFY(trainingResult.diagnostics.modelUpdatesAccepted == 1);
    QVERIFY(trainingResult.diagnostics.modelUpdatesRejected == 0);

    // The learned weights are approximately 2 and 0.5. At DBL_MAX the first
    // target prediction overflows while the second prediction remains finite,
    // so the selected fallback must preserve every target at this sample.
    MatrixXd overflowProbe(4, 1);
    overflowProbe << std::numeric_limits<double>::max(),
                     7.0,
                     11.0,
                     102.0;
    const MatrixXd originalOverflowProbe = overflowProbe;

    const DenoiserProcessResult overflowResult =
        denoiser.process(overflowProbe, DenoisingMode::ApplyOnly);
    const double expectedFallbackRms = std::sqrt(85.0);

    QVERIFY(overflowResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(overflowProbe.allFinite());
    QVERIFY((overflowProbe.array() == originalOverflowProbe.array()).all());
    QVERIFY(overflowResult.diagnostics.modelGeneration == 1);
    QVERIFY(overflowResult.diagnostics.modelUpdatesAccepted == 0);
    QVERIFY(overflowResult.diagnostics.modelUpdatesRejected == 0);
    QVERIFY(std::isfinite(overflowResult.diagnostics.inputRms));
    QVERIFY(std::isfinite(overflowResult.diagnostics.outputRms));
    QVERIFY(std::isfinite(overflowResult.diagnostics.estimatedNoiseRms));
    QVERIFY(std::abs(overflowResult.diagnostics.inputRms - expectedFallbackRms)
            <= 1e-12);
    QVERIFY(std::abs(overflowResult.diagnostics.outputRms - expectedFallbackRms)
            <= 1e-12);
    QVERIFY(overflowResult.diagnostics.estimatedNoiseRms == 0.0);

    MatrixXd normalProbe(4, 1);
    normalProbe << 3.0,
                   6.0,
                   1.5,
                   103.0;
    const MatrixXd originalNormalProbe = normalProbe;

    const DenoiserProcessResult normalResult =
        denoiser.process(normalProbe, DenoisingMode::ApplyOnly);

    QVERIFY(normalResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(normalProbe.allFinite());
    QVERIFY(normalProbe(0, 0) == originalNormalProbe(0, 0));
    QVERIFY(normalProbe(3, 0) == originalNormalProbe(3, 0));
    QVERIFY(std::abs(normalProbe(1, 0)) <= 1e-5);
    QVERIFY(std::abs(normalProbe(2, 0)) <= 1e-5);
    QVERIFY(normalResult.diagnostics.modelGeneration == 1);
    QVERIFY(normalResult.diagnostics.modelUpdatesAccepted == 0);
    QVERIFY(normalResult.diagnostics.modelUpdatesRejected == 0);
}

//=============================================================================================================

void TestCausalReferenceDenoiser::meetsQuantitativeSyntheticAcceptance()
{
    constexpr Eigen::Index referenceCount = 2;
    constexpr Eigen::Index tapCount = 4;
    constexpr Eigen::Index featureCount = referenceCount * tapCount;
    constexpr Eigen::Index blockSamples = 128;
    constexpr Eigen::Index trainingBlockCount = 768;
    constexpr Eigen::Index trainingSampleCount = trainingBlockCount * blockSamples;
    constexpr Eigen::Index evaluationSampleCount = blockSamples;
    constexpr Eigen::Index totalSampleCount = trainingSampleCount + evaluationSampleCount;
    constexpr double twoPi = 6.283185307179586476925286766559;

    CausalReferenceDenoiserConfig config;
    config.samplingFrequencyHz = 1000.0;
    config.channelCount = 3;
    config.maxBlockSamples = blockSamples;
    config.referenceRows = VectorXi(referenceCount);
    config.referenceRows << 0, 1;
    config.targetRows = VectorXi(1);
    config.targetRows << 2;
    config.tapCount = tapCount;
    config.adaptationIntervalSamples = blockSamples;
    config.memoryTimeSeconds = 30.0;
    config.regularization = 1e-3;

    // Precompute one uninterrupted finite stream. Independent fixed xorshift32
    // excitations, short AR memories, and multisine terms keep both references
    // broadband while generator/filter state remains continuous across blocks.
    MatrixXd references(referenceCount, totalSampleCount);
    std::uint32_t generatorState0 = 0xA341316Cu;
    std::uint32_t generatorState1 = 0xC8013EA4u;
    const auto nextExcitation = [](std::uint32_t& state) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        const double unit = static_cast<double>(state)
                            / static_cast<double>(std::numeric_limits<std::uint32_t>::max());
        return 2.0 * unit - 1.0;
    };

    double referenceMemory0 = 0.0;
    double referenceMemory1 = 0.0;
    for (Eigen::Index sample = 0; sample < totalSampleCount; ++sample) {
        referenceMemory0 = 0.35 * referenceMemory0
                           + 0.65 * nextExcitation(generatorState0);
        referenceMemory1 = -0.25 * referenceMemory1
                           + 0.75 * nextExcitation(generatorState1);
        const double time = static_cast<double>(sample);
        references(0, sample) = referenceMemory0
                                + 0.30 * std::sin(0.071 * time)
                                + 0.20 * std::cos(0.173 * time);
        references(1, sample) = referenceMemory1
                                + 0.25 * std::cos(0.097 * time)
                                - 0.18 * std::sin(0.257 * time);
    }
    QVERIFY(references.allFinite());

    VectorXd knownWeights(featureCount);
    knownWeights << 1.40, -1.10,
                    0.80,  0.65,
                   -0.55,  0.45,
                    0.35, -0.25;

    // Construct y_noise(t) from phi(t)=[r0(t),r1(t),r0(t-1),r1(t-1),...].
    // Samples before t=0 are exactly zero, matching the denoiser's causal
    // prehistory; its first tapCount-1 samples are therefore warmup only.
    RowVectorXd environmentalNoise(totalSampleCount);
    for (Eigen::Index sample = 0; sample < totalSampleCount; ++sample) {
        double noise = 0.0;
        Eigen::Index weight = 0;
        for (Eigen::Index lag = 0; lag < tapCount; ++lag) {
            for (Eigen::Index reference = 0; reference < referenceCount; ++reference) {
                const double referenceValue = sample >= lag
                                                  ? references(reference, sample - lag)
                                                  : 0.0;
                noise += knownWeights(weight) * referenceValue;
                ++weight;
            }
        }
        environmentalNoise(sample) = noise;
    }
    QVERIFY(environmentalNoise.allFinite());

    CausalReferenceDenoiser denoiser;
    QVERIFY(denoiser.configure(config) == DenoiserStatus::Configured);

    MatrixXd trainingBlock(config.channelCount, blockSamples);
    std::uint64_t acceptedUpdates = 0;
    std::uint64_t rejectedUpdates = 0;
    std::uint64_t modelGeneration = 0;
    for (Eigen::Index blockIndex = 0; blockIndex < trainingBlockCount; ++blockIndex) {
        const Eigen::Index firstSample = blockIndex * blockSamples;
        trainingBlock.topRows(referenceCount) =
            references.middleCols(firstSample, blockSamples);
        trainingBlock.row(2) = environmentalNoise.segment(firstSample, blockSamples);

        const DenoiserProcessResult trainingResult =
            denoiser.process(trainingBlock, DenoisingMode::ApplyAndLearn);

        QVERIFY(trainingResult.status == DenoiserProcessStatus::Processed);
        QVERIFY(trainingBlock.allFinite());
        QVERIFY((trainingBlock.topRows(referenceCount).array()
                 == references.middleCols(firstSample, blockSamples).array()).all());
        acceptedUpdates += trainingResult.diagnostics.modelUpdatesAccepted;
        rejectedUpdates += trainingResult.diagnostics.modelUpdatesRejected;
        modelGeneration = trainingResult.diagnostics.modelGeneration;
    }

    const Eigen::Index eligibleTrainingSamples = trainingSampleCount - (tapCount - 1);
    const std::uint64_t expectedAcceptedUpdates = static_cast<std::uint64_t>(
        eligibleTrainingSamples / config.adaptationIntervalSamples);
    QVERIFY(acceptedUpdates == expectedAcceptedUpdates);
    QVERIFY(rejectedUpdates == 0);
    QVERIFY(modelGeneration == expectedAcceptedUpdates);

    RowVectorXd clean(evaluationSampleCount);
    for (Eigen::Index sample = 0; sample < evaluationSampleCount; ++sample) {
        const Eigen::Index absoluteSample = trainingSampleCount + sample;
        const double timeSeconds = static_cast<double>(absoluteSample)
                                   / config.samplingFrequencyHz;
        clean(sample) = 0.65 * std::sin(twoPi * 62.5 * timeSeconds + 0.31);
    }

    const RowVectorXd evaluationNoise =
        environmentalNoise.segment(trainingSampleCount, evaluationSampleCount);
    MatrixXd evaluationBlock(config.channelCount, evaluationSampleCount);
    evaluationBlock.topRows(referenceCount) =
        references.middleCols(trainingSampleCount, evaluationSampleCount);
    evaluationBlock.row(2) = evaluationNoise + clean;

    const DenoiserProcessResult evaluationResult =
        denoiser.process(evaluationBlock, DenoisingMode::ApplyOnly);

    QVERIFY(evaluationResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(evaluationBlock.allFinite());
    QVERIFY((evaluationBlock.topRows(referenceCount).array()
             == references.middleCols(trainingSampleCount,
                                       evaluationSampleCount).array()).all());
    QVERIFY(evaluationResult.diagnostics.featureCount == featureCount);
    QVERIFY(evaluationResult.diagnostics.modelGeneration == expectedAcceptedUpdates);
    QVERIFY(evaluationResult.diagnostics.modelUpdatesAccepted == 0);
    QVERIFY(evaluationResult.diagnostics.modelUpdatesRejected == 0);

    const RowVectorXd output = evaluationBlock.row(2);
    const RowVectorXd residualNoise = output - clean;
    const double sampleCount = static_cast<double>(evaluationSampleCount);
    const double environmentalNoiseRms = std::sqrt(evaluationNoise.squaredNorm()
                                                   / sampleCount);
    const double residualNoiseRms = std::sqrt(residualNoise.squaredNorm()
                                              / sampleCount);
    QVERIFY(environmentalNoiseRms > 0.0);
    QVERIFY(residualNoiseRms > 0.0);
    const double noiseReductionDb =
        20.0 * std::log10(environmentalNoiseRms / residualNoiseRms);

    const double cleanEnergy = clean.squaredNorm();
    QVERIFY(cleanEnergy > 0.0);
    const double projectedCleanAmplitude = output.dot(clean) / cleanEnergy;
    const double cleanAmplitudeError = std::abs(projectedCleanAmplitude - 1.0);

    qInfo() << "synthetic environmental-noise reduction (dB)" << noiseReductionDb
            << "clean projection amplitude error" << cleanAmplitudeError;
    QVERIFY(std::isfinite(noiseReductionDb));
    QVERIFY(std::isfinite(cleanAmplitudeError));
    QVERIFY(noiseReductionDb >= 10.0);
    QVERIFY(cleanAmplitudeError <= 0.02);
}

//=============================================================================================================

void TestCausalReferenceDenoiser::processDoesNotAllocateAfterConfigure()
{
#ifdef EIGEN_NO_DEBUG
    QSKIP("Eigen assertions are disabled, so the runtime malloc guard is ineffective.");
#else
    CausalReferenceDenoiserConfig config;
    config.samplingFrequencyHz = 1000.0;
    config.channelCount = 5;
    config.maxBlockSamples = 7;
    config.referenceRows = VectorXi(2);
    config.referenceRows << 0, 1;
    config.targetRows = VectorXi(2);
    config.targetRows << 2, 3;
    config.tapCount = 4;
    config.adaptationIntervalSamples = 4;
    config.memoryTimeSeconds = 300.0;
    config.regularization = 1e-8;

    CausalReferenceDenoiser denoiser;
    QVERIFY(denoiser.configure(config) == DenoiserStatus::Configured);

    // The first three columns fill four-tap history. The final four columns
    // form one complete P=8 learning epoch and exercise the loaded LDLT solve.
    MatrixXd learningBlock(5, 7);
    learningBlock << 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
                     2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0,
                     3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0,
                    -4.0, -4.0, -4.0, -4.0, -4.0, -4.0, -4.0,
                   100.0, 101.0, 102.0, 103.0, 104.0, 105.0, 106.0;
    const MatrixXd originalLearningBlock = learningBlock;

    // Constant continuation references reproduce the learned feature and
    // make both target rows an observable committed-model probe.
    MatrixXd applyOnlyBlock(5, 2);
    applyOnlyBlock << 1.0, 1.0,
                      2.0, 2.0,
                      3.0, 3.0,
                     -4.0, -4.0,
                    107.0, 108.0;
    const MatrixXd originalApplyOnlyBlock = applyOnlyBlock;

    const bool learningMallocWasAllowed = Eigen::internal::is_malloc_allowed();
    Eigen::internal::set_is_malloc_allowed(false);
    const DenoiserProcessResult learningResult =
        denoiser.process(learningBlock, DenoisingMode::ApplyAndLearn);
    Eigen::internal::set_is_malloc_allowed(learningMallocWasAllowed);

    QVERIFY(learningResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(learningBlock.allFinite());
    QVERIFY(learningResult.diagnostics.referenceRowCount == 2);
    QVERIFY(learningResult.diagnostics.targetRowCount == 2);
    QVERIFY(learningResult.diagnostics.featureCount == 8);
    QVERIFY(learningResult.diagnostics.warmupSamplesRemaining == 0);
    QVERIFY(learningResult.diagnostics.modelUpdatesAccepted == 1);
    QVERIFY(learningResult.diagnostics.modelUpdatesRejected == 0);
    QVERIFY(learningResult.diagnostics.modelGeneration == 1);
    QVERIFY(std::isfinite(learningResult.diagnostics.inputRms));
    QVERIFY(std::isfinite(learningResult.diagnostics.outputRms));
    QVERIFY(std::isfinite(learningResult.diagnostics.estimatedNoiseRms));
    QVERIFY((learningBlock.row(0).array()
             == originalLearningBlock.row(0).array()).all());
    QVERIFY((learningBlock.row(1).array()
             == originalLearningBlock.row(1).array()).all());
    QVERIFY((learningBlock.row(4).array()
             == originalLearningBlock.row(4).array()).all());

    const bool applyOnlyMallocWasAllowed = Eigen::internal::is_malloc_allowed();
    Eigen::internal::set_is_malloc_allowed(false);
    const DenoiserProcessResult applyOnlyResult =
        denoiser.process(applyOnlyBlock, DenoisingMode::ApplyOnly);
    Eigen::internal::set_is_malloc_allowed(applyOnlyMallocWasAllowed);

    QVERIFY(applyOnlyResult.status == DenoiserProcessStatus::Processed);
    QVERIFY(applyOnlyBlock.allFinite());
    QVERIFY(applyOnlyResult.diagnostics.modelGeneration == 1);
    QVERIFY(applyOnlyResult.diagnostics.modelUpdatesAccepted == 0);
    QVERIFY(applyOnlyResult.diagnostics.modelUpdatesRejected == 0);
    QVERIFY(std::isfinite(applyOnlyResult.diagnostics.inputRms));
    QVERIFY(std::isfinite(applyOnlyResult.diagnostics.outputRms));
    QVERIFY(std::isfinite(applyOnlyResult.diagnostics.estimatedNoiseRms));
    QVERIFY(applyOnlyResult.diagnostics.estimatedNoiseRms > 0.0);
    QVERIFY((applyOnlyBlock.row(0).array()
             == originalApplyOnlyBlock.row(0).array()).all());
    QVERIFY((applyOnlyBlock.row(1).array()
             == originalApplyOnlyBlock.row(1).array()).all());
    QVERIFY((applyOnlyBlock.row(4).array()
             == originalApplyOnlyBlock.row(4).array()).all());
    QVERIFY((applyOnlyBlock.row(2).array().abs() <= 1e-6).all());
    QVERIFY((applyOnlyBlock.row(3).array().abs() <= 1e-6).all());
#endif
}

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    TestCausalReferenceDenoiser testObject;
    const int result = QTest::qExec(&testObject, argc, argv);
    std::fprintf(stdout,
                 "\n[Adaptive Denoising] numerical core: %s (exit %d)\n",
                 result == 0 ? "PASS" : "FAIL",
                 result);
    std::fflush(stdout);
    return result;
}

#include "test_causal_reference_denoiser.moc"
