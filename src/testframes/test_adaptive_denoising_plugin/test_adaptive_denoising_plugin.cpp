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

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <limits>
#include <new>
#include <thread>
#include <type_traits>
#include <vector>

#if defined(__unix__) && !defined(__APPLE__) && (defined(__GNUC__) || defined(__clang__))
#include <cerrno>
#include <sys/types.h>
#include <unistd.h>
#endif

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QtTest>

//=============================================================================================================

namespace
{

// Count only successful global allocations made by the producer while its thread-local gate is enabled.
thread_local bool g_countTestAllocations = false;
std::atomic<std::uint64_t> g_countedTestAllocations(0);

#if defined(__unix__) && !defined(__APPLE__) && (defined(__GNUC__) || defined(__clang__))
thread_local bool g_interruptNextQueueWrite = false;
std::atomic<int> g_interruptedQueueWriteCount(0);
#endif

void* allocateTestStorageNoThrow(std::size_t size) noexcept
{
    void* const storage = std::malloc(size == 0 ? 1 : size);
    if (storage != nullptr && g_countTestAllocations) {
        g_countedTestAllocations.fetch_add(1, std::memory_order_relaxed);
    }
    return storage;
}

void* allocateTestStorage(std::size_t size)
{
    void* const storage = allocateTestStorageNoThrow(size);
    if (storage == nullptr) {
        throw std::bad_alloc();
    }
    return storage;
}

} // namespace

#if defined(__unix__) && !defined(__APPLE__) && (defined(__GNUC__) || defined(__clang__))
extern "C" ssize_t __real_write(int descriptor, const void* buffer, std::size_t count);

extern "C" ssize_t __wrap_write(int descriptor, const void* buffer, std::size_t count)
{
    if (g_interruptNextQueueWrite) {
        g_interruptNextQueueWrite = false;
        errno = EINTR;
        g_interruptedQueueWriteCount.fetch_add(1, std::memory_order_acq_rel);
        return -1;
    }

    return __real_write(descriptor, buffer, count);
}
#endif

//=============================================================================================================

void* operator new(std::size_t size)
{
    return allocateTestStorage(size);
}

void* operator new[](std::size_t size)
{
    return allocateTestStorage(size);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    return allocateTestStorageNoThrow(size);
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept
{
    return allocateTestStorageNoThrow(size);
}

void operator delete(void* memory) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, const std::nothrow_t&) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory, const std::nothrow_t&) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory, std::size_t) noexcept
{
    std::free(memory);
}

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

using NativeFiffInfoHandle = QSharedPointer<const FIFFLIB::FiffInfo>;
using MutableNativeFiffInfoHandle = QSharedPointer<FIFFLIB::FiffInfo>;

static_assert(
    std::is_nothrow_copy_constructible<NativeFiffInfoHandle>::value,
    "Native FIFF metadata handle must be nothrow copy constructible");
static_assert(
    std::is_nothrow_copy_assignable<NativeFiffInfoHandle>::value,
    "Native FIFF metadata handle must be nothrow copy assignable");
static_assert(
    std::is_nothrow_constructible<NativeFiffInfoHandle, MutableNativeFiffInfoHandle>::value,
    "Mutable native FIFF metadata handle must convert to const without throwing");

//=============================================================================================================

namespace
{

constexpr Index kChannelCount = 6;
constexpr Index kBlockSamples = 16;
constexpr Index kTargetRow = 2;

struct FakeMetadataLifetime
{
    FakeMetadataLifetime() noexcept
    : token(0)
    , liveCount(0)
    , deleterCount(0)
    {
    }

    std::uint64_t  token;
    std::atomic<int> liveCount;
    std::atomic<int> deleterCount;
};

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

AdaptiveDenoisingStreamDescriptor streamWithGoodReferences(Index referenceCount)
{
    AdaptiveDenoisingStreamDescriptor stream;
    stream.samplingFrequencyHz = 1000.0;
    stream.channels.reserve(static_cast<std::size_t>(referenceCount + 1));

    for (Index reference = 0; reference < referenceCount; ++reference) {
        stream.channels.push_back({FIFFV_REF_MEG_CH, false});
    }

    stream.channels.push_back({FIFFV_MEG_CH, false});
    return stream;
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

NativeFiffInfoHandle fakeFiffInfoHandle(const std::uint64_t& token)
{
    const FIFFLIB::FiffInfo* const fakePointer =
        reinterpret_cast<const FIFFLIB::FiffInfo*>(&token);
    return NativeFiffInfoHandle(
        fakePointer,
        [](const FIFFLIB::FiffInfo*) noexcept {});
}

NativeFiffInfoHandle fakeFiffInfoHandle(FakeMetadataLifetime& lifetime)
{
    const FIFFLIB::FiffInfo* const fakePointer =
        reinterpret_cast<const FIFFLIB::FiffInfo*>(&lifetime.token);
    FakeMetadataLifetime* const lifetimePointer = &lifetime;
    lifetime.liveCount.store(1, std::memory_order_release);
    lifetime.deleterCount.store(0, std::memory_order_release);
    return NativeFiffInfoHandle(
        fakePointer,
        [lifetimePointer](const FIFFLIB::FiffInfo*) noexcept {
            lifetimePointer->deleterCount.fetch_add(1, std::memory_order_acq_rel);
            lifetimePointer->liveCount.store(0, std::memory_order_release);
        });
}

bool sameMetadata(
    const NativeFiffInfoHandle& first,
    const NativeFiffInfoHandle& second) noexcept
{
    return first.data() == second.data();
}

bool matrixAllEquals(const MatrixXd& actual, double expected) noexcept
{
    for (Index row = 0; row < actual.rows(); ++row) {
        for (Index column = 0; column < actual.cols(); ++column) {
            if (actual(row, column) != expected) {
                return false;
            }
        }
    }

    return true;
}

bool destinationPreserves(
    const AdaptiveDenoisingQueuedBlock& destination,
    Index expectedMatrixRows,
    Index expectedMatrixColumns,
    double expectedData,
    Index expectedRowCount,
    Index expectedSampleCount,
    const NativeFiffInfoHandle& expectedMetadata) noexcept
{
    return destination.data.rows() == expectedMatrixRows
        && destination.data.cols() == expectedMatrixColumns
        && matrixAllEquals(destination.data, expectedData)
        && destination.rowCount == expectedRowCount
        && destination.sampleCount == expectedSampleCount
        && sameMetadata(destination.fiffInfo, expectedMetadata);
}

#if defined(__unix__) && !defined(__APPLE__) && (defined(__GNUC__) || defined(__clang__))

bool destinationContainsBlock(
    const AdaptiveDenoisingQueuedBlock& destination,
    const MatrixXd& expectedBlock,
    const NativeFiffInfoHandle& expectedMetadata,
    Index expectedMatrixRows,
    Index expectedMatrixColumns,
    double expectedTail) noexcept
{
    if (destination.data.rows() != expectedMatrixRows
        || destination.data.cols() != expectedMatrixColumns
        || destination.rowCount != expectedBlock.rows()
        || destination.sampleCount != expectedBlock.cols()
        || !sameMetadata(destination.fiffInfo, expectedMetadata)) {
        return false;
    }

    for (Index row = 0; row < destination.data.rows(); ++row) {
        for (Index column = 0; column < destination.data.cols(); ++column) {
            const bool inBlock = row < expectedBlock.rows() && column < expectedBlock.cols();
            const double expected = inBlock ? expectedBlock(row, column) : expectedTail;
            if (destination.data(row, column) != expected) {
                return false;
            }
        }
    }

    return true;
}

#endif

template<typename Predicate>
bool waitUntil(Predicate predicate, int timeoutMilliseconds) noexcept
{
    const auto deadline = std::chrono::steady_clock::now()
        + std::chrono::milliseconds(timeoutMilliseconds);

    while (!predicate()) {
        if (std::chrono::steady_clock::now() >= deadline) {
            return predicate();
        }
        std::this_thread::yield();
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
    void validReconfigureResetsAndRelearnsModel();
    void acceptsInclusiveLegalBoundaries_data();
    void acceptsInclusiveLegalBoundaries();
    void queuePreservesFifoDropNewestAndMetadata();
    void queueStopWakesWaiterAndReconfigureStartsFresh();
    void queueRejectsInvalidInputsWithoutConsumingState();
    void queueAcceptsVariableRowsWithNativeMetadata();
    void queueSustainedOverlappingSpscPreservesAcceptedStream();
    void queueStopRacesActiveProducerAndConsumer();
    void queueStopsWithPendingBlockAndPreservesDestination();
    void queueRetainsNativeMetadataUntilPoppedHandleCleared();
#if defined(__unix__) && !defined(__APPLE__) && (defined(__GNUC__) || defined(__clang__))
    void queuePopsPromptlyAfterInterruptedProducerSignal();
    void queueStopsPromptlyAfterInterruptedStopSignal();
#endif
};

//=============================================================================================================

void TestAdaptiveDenoisingPlugin::acceptsInclusiveLegalBoundaries_data()
{
    QTest::addColumn<int>("tapCount");
    QTest::addColumn<int>("adaptationIntervalSamples");
    QTest::addColumn<double>("memoryTimeSeconds");
    QTest::addColumn<double>("regularization");
    QTest::addColumn<int>("referenceCount");
    QTest::addColumn<int>("targetCount");
    QTest::addColumn<int>("featureCount");

    QTest::newRow("taps=32") << 32 << 128 << 30.0 << 1e-3 << 1 << 1 << 32;
    QTest::newRow("adaptation-interval=2048")
        << 4 << 2048 << 30.0 << 1e-3 << 1 << 1 << 4;
    QTest::newRow("memory=1") << 4 << 128 << 1.0 << 1e-3 << 1 << 1 << 4;
    QTest::newRow("memory=300") << 4 << 128 << 300.0 << 1e-3 << 1 << 1 << 4;
    QTest::newRow("regularization=1") << 4 << 128 << 30.0 << 1.0 << 1 << 1 << 4;
    QTest::newRow("P=256") << 32 << 128 << 30.0 << 1e-3 << 8 << 1 << 256;
}

//=============================================================================================================

void TestAdaptiveDenoisingPlugin::acceptsInclusiveLegalBoundaries()
{
    QFETCH(int, tapCount);
    QFETCH(int, adaptationIntervalSamples);
    QFETCH(double, memoryTimeSeconds);
    QFETCH(double, regularization);
    QFETCH(int, referenceCount);
    QFETCH(int, targetCount);
    QFETCH(int, featureCount);

    const AdaptiveDenoisingStreamDescriptor stream =
        streamWithGoodReferences(static_cast<Index>(referenceCount));
    AdaptiveDenoisingSettings settings;
    settings.tapCount = static_cast<Index>(tapCount);
    settings.adaptationIntervalSamples = static_cast<Index>(adaptationIntervalSamples);
    settings.memoryTimeSeconds = memoryTimeSeconds;
    settings.regularization = regularization;

    AdaptiveDenoisingProcessor processor;
    const AdaptiveDenoisingConfigureResult configureResult =
        processor.configure(stream, kBlockSamples, settings);

    QVERIFY(configureResult.status == AdaptiveDenoisingConfigureStatus::Ready);
    QCOMPARE(configureResult.referenceCount, Index(referenceCount));
    QCOMPARE(configureResult.targetCount, Index(targetCount));
    QCOMPARE(configureResult.featureCount, Index(featureCount));

    const AdaptiveDenoisingConfigureResult snapshot = processor.configuration();
    QVERIFY(snapshot.status == AdaptiveDenoisingConfigureStatus::Ready);
    QCOMPARE(snapshot.referenceCount, Index(referenceCount));
    QCOMPARE(snapshot.targetCount, Index(targetCount));
    QCOMPARE(snapshot.featureCount, Index(featureCount));
}

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
    config.maxChannelCount = 2;
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

    const std::uint64_t metadataTokenA = 1;
    const std::uint64_t metadataTokenB = 2;
    const std::uint64_t metadataTokenC = 3;
    const NativeFiffInfoHandle metadataA = fakeFiffInfoHandle(metadataTokenA);
    const NativeFiffInfoHandle metadataB = fakeFiffInfoHandle(metadataTokenB);
    const NativeFiffInfoHandle metadataC = fakeFiffInfoHandle(metadataTokenC);

    QVERIFY(!sameMetadata(metadataA, metadataB));
    QVERIFY(!sameMetadata(metadataA, metadataC));
    QVERIFY(!sameMetadata(metadataB, metadataC));

    QVERIFY(queue.tryPush(blockA, metadataA) == AdaptiveDenoisingQueuePushStatus::Pushed);
    QVERIFY(queue.tryPush(blockB, metadataB) == AdaptiveDenoisingQueuePushStatus::Pushed);

    blockA.array() += 1000.0;
    blockB.array() += 2000.0;

    QVERIFY(queue.tryPush(blockC, metadataC) == AdaptiveDenoisingQueuePushStatus::Full);

    AdaptiveDenoisingQueuedBlock destination;
    destination.data.resize(config.maxChannelCount, config.maxBlockSamples);

    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Popped);
    QCOMPARE(destination.rowCount, Index(2));
    QCOMPARE(destination.sampleCount, Index(3));
    QVERIFY(sameMetadata(destination.fiffInfo, metadataA));
    for (Index row = 0; row < originalA.rows(); ++row) {
        QVERIFY(rowEquals(destination.data, originalA, row));
    }

    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Popped);
    QCOMPARE(destination.rowCount, Index(2));
    QCOMPARE(destination.sampleCount, Index(4));
    QVERIFY(sameMetadata(destination.fiffInfo, metadataB));
    for (Index row = 0; row < originalB.rows(); ++row) {
        QVERIFY(rowEquals(destination.data, originalB, row));
    }

    const std::uint64_t timeoutMetadataToken = 3001;
    const NativeFiffInfoHandle timeoutMetadata = fakeFiffInfoHandle(timeoutMetadataToken);
    destination.data.setConstant(-3001.0);
    destination.rowCount = 3001;
    destination.sampleCount = 3002;
    destination.fiffInfo = timeoutMetadata;
    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Timeout);
    QVERIFY(destinationPreserves(
        destination, config.maxChannelCount, config.maxBlockSamples, -3001.0, 3001, 3002,
        timeoutMetadata));

    QVERIFY(queue.tryPush(blockC, metadataC) == AdaptiveDenoisingQueuePushStatus::Pushed);
    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Popped);
    QCOMPARE(destination.rowCount, Index(2));
    QCOMPARE(destination.sampleCount, Index(2));
    QVERIFY(sameMetadata(destination.fiffInfo, metadataC));
    for (Index row = 0; row < originalC.rows(); ++row) {
        QVERIFY(rowEquals(destination.data, originalC, row));
    }

    destination.data.setConstant(-3003.0);
    destination.rowCount = 3003;
    destination.sampleCount = 3004;
    destination.fiffInfo = timeoutMetadata;
    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Timeout);
    QVERIFY(destinationPreserves(
        destination, config.maxChannelCount, config.maxBlockSamples, -3003.0, 3003, 3004,
        timeoutMetadata));
}

//=============================================================================================================

void TestAdaptiveDenoisingPlugin::queueStopWakesWaiterAndReconfigureStartsFresh()
{
    AdaptiveDenoisingBlockQueue queue;
    AdaptiveDenoisingBlockQueueConfig config;
    config.maxChannelCount = 2;
    config.maxBlockSamples = 4;
    config.capacity = 1;

    QVERIFY(queue.configure(config) == AdaptiveDenoisingQueueConfigureStatus::Ready);
    QVERIFY(queue.configure(config) == AdaptiveDenoisingQueueConfigureStatus::AlreadyRunning);

    MatrixXd preludeBlock(2, 1);
    preludeBlock << 4101.0,
                    4201.0;
    const NativeFiffInfoHandle preludeMetadata = fakeFiffInfoHandle(std::uint64_t(4102));
    AdaptiveDenoisingQueuedBlock preludeDestination;
    preludeDestination.data.resize(config.maxChannelCount, config.maxBlockSamples);
    preludeDestination.data.setConstant(-4103.0);
    preludeDestination.rowCount = 4103;
    preludeDestination.sampleCount = 4104;
    const NativeFiffInfoHandle preludeDestinationMetadata =
        fakeFiffInfoHandle(std::uint64_t(4105));
    preludeDestination.fiffInfo = preludeDestinationMetadata;
    const AdaptiveDenoisingQueuePushStatus preludePushStatus =
        queue.tryPush(preludeBlock, preludeMetadata);
    const AdaptiveDenoisingQueuePopStatus preludePopStatus =
        queue.waitPop(preludeDestination, 0);

    constexpr int kConsumerTimeoutMilliseconds = 3000;
    constexpr int kProbeTimeoutMilliseconds = 50;
    constexpr int kProbeLowerBoundMilliseconds = 5;
    constexpr int kProbeUpperBoundMilliseconds = 1000;
    constexpr int kStopSchedulingAllowanceMilliseconds = 50;
    constexpr int kWakeUpperBoundMilliseconds = 1500;
    constexpr int kEntryWaitBudgetMilliseconds = 1000;

    const std::uint64_t consumerSentinelMetadataToken = 4001;
    const NativeFiffInfoHandle consumerSentinelMetadata =
        fakeFiffInfoHandle(consumerSentinelMetadataToken);
    AdaptiveDenoisingQueuedBlock consumerDestination;
    consumerDestination.data.resize(config.maxChannelCount, config.maxBlockSamples);
    consumerDestination.data.setConstant(-4001.0);
    consumerDestination.rowCount = 4001;
    consumerDestination.sampleCount = 4002;
    consumerDestination.fiffInfo = consumerSentinelMetadata;

    std::atomic<bool> consumerEntered(false);
    std::atomic<bool> consumerFinished(false);
    std::atomic<bool> invalidConsumerStatus(false);
    std::atomic<int> probeStatus(
        static_cast<int>(AdaptiveDenoisingQueuePopStatus::Timeout));
    std::atomic<int> observedStatus(
        static_cast<int>(AdaptiveDenoisingQueuePopStatus::Timeout));
    std::atomic<std::int64_t> consumerEnteredMilliseconds(-1);
    std::atomic<std::int64_t> probeElapsedMilliseconds(-1);
    std::atomic<std::int64_t> observedElapsedMilliseconds(-1);

    const auto testStartedAt = std::chrono::steady_clock::now();
    std::thread consumer([&] {
        consumerDestination.data.setConstant(-4001.0);
        consumerDestination.rowCount = 4001;
        consumerDestination.sampleCount = 4002;
        consumerDestination.fiffInfo = consumerSentinelMetadata;
        const auto probeStartedAt = std::chrono::steady_clock::now();
        const AdaptiveDenoisingQueuePopStatus probeResult =
            queue.waitPop(consumerDestination, kProbeTimeoutMilliseconds);
        const auto probeFinishedAt = std::chrono::steady_clock::now();
        probeStatus.store(static_cast<int>(probeResult), std::memory_order_release);
        probeElapsedMilliseconds.store(
            std::chrono::duration_cast<std::chrono::milliseconds>(probeFinishedAt - probeStartedAt)
                .count(),
            std::memory_order_release);
        if (probeResult != AdaptiveDenoisingQueuePopStatus::Timeout
            || !destinationPreserves(
                consumerDestination,
                config.maxChannelCount,
                config.maxBlockSamples,
                -4001.0,
                4001,
                4002,
                consumerSentinelMetadata)) {
            invalidConsumerStatus.store(true, std::memory_order_release);
        }

        const auto enteredAt = std::chrono::steady_clock::now();
        consumerEnteredMilliseconds.store(
            std::chrono::duration_cast<std::chrono::milliseconds>(enteredAt - testStartedAt).count(),
            std::memory_order_release);
        consumerEntered.store(true, std::memory_order_release);

        const auto waitStartedAt = std::chrono::steady_clock::now();
        const AdaptiveDenoisingQueuePopStatus status =
            queue.waitPop(consumerDestination, kConsumerTimeoutMilliseconds);
        const auto waitFinishedAt = std::chrono::steady_clock::now();

        observedStatus.store(static_cast<int>(status), std::memory_order_release);
        observedElapsedMilliseconds.store(
            std::chrono::duration_cast<std::chrono::milliseconds>(waitFinishedAt - waitStartedAt)
                .count(),
            std::memory_order_release);
        consumerFinished.store(true, std::memory_order_release);
    });

    const auto entryDeadline =
        std::chrono::steady_clock::now()
        + std::chrono::milliseconds(kEntryWaitBudgetMilliseconds);
    while (!consumerEntered.load(std::memory_order_acquire)
           && std::chrono::steady_clock::now() < entryDeadline) {
        std::this_thread::yield();
    }

    const bool consumerEnteredBeforeStop = consumerEntered.load(std::memory_order_acquire);
    std::this_thread::sleep_for(
        std::chrono::milliseconds(kStopSchedulingAllowanceMilliseconds));
    const bool consumerWasWaitingBeforeStop = !consumerFinished.load(std::memory_order_acquire);
    const auto stopStartedAt = std::chrono::steady_clock::now();
    queue.stop();
    queue.stop();
    consumer.join();

    const std::int64_t enteredMilliseconds =
        consumerEnteredMilliseconds.load(std::memory_order_acquire);
    const std::int64_t stopMilliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(stopStartedAt - testStartedAt).count();
    const std::int64_t stopDelayMilliseconds = stopMilliseconds - enteredMilliseconds;
    const std::int64_t probeElapsed =
        probeElapsedMilliseconds.load(std::memory_order_acquire);
    const std::int64_t observedElapsed =
        observedElapsedMilliseconds.load(std::memory_order_acquire);

    qInfo() << "queue stop wake timeoutMs" << kConsumerTimeoutMilliseconds
            << "probeStatus" << probeStatus.load(std::memory_order_acquire)
            << "probeElapsedMs" << probeElapsed
            << "stopDelayMs" << stopDelayMilliseconds
            << "observedElapsedMs" << observedElapsed
            << "enteredBeforeStop" << consumerEnteredBeforeStop
            << "waitingBeforeStop" << consumerWasWaitingBeforeStop;

    QVERIFY(consumerEnteredBeforeStop);
    QVERIFY(consumerWasWaitingBeforeStop);
    QVERIFY(!invalidConsumerStatus.load(std::memory_order_acquire));
    QCOMPARE(preludePushStatus, AdaptiveDenoisingQueuePushStatus::Pushed);
    QCOMPARE(preludePopStatus, AdaptiveDenoisingQueuePopStatus::Popped);
    QCOMPARE(preludeDestination.rowCount, preludeBlock.rows());
    QCOMPARE(preludeDestination.sampleCount, preludeBlock.cols());
    QVERIFY(sameMetadata(preludeDestination.fiffInfo, preludeMetadata));
    for (Index row = 0; row < config.maxChannelCount; ++row) {
        for (Index column = 0; column < config.maxBlockSamples; ++column) {
            const bool inBlock = row < preludeBlock.rows() && column < preludeBlock.cols();
            const double expected = inBlock ? preludeBlock(row, column) : -4103.0;
            QVERIFY(preludeDestination.data(row, column) == expected);
        }
    }
    QCOMPARE(probeStatus.load(std::memory_order_acquire),
             static_cast<int>(AdaptiveDenoisingQueuePopStatus::Timeout));
    QVERIFY(probeElapsed >= kProbeLowerBoundMilliseconds);
    QVERIFY(probeElapsed < kProbeUpperBoundMilliseconds);
    QVERIFY(observedStatus.load(std::memory_order_acquire)
            == static_cast<int>(AdaptiveDenoisingQueuePopStatus::Stopped));
    QVERIFY(observedElapsed >= 0);
    QVERIFY(observedElapsed < kWakeUpperBoundMilliseconds);
    QVERIFY(destinationPreserves(
        consumerDestination, config.maxChannelCount, config.maxBlockSamples, -4001.0, 4001, 4002,
        consumerSentinelMetadata));

    MatrixXd stoppedBlock(2, 1);
    stoppedBlock(0, 0) = -101.0;
    stoppedBlock(1, 0) = 202.0;
    const std::uint64_t stoppedMetadataToken = 4;
    const NativeFiffInfoHandle stoppedMetadata =
        fakeFiffInfoHandle(stoppedMetadataToken);

    QVERIFY(queue.tryPush(stoppedBlock, stoppedMetadata)
            == AdaptiveDenoisingQueuePushStatus::Stopped);

    AdaptiveDenoisingQueuedBlock stoppedDestination;
    stoppedDestination.data.resize(config.maxChannelCount, config.maxBlockSamples);
    stoppedDestination.data.setConstant(-4003.0);
    stoppedDestination.rowCount = 4003;
    stoppedDestination.sampleCount = 4004;
    stoppedDestination.fiffInfo = consumerSentinelMetadata;
    QVERIFY(queue.waitPop(stoppedDestination, 0) == AdaptiveDenoisingQueuePopStatus::Stopped);
    QVERIFY(destinationPreserves(
        stoppedDestination, config.maxChannelCount, config.maxBlockSamples, -4003.0, 4003, 4004,
        consumerSentinelMetadata));

    QVERIFY(queue.configure(config) == AdaptiveDenoisingQueueConfigureStatus::Ready);

    MatrixXd freshBlock(2, 2);
    freshBlock(0, 0) = -7.0;
    freshBlock(0, 1) = 8.0;
    freshBlock(1, 0) = 70.0;
    freshBlock(1, 1) = 80.0;
    const MatrixXd expectedFreshBlock = freshBlock;

    const std::uint64_t freshMetadataToken = 5;
    const NativeFiffInfoHandle freshMetadata = fakeFiffInfoHandle(freshMetadataToken);
    QVERIFY(!sameMetadata(freshMetadata, stoppedMetadata));

    QVERIFY(queue.tryPush(freshBlock, freshMetadata)
            == AdaptiveDenoisingQueuePushStatus::Pushed);

    AdaptiveDenoisingQueuedBlock freshDestination;
    freshDestination.data.resize(config.maxChannelCount, config.maxBlockSamples);
    QVERIFY(queue.waitPop(freshDestination, 0) == AdaptiveDenoisingQueuePopStatus::Popped);
    QCOMPARE(freshDestination.rowCount, Index(2));
    QCOMPARE(freshDestination.sampleCount, Index(2));
    QVERIFY(sameMetadata(freshDestination.fiffInfo, freshMetadata));
    for (Index row = 0; row < expectedFreshBlock.rows(); ++row) {
        QVERIFY(rowEquals(freshDestination.data, expectedFreshBlock, row));
    }

    freshDestination.data.setConstant(-4005.0);
    freshDestination.rowCount = 4005;
    freshDestination.sampleCount = 4006;
    freshDestination.fiffInfo = consumerSentinelMetadata;
    QVERIFY(queue.waitPop(freshDestination, 0) == AdaptiveDenoisingQueuePopStatus::Timeout);
    QVERIFY(destinationPreserves(
        freshDestination, config.maxChannelCount, config.maxBlockSamples, -4005.0, 4005, 4006,
        consumerSentinelMetadata));
}

//=============================================================================================================

void TestAdaptiveDenoisingPlugin::queueRejectsInvalidInputsWithoutConsumingState()
{
    AdaptiveDenoisingBlockQueue queue;
    AdaptiveDenoisingBlockQueueConfig validConfig;
    validConfig.maxChannelCount = 2;
    validConfig.maxBlockSamples = 4;
    validConfig.capacity = 1;

    AdaptiveDenoisingBlockQueueConfig invalidConfig = validConfig;
    invalidConfig.maxChannelCount = 0;
    QVERIFY(queue.configure(invalidConfig)
            == AdaptiveDenoisingQueueConfigureStatus::InvalidConfiguration);

    invalidConfig = validConfig;
    invalidConfig.maxBlockSamples = 0;
    QVERIFY(queue.configure(invalidConfig)
            == AdaptiveDenoisingQueueConfigureStatus::InvalidConfiguration);

    invalidConfig = validConfig;
    invalidConfig.capacity = 0;
    QVERIFY(queue.configure(invalidConfig)
            == AdaptiveDenoisingQueueConfigureStatus::InvalidConfiguration);

    invalidConfig = validConfig;
    invalidConfig.capacity = static_cast<std::size_t>(std::numeric_limits<int>::max()) + 1U;
    QVERIFY(queue.configure(invalidConfig)
            == AdaptiveDenoisingQueueConfigureStatus::InvalidConfiguration);

    QVERIFY(queue.configure(validConfig) == AdaptiveDenoisingQueueConfigureStatus::Ready);
    queue.stop();

    invalidConfig = validConfig;
    invalidConfig.maxChannelCount = 0;
    QVERIFY(queue.configure(invalidConfig)
            == AdaptiveDenoisingQueueConfigureStatus::InvalidConfiguration);

    invalidConfig = validConfig;
    invalidConfig.maxBlockSamples = 0;
    QVERIFY(queue.configure(invalidConfig)
            == AdaptiveDenoisingQueueConfigureStatus::InvalidConfiguration);

    invalidConfig = validConfig;
    invalidConfig.capacity = 0;
    QVERIFY(queue.configure(invalidConfig)
            == AdaptiveDenoisingQueueConfigureStatus::InvalidConfiguration);

    invalidConfig = validConfig;
    invalidConfig.capacity = static_cast<std::size_t>(std::numeric_limits<int>::max()) + 1U;
    QVERIFY(queue.configure(invalidConfig)
            == AdaptiveDenoisingQueueConfigureStatus::InvalidConfiguration);

    QVERIFY(queue.configure(validConfig) == AdaptiveDenoisingQueueConfigureStatus::Ready);

    const NativeFiffInfoHandle nullMetadata;

    MatrixXd wrongRows = MatrixXd::Constant(3, 3, -1.0);
    MatrixXd zeroColumns(2, 0);
    MatrixXd tooManyColumns = MatrixXd::Constant(2, 5, -2.0);
    QVERIFY(queue.tryPush(wrongRows, nullMetadata)
            == AdaptiveDenoisingQueuePushStatus::InvalidBlock);
    QVERIFY(queue.tryPush(zeroColumns, nullMetadata)
            == AdaptiveDenoisingQueuePushStatus::InvalidBlock);
    QVERIFY(queue.tryPush(tooManyColumns, nullMetadata)
            == AdaptiveDenoisingQueuePushStatus::InvalidBlock);

    MatrixXd firstBlock(2, 3);
    firstBlock(0, 0) = 1.0;
    firstBlock(0, 1) = 2.0;
    firstBlock(0, 2) = 3.0;
    firstBlock(1, 0) = 11.0;
    firstBlock(1, 1) = 12.0;
    firstBlock(1, 2) = 13.0;
    const MatrixXd expectedFirstBlock = firstBlock;
    QVERIFY(queue.tryPush(firstBlock, nullMetadata)
            == AdaptiveDenoisingQueuePushStatus::Pushed);

    MatrixXd secondBlock(2, 2);
    secondBlock(0, 0) = 21.0;
    secondBlock(0, 1) = 22.0;
    secondBlock(1, 0) = 31.0;
    secondBlock(1, 1) = 32.0;
    QVERIFY(queue.tryPush(secondBlock, nullMetadata)
            == AdaptiveDenoisingQueuePushStatus::Full);

    AdaptiveDenoisingQueuedBlock wrongRowsDestination;
    wrongRowsDestination.data = MatrixXd::Constant(3, 4, -10.0);
    wrongRowsDestination.rowCount = 310;
    wrongRowsDestination.sampleCount = 310;
    const std::uint64_t wrongRowsMetadataToken = 310;
    wrongRowsDestination.fiffInfo = fakeFiffInfoHandle(wrongRowsMetadataToken);
    const MatrixXd expectedWrongRowsData = wrongRowsDestination.data;
    const auto expectedWrongRowsMetadata = wrongRowsDestination.fiffInfo;

    QVERIFY(queue.waitPop(wrongRowsDestination, 0)
            == AdaptiveDenoisingQueuePopStatus::InvalidDestination);
    QVERIFY(matrixEquals(wrongRowsDestination.data, expectedWrongRowsData));
    QCOMPARE(wrongRowsDestination.rowCount, Index(310));
    QCOMPARE(wrongRowsDestination.sampleCount, Index(310));
    QVERIFY(sameMetadata(wrongRowsDestination.fiffInfo, expectedWrongRowsMetadata));
    QVERIFY(wrongRowsDestination.fiffInfo.data() != nullptr);

    AdaptiveDenoisingQueuedBlock wrongColumnsDestination;
    wrongColumnsDestination.data = MatrixXd::Constant(2, 3, -20.0);
    wrongColumnsDestination.rowCount = 320;
    wrongColumnsDestination.sampleCount = 320;
    const std::uint64_t wrongColumnsMetadataToken = 320;
    wrongColumnsDestination.fiffInfo = fakeFiffInfoHandle(wrongColumnsMetadataToken);
    const MatrixXd expectedWrongColumnsData = wrongColumnsDestination.data;
    const auto expectedWrongColumnsMetadata = wrongColumnsDestination.fiffInfo;

    QVERIFY(queue.waitPop(wrongColumnsDestination, 0)
            == AdaptiveDenoisingQueuePopStatus::InvalidDestination);
    QVERIFY(matrixEquals(wrongColumnsDestination.data, expectedWrongColumnsData));
    QCOMPARE(wrongColumnsDestination.rowCount, Index(320));
    QCOMPARE(wrongColumnsDestination.sampleCount, Index(320));
    QVERIFY(sameMetadata(wrongColumnsDestination.fiffInfo, expectedWrongColumnsMetadata));
    QVERIFY(wrongColumnsDestination.fiffInfo.data() != nullptr);

    AdaptiveDenoisingQueuedBlock destination;
    destination.data = MatrixXd::Constant(2, 4, -30.0);
    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Popped);
    QCOMPARE(destination.rowCount, Index(2));
    QCOMPARE(destination.sampleCount, Index(3));
    QVERIFY(!destination.fiffInfo);
    for (Index row = 0; row < expectedFirstBlock.rows(); ++row) {
        QVERIFY(rowEquals(destination.data, expectedFirstBlock, row));
    }

    destination.data.setConstant(-3301.0);
    destination.rowCount = 3301;
    destination.sampleCount = 3302;
    const std::uint64_t timeoutMetadataToken = 3301;
    const NativeFiffInfoHandle timeoutMetadata = fakeFiffInfoHandle(timeoutMetadataToken);
    destination.fiffInfo = timeoutMetadata;
    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Timeout);
    QVERIFY(destinationPreserves(
        destination, validConfig.maxChannelCount, validConfig.maxBlockSamples, -3301.0, 3301, 3302,
        timeoutMetadata));
}

//=============================================================================================================

void TestAdaptiveDenoisingPlugin::queueAcceptsVariableRowsWithNativeMetadata()
{
    AdaptiveDenoisingBlockQueue queue;
    AdaptiveDenoisingBlockQueueConfig config;
    config.maxChannelCount = 4;
    config.maxBlockSamples = 4;
    config.capacity = 3;

    QVERIFY(queue.configure(config) == AdaptiveDenoisingQueueConfigureStatus::Ready);

    MatrixXd blockA(2, 3);
    blockA << 1.0, 2.0, 3.0,
              11.0, 12.0, 13.0;
    const MatrixXd originalA = blockA;

    MatrixXd blockB(4, 2);
    blockB << 21.0, 22.0,
              31.0, 32.0,
              41.0, 42.0,
              51.0, 52.0;
    const MatrixXd originalB = blockB;

    MatrixXd blockC(3, 4);
    blockC << 61.0, 62.0, 63.0, 64.0,
              71.0, 72.0, 73.0, 74.0,
              81.0, 82.0, 83.0, 84.0;
    const MatrixXd originalC = blockC;

    const std::uint64_t metadataTokenA = 101;
    const std::uint64_t metadataTokenB = 202;
    const std::uint64_t metadataTokenC = 303;
    const NativeFiffInfoHandle metadataA = fakeFiffInfoHandle(metadataTokenA);
    const NativeFiffInfoHandle metadataB = fakeFiffInfoHandle(metadataTokenB);
    const NativeFiffInfoHandle metadataC = fakeFiffInfoHandle(metadataTokenC);

    QVERIFY(metadataA.data() != nullptr);
    QVERIFY(metadataB.data() != nullptr);
    QVERIFY(metadataC.data() != nullptr);
    QVERIFY(metadataA.data() != metadataB.data());
    QVERIFY(metadataA.data() != metadataC.data());
    QVERIFY(metadataB.data() != metadataC.data());

    QVERIFY(queue.tryPush(blockA, metadataA) == AdaptiveDenoisingQueuePushStatus::Pushed);
    QVERIFY(queue.tryPush(blockB, metadataB) == AdaptiveDenoisingQueuePushStatus::Pushed);
    QVERIFY(queue.tryPush(blockC, metadataC) == AdaptiveDenoisingQueuePushStatus::Pushed);

    blockA.array() += 1000.0;
    blockB.array() += 2000.0;
    blockC.array() += 3000.0;

    const double sentinel = -9999.0;
    AdaptiveDenoisingQueuedBlock destination;
    destination.data.resize(config.maxChannelCount, config.maxBlockSamples);

    destination.data.setConstant(sentinel);
    destination.rowCount = -1;
    destination.sampleCount = -1;
    destination.fiffInfo.reset();
    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Popped);
    QCOMPARE(destination.rowCount, Index(2));
    QCOMPARE(destination.sampleCount, Index(3));
    QVERIFY(destination.fiffInfo.data() == metadataA.data());
    for (Index row = 0; row < config.maxChannelCount; ++row) {
        for (Index column = 0; column < config.maxBlockSamples; ++column) {
            if (row < originalA.rows() && column < originalA.cols()) {
                QVERIFY(destination.data(row, column) == originalA(row, column));
            } else {
                QVERIFY(destination.data(row, column) == sentinel);
            }
        }
    }

    destination.data.setConstant(sentinel);
    destination.rowCount = -1;
    destination.sampleCount = -1;
    destination.fiffInfo.reset();
    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Popped);
    QCOMPARE(destination.rowCount, Index(4));
    QCOMPARE(destination.sampleCount, Index(2));
    QVERIFY(destination.fiffInfo.data() == metadataB.data());
    for (Index row = 0; row < config.maxChannelCount; ++row) {
        for (Index column = 0; column < config.maxBlockSamples; ++column) {
            if (row < originalB.rows() && column < originalB.cols()) {
                QVERIFY(destination.data(row, column) == originalB(row, column));
            } else {
                QVERIFY(destination.data(row, column) == sentinel);
            }
        }
    }

    destination.data.setConstant(sentinel);
    destination.rowCount = -1;
    destination.sampleCount = -1;
    destination.fiffInfo.reset();
    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Popped);
    QCOMPARE(destination.rowCount, Index(3));
    QCOMPARE(destination.sampleCount, Index(4));
    QVERIFY(destination.fiffInfo.data() == metadataC.data());
    for (Index row = 0; row < config.maxChannelCount; ++row) {
        for (Index column = 0; column < config.maxBlockSamples; ++column) {
            if (row < originalC.rows() && column < originalC.cols()) {
                QVERIFY(destination.data(row, column) == originalC(row, column));
            } else {
                QVERIFY(destination.data(row, column) == sentinel);
            }
        }
    }

    const std::uint64_t timeoutMetadataToken = 3401;
    const NativeFiffInfoHandle timeoutMetadata = fakeFiffInfoHandle(timeoutMetadataToken);
    destination.data.setConstant(-3401.0);
    destination.rowCount = 3401;
    destination.sampleCount = 3402;
    destination.fiffInfo = timeoutMetadata;
    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Timeout);
    QVERIFY(destinationPreserves(
        destination, config.maxChannelCount, config.maxBlockSamples, -3401.0, 3401, 3402,
        timeoutMetadata));
}

//=============================================================================================================

void TestAdaptiveDenoisingPlugin::queueSustainedOverlappingSpscPreservesAcceptedStream()
{
    constexpr std::size_t kTrafficBlockCount = 512;
    constexpr Index kMaxChannelCount = 4;
    constexpr Index kMaxBlockSamples = 8;
    constexpr std::size_t kQueueCapacity = 8;
    constexpr double kPayloadBase = 1000000.0;
    constexpr double kPayloadStride = 10000.0;
    constexpr double kDestinationSentinel = -5101.0;
    constexpr Index kDestinationRowSentinel = 5101;
    constexpr Index kDestinationSampleSentinel = 5102;
    constexpr int kReadyBudgetMilliseconds = 1000;
    constexpr int kPhaseBudgetMilliseconds = 1000;
    constexpr int kProducerBudgetMilliseconds = 5000;
    constexpr int kConsumerBudgetMilliseconds = 2000;
    constexpr int kConsumerWaitMilliseconds = 25;
    constexpr std::int64_t kProducerCallUpperBoundNanoseconds = 1000000000LL;

    AdaptiveDenoisingBlockQueue queue;
    AdaptiveDenoisingBlockQueueConfig config;
    config.maxChannelCount = kMaxChannelCount;
    config.maxBlockSamples = kMaxBlockSamples;
    config.capacity = kQueueCapacity;

    std::array<MatrixXd, kTrafficBlockCount> producerBlocks;
    std::array<Index, kTrafficBlockCount> rowCounts{};
    std::array<Index, kTrafficBlockCount> sampleCounts{};
    std::array<FakeMetadataLifetime, kTrafficBlockCount> metadataLifetimes;
    std::array<NativeFiffInfoHandle, kTrafficBlockCount> metadataHandles;
    std::array<const FIFFLIB::FiffInfo*, kTrafficBlockCount> metadataPointers{};

    for (std::size_t index = 0; index < kTrafficBlockCount; ++index) {
        rowCounts[index] = Index(1) + static_cast<Index>(index % static_cast<std::size_t>(kMaxChannelCount));
        sampleCounts[index] =
            Index(1) + static_cast<Index>((index * 3U) % static_cast<std::size_t>(kMaxBlockSamples));
        if (index == kQueueCapacity) {
            rowCounts[index] = kMaxChannelCount;
            sampleCounts[index] = kMaxBlockSamples;
        }
        producerBlocks[index].resize(rowCounts[index], sampleCounts[index]);

        const double payloadBase = kPayloadBase + static_cast<double>(index) * kPayloadStride;
        for (Index row = 0; row < rowCounts[index]; ++row) {
            for (Index column = 0; column < sampleCounts[index]; ++column) {
                producerBlocks[index](row, column) =
                    payloadBase + static_cast<double>(row * 100 + column);
            }
        }

        metadataLifetimes[index].token = 510000U + static_cast<std::uint64_t>(index);
        metadataHandles[index] = fakeFiffInfoHandle(metadataLifetimes[index]);
        metadataPointers[index] = metadataHandles[index].data();
    }

    QVERIFY(queue.configure(config) == AdaptiveDenoisingQueueConfigureStatus::Ready);

    std::array<std::size_t, kTrafficBlockCount> acceptedSequences{};
    std::array<std::int64_t, kTrafficBlockCount> producerLatencies{};
    std::array<std::size_t, kTrafficBlockCount> poppedSequences{};
    std::array<Index, kTrafficBlockCount> poppedRowCounts{};
    std::array<Index, kTrafficBlockCount> poppedSampleCounts{};
    std::array<const FIFFLIB::FiffInfo*, kTrafficBlockCount> poppedMetadata{};
    std::array<bool, kTrafficBlockCount> seenSequences{};

    const std::uint64_t destinationSentinelToken = 5103;
    const NativeFiffInfoHandle destinationSentinel =
        fakeFiffInfoHandle(destinationSentinelToken);
    AdaptiveDenoisingQueuedBlock destination;
    destination.data.resize(kMaxChannelCount, kMaxBlockSamples);

    std::atomic<bool> producerReady(false);
    std::atomic<bool> consumerReady(false);
    std::atomic<bool> start(false);
    std::atomic<bool> abortRequested(false);
    std::atomic<bool> producerWaitingForFirstPop(false);
    std::atomic<bool> consumerFirstPopObserved(false);
    std::atomic<bool> consumerPoppedBeforeProducerCompletion(false);
    std::atomic<bool> wrappedPushObserved(false);
    std::atomic<bool> producerFinished(false);
    std::atomic<bool> consumerFinished(false);
    std::atomic<bool> producerInvalid(false);
    std::atomic<bool> consumerInvalid(false);
    std::atomic<std::size_t> acceptedCount(0);
    std::atomic<std::size_t> poppedCount(0);
    std::atomic<int> pushedCount(0);
    std::atomic<int> fullCount(0);

    std::thread producer([&] {
        producerReady.store(true, std::memory_order_release);
        const auto producerDeadline = std::chrono::steady_clock::now()
            + std::chrono::milliseconds(kProducerBudgetMilliseconds);
        while (!start.load(std::memory_order_acquire)
               && !abortRequested.load(std::memory_order_acquire)
               && std::chrono::steady_clock::now() < producerDeadline) {
            std::this_thread::yield();
        }

        std::size_t localAcceptedCount = 0;
        int localPushedCount = 0;
        int localFullCount = 0;

        if (!start.load(std::memory_order_acquire)
            || abortRequested.load(std::memory_order_acquire)) {
            if (!abortRequested.load(std::memory_order_acquire)) {
                producerInvalid.store(true, std::memory_order_release);
            }
        } else {
            for (std::size_t index = 0; index < kTrafficBlockCount; ++index) {
                if (abortRequested.load(std::memory_order_acquire)
                    || std::chrono::steady_clock::now() >= producerDeadline) {
                    if (!abortRequested.load(std::memory_order_acquire)) {
                        producerInvalid.store(true, std::memory_order_release);
                    }
                    break;
                }

                if (index == kQueueCapacity) {
                    producerWaitingForFirstPop.store(true, std::memory_order_release);
                    while (!consumerFirstPopObserved.load(std::memory_order_acquire)
                           && !abortRequested.load(std::memory_order_acquire)
                           && std::chrono::steady_clock::now() < producerDeadline) {
                        std::this_thread::yield();
                    }
                    if (!consumerFirstPopObserved.load(std::memory_order_acquire)) {
                        if (!abortRequested.load(std::memory_order_acquire)) {
                            producerInvalid.store(true, std::memory_order_release);
                        }
                        break;
                    }
                }

                const auto callStartedAt = std::chrono::steady_clock::now();
                // tryPush is noexcept; keep flag cleanup as the immediately following statement.
                g_countTestAllocations = true;
                const AdaptiveDenoisingQueuePushStatus status =
                    queue.tryPush(producerBlocks[index], metadataHandles[index]);
                g_countTestAllocations = false;
                const auto callFinishedAt = std::chrono::steady_clock::now();
                producerLatencies[index] =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        callFinishedAt - callStartedAt)
                        .count();

                if (status == AdaptiveDenoisingQueuePushStatus::Pushed) {
                    acceptedSequences[localAcceptedCount++] = index;
                    ++localPushedCount;
                    if (index == kQueueCapacity) {
                        wrappedPushObserved.store(true, std::memory_order_release);
                    }
                    metadataHandles[index].reset();
                } else if (status == AdaptiveDenoisingQueuePushStatus::Full) {
                    ++localFullCount;
                    metadataHandles[index].reset();
                } else if (status == AdaptiveDenoisingQueuePushStatus::Stopped) {
                    if (!abortRequested.load(std::memory_order_acquire)) {
                        producerInvalid.store(true, std::memory_order_release);
                    }
                    metadataHandles[index].reset();
                    break;
                } else {
                    producerInvalid.store(true, std::memory_order_release);
                    metadataHandles[index].reset();
                    break;
                }

                if ((index & 7U) == 0U) {
                    std::this_thread::yield();
                }
            }
        }

        for (std::size_t index = 0; index < kTrafficBlockCount; ++index) {
            metadataHandles[index].reset();
        }
        acceptedCount.store(localAcceptedCount, std::memory_order_release);
        pushedCount.store(localPushedCount, std::memory_order_release);
        fullCount.store(localFullCount, std::memory_order_release);
        producerFinished.store(true, std::memory_order_release);
    });

    std::thread consumer([&] {
        consumerReady.store(true, std::memory_order_release);
        const auto consumerDeadline = std::chrono::steady_clock::now()
            + std::chrono::milliseconds(
                kProducerBudgetMilliseconds + kConsumerBudgetMilliseconds + 1000);
        while (!start.load(std::memory_order_acquire)
               && !abortRequested.load(std::memory_order_acquire)
               && std::chrono::steady_clock::now() < consumerDeadline) {
            std::this_thread::yield();
        }

        std::size_t localPoppedCount = 0;
        bool producerDoneSeen = false;
        std::chrono::steady_clock::time_point drainDeadline;

        while (!producerWaitingForFirstPop.load(std::memory_order_acquire)
               && !abortRequested.load(std::memory_order_acquire)
               && std::chrono::steady_clock::now() < consumerDeadline) {
            std::this_thread::yield();
        }

        if (!start.load(std::memory_order_acquire)
            || !producerWaitingForFirstPop.load(std::memory_order_acquire)) {
            if (!abortRequested.load(std::memory_order_acquire)) {
                consumerInvalid.store(true, std::memory_order_release);
            }
            poppedCount.store(0, std::memory_order_release);
            consumerFinished.store(true, std::memory_order_release);
            return;
        }

        while (true) {
            if (std::chrono::steady_clock::now() >= consumerDeadline) {
                consumerInvalid.store(true, std::memory_order_release);
                break;
            }

            destination.data.setConstant(kDestinationSentinel);
            destination.rowCount = kDestinationRowSentinel;
            destination.sampleCount = kDestinationSampleSentinel;
            destination.fiffInfo = destinationSentinel;

                const AdaptiveDenoisingQueuePopStatus status =
                queue.waitPop(destination, kConsumerWaitMilliseconds);

            if (status == AdaptiveDenoisingQueuePopStatus::Popped) {
                if (localPoppedCount == 0) {
                    consumerFirstPopObserved.store(true, std::memory_order_release);
                    if (!producerFinished.load(std::memory_order_acquire)) {
                        consumerPoppedBeforeProducerCompletion.store(
                            true, std::memory_order_release);
                    }
                }

                std::size_t sequence = kTrafficBlockCount;
                for (std::size_t candidate = 0; candidate < kTrafficBlockCount; ++candidate) {
                    if (destination.data(0, 0)
                        == kPayloadBase + static_cast<double>(candidate) * kPayloadStride) {
                        sequence = candidate;
                        break;
                    }
                }

                bool blockValid = sequence < kTrafficBlockCount
                    && !seenSequences[sequence]
                    && destination.rowCount == rowCounts[sequence]
                    && destination.sampleCount == sampleCounts[sequence]
                    && destination.fiffInfo.data() == metadataPointers[sequence]
                    && destination.data.rows() == kMaxChannelCount
                    && destination.data.cols() == kMaxBlockSamples;

                if (blockValid) {
                    for (Index row = 0; row < kMaxChannelCount; ++row) {
                        for (Index column = 0; column < kMaxBlockSamples; ++column) {
                            const bool inBlock = row < rowCounts[sequence]
                                && column < sampleCounts[sequence];
                            const double expected = inBlock
                                ? producerBlocks[sequence](row, column)
                                : kDestinationSentinel;
                            if (destination.data(row, column) != expected) {
                                blockValid = false;
                            }
                        }
                    }
                }

                if (!blockValid || localPoppedCount >= kTrafficBlockCount) {
                    consumerInvalid.store(true, std::memory_order_release);
                } else {
                    seenSequences[sequence] = true;
                    poppedSequences[localPoppedCount] = sequence;
                    poppedRowCounts[localPoppedCount] = destination.rowCount;
                    poppedSampleCounts[localPoppedCount] = destination.sampleCount;
                    poppedMetadata[localPoppedCount] = destination.fiffInfo.data();
                    ++localPoppedCount;
                }

                destination.fiffInfo.reset();
                continue;
            }

            if (status == AdaptiveDenoisingQueuePopStatus::Timeout) {
                if (!destinationPreserves(
                        destination,
                        kMaxChannelCount,
                        kMaxBlockSamples,
                        kDestinationSentinel,
                        kDestinationRowSentinel,
                        kDestinationSampleSentinel,
                        destinationSentinel)) {
                    consumerInvalid.store(true, std::memory_order_release);
                }

                if (!producerFinished.load(std::memory_order_acquire)) {
                    continue;
                }

                if (!producerDoneSeen) {
                    producerDoneSeen = true;
                    drainDeadline = std::chrono::steady_clock::now()
                        + std::chrono::milliseconds(kConsumerBudgetMilliseconds);
                }
                if (std::chrono::steady_clock::now() >= drainDeadline) {
                    break;
                }
                continue;
            }

            if (!abortRequested.load(std::memory_order_acquire)) {
                consumerInvalid.store(true, std::memory_order_release);
            }
            break;
        }

        poppedCount.store(localPoppedCount, std::memory_order_release);
        consumerFinished.store(true, std::memory_order_release);
    });

    const bool producerReadyObserved = waitUntil(
        [&] { return producerReady.load(std::memory_order_acquire); },
        kReadyBudgetMilliseconds);
    const bool consumerReadyObserved = waitUntil(
        [&] { return consumerReady.load(std::memory_order_acquire); },
        kReadyBudgetMilliseconds);
    g_countedTestAllocations.store(0, std::memory_order_relaxed);
    start.store(true, std::memory_order_release);

    const bool producerHoldingObserved = waitUntil(
        [&] { return producerWaitingForFirstPop.load(std::memory_order_acquire); },
        kPhaseBudgetMilliseconds);
    const bool consumerFirstPopObservedWithinBudget = waitUntil(
        [&] { return consumerFirstPopObserved.load(std::memory_order_acquire); },
        kPhaseBudgetMilliseconds);
    const bool wrappedPushObservedWithinBudget = waitUntil(
        [&] { return wrappedPushObserved.load(std::memory_order_acquire); },
        kPhaseBudgetMilliseconds);
    if (!producerHoldingObserved
        || !consumerFirstPopObservedWithinBudget
        || !wrappedPushObservedWithinBudget) {
        abortRequested.store(true, std::memory_order_release);
        queue.stop();
    }

    const bool producerFinishedWithinBudget = waitUntil(
        [&] { return producerFinished.load(std::memory_order_acquire); },
        kProducerBudgetMilliseconds);
    if (!producerFinishedWithinBudget) {
        abortRequested.store(true, std::memory_order_release);
        queue.stop();
    }
    producer.join();

    const bool consumerFinishedWithinBudget = waitUntil(
        [&] { return consumerFinished.load(std::memory_order_acquire); },
        kConsumerBudgetMilliseconds + kConsumerWaitMilliseconds + 1000);
    if (!consumerFinishedWithinBudget) {
        abortRequested.store(true, std::memory_order_release);
        queue.stop();
    }
    consumer.join();

    const std::uint64_t countedAllocations =
        g_countedTestAllocations.load(std::memory_order_acquire);
    const std::size_t accepted = acceptedCount.load(std::memory_order_acquire);
    const std::size_t popped = poppedCount.load(std::memory_order_acquire);
    const int pushed = pushedCount.load(std::memory_order_acquire);
    const int full = fullCount.load(std::memory_order_acquire);

    std::int64_t maximumProducerLatency = 0;
    for (std::size_t index = 0; index < kTrafficBlockCount; ++index) {
        if (producerLatencies[index] > maximumProducerLatency) {
            maximumProducerLatency = producerLatencies[index];
        }
    }

    qInfo() << "queue SPSC traffic pushed" << pushed
            << "full" << full
            << "popped" << static_cast<qulonglong>(popped)
            << "maxProducerCallNs" << static_cast<qlonglong>(maximumProducerLatency)
            << "countedProducerAllocations" << static_cast<qulonglong>(countedAllocations);

    QVERIFY(producerReadyObserved);
    QVERIFY(consumerReadyObserved);
    QVERIFY(producerFinishedWithinBudget);
    QVERIFY(consumerFinishedWithinBudget);
    QVERIFY(!producerInvalid.load(std::memory_order_acquire));
    QVERIFY(!consumerInvalid.load(std::memory_order_acquire));
    QVERIFY(producerHoldingObserved);
    QVERIFY(consumerFirstPopObservedWithinBudget);
    QVERIFY(consumerPoppedBeforeProducerCompletion.load(std::memory_order_acquire));
    QVERIFY(wrappedPushObservedWithinBudget);
    QCOMPARE(pushed + full, static_cast<int>(kTrafficBlockCount));
    QCOMPARE(accepted, static_cast<std::size_t>(pushed));
    QVERIFY(pushed > static_cast<int>(kQueueCapacity));
    QVERIFY(accepted > kQueueCapacity);
    QCOMPARE(popped, accepted);
    QVERIFY(maximumProducerLatency >= 0);
    QVERIFY(maximumProducerLatency < kProducerCallUpperBoundNanoseconds);
    QCOMPARE(countedAllocations, std::uint64_t(0));

    for (std::size_t index = 0; index < kQueueCapacity; ++index) {
        QCOMPARE(acceptedSequences[index], index);
    }
    QCOMPARE(acceptedSequences[kQueueCapacity], kQueueCapacity);
    QVERIFY(rowCounts[kQueueCapacity] != rowCounts[0]
            || sampleCounts[kQueueCapacity] != sampleCounts[0]);
    QVERIFY(metadataPointers[kQueueCapacity] != metadataPointers[0]);

    for (std::size_t position = 0; position < accepted; ++position) {
        const std::size_t expectedSequence = acceptedSequences[position];
        QCOMPARE(poppedSequences[position], expectedSequence);
        QCOMPARE(poppedRowCounts[position], rowCounts[expectedSequence]);
        QCOMPARE(poppedSampleCounts[position], sampleCounts[expectedSequence]);
        QVERIFY(poppedMetadata[position] == metadataPointers[expectedSequence]);
    }
}

//=============================================================================================================

void TestAdaptiveDenoisingPlugin::queueStopRacesActiveProducerAndConsumer()
{
    constexpr int kReadyBudgetMilliseconds = 1000;
    constexpr int kPhaseBudgetMilliseconds = 1000;
    constexpr int kEmptyWaitMilliseconds = 50;
    constexpr int kEmptyWaitLowerBoundMilliseconds = 5;
    constexpr int kEmptyWaitUpperBoundMilliseconds = 1000;
    constexpr int kFirstWaitMilliseconds = 1000;
    constexpr int kStopWaitMilliseconds = 2000;
    constexpr int kStopWaitLowerBoundMilliseconds = 5;
    constexpr int kThreadBudgetMilliseconds = 4000;
    constexpr int kStopSchedulingAllowanceMilliseconds = 20;
    constexpr int kStopToJoinUpperBoundMilliseconds = 1500;

    AdaptiveDenoisingBlockQueue queue;
    AdaptiveDenoisingBlockQueueConfig config;
    config.maxChannelCount = 3;
    config.maxBlockSamples = 5;
    config.capacity = 1;

    QVERIFY(queue.configure(config) == AdaptiveDenoisingQueueConfigureStatus::Ready);

    MatrixXd preludeBlock(2, 3);
    preludeBlock << 6101.0, 6102.0, 6103.0,
                    6201.0, 6202.0, 6203.0;
    const NativeFiffInfoHandle preludeMetadata = fakeFiffInfoHandle(std::uint64_t(6104));
    const double preludeDestinationSentinel = -6101.0;
    const Index preludeDestinationRowSentinel = 6101;
    const Index preludeDestinationSampleSentinel = 6102;
    const NativeFiffInfoHandle preludeDestinationMetadata =
        fakeFiffInfoHandle(std::uint64_t(6103));

    AdaptiveDenoisingQueuedBlock preludeDestination;
    preludeDestination.data.resize(config.maxChannelCount, config.maxBlockSamples);
    preludeDestination.data.setConstant(preludeDestinationSentinel);
    preludeDestination.rowCount = preludeDestinationRowSentinel;
    preludeDestination.sampleCount = preludeDestinationSampleSentinel;
    preludeDestination.fiffInfo = preludeDestinationMetadata;

    const AdaptiveDenoisingQueuePushStatus preludePushStatus =
        queue.tryPush(preludeBlock, preludeMetadata);
    const AdaptiveDenoisingQueuePopStatus preludePopStatus =
        queue.waitPop(preludeDestination, 0);

    MatrixXd emptyDestinationData =
        MatrixXd::Constant(config.maxChannelCount, config.maxBlockSamples, -6111.0);
    AdaptiveDenoisingQueuedBlock emptyDestination;
    emptyDestination.data = emptyDestinationData;
    emptyDestination.rowCount = 6111;
    emptyDestination.sampleCount = 6112;
    const NativeFiffInfoHandle emptyDestinationMetadata =
        fakeFiffInfoHandle(std::uint64_t(6113));
    emptyDestination.fiffInfo = emptyDestinationMetadata;
    const auto emptyWaitStartedAt = std::chrono::steady_clock::now();
    const AdaptiveDenoisingQueuePopStatus emptyWaitStatus =
        queue.waitPop(emptyDestination, kEmptyWaitMilliseconds);
    const auto emptyWaitFinishedAt = std::chrono::steady_clock::now();
    const std::int64_t emptyWaitElapsedMilliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            emptyWaitFinishedAt - emptyWaitStartedAt)
            .count();

    MatrixXd producerBlock(3, 2);
    producerBlock << 6301.0, 6302.0,
                     6401.0, 6402.0,
                     6501.0, 6502.0;
    const NativeFiffInfoHandle producerMetadata = fakeFiffInfoHandle(std::uint64_t(6303));
    const double activeDestinationSentinel = -6201.0;
    const Index activeDestinationRowSentinel = 6201;
    const Index activeDestinationSampleSentinel = 6202;
    const NativeFiffInfoHandle activeDestinationMetadata =
        fakeFiffInfoHandle(std::uint64_t(6203));
    const double stoppedDestinationSentinel = -6301.0;
    const Index stoppedDestinationRowSentinel = 6301;
    const Index stoppedDestinationSampleSentinel = 6302;
    const NativeFiffInfoHandle stoppedDestinationMetadata =
        fakeFiffInfoHandle(std::uint64_t(6303));

    AdaptiveDenoisingQueuedBlock consumerDestination;
    consumerDestination.data.resize(config.maxChannelCount, config.maxBlockSamples);
    MatrixXd poppedActiveSnapshot =
        MatrixXd::Constant(config.maxChannelCount, config.maxBlockSamples, 0.0);

    const auto destinationContainsBlock = [&](const AdaptiveDenoisingQueuedBlock& destination,
                                               const MatrixXd& expectedBlock,
                                               const NativeFiffInfoHandle& expectedMetadata,
                                               double expectedTail) {
        if (destination.data.rows() != config.maxChannelCount
            || destination.data.cols() != config.maxBlockSamples
            || destination.rowCount != expectedBlock.rows()
            || destination.sampleCount != expectedBlock.cols()
            || !sameMetadata(destination.fiffInfo, expectedMetadata)) {
            return false;
        }

        for (Index row = 0; row < config.maxChannelCount; ++row) {
            for (Index column = 0; column < config.maxBlockSamples; ++column) {
                const bool inBlock = row < expectedBlock.rows() && column < expectedBlock.cols();
                const double expected = inBlock ? expectedBlock(row, column) : expectedTail;
                if (destination.data(row, column) != expected) {
                    return false;
                }
            }
        }

        return true;
    };

    std::atomic<bool> producerReady(false);
    std::atomic<bool> consumerReady(false);
    std::atomic<bool> start(false);
    std::atomic<bool> stopCompleted(false);
    std::atomic<bool> producerLive(false);
    std::atomic<bool> producerFinished(false);
    std::atomic<bool> producerPushedObserved(false);
    std::atomic<bool> producerStoppedObserved(false);
    std::atomic<bool> consumerFirstWaitStarted(false);
    std::atomic<bool> consumerPoppedObserved(false);
    std::atomic<bool> consumerStopWaitStarted(false);
    std::atomic<bool> consumerStoppedObserved(false);
    std::atomic<bool> consumerFinished(false);
    std::atomic<bool> invalidProducerStatus(false);
    std::atomic<bool> invalidConsumerStatus(false);
    std::atomic<int> producerFirstStatus(-1);
    std::atomic<int> producerStoppedStatus(-1);
    std::atomic<int> producerAttempts(0);
    std::atomic<int> consumerFirstStatus(-1);
    std::atomic<int> consumerStoppedStatus(-1);
    std::atomic<std::int64_t> consumerStoppedElapsedMilliseconds(-1);
    std::atomic<int> consumerPopped(0);
    std::atomic<int> consumerTimeout(0);
    std::atomic<Index> consumerPoppedRows(0);
    std::atomic<Index> consumerPoppedSamples(0);
    std::atomic<const FIFFLIB::FiffInfo*> consumerPoppedMetadata(nullptr);

    std::thread consumer([&] {
        consumerReady.store(true, std::memory_order_release);
        const auto consumerDeadline = std::chrono::steady_clock::now()
            + std::chrono::milliseconds(kThreadBudgetMilliseconds);
        while (!start.load(std::memory_order_acquire)
               && std::chrono::steady_clock::now() < consumerDeadline) {
            std::this_thread::yield();
        }

        if (!start.load(std::memory_order_acquire)) {
            invalidConsumerStatus.store(true, std::memory_order_release);
            consumerFinished.store(true, std::memory_order_release);
            return;
        }

        consumerDestination.data.setConstant(activeDestinationSentinel);
        consumerDestination.rowCount = activeDestinationRowSentinel;
        consumerDestination.sampleCount = activeDestinationSampleSentinel;
        consumerDestination.fiffInfo = activeDestinationMetadata;
        consumerFirstWaitStarted.store(true, std::memory_order_release);
        const AdaptiveDenoisingQueuePopStatus firstStatus =
            queue.waitPop(consumerDestination, kFirstWaitMilliseconds);
        consumerFirstStatus.store(static_cast<int>(firstStatus), std::memory_order_release);

        if (firstStatus == AdaptiveDenoisingQueuePopStatus::Popped) {
            if (!destinationContainsBlock(
                    consumerDestination, producerBlock, producerMetadata, activeDestinationSentinel)) {
                invalidConsumerStatus.store(true, std::memory_order_release);
            } else {
                for (Index row = 0; row < config.maxChannelCount; ++row) {
                    for (Index column = 0; column < config.maxBlockSamples; ++column) {
                        poppedActiveSnapshot(row, column) = consumerDestination.data(row, column);
                    }
                }
                consumerPoppedRows.store(consumerDestination.rowCount, std::memory_order_release);
                consumerPoppedSamples.store(
                    consumerDestination.sampleCount, std::memory_order_release);
                consumerPoppedMetadata.store(
                    consumerDestination.fiffInfo.data(), std::memory_order_release);
                consumerPopped.fetch_add(1, std::memory_order_acq_rel);
                consumerPoppedObserved.store(true, std::memory_order_release);
            }
        } else if (firstStatus == AdaptiveDenoisingQueuePopStatus::Timeout) {
            if (!destinationPreserves(
                    consumerDestination,
                    config.maxChannelCount,
                    config.maxBlockSamples,
                    activeDestinationSentinel,
                    activeDestinationRowSentinel,
                    activeDestinationSampleSentinel,
                    activeDestinationMetadata)) {
                invalidConsumerStatus.store(true, std::memory_order_release);
            }
            consumerTimeout.fetch_add(1, std::memory_order_acq_rel);
        } else if (firstStatus == AdaptiveDenoisingQueuePopStatus::Stopped) {
            if (!destinationPreserves(
                    consumerDestination,
                    config.maxChannelCount,
                    config.maxBlockSamples,
                    activeDestinationSentinel,
                    activeDestinationRowSentinel,
                    activeDestinationSampleSentinel,
                    activeDestinationMetadata)) {
                invalidConsumerStatus.store(true, std::memory_order_release);
            }
        } else {
            invalidConsumerStatus.store(true, std::memory_order_release);
        }

        consumerDestination.fiffInfo.reset();
        consumerStopWaitStarted.store(true, std::memory_order_release);
        while (std::chrono::steady_clock::now() < consumerDeadline) {
            consumerDestination.data.setConstant(stoppedDestinationSentinel);
            consumerDestination.rowCount = stoppedDestinationRowSentinel;
            consumerDestination.sampleCount = stoppedDestinationSampleSentinel;
            consumerDestination.fiffInfo = stoppedDestinationMetadata;
            const auto stoppedWaitStartedAt = std::chrono::steady_clock::now();
            const AdaptiveDenoisingQueuePopStatus status =
                queue.waitPop(consumerDestination, kStopWaitMilliseconds);
            const auto stoppedWaitFinishedAt = std::chrono::steady_clock::now();
            consumerStoppedStatus.store(static_cast<int>(status), std::memory_order_release);

            if (status == AdaptiveDenoisingQueuePopStatus::Stopped) {
                consumerStoppedElapsedMilliseconds.store(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        stoppedWaitFinishedAt - stoppedWaitStartedAt)
                        .count(),
                    std::memory_order_release);
                if (!destinationPreserves(
                        consumerDestination,
                        config.maxChannelCount,
                        config.maxBlockSamples,
                        stoppedDestinationSentinel,
                        stoppedDestinationRowSentinel,
                        stoppedDestinationSampleSentinel,
                        stoppedDestinationMetadata)) {
                    invalidConsumerStatus.store(true, std::memory_order_release);
                }
                consumerStoppedObserved.store(true, std::memory_order_release);
                break;
            }

            if (status == AdaptiveDenoisingQueuePopStatus::Timeout) {
                if (!destinationPreserves(
                        consumerDestination,
                        config.maxChannelCount,
                        config.maxBlockSamples,
                        stoppedDestinationSentinel,
                        stoppedDestinationRowSentinel,
                        stoppedDestinationSampleSentinel,
                        stoppedDestinationMetadata)) {
                    invalidConsumerStatus.store(true, std::memory_order_release);
                }
                consumerTimeout.fetch_add(1, std::memory_order_acq_rel);
                continue;
            }

            invalidConsumerStatus.store(true, std::memory_order_release);
            break;
        }

        if (!consumerStoppedObserved.load(std::memory_order_acquire)) {
            invalidConsumerStatus.store(true, std::memory_order_release);
        }
        consumerFinished.store(true, std::memory_order_release);
    });

    std::thread producer([&] {
        producerReady.store(true, std::memory_order_release);
        const auto producerDeadline = std::chrono::steady_clock::now()
            + std::chrono::milliseconds(kThreadBudgetMilliseconds);
        while (!start.load(std::memory_order_acquire)
               && std::chrono::steady_clock::now() < producerDeadline) {
            std::this_thread::yield();
        }

        if (!start.load(std::memory_order_acquire)) {
            invalidProducerStatus.store(true, std::memory_order_release);
            producerFinished.store(true, std::memory_order_release);
            return;
        }

        producerLive.store(true, std::memory_order_release);
        while (!consumerFirstWaitStarted.load(std::memory_order_acquire)
               && !stopCompleted.load(std::memory_order_acquire)
               && std::chrono::steady_clock::now() < producerDeadline) {
            std::this_thread::yield();
        }

        producerAttempts.fetch_add(1, std::memory_order_acq_rel);
        const AdaptiveDenoisingQueuePushStatus firstStatus =
            queue.tryPush(producerBlock, producerMetadata);
        producerFirstStatus.store(static_cast<int>(firstStatus), std::memory_order_release);
        if (firstStatus == AdaptiveDenoisingQueuePushStatus::Pushed) {
            producerPushedObserved.store(true, std::memory_order_release);
        } else {
            if (firstStatus != AdaptiveDenoisingQueuePushStatus::Stopped
                || !stopCompleted.load(std::memory_order_acquire)) {
                invalidProducerStatus.store(true, std::memory_order_release);
            }
        }

        while (!stopCompleted.load(std::memory_order_acquire)
               && std::chrono::steady_clock::now() < producerDeadline) {
            std::this_thread::yield();
        }

        if (!stopCompleted.load(std::memory_order_acquire)) {
            queue.stop();
            stopCompleted.store(true, std::memory_order_release);
        }

        producerAttempts.fetch_add(1, std::memory_order_acq_rel);
        const AdaptiveDenoisingQueuePushStatus stoppedStatus =
            queue.tryPush(producerBlock, producerMetadata);
        producerStoppedStatus.store(static_cast<int>(stoppedStatus), std::memory_order_release);
        if (stoppedStatus == AdaptiveDenoisingQueuePushStatus::Stopped) {
            producerStoppedObserved.store(true, std::memory_order_release);
        } else {
            invalidProducerStatus.store(true, std::memory_order_release);
        }
        producerLive.store(false, std::memory_order_release);
        producerFinished.store(true, std::memory_order_release);
    });

    const bool consumerReadyObserved = waitUntil(
        [&] { return consumerReady.load(std::memory_order_acquire); }, 1000);
    const bool producerReadyObserved = waitUntil(
        [&] { return producerReady.load(std::memory_order_acquire); }, 1000);
    start.store(true, std::memory_order_release);

    const bool producerLiveObserved = waitUntil(
        [&] { return producerLive.load(std::memory_order_acquire); }, kPhaseBudgetMilliseconds);
    const bool consumerFirstWaitObserved = waitUntil(
        [&] { return consumerFirstWaitStarted.load(std::memory_order_acquire); },
        kPhaseBudgetMilliseconds);
    const bool producerPushObserved = waitUntil(
        [&] { return producerPushedObserved.load(std::memory_order_acquire); },
        kPhaseBudgetMilliseconds);
    const bool consumerPopObserved = waitUntil(
        [&] { return consumerPoppedObserved.load(std::memory_order_acquire); },
        kPhaseBudgetMilliseconds);
    const bool consumerStopWaitObserved = waitUntil(
        [&] { return consumerStopWaitStarted.load(std::memory_order_acquire); },
        kPhaseBudgetMilliseconds);
    const bool stopPhaseReady = producerLiveObserved
        && consumerFirstWaitObserved
        && producerPushObserved
        && consumerPopObserved
        && consumerStopWaitObserved;

    std::this_thread::sleep_for(std::chrono::milliseconds(kStopSchedulingAllowanceMilliseconds));
    const bool producerWasLiveAtStop =
        producerLive.load(std::memory_order_acquire)
        && !producerFinished.load(std::memory_order_acquire);
    const bool consumerWasWaitingAtStop =
        consumerStopWaitStarted.load(std::memory_order_acquire)
        && !consumerFinished.load(std::memory_order_acquire);
    const auto stopStartedAt = std::chrono::steady_clock::now();
    queue.stop();
    stopCompleted.store(true, std::memory_order_release);

    const bool producerStoppedObservedWithinBudget = waitUntil(
        [&] { return producerStoppedObserved.load(std::memory_order_acquire); },
        kPhaseBudgetMilliseconds);
    const bool consumerStoppedObservedWithinBudget = waitUntil(
        [&] { return consumerStoppedObserved.load(std::memory_order_acquire); },
        kPhaseBudgetMilliseconds);
    const bool producerFinishedWithinBudget = waitUntil(
        [&] { return producerFinished.load(std::memory_order_acquire); },
        kPhaseBudgetMilliseconds);
    if (!producerFinishedWithinBudget) {
        queue.stop();
        stopCompleted.store(true, std::memory_order_release);
    }
    const bool consumerFinishedWithinBudget = waitUntil(
        [&] { return consumerFinished.load(std::memory_order_acquire); },
        kPhaseBudgetMilliseconds);
    if (!consumerFinishedWithinBudget) {
        queue.stop();
        stopCompleted.store(true, std::memory_order_release);
    }
    producer.join();
    consumer.join();
    const auto threadsJoinedAt = std::chrono::steady_clock::now();
    const std::int64_t stopToJoinMilliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(threadsJoinedAt - stopStartedAt)
            .count();

    const int attempts = producerAttempts.load(std::memory_order_acquire);
    const int popped = consumerPopped.load(std::memory_order_acquire);
    const int timeout = consumerTimeout.load(std::memory_order_acquire);
    const AdaptiveDenoisingQueuePushStatus observedFirstPushStatus =
        static_cast<AdaptiveDenoisingQueuePushStatus>(
            producerFirstStatus.load(std::memory_order_acquire));
    const AdaptiveDenoisingQueuePushStatus observedStoppedPushStatus =
        static_cast<AdaptiveDenoisingQueuePushStatus>(
            producerStoppedStatus.load(std::memory_order_acquire));
    const AdaptiveDenoisingQueuePopStatus observedFirstPopStatus =
        static_cast<AdaptiveDenoisingQueuePopStatus>(
            consumerFirstStatus.load(std::memory_order_acquire));
    const AdaptiveDenoisingQueuePopStatus observedStoppedPopStatus =
        static_cast<AdaptiveDenoisingQueuePopStatus>(
            consumerStoppedStatus.load(std::memory_order_acquire));
    const std::int64_t stoppedWaitElapsedMilliseconds =
        consumerStoppedElapsedMilliseconds.load(std::memory_order_acquire);

    qInfo() << "queue active stop attempts" << attempts
            << "preludePush" << static_cast<int>(preludePushStatus)
            << "preludePop" << static_cast<int>(preludePopStatus)
            << "emptyWaitMs" << emptyWaitElapsedMilliseconds
            << "firstPush" << static_cast<int>(observedFirstPushStatus)
            << "firstPop" << static_cast<int>(observedFirstPopStatus)
            << "stoppedPush" << static_cast<int>(observedStoppedPushStatus)
            << "stoppedPop" << static_cast<int>(observedStoppedPopStatus)
            << "stoppedWaitMs" << stoppedWaitElapsedMilliseconds
            << "popped" << popped
            << "timeout" << timeout
            << "stopToJoinMs" << stopToJoinMilliseconds;

    QVERIFY(consumerReadyObserved);
    QVERIFY(producerReadyObserved);
    QVERIFY(stopPhaseReady);
    QVERIFY(producerWasLiveAtStop);
    QVERIFY(consumerWasWaitingAtStop);
    QVERIFY(producerStoppedObservedWithinBudget);
    QVERIFY(consumerStoppedObservedWithinBudget);
    QVERIFY(producerFinishedWithinBudget);
    QVERIFY(consumerFinishedWithinBudget);
    QVERIFY(!invalidProducerStatus.load(std::memory_order_acquire));
    QVERIFY(!invalidConsumerStatus.load(std::memory_order_acquire));
    QCOMPARE(preludePushStatus, AdaptiveDenoisingQueuePushStatus::Pushed);
    QCOMPARE(preludePopStatus, AdaptiveDenoisingQueuePopStatus::Popped);
    QCOMPARE(emptyWaitStatus, AdaptiveDenoisingQueuePopStatus::Timeout);
    QVERIFY(emptyWaitElapsedMilliseconds >= kEmptyWaitLowerBoundMilliseconds);
    QVERIFY(emptyWaitElapsedMilliseconds < kEmptyWaitUpperBoundMilliseconds);
    QVERIFY(destinationContainsBlock(
        preludeDestination, preludeBlock, preludeMetadata, preludeDestinationSentinel));
    QVERIFY(destinationPreserves(
        emptyDestination,
        config.maxChannelCount,
        config.maxBlockSamples,
        -6111.0,
        6111,
        6112,
        emptyDestinationMetadata));
    QCOMPARE(observedFirstPushStatus, AdaptiveDenoisingQueuePushStatus::Pushed);
    QCOMPARE(observedFirstPopStatus, AdaptiveDenoisingQueuePopStatus::Popped);
    QCOMPARE(observedStoppedPushStatus, AdaptiveDenoisingQueuePushStatus::Stopped);
    QCOMPARE(observedStoppedPopStatus, AdaptiveDenoisingQueuePopStatus::Stopped);
    QVERIFY(stoppedWaitElapsedMilliseconds >= kStopWaitLowerBoundMilliseconds);
    QVERIFY(stoppedWaitElapsedMilliseconds < kStopToJoinUpperBoundMilliseconds);
    QCOMPARE(attempts, 2);
    QCOMPARE(popped, 1);
    QCOMPARE(consumerTimeout.load(std::memory_order_acquire), 0);
    QCOMPARE(consumerPoppedRows.load(std::memory_order_acquire), producerBlock.rows());
    QCOMPARE(consumerPoppedSamples.load(std::memory_order_acquire), producerBlock.cols());
    QVERIFY(consumerPoppedMetadata.load(std::memory_order_acquire) == producerMetadata.data());
    QVERIFY(destinationContainsBlock(
        AdaptiveDenoisingQueuedBlock{poppedActiveSnapshot,
                                     producerBlock.rows(),
                                     producerBlock.cols(),
                                     producerMetadata},
        producerBlock,
        producerMetadata,
        activeDestinationSentinel));
    QVERIFY(destinationPreserves(
        consumerDestination,
        config.maxChannelCount,
        config.maxBlockSamples,
        stoppedDestinationSentinel,
        stoppedDestinationRowSentinel,
        stoppedDestinationSampleSentinel,
        stoppedDestinationMetadata));
    QVERIFY(stopToJoinMilliseconds >= 0);
    QVERIFY(stopToJoinMilliseconds < kStopToJoinUpperBoundMilliseconds);
}

//=============================================================================================================

void TestAdaptiveDenoisingPlugin::queueStopsWithPendingBlockAndPreservesDestination()
{
    AdaptiveDenoisingBlockQueue queue;
    AdaptiveDenoisingBlockQueueConfig config;
    config.maxChannelCount = 3;
    config.maxBlockSamples = 5;
    config.capacity = 2;

    QVERIFY(queue.configure(config) == AdaptiveDenoisingQueueConfigureStatus::Ready);

    MatrixXd pendingBlock(2, 3);
    pendingBlock << 7101.0, 7102.0, 7103.0,
                    7201.0, 7202.0, 7203.0;
    const NativeFiffInfoHandle pendingMetadata = fakeFiffInfoHandle(std::uint64_t(7101));
    QVERIFY(queue.tryPush(pendingBlock, pendingMetadata)
            == AdaptiveDenoisingQueuePushStatus::Pushed);

    const NativeFiffInfoHandle destinationMetadata = fakeFiffInfoHandle(std::uint64_t(7102));
    AdaptiveDenoisingQueuedBlock destination;
    destination.data.resize(config.maxChannelCount, config.maxBlockSamples);
    destination.data.setConstant(-7101.0);
    destination.rowCount = 7101;
    destination.sampleCount = 7102;
    destination.fiffInfo = destinationMetadata;

    queue.stop();
    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Stopped);
    QVERIFY(destinationPreserves(
        destination, config.maxChannelCount, config.maxBlockSamples, -7101.0, 7101, 7102,
        destinationMetadata));

    QVERIFY(queue.configure(config) == AdaptiveDenoisingQueueConfigureStatus::Ready);

    destination.data.setConstant(-7103.0);
    destination.rowCount = 7103;
    destination.sampleCount = 7104;
    destination.fiffInfo = destinationMetadata;
    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Timeout);
    QVERIFY(destinationPreserves(
        destination, config.maxChannelCount, config.maxBlockSamples, -7103.0, 7103, 7104,
        destinationMetadata));

    MatrixXd freshBlock(3, 2);
    freshBlock << 8101.0, 8102.0,
                  8201.0, 8202.0,
                  8301.0, 8302.0;
    const MatrixXd expectedFreshBlock = freshBlock;
    const NativeFiffInfoHandle freshMetadata = fakeFiffInfoHandle(std::uint64_t(8101));
    QVERIFY(queue.tryPush(freshBlock, freshMetadata)
            == AdaptiveDenoisingQueuePushStatus::Pushed);

    destination.data.setConstant(-7105.0);
    destination.rowCount = 7105;
    destination.sampleCount = 7106;
    destination.fiffInfo = destinationMetadata;
    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Popped);
    QCOMPARE(destination.rowCount, Index(3));
    QCOMPARE(destination.sampleCount, Index(2));
    QVERIFY(sameMetadata(destination.fiffInfo, freshMetadata));
    for (Index row = 0; row < config.maxChannelCount; ++row) {
        for (Index column = 0; column < config.maxBlockSamples; ++column) {
            if (row < expectedFreshBlock.rows() && column < expectedFreshBlock.cols()) {
                QVERIFY(destination.data(row, column) == expectedFreshBlock(row, column));
            } else {
                QVERIFY(destination.data(row, column) == -7105.0);
            }
        }
    }
}

//=============================================================================================================

void TestAdaptiveDenoisingPlugin::queueRetainsNativeMetadataUntilPoppedHandleCleared()
{
    constexpr std::size_t kMetadataBlockCount = 3;

    AdaptiveDenoisingBlockQueue queue;
    AdaptiveDenoisingBlockQueueConfig config;
    config.maxChannelCount = 4;
    config.maxBlockSamples = 5;
    config.capacity = kMetadataBlockCount;

    std::array<MatrixXd, kMetadataBlockCount> blocks;
    std::array<Index, kMetadataBlockCount> rowCounts{{2, 4, 3}};
    std::array<Index, kMetadataBlockCount> sampleCounts{{3, 2, 5}};
    std::array<FakeMetadataLifetime, kMetadataBlockCount> lifetimes;
    std::array<NativeFiffInfoHandle, kMetadataBlockCount> handles;
    std::array<const FIFFLIB::FiffInfo*, kMetadataBlockCount> metadataPointers{};

    for (std::size_t index = 0; index < kMetadataBlockCount; ++index) {
        blocks[index].resize(rowCounts[index], sampleCounts[index]);
        for (Index row = 0; row < rowCounts[index]; ++row) {
            for (Index column = 0; column < sampleCounts[index]; ++column) {
                blocks[index](row, column) =
                    9100.0 + static_cast<double>(index * 1000U + row * 100U + column);
            }
        }

        lifetimes[index].token = 9101U + static_cast<std::uint64_t>(index);
        handles[index] = fakeFiffInfoHandle(lifetimes[index]);
        metadataPointers[index] = handles[index].data();
    }

    QVERIFY(queue.configure(config) == AdaptiveDenoisingQueueConfigureStatus::Ready);

    for (std::size_t index = 0; index < kMetadataBlockCount; ++index) {
        const AdaptiveDenoisingQueuePushStatus status =
            queue.tryPush(blocks[index], handles[index]);
        handles[index].reset();
        QVERIFY(status == AdaptiveDenoisingQueuePushStatus::Pushed);
        QCOMPARE(lifetimes[index].liveCount.load(std::memory_order_acquire), 1);
        QCOMPARE(lifetimes[index].deleterCount.load(std::memory_order_acquire), 0);
    }

    AdaptiveDenoisingQueuedBlock destination;
    destination.data.resize(config.maxChannelCount, config.maxBlockSamples);
    for (std::size_t index = 0; index < kMetadataBlockCount; ++index) {
        destination.data.setConstant(-9200.0);
        destination.rowCount = 9201;
        destination.sampleCount = 9202;
        destination.fiffInfo.reset();

        QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Popped);
        QCOMPARE(destination.rowCount, rowCounts[index]);
        QCOMPARE(destination.sampleCount, sampleCounts[index]);
        QVERIFY(destination.fiffInfo.data() == metadataPointers[index]);
        for (Index row = 0; row < config.maxChannelCount; ++row) {
            for (Index column = 0; column < config.maxBlockSamples; ++column) {
                if (row < blocks[index].rows() && column < blocks[index].cols()) {
                    QVERIFY(destination.data(row, column) == blocks[index](row, column));
                } else {
                    QVERIFY(destination.data(row, column) == -9200.0);
                }
            }
        }
        QCOMPARE(lifetimes[index].liveCount.load(std::memory_order_acquire), 1);
        QCOMPARE(lifetimes[index].deleterCount.load(std::memory_order_acquire), 0);

        destination.fiffInfo.reset();
        QCOMPARE(lifetimes[index].liveCount.load(std::memory_order_acquire), 0);
        QCOMPARE(lifetimes[index].deleterCount.load(std::memory_order_acquire), 1);
    }

    destination.data.setConstant(-9203.0);
    destination.rowCount = 9203;
    destination.sampleCount = 9204;
    const NativeFiffInfoHandle timeoutMetadata = fakeFiffInfoHandle(std::uint64_t(9203));
    destination.fiffInfo = timeoutMetadata;
    QVERIFY(queue.waitPop(destination, 0) == AdaptiveDenoisingQueuePopStatus::Timeout);
    QVERIFY(destinationPreserves(
        destination, config.maxChannelCount, config.maxBlockSamples, -9203.0, 9203, 9204,
        timeoutMetadata));
}

#if defined(__unix__) && !defined(__APPLE__) && (defined(__GNUC__) || defined(__clang__))

//=============================================================================================================

void TestAdaptiveDenoisingPlugin::queuePopsPromptlyAfterInterruptedProducerSignal()
{
    constexpr Index kMaxChannelCount = 3;
    constexpr Index kMaxBlockSamples = 5;
    constexpr int kWaitMilliseconds = 3000;
    constexpr int kPromptUpperBoundMilliseconds = 500;
    constexpr int kReadyBudgetMilliseconds = 1000;
    constexpr int kCompletionBudgetMilliseconds = kWaitMilliseconds + 1000;
    constexpr int kFallbackBudgetMilliseconds = 1000;
    constexpr int kSchedulingAllowanceMilliseconds = 50;

    AdaptiveDenoisingBlockQueue queue;
    AdaptiveDenoisingBlockQueueConfig config;
    config.maxChannelCount = kMaxChannelCount;
    config.maxBlockSamples = kMaxBlockSamples;
    config.capacity = 1;

    QVERIFY(queue.configure(config) == AdaptiveDenoisingQueueConfigureStatus::Ready);

    MatrixXd producerBlock(2, 3);
    producerBlock << 1201.0, 1202.0, 1203.0,
                     1301.0, 1302.0, 1303.0;
    const NativeFiffInfoHandle producerMetadata = fakeFiffInfoHandle(std::uint64_t(1204));

    constexpr double kDestinationTail = -1205.0;
    const NativeFiffInfoHandle destinationMetadata = fakeFiffInfoHandle(std::uint64_t(1206));
    AdaptiveDenoisingQueuedBlock destination;
    destination.data.resize(config.maxChannelCount, config.maxBlockSamples);
    destination.data.setConstant(kDestinationTail);
    destination.rowCount = 1207;
    destination.sampleCount = 1208;
    destination.fiffInfo = destinationMetadata;

    std::atomic<bool> start(false);
    std::atomic<bool> consumerReady(false);
    std::atomic<bool> consumerWaiting(false);
    std::atomic<bool> consumerFinished(false);
    std::atomic<bool> producerReady(false);
    std::atomic<bool> producerFinished(false);
    std::atomic<bool> invalidThreadState(false);
    std::atomic<int> observedPushStatus(-1);
    std::atomic<int> observedPopStatus(-1);
    std::atomic<std::int64_t> popElapsedMilliseconds(-1);
    g_interruptedQueueWriteCount.store(0, std::memory_order_release);

    std::thread consumer([&] {
        consumerReady.store(true, std::memory_order_release);
        const auto controlDeadline = std::chrono::steady_clock::now()
            + std::chrono::milliseconds(kCompletionBudgetMilliseconds);
        while (!start.load(std::memory_order_acquire)
               && std::chrono::steady_clock::now() < controlDeadline) {
            std::this_thread::yield();
        }

        if (!start.load(std::memory_order_acquire)) {
            invalidThreadState.store(true, std::memory_order_release);
            consumerFinished.store(true, std::memory_order_release);
            return;
        }

        destination.data.setConstant(kDestinationTail);
        destination.rowCount = 1207;
        destination.sampleCount = 1208;
        destination.fiffInfo = destinationMetadata;
        consumerWaiting.store(true, std::memory_order_release);
        const auto waitStartedAt = std::chrono::steady_clock::now();
        const AdaptiveDenoisingQueuePopStatus status =
            queue.waitPop(destination, kWaitMilliseconds);
        const auto waitFinishedAt = std::chrono::steady_clock::now();
        observedPopStatus.store(static_cast<int>(status), std::memory_order_release);
        popElapsedMilliseconds.store(
            std::chrono::duration_cast<std::chrono::milliseconds>(waitFinishedAt - waitStartedAt)
                .count(),
            std::memory_order_release);
        consumerFinished.store(true, std::memory_order_release);
    });

    std::thread producer([&] {
        producerReady.store(true, std::memory_order_release);
        const auto controlDeadline = std::chrono::steady_clock::now()
            + std::chrono::milliseconds(kCompletionBudgetMilliseconds);
        while (!start.load(std::memory_order_acquire)
               && std::chrono::steady_clock::now() < controlDeadline) {
            std::this_thread::yield();
        }

        while (!consumerWaiting.load(std::memory_order_acquire)
               && std::chrono::steady_clock::now() < controlDeadline) {
            std::this_thread::yield();
        }

        if (!start.load(std::memory_order_acquire)
            || !consumerWaiting.load(std::memory_order_acquire)) {
            invalidThreadState.store(true, std::memory_order_release);
            producerFinished.store(true, std::memory_order_release);
            return;
        }

        g_interruptNextQueueWrite = true;
        const AdaptiveDenoisingQueuePushStatus status =
            queue.tryPush(producerBlock, producerMetadata);
        g_interruptNextQueueWrite = false;
        observedPushStatus.store(static_cast<int>(status), std::memory_order_release);
        producerFinished.store(true, std::memory_order_release);
    });

    const bool consumerReadyObserved = waitUntil(
        [&] { return consumerReady.load(std::memory_order_acquire); }, kReadyBudgetMilliseconds);
    const bool producerReadyObserved = waitUntil(
        [&] { return producerReady.load(std::memory_order_acquire); }, kReadyBudgetMilliseconds);
    start.store(true, std::memory_order_release);

    const bool consumerWaitingObserved = waitUntil(
        [&] { return consumerWaiting.load(std::memory_order_acquire); }, kReadyBudgetMilliseconds);
    std::this_thread::sleep_for(std::chrono::milliseconds(kSchedulingAllowanceMilliseconds));

    const bool consumerFinishedWithinBudget = waitUntil(
        [&] { return consumerFinished.load(std::memory_order_acquire); },
        kCompletionBudgetMilliseconds);
    const bool producerFinishedWithinBudget = waitUntil(
        [&] { return producerFinished.load(std::memory_order_acquire); },
        kFallbackBudgetMilliseconds);

    if (!consumerFinishedWithinBudget || !producerFinishedWithinBudget) {
        queue.stop();
    }

    const bool consumerFinishedAfterFallback = consumerFinishedWithinBudget || waitUntil(
        [&] { return consumerFinished.load(std::memory_order_acquire); },
        kFallbackBudgetMilliseconds);
    const bool producerFinishedAfterFallback = producerFinishedWithinBudget || waitUntil(
        [&] { return producerFinished.load(std::memory_order_acquire); },
        kFallbackBudgetMilliseconds);

    producer.join();
    consumer.join();
    queue.stop();

    const AdaptiveDenoisingQueuePushStatus observedPush =
        static_cast<AdaptiveDenoisingQueuePushStatus>(
            observedPushStatus.load(std::memory_order_acquire));
    const AdaptiveDenoisingQueuePopStatus observedPop =
        static_cast<AdaptiveDenoisingQueuePopStatus>(
            observedPopStatus.load(std::memory_order_acquire));
    const std::int64_t popElapsed = popElapsedMilliseconds.load(std::memory_order_acquire);
    const int interruptedWriteCount = g_interruptedQueueWriteCount.load(std::memory_order_acquire);

    QVERIFY(consumerReadyObserved);
    QVERIFY(producerReadyObserved);
    QVERIFY(consumerWaitingObserved);
    QVERIFY(consumerFinishedAfterFallback);
    QVERIFY(producerFinishedAfterFallback);
    QVERIFY(!invalidThreadState.load(std::memory_order_acquire));
    QCOMPARE(observedPush, AdaptiveDenoisingQueuePushStatus::Pushed);
    QCOMPARE(observedPop, AdaptiveDenoisingQueuePopStatus::Popped);
    QVERIFY(popElapsed >= 0);
    QVERIFY(popElapsed < kPromptUpperBoundMilliseconds);
    QCOMPARE(interruptedWriteCount, 1);
    QVERIFY(destinationContainsBlock(
        destination,
        producerBlock,
        producerMetadata,
        config.maxChannelCount,
        config.maxBlockSamples,
        kDestinationTail));
}

//=============================================================================================================

void TestAdaptiveDenoisingPlugin::queueStopsPromptlyAfterInterruptedStopSignal()
{
    constexpr Index kMaxChannelCount = 3;
    constexpr Index kMaxBlockSamples = 5;
    constexpr int kWaitMilliseconds = 3000;
    constexpr int kPromptUpperBoundMilliseconds = 500;
    constexpr int kReadyBudgetMilliseconds = 1000;
    constexpr int kCompletionBudgetMilliseconds = kWaitMilliseconds + 1000;
    constexpr int kFallbackBudgetMilliseconds = 1000;
    constexpr int kSchedulingAllowanceMilliseconds = 50;

    AdaptiveDenoisingBlockQueue queue;
    AdaptiveDenoisingBlockQueueConfig config;
    config.maxChannelCount = kMaxChannelCount;
    config.maxBlockSamples = kMaxBlockSamples;
    config.capacity = 1;

    QVERIFY(queue.configure(config) == AdaptiveDenoisingQueueConfigureStatus::Ready);

    constexpr double kDestinationTail = -2201.0;
    const NativeFiffInfoHandle destinationMetadata = fakeFiffInfoHandle(std::uint64_t(2202));
    AdaptiveDenoisingQueuedBlock destination;
    destination.data.resize(config.maxChannelCount, config.maxBlockSamples);
    destination.data.setConstant(kDestinationTail);
    destination.rowCount = 2203;
    destination.sampleCount = 2204;
    destination.fiffInfo = destinationMetadata;

    std::atomic<bool> start(false);
    std::atomic<bool> consumerReady(false);
    std::atomic<bool> consumerWaiting(false);
    std::atomic<bool> consumerFinished(false);
    std::atomic<bool> invalidThreadState(false);
    std::atomic<int> observedPopStatus(-1);
    std::atomic<std::int64_t> popElapsedMilliseconds(-1);
    g_interruptedQueueWriteCount.store(0, std::memory_order_release);

    std::thread consumer([&] {
        consumerReady.store(true, std::memory_order_release);
        const auto controlDeadline = std::chrono::steady_clock::now()
            + std::chrono::milliseconds(kCompletionBudgetMilliseconds);
        while (!start.load(std::memory_order_acquire)
               && std::chrono::steady_clock::now() < controlDeadline) {
            std::this_thread::yield();
        }

        if (!start.load(std::memory_order_acquire)) {
            invalidThreadState.store(true, std::memory_order_release);
            consumerFinished.store(true, std::memory_order_release);
            return;
        }

        destination.data.setConstant(kDestinationTail);
        destination.rowCount = 2203;
        destination.sampleCount = 2204;
        destination.fiffInfo = destinationMetadata;
        consumerWaiting.store(true, std::memory_order_release);
        const auto waitStartedAt = std::chrono::steady_clock::now();
        const AdaptiveDenoisingQueuePopStatus status =
            queue.waitPop(destination, kWaitMilliseconds);
        const auto waitFinishedAt = std::chrono::steady_clock::now();
        observedPopStatus.store(static_cast<int>(status), std::memory_order_release);
        popElapsedMilliseconds.store(
            std::chrono::duration_cast<std::chrono::milliseconds>(waitFinishedAt - waitStartedAt)
                .count(),
            std::memory_order_release);
        consumerFinished.store(true, std::memory_order_release);
    });

    const bool consumerReadyObserved = waitUntil(
        [&] { return consumerReady.load(std::memory_order_acquire); }, kReadyBudgetMilliseconds);
    start.store(true, std::memory_order_release);

    const bool consumerWaitingObserved = waitUntil(
        [&] { return consumerWaiting.load(std::memory_order_acquire); }, kReadyBudgetMilliseconds);
    std::this_thread::sleep_for(std::chrono::milliseconds(kSchedulingAllowanceMilliseconds));

    g_interruptNextQueueWrite = true;
    queue.stop();
    g_interruptNextQueueWrite = false;

    const bool consumerFinishedWithinBudget = waitUntil(
        [&] { return consumerFinished.load(std::memory_order_acquire); },
        kCompletionBudgetMilliseconds);
    if (!consumerFinishedWithinBudget) {
        queue.stop();
    }

    const bool consumerFinishedAfterFallback = consumerFinishedWithinBudget || waitUntil(
        [&] { return consumerFinished.load(std::memory_order_acquire); },
        kFallbackBudgetMilliseconds);

    consumer.join();
    queue.stop();

    const AdaptiveDenoisingQueuePopStatus observedPop =
        static_cast<AdaptiveDenoisingQueuePopStatus>(
            observedPopStatus.load(std::memory_order_acquire));
    const std::int64_t popElapsed = popElapsedMilliseconds.load(std::memory_order_acquire);
    const int interruptedWriteCount = g_interruptedQueueWriteCount.load(std::memory_order_acquire);

    QVERIFY(consumerReadyObserved);
    QVERIFY(consumerWaitingObserved);
    QVERIFY(consumerFinishedAfterFallback);
    QVERIFY(!invalidThreadState.load(std::memory_order_acquire));
    QCOMPARE(observedPop, AdaptiveDenoisingQueuePopStatus::Stopped);
    QVERIFY(popElapsed >= 0);
    QVERIFY(popElapsed < kPromptUpperBoundMilliseconds);
    QCOMPARE(interruptedWriteCount, 1);
    QVERIFY(destinationPreserves(
        destination,
        config.maxChannelCount,
        config.maxBlockSamples,
        kDestinationTail,
        2203,
        2204,
        destinationMetadata));
}

#endif

//=============================================================================================================

QTEST_APPLESS_MAIN(TestAdaptiveDenoisingPlugin)

#include "test_adaptive_denoising_plugin.moc"
