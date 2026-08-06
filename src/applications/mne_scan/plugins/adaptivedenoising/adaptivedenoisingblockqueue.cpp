//=============================================================================================================
/**
 * @file     adaptivedenoisingblockqueue.cpp
 * @brief    Preallocated single-producer/single-consumer input queue for adaptive denoising.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "adaptivedenoisingblockqueue.h"

#include <atomic>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#endif

//=============================================================================================================
// DEFINE NAMESPACE ADAPTIVEDENOISINGPLUGIN
//=============================================================================================================

namespace ADAPTIVEDENOISINGPLUGIN
{

//=============================================================================================================

namespace
{

using NativeFiffInfoHandle = QSharedPointer<const FIFFLIB::FiffInfo>;
using AtomicWord = unsigned int;

constexpr AtomicWord kStopped = 0U;
constexpr AtomicWord kRunning = 1U;

// ATOMIC_INT_LOCK_FREE == 2 is the C++14 compile-time proof that every explicit queue atomic below is always
// lock-free. Keeping capacity at or below INT_MAX also keeps the live SPSC distance below half the unsigned
// sequence range, so modulo subtraction remains unambiguous across counter wrap.
static_assert(ATOMIC_INT_LOCK_FREE == 2, "Queue atomic state must always be lock-free");
static_assert(
    std::numeric_limits<AtomicWord>::is_modulo,
    "Queue sequence state requires modulo unsigned arithmetic");
static_assert(
    std::numeric_limits<AtomicWord>::max() / 2U
        >= static_cast<AtomicWord>(std::numeric_limits<int>::max()),
    "INT_MAX queue capacity must fit below half the sequence range");

static_assert(
    std::is_nothrow_copy_constructible<NativeFiffInfoHandle>::value,
    "Native FIFF metadata handle copy construction must not throw");
static_assert(
    std::is_nothrow_copy_assignable<NativeFiffInfoHandle>::value,
    "Native FIFF metadata handle copy assignment must not throw");
static_assert(
    std::is_nothrow_move_constructible<NativeFiffInfoHandle>::value,
    "Native FIFF metadata handle move construction must not throw");
static_assert(
    std::is_nothrow_move_assignable<NativeFiffInfoHandle>::value,
    "Native FIFF metadata handle move assignment must not throw");

enum class NativeWaitStatus : std::uint8_t {
    Signaled,
    TimedOut,
    Failed
};

//=============================================================================================================

class NativeWake final
{
public:
#if defined(_WIN32)
    NativeWake()
    : m_event(CreateEventW(nullptr, FALSE, FALSE, nullptr))
    {
        if (m_event == nullptr) {
            throw std::runtime_error("Could not create adaptive denoising queue wake event");
        }
    }

    ~NativeWake() noexcept
    {
        CloseHandle(m_event);
    }

    void signal() noexcept
    {
        // Auto-reset event state is sticky and coalesces repeated publications. SetEvent is attempted once.
        (void)SetEvent(m_event);
    }

    NativeWaitStatus wait(int timeoutMilliseconds) noexcept
    {
        const DWORD result = WaitForSingleObject(m_event, static_cast<DWORD>(timeoutMilliseconds));
        if (result == WAIT_OBJECT_0) {
            return NativeWaitStatus::Signaled;
        }
        if (result == WAIT_TIMEOUT) {
            return NativeWaitStatus::TimedOut;
        }
        return NativeWaitStatus::Failed;
    }

private:
    HANDLE m_event;
#else
    NativeWake()
    : m_readDescriptor(-1)
    , m_writeDescriptor(-1)
    {
        int descriptors[2] = {-1, -1};
        if (::pipe(descriptors) != 0) {
            throw std::runtime_error("Could not create adaptive denoising queue wake pipe");
        }

        if (!configureDescriptor(descriptors[0]) || !configureDescriptor(descriptors[1])) {
            (void)::close(descriptors[0]);
            (void)::close(descriptors[1]);
            throw std::runtime_error("Could not configure adaptive denoising queue wake pipe");
        }

        m_readDescriptor = descriptors[0];
        m_writeDescriptor = descriptors[1];
    }

    ~NativeWake() noexcept
    {
        (void)::close(m_readDescriptor);
        (void)::close(m_writeDescriptor);
    }

    void signal() noexcept
    {
        const unsigned char wakeByte = 1U;
        // Make exactly one nonblocking attempt. EAGAIN coalesces with a pending byte; EINTR may lose this byte,
        // so the consumer's bounded poll slices provide the next atomic sequence/running recheck.
        const ssize_t result = ::write(m_writeDescriptor, &wakeByte, sizeof(wakeByte));
        (void)result;
    }

    NativeWaitStatus wait(int timeoutMilliseconds) noexcept
    {
        struct pollfd descriptor;
        descriptor.fd = m_readDescriptor;
        descriptor.events = POLLIN;
        descriptor.revents = 0;

        constexpr int kMaximumPollWaitMilliseconds = 25;
        const int boundedTimeoutMilliseconds = timeoutMilliseconds <= 0
            ? 0
            : (timeoutMilliseconds < kMaximumPollWaitMilliseconds
                ? timeoutMilliseconds
                : kMaximumPollWaitMilliseconds);
        // A producer/stop write interrupted before transferring its byte leaves no sticky descriptor state.
        // This finite slice returns control to waitPop's unchanged atomic rechecks well before its outer deadline.
        const int result = ::poll(&descriptor, 1, boundedTimeoutMilliseconds);
        if (result == 0) {
            return NativeWaitStatus::TimedOut;
        }
        if (result < 0) {
            // The outer deadline bounds any recheck after an interrupted poll.
            return errno == EINTR ? NativeWaitStatus::Signaled : NativeWaitStatus::Failed;
        }
        if ((descriptor.revents & POLLIN) == 0) {
            return NativeWaitStatus::Failed;
        }

        drain();
        return NativeWaitStatus::Signaled;
    }

private:
    static bool configureDescriptor(int descriptor) noexcept
    {
        const int statusFlags = ::fcntl(descriptor, F_GETFL, 0);
        if (statusFlags < 0 || ::fcntl(descriptor, F_SETFL, statusFlags | O_NONBLOCK) < 0) {
            return false;
        }

        const int descriptorFlags = ::fcntl(descriptor, F_GETFD, 0);
        return descriptorFlags >= 0
            && ::fcntl(descriptor, F_SETFD, descriptorFlags | FD_CLOEXEC) >= 0;
    }

    void drain() noexcept
    {
        unsigned char wakeBytes[256];
        // Every successful read consumes finite pipe state. The loop ends at the first nonblocking empty read.
        while (::read(m_readDescriptor, wakeBytes, sizeof(wakeBytes)) > 0) {
        }
    }

    int m_readDescriptor;
    int m_writeDescriptor;
#endif

    NativeWake(const NativeWake&) = delete;
    NativeWake& operator=(const NativeWake&) = delete;
};

} // namespace

//=============================================================================================================

class AdaptiveDenoisingBlockQueue::Impl final
{
public:
    struct Slot
    {
        Slot(Eigen::Index maxChannelCount, Eigen::Index maxBlockSamples)
        : data(maxChannelCount, maxBlockSamples)
        , rowCount(0)
        , sampleCount(0)
        {
        }

        Eigen::MatrixXd                         data;
        Eigen::Index                            rowCount;
        Eigen::Index                            sampleCount;
        NativeFiffInfoHandle                    fiffInfo;
    };

    explicit Impl(const AdaptiveDenoisingBlockQueueConfig& config)
    : maxChannelCount(config.maxChannelCount)
    , maxBlockSamples(config.maxBlockSamples)
    , capacity(config.capacity)
    , running(kStopped)
    , producerSequence(0U)
    , consumerSequence(0U)
    , producerIndex(0)
    , consumerIndex(0)
    {
        queueSlots.reserve(capacity);
        for (std::size_t index = 0; index < capacity; ++index) {
            queueSlots.emplace_back(maxChannelCount, maxBlockSamples);
        }
    }

    const Eigen::Index maxChannelCount;
    const Eigen::Index maxBlockSamples;
    const std::size_t  capacity;
    NativeWake         wake;
    std::vector<Slot>  queueSlots;
    std::atomic<AtomicWord> running;
    std::atomic<AtomicWord> producerSequence;
    std::atomic<AtomicWord> consumerSequence;
    std::size_t        producerIndex;
    std::size_t        consumerIndex;
};

//=============================================================================================================

AdaptiveDenoisingBlockQueue::AdaptiveDenoisingBlockQueue() noexcept = default;

//=============================================================================================================

AdaptiveDenoisingBlockQueue::~AdaptiveDenoisingBlockQueue() = default;

//=============================================================================================================

AdaptiveDenoisingQueueConfigureStatus AdaptiveDenoisingBlockQueue::configure(
    const AdaptiveDenoisingBlockQueueConfig& config)
{
    if (m_impl && m_impl->running.load(std::memory_order_acquire) == kRunning) {
        return AdaptiveDenoisingQueueConfigureStatus::AlreadyRunning;
    }

    if (config.maxChannelCount <= 0
        || config.maxBlockSamples <= 0
        || config.capacity == 0
        || config.capacity > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return AdaptiveDenoisingQueueConfigureStatus::InvalidConfiguration;
    }

    std::unique_ptr<Impl> candidate(new Impl(config));
    candidate->running.store(kRunning, std::memory_order_release);
    m_impl.swap(candidate);
    return AdaptiveDenoisingQueueConfigureStatus::Ready;
}

//=============================================================================================================

AdaptiveDenoisingQueuePushStatus AdaptiveDenoisingBlockQueue::tryPush(
    Eigen::Ref<const Eigen::MatrixXd> block,
    QSharedPointer<const FIFFLIB::FiffInfo> fiffInfo) noexcept
{
    Impl* const impl = m_impl.get();
    if (!impl || impl->running.load(std::memory_order_acquire) != kRunning) {
        return AdaptiveDenoisingQueuePushStatus::Stopped;
    }

    const Eigen::Index rowCount = block.rows();
    const Eigen::Index sampleCount = block.cols();
    if (rowCount <= 0
        || rowCount > impl->maxChannelCount
        || sampleCount <= 0
        || sampleCount > impl->maxBlockSamples) {
        return AdaptiveDenoisingQueuePushStatus::InvalidBlock;
    }

    const AtomicWord producerSequence = impl->producerSequence.load(std::memory_order_relaxed);
    const AtomicWord consumerSequence = impl->consumerSequence.load(std::memory_order_acquire);
    if (producerSequence - consumerSequence == static_cast<AtomicWord>(impl->capacity)) {
        return impl->running.load(std::memory_order_acquire) == kRunning
            ? AdaptiveDenoisingQueuePushStatus::Full
            : AdaptiveDenoisingQueuePushStatus::Stopped;
    }

    // This final observation is the push/stop linearization point. Once it observes running, the caller's
    // quiescence rule lets this already-linearized push finish even if stop release-publishes concurrently.
    if (impl->running.load(std::memory_order_acquire) != kRunning) {
        return AdaptiveDenoisingQueuePushStatus::Stopped;
    }

    Impl::Slot& slot = impl->queueSlots[impl->producerIndex];
    for (Eigen::Index column = 0; column < sampleCount; ++column) {
        for (Eigen::Index row = 0; row < rowCount; ++row) {
            slot.data(row, column) = block(row, column);
        }
    }
    slot.rowCount = rowCount;
    slot.sampleCount = sampleCount;
    slot.fiffInfo = std::move(fiffInfo);

    ++impl->producerIndex;
    if (impl->producerIndex == impl->capacity) {
        impl->producerIndex = 0;
    }

    impl->producerSequence.store(producerSequence + 1U, std::memory_order_release);
    impl->wake.signal();
    return AdaptiveDenoisingQueuePushStatus::Pushed;
}

//=============================================================================================================

AdaptiveDenoisingQueuePopStatus AdaptiveDenoisingBlockQueue::waitPop(
    AdaptiveDenoisingQueuedBlock& destination,
    int timeoutMilliseconds) noexcept
{
    Impl* const impl = m_impl.get();
    if (!impl || impl->running.load(std::memory_order_acquire) != kRunning) {
        return AdaptiveDenoisingQueuePopStatus::Stopped;
    }

    if (destination.data.rows() != impl->maxChannelCount
        || destination.data.cols() != impl->maxBlockSamples) {
        return AdaptiveDenoisingQueuePopStatus::InvalidDestination;
    }

    const bool mayWait = timeoutMilliseconds > 0;
    const std::chrono::steady_clock::time_point deadline = mayWait
        ? std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMilliseconds)
        : std::chrono::steady_clock::time_point();
    bool nativeWaitFailed = false;

    for (;;) {
        if (impl->running.load(std::memory_order_acquire) != kRunning) {
            return AdaptiveDenoisingQueuePopStatus::Stopped;
        }

        const AtomicWord consumerSequence = impl->consumerSequence.load(std::memory_order_relaxed);
        const AtomicWord producerSequence = impl->producerSequence.load(std::memory_order_acquire);
        if (producerSequence != consumerSequence) {
            // Acquire of producerSequence publishes the complete slot. This last running observation commits
            // the pop before a concurrent stop; no destination field is touched on the stopped path.
            if (impl->running.load(std::memory_order_acquire) != kRunning) {
                return AdaptiveDenoisingQueuePopStatus::Stopped;
            }

            Impl::Slot& slot = impl->queueSlots[impl->consumerIndex];
            for (Eigen::Index column = 0; column < slot.sampleCount; ++column) {
                for (Eigen::Index row = 0; row < slot.rowCount; ++row) {
                    destination.data(row, column) = slot.data(row, column);
                }
            }
            destination.rowCount = slot.rowCount;
            destination.sampleCount = slot.sampleCount;
            destination.fiffInfo = std::move(slot.fiffInfo);

            ++impl->consumerIndex;
            if (impl->consumerIndex == impl->capacity) {
                impl->consumerIndex = 0;
            }

            impl->consumerSequence.store(consumerSequence + 1U, std::memory_order_release);
            return AdaptiveDenoisingQueuePopStatus::Popped;
        }

        // Close the empty-check/wait race. A later push or stop leaves sticky native state for the bounded wait.
        if (impl->running.load(std::memory_order_acquire) != kRunning) {
            return AdaptiveDenoisingQueuePopStatus::Stopped;
        }
        if (!mayWait || nativeWaitFailed) {
            return AdaptiveDenoisingQueuePopStatus::Timeout;
        }

        const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            return AdaptiveDenoisingQueuePopStatus::Timeout;
        }

        const std::chrono::steady_clock::duration remaining = deadline - now;
        std::chrono::milliseconds remainingMilliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(remaining);
        if (remainingMilliseconds < remaining) {
            remainingMilliseconds += std::chrono::milliseconds(1);
        }

        const NativeWaitStatus waitStatus = impl->wake.wait(
            static_cast<int>(remainingMilliseconds.count()));
        nativeWaitFailed = waitStatus == NativeWaitStatus::Failed;
        // Signaled wakes can be stale/coalesced; timed-out or interrupted waits also receive one atomic recheck.
        // POSIX slices additionally bound progress when a producer/stop write lost its byte to EINTR.
    }
}

//=============================================================================================================

void AdaptiveDenoisingBlockQueue::stop() noexcept
{
    Impl* const impl = m_impl.get();
    if (impl && impl->running.exchange(kStopped, std::memory_order_release) == kRunning) {
        impl->wake.signal();
    }
}

//=============================================================================================================

} // NAMESPACE
