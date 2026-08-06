# Causal Reference Denoising Specification

## Goal

Add an independent `mne_scan` algorithm plugin named **Adaptive Denoising**.
It uses good `FIFFV_REF_MEG_CH` rows to estimate environmental interference
in good `FIFFV_MEG_CH` target rows while preserving every other row.

The first algorithm is causal time-shifted reference regression with a
block-recursive exponentially weighted least-squares model (EWLS). It is a
learning project for streaming state, fixed update epochs, and Eigen matrix
operations. HFC, rSSS/AdaptiveTSSS, ICA/ORICA, synthetic-reference discovery,
and `mne_rt_server` integration are not part of v1.

## Delivery priority

The v1 schedule is engineering-first. The numerical effect gates are already
GREEN and are retained as regression tests, but no further estimator tuning or
additional denoising algorithms block delivery. Remaining effort is ordered by
integration risk: deterministic processor mapping and safe pass-through,
nonblocking acquisition/worker lifecycle, buildable plugin shell, minimal
controls/diagnostics, then example/benchmark/learning documentation. The user
accepts the focused targets and example; full `mne_scan + mne_rt_server` is not
required.

The GitHub work breakdown keeps issue #2 as the feature epic and uses five
remaining child issues:

1. `#4`: plugin processor, channel mapping and focused adapter tests.
2. `#5`: nonblocking drop-newest queue and processing-thread lifecycle.
3. `#6`: `scan_adaptivedenoising` plugin shell, block-boundary settings and minimal UI.
4. `#7`: synthetic example, real-time benchmark and Eigen/streaming learning docs.
5. `#3`: focused integration verification, tracked P3 closure and final review.

Each child issue is split into reviewable minimal REQUEST blocks. Tests and
production GREEN changes are separate blocks where practical, but speed takes
precedence over mechanically forcing RED when an existing public contract
already has adequate regression coverage.

## Research and licensing

- Signal model: de Cheveigne and Simon, *Denoising based on Time-Shift PCA*:
  https://pmc.ncbi.nlm.nih.gov/articles/PMC2018742/
- Online MEG precedent: *MEG adaptive noise suppression using Fast LMS*:
  https://doi.org/10.1109/CNE.2005.1419543
- BSD behavior reference: MEEGkit TSPCA/TSR:
  https://nbara.github.io/python-meegkit/modules/meegkit.tspca.html
- GPL research reference: FieldTrip `ft_denoise_tsr`:
  https://github.com/fieldtrip/fieldtrip

The implementation is written independently from the published equations.
No GPL source is copied, translated, or linked into MNE-CPP.

## Selected module and interface

The seam is an Eigen/STL-only concrete deep module in `RTPROCESSINGLIB`.
There is no v1 strategy base, algorithm registry, Qt type, or FIFF type in the
numerical interface.

```cpp
enum class DenoisingMode {
    BypassTrackHistory,
    ApplyOnly,
    ApplyAndLearn
};

struct CausalReferenceDenoiserConfig {
    double samplingFrequencyHz;
    Eigen::Index channelCount;
    Eigen::Index maxBlockSamples;
    Eigen::VectorXi referenceRows;
    Eigen::VectorXi targetRows;
    Eigen::Index tapCount;
    Eigen::Index adaptationIntervalSamples;
    double memoryTimeSeconds;
    double regularization;
};

class CausalReferenceDenoiser final {
public:
    DenoiserStatus configure(const CausalReferenceDenoiserConfig& config);
    DenoiserProcessResult process(Eigen::Ref<Eigen::MatrixXd> block,
                                  DenoisingMode mode) noexcept;
    void reset() noexcept;
};
```

`configure` is transactional and is the only allocation phase. A rejected
configuration preserves the previous committed configuration and state. The
module is single-stream and worker-thread owned.

Configuration invariants:

- finite positive sampling frequency and memory time;
- positive channel count, block bound, tap count, and update interval;
- finite regularization in the supported range;
- unique, in-range, non-overlapping reference and target rows;
- at least one reference and one target row;
- `referenceRows.size() * tapCount <= 256`.

Defaults are 4 taps, a 128-sample update interval, 30 seconds of memory, and
`1e-3` relative regularization. UI ranges are 1-32 taps, 16-2048 samples,
1-300 seconds, and `1e-8`-1 regularization.

## Streaming behavior

Rows are channels and columns are chronological samples. For `R` references,
`L` taps, and `M` targets, `P = R * L` and the feature vector is tap-major:

```text
phi(t) = [r0(t), r1(t), ..., r0(t-1), r1(t-1), ...]^T
```

The module owns lag history, `G(P x P)`, `H(M x P)`, `W(M x P)`, pending epoch
statistics, and all scratch matrices. After reset, the first `L - 1` samples
only populate causal history. The initial zero model passes the first complete
adaptation epoch unchanged.

For sample forgetting factor `lambda = exp(-1 / (fs * tau))`, each fixed epoch
updates weighted normal equations and solves

```text
A = symmetrize(Gcandidate) + regularization * D
A * Wcandidate^T = Hcandidate^T
```

with scale-aware diagonal `D` and Eigen `LDLT`; never form an inverse. Apply
the committed `W` before accumulating the raw current target into a candidate.
A model committed at an epoch boundary affects only later samples.

Modes:

- `BypassTrackHistory`: preserve the block exactly, advance lag history only.
- `ApplyOnly`: apply a ready model and advance history without learning.
- `ApplyAndLearn`: apply the old model, then learn for future samples.

Only target rows may change. Reference, STIM, bad, and unrelated rows remain
value-identical. Shape errors or selected NaN/Inf values pass the whole block
through without state change. A failed candidate solve retains the committed
model and reports a numerical status.

After configuration, `process` performs no heap allocation, locking, Qt/FIFF
access, or string construction. Cost is `O(N(P^2 + MP)) + O(P^3/K)`.

## Selected fixed-size diagnostics contract

Primary block validity remains orthogonal to model-update events. A valid
`ApplyAndLearn` block is `Processed` even if update boundaries are rejected;
accepted/rejected boundary counts are per call and may both exceed one.

```cpp
enum class DenoiserProcessStatus : std::uint8_t {
    NotConfigured,
    InvalidShape,
    NonFiniteInput,
    Bypassed,
    Processed
};

struct DenoiserProcessDiagnostics {
    Eigen::Index referenceRowCount;
    Eigen::Index targetRowCount;
    Eigen::Index featureCount;
    Eigen::Index warmupSamplesRemaining;
    std::uint64_t modelGeneration;
    std::uint64_t modelUpdatesAccepted;
    std::uint64_t modelUpdatesRejected;
    double inputRms;
    double outputRms;
    double estimatedNoiseRms;
};

struct DenoiserProcessResult {
    DenoiserProcessStatus status;
    DenoiserProcessDiagnostics diagnostics;
};
```

The result is the sole diagnostics snapshot; no getter, event vector, Qt/FIFF
type, string, or runtime strategy seam is added. `droppedBlocks` remains
adapter-owned. Counts describe the committed configuration and the post-call
state. `modelGeneration` increments once per accepted boundary and resets to
zero on configure/reset. `warmupSamplesRemaining` is post-call. Bypass advances
history but pauses the learning epoch; ApplyOnly does the same while applying
the committed model. Shape/nonfinite errors preserve state, report the
committed snapshot and zero update-event counts, and use quiet NaN RMS values.
An unconfigured call reports zero snapshot fields and quiet NaN RMS values.

For valid calls, RMS covers all selected targets and all columns, including
pass-through warmup. Input is original target data, output is returned target
data, and estimated noise is the prediction actually subtracted. Bypass has
equal input/output RMS and zero noise RMS. RMS accumulation must use scaled
sum-of-squares so large finite values do not overflow the diagnostics.

At a rejected boundary, output/history remain processed with the old committed
model, generation is unchanged, and the per-call rejection count increments.
Committed finite `G/H` are kept separate from preallocated pending-epoch
statistics. A poisoned/failed epoch is discarded after aging committed `G/H`
by its elapsed forgetting; a later epoch can recover without allocation.

If finite committed weights and finite selected input nevertheless produce a
non-finite prediction or target residual, application falls back atomically for
that sample: every selected target value at that sample remains at its original
finite value and the prediction actually subtracted for diagnostics is zero.
The valid block remains `Processed`; causal reference history advances, and an
ApplyAndLearn call may still feed the raw target/feature into the transactional
epoch so any poisoned candidate is rejected at its normal boundary. This
sample-wide policy avoids partial target corruption, saturation, rollback
buffers, and a new public diagnostics/event seam while guaranteeing finite
output for finite selected input.

## Plugin behavior

Add `adaptivedenoising` / `scan_adaptivedenoising` without changing the
existing `noisereduction`, AdaptiveTSSS, or `AbstractAlgorithm` interface.

- References: good `FIFFV_REF_MEG_CH` channels.
- Targets: good `FIFFV_MEG_CH` channels.
- Missing/invalid layouts: safe pass-through with diagnostics.
- Input callback: one non-blocking queue push; if full, drop the newest block
  and increment `droppedBlocks` instead of busy-waiting.
- UI/settings/reset: serialized at worker block boundaries.
- Controls: enabled, taps, interval, memory, regularization, freeze, reset.
- Diagnostics: status, row/feature counts, warmup, model generation, RMS values,
  and dropped blocks.

### Adapter data seam refinement

The existing `UTILSLIB::CircularBuffer` is not the v1 input seam: its
`push()` uses a 1000 ms semaphore timeout and existing mne_scan algorithms
retry in a busy loop. Changing that global type would broaden risk to unrelated
plugins. `adaptivedenoising` instead owns two private, focused components:

- A bounded single-producer/single-consumer block queue preallocates its slot
  array. `tryPush(block, fiffInfo)` performs exactly one
  `QSemaphore::tryAcquire(1, 0)`; on failure it returns immediately so the
  plugin can atomically increment `droppedBlocks` and drop that newest block.
  On success the slot deep-copies the matrix before the measurement callback
  returns and retains the shared immutable metadata pointer. There is no retry,
  sleep, busy wait or settings/model work in the callback. Worker-side pop may
  use an interruptible/timed wait; clear/resize occurs only while stopped.
- A worker-owned adapter processor resolves good reference/target rows from
  `FiffInfo`, owns `CausalReferenceDenoiser`, compares layout/settings at each
  dequeued block boundary, and either reconfigures transactionally or forwards
  the exact block with a bounded status reason. It is compiled into both the
  plugin and the focused adapter test, so FIFF selection, metadata change,
  settings boundaries and row-preserving processing do not require launching
  the full mne_scan GUI.

The queued metadata/block pair preserves FIFO ordering and gives the processing
thread all information needed to handle a layout transition at the same block
boundary as its samples. UI pending settings/reset state remains a separate
UI-to-worker boundary and is never locked by the acquisition callback.

### Frozen adapter ingress seam

Read-only inspection of the actual `RealTimeMultiSampleArray` and plugin
lifecycle resolves the initial ownership/bootstrap questions as follows:

- The private queue uses `QSharedPointer<const FiffInfo>`, matching the native
  measurement ownership type. Copying the Qt handle increments the existing
  control block and does not create a new standard-library control block in the
  acquisition callback. The queue still forward-declares and never dereferences
  `FiffInfo`; only the processing worker converts metadata to data-only channel
  descriptors.
- Queue configuration describes maxima, not an exact input shape. Each slot is
  preallocated as `maxChannelCount x maxBlockSamples` and records both
  `rowCount` and `sampleCount`. A push accepts any positive shape within those
  bounds, deep-copies only the top-left valid region and publishes the matching
  metadata handle. This lets a row-count transition reach the worker in FIFO
  order instead of being rejected under stale picks.
- The plugin configures the queue in `start()`, before acquisition callbacks,
  with v1 limits of 512 channels, 2048 samples per block and capacity four.
  Slot matrices therefore reserve about 32 MiB total. BabyMEG-scale 270x128
  blocks fit with wide margin. Oversized/empty inputs return `InvalidBlock` and
  are counted as dropped without retry; v1 does not allocate a larger queue in
  the callback.
- The consumer owns a maximum-sized destination, receives the two valid extents
  and copies only that region into an exact-sized worker matrix. Any allocation
  caused by a first/new shape occurs on the processing worker at its block
  boundary, where metadata/settings/reset reconfiguration is also serialized.

`RealTimeMultiSampleArray::info()` itself takes the measurement's internal Qt
mutex. The adapter must call it once per notification to pair each matrix with
the current native metadata handle; this is an upstream accessor constraint,
not a queue wait or algorithm lock. For every matrix in the notification the
plugin then performs exactly one zero-time `tryPush`, with no retry, FIFF picks,
model configuration, settings work or busy wait in the callback. No
`AbstractAlgorithm`, global measurement or existing circular-buffer interface
is changed.

The pending `R-QUEUE-001` review remains useful for stop/publication races in
the original exact-row implementation. Its findings must be applied to the
native-ownership/variable-row implementation, followed by a fresh formal queue
gate before plugin lifecycle code is accepted.

### Frozen plugin caller lifecycle

`AdaptiveDenoising` is the only `AbstractAlgorithm` adapter. It owns the queue,
one worker-thread `AdaptiveDenoisingProcessor`, exact-sized worker matrix,
pending UI snapshot, atomic dropped-block counter and input/output RTMSA
connectors. No new base class, registry or change to `AbstractAlgorithm` is
introduced.

- `init()` creates DirectConnection input and RTMSA output connectors and the
  teaching controls on the GUI thread. `start()` allocates the maximum queue
  and consumer matrix before starting the worker. `stop()` stops/wakes the
  queue, requests interruption, waits for the worker, clears output and leaves
  a fresh later `start()` to reconfigure the queue.
- Because the DirectConnection producer is not the plugin worker, lifecycle
  owns an atomic accepting-input gate plus an in-flight producer guard.
  `start()` publishes acceptance only after queue/destination configuration;
  `stop()` clears acceptance before stopping the queue, waits for the worker,
  and confirms any callback that had already entered has left before a future
  start may replace the stopped PImpl. The callback guard uses atomic entry/
  exit only and never waits or takes the UI/settings mutex. This enforces the
  queue's documented quiescence precondition across stop/restart without
  assuming that connector disconnection is synchronous.
- `update()` dynamically identifies RTMSA, takes one native `info()` snapshot
  per notification, and calls `tryPush(matrix, info)` exactly once for every
  matrix in that notification. It never initializes output/UI, resolves FIFF
  rows, reads settings, configures/resets a model, retries or waits. Every
  non-`Pushed` result increments the atomic dropped-block count.
- `run()` owns dequeue, exact-region copy and all model state. At each popped
  block boundary it consumes one mutex-protected pending settings/reset
  snapshot. New metadata identity, sampling rate, row count, block width or
  settings revision rebuilds the data-only descriptor and configures a fresh
  processor. `bads.contains(ch.ch_name)` is evaluated only here.
- A returned invalid/missing configuration already disarms the processor. If
  allocation throws while configuring a new layout, the worker catches it,
  deliberately disarms through an invalid descriptor, reports a fixed
  configuration-exception status and forwards the current block unchanged;
  it never applies preserved old ownership to new metadata.
- Disabled, frozen and enabled map to `BypassTrackHistory`, `ApplyOnly` and
  `ApplyAndLearn`. Reset is consumed before the current block. Output metadata
  is initialized/reinitialized on the worker for a new valid `FiffInfo` handle,
  then the exact block is emitted in dequeue FIFO order whether denoising is
  active or safely bypassed.
- UI setters touch only the pending snapshot under a GUI/worker mutex. The
  acquisition callback never locks it. Worker diagnostics are emitted through
  queued Qt signals and display configure/process status, R/M/P, warmup,
  generation, input/output/estimated-noise RMS and atomic dropped count.

The legacy output initializer is not const-correct: input `info()` returns
`QSharedPointer<FiffInfo>`, the queue intentionally retains it as
`QSharedPointer<const FiffInfo>`, while
`RealTimeMultiSampleArray::initFromFiffInfo` accepts only
`QSharedPointer<FiffInfo>`. The worker may use one explicit Qt `constCast` only
at that output-initialization adapter call. Neither the plugin nor output path
may mutate the metadata object; all FIFF inspection remains const. This local
compatibility bridge is preferable to weakening queue ownership or changing
the global measurement interface within this feature.

The plugin target links only the dependencies it uses: Qt Core/Widgets,
`mne_utils`, `mne_fiff`, `mne_rtprocessing`, Eigen, `scShared` and `scMeas`.
It does not inherit the broad dependency set of `noisereduction` and does not
touch that plugin or AdaptiveTSSS.

The selected plugin-private queue interface transports `FiffInfo` ownership
without including or inspecting its definition. The header forward-declares
`FIFFLIB::FiffInfo`; only the later plugin adapter dereferences it.

```cpp
enum class AdaptiveDenoisingQueueConfigureStatus : std::uint8_t {
    Ready,
    InvalidConfiguration,
    AlreadyRunning
};

enum class AdaptiveDenoisingQueuePushStatus : std::uint8_t {
    Pushed,
    Full,
    InvalidBlock,
    Stopped
};

enum class AdaptiveDenoisingQueuePopStatus : std::uint8_t {
    Popped,
    Timeout,
    Stopped,
    InvalidDestination
};

struct AdaptiveDenoisingBlockQueueConfig {
    Eigen::Index maxChannelCount;
    Eigen::Index maxBlockSamples;
    std::size_t capacity;
};

struct AdaptiveDenoisingQueuedBlock {
    Eigen::MatrixXd data; // caller preallocates maxima
    Eigen::Index rowCount;
    Eigen::Index sampleCount;
    QSharedPointer<const FIFFLIB::FiffInfo> fiffInfo;
};

class AdaptiveDenoisingBlockQueue final {
public:
    AdaptiveDenoisingBlockQueue() noexcept;
    ~AdaptiveDenoisingBlockQueue();
    AdaptiveDenoisingBlockQueue(const AdaptiveDenoisingBlockQueue&) = delete;
    AdaptiveDenoisingBlockQueue& operator=(const AdaptiveDenoisingBlockQueue&) = delete;
    AdaptiveDenoisingBlockQueue(AdaptiveDenoisingBlockQueue&&) = delete;
    AdaptiveDenoisingBlockQueue& operator=(AdaptiveDenoisingBlockQueue&&) = delete;

    AdaptiveDenoisingQueueConfigureStatus configure(
        const AdaptiveDenoisingBlockQueueConfig& config);
    AdaptiveDenoisingQueuePushStatus tryPush(
        Eigen::Ref<const Eigen::MatrixXd> block,
        QSharedPointer<const FIFFLIB::FiffInfo> fiffInfo) noexcept;
    AdaptiveDenoisingQueuePopStatus waitPop(
        AdaptiveDenoisingQueuedBlock& destination,
        int timeoutMilliseconds) noexcept;
    void stop() noexcept;
};
```

`configure` is transactional, allocates every maximum-sized matrix slot and
starts an empty queue; it is rejected while already running. The producer
validates `0 < rows <= maxChannelCount` and
`0 < cols <= maxBlockSamples`, performs exactly one zero-time free-slot
semaphore acquire, deep-copies the valid top-left region, retains the native
shared immutable metadata handle and publishes the slot. `Full` does not
advance or overwrite either ring position, so the newest input is dropped.

The consumer provides a preallocated `maxChannelCount x maxBlockSamples`
matrix; `waitPop` copies only the valid top-left region and returns `rowCount`,
`sampleCount` and metadata in the same FIFO order. The unused destination tail
is unspecified. `stop()` wakes a timed waiter and prevents new pushes; a later
successful configure creates fresh preallocated state. Push/pop/stop perform
no heap allocation, string construction or busy wait. `QSharedPointer` copies
reuse their control block; null metadata is transported faithfully so the
later worker can fail closed on missing metadata.

The focused `test_adaptive_denoising_plugin` target compiles private queue,
processor and numerical sources directly and initially links only Qt Core/Test
and Eigen. Its worker-owned processor consumes an immutable data-only stream
descriptor: sampling frequency plus one `{fiffKind, isBad}` entry per row. Tests
use the real FIFF integer constants to prove REF_MEG/MEG/STIM/other selection,
bad exclusion and behavior, without requiring construction/linkage of the full
FIFF library. The real plugin has one local conversion loop from `FiffInfo`
(`ch.kind` and `bads.contains(ch.ch_name)`) to this descriptor at the worker
metadata boundary.

This split is required by a measured baseline/toolchain incompatibility rather
than by the desired production architecture: with MSVC 14.51 and the installed
Qt 5.15.2 headers, building existing `mne_fiff` fails inside Qt `qlist.h`
because the new STL removed `stdext::make_checked_array_iterator`. No plugin
source participates in that failure. Do not patch vendor Qt or broaden this
feature into a repository-wide FIFF/toolchain migration. The real
`FiffInfo` conversion and shared/static client linkage are verified by the
tracked `R-CORE-LINK-001` smoke on a supported compiler/Qt combination (or a
separately approved narrow compatibility fix) before final integration.

`scShared`, `scMeas`, Widgets, plugin metadata and `AbstractAlgorithm` enter
only the real `scan_adaptivedenoising` target and a later lifecycle slice that
genuinely exercises them; basic mapping, queue and processor tests stay outside
the full mne_scan GUI dependency graph.

The selected plugin-private processor seam is concrete and has no strategy or
registry interface:

```cpp
struct AdaptiveDenoisingChannelDescriptor {
    int fiffKind;
    bool isBad;
};

struct AdaptiveDenoisingStreamDescriptor {
    double samplingFrequencyHz;
    std::vector<AdaptiveDenoisingChannelDescriptor> channels;
};

struct AdaptiveDenoisingSettings {
    Eigen::Index tapCount = 4;
    Eigen::Index adaptationIntervalSamples = 128;
    double memoryTimeSeconds = 30.0;
    double regularization = 1e-3;
};

enum class AdaptiveDenoisingConfigureStatus : std::uint8_t {
    Ready,
    InvalidMetadata,
    MissingReferences,
    MissingTargets,
    InvalidSettings
};

struct AdaptiveDenoisingConfigureResult {
    AdaptiveDenoisingConfigureStatus status;
    Eigen::Index referenceCount;
    Eigen::Index targetCount;
    Eigen::Index featureCount;
};

class AdaptiveDenoisingProcessor final {
public:
    AdaptiveDenoisingConfigureResult configure(
        const AdaptiveDenoisingStreamDescriptor& stream,
        Eigen::Index maxBlockSamples,
        const AdaptiveDenoisingSettings& settings);
    RTPROCESSINGLIB::DenoiserProcessResult process(
        Eigen::Ref<Eigen::MatrixXd> block,
        RTPROCESSINGLIB::DenoisingMode mode) noexcept;
    void reset() noexcept;
    AdaptiveDenoisingConfigureResult configuration() const noexcept;
};
```

`configure` runs only on the processing thread at a block boundary. It validates
finite positive sampling frequency, descriptor count/block bound and UI setting
ranges, selects good `FIFFV_REF_MEG_CH` rows as references and good
`FIFFV_MEG_CH` rows as targets, and delegates feature-limit validation to the
numerical config. Every configure attempt commits the adapter availability:
invalid/missing layouts disarm the old numerical model so later `process` calls
cannot accidentally apply weights from previous metadata. While disarmed,
`process` preserves the whole block and reports the numerical NotConfigured
shape of diagnostics; no string is constructed there. A Ready configure creates
a new numerical configuration and resets the model. `configuration()` is a
fixed-size worker/UI snapshot.

`AdaptiveDenoisingProcessor` is explicitly default-constructible, single-
worker owned, noncopyable and nonmovable. Moving the numerical ownership
without atomically changing the fixed configuration snapshot would make a
moved-from instance report `Ready` while processing as unconfigured. V1 has no
real second ownership adapter requiring a move seam, so copy and move
construction/assignment are deleted and enforced by C++14 type traits.

The real `FiffInfo` conversion treats each row bad when
`info.bads.contains(ch.ch_name)`; STIM, misc, bad MEG/reference and all unrelated
rows are never selected and therefore must remain value-identical. Enabled/
frozen controls map outside this processor to ApplyAndLearn/ApplyOnly;
disabled maps to BypassTrackHistory so causal reference history continues.

## Acceptance criteria

- Correlated environmental noise is reduced by at least 10 dB after warmup.
- Known clean-component amplitude error is at most 2 percent.
- Finite-stream outputs from different input chunking agree within `1e-10`
  relative tolerance.
- Public-interface tests cover transactionality, selections, lag ordering,
  causality, forgetting, rank deficiency, solver rejection, reset, all modes,
  non-finite data, and row preservation.
- An Eigen malloc guard demonstrates no hot-path allocation after configure.
- Release benchmark records p50/p95 for 1000 Hz and 128-sample blocks; on the
  development machine p95 is less than one block duration.
- Focused tests and example pass without starting `mne_rt_server`.

## Worker lifecycle and model selection

- The manager is this user-visible conversation, requested to run on
  `gpt-5.6-sol` with ultra reasoning. It controls direction, GitHub issues,
  integration and final acceptance and does not create internal subagents.
- Implementation and review work run in newly created user-visible work
  conversations with independent worktrees. The manager decides whether a
  work conversation remains reusable after its response is integrated. Keep
  conversations whose same responsibility and context remain useful; retire
  one-shot, failed, superseded, or stale-worktree conversations after their
  evidence is durable.
- A worker receives one logged `REQUEST <id>` and, on completion, first writes
  `RESPONSE <id>` evidence to the durable ledger, then sends that response to
  the manager conversation. The manager does not continuously poll work
  conversations. Cross-conversation questions use the same REQUEST/RESPONSE
  envelope and are relayed by the manager.
- A dedicated visible Sol/ultra review conversation reviews each milestone
  commit without editing it. Findings return to a new or still-relevant
  implementation conversation; the review conversation never owns fixes.
- The current app exposes thread archiving rather than hard deletion. Archive
  non-reusable work conversations; use hard deletion instead if a supported
  thread-delete operation becomes available. Never remove app-owned worktrees
  manually as a substitute for conversation lifecycle management.
- Test and implementation workers default to `gpt-5.6-luna` at `max`, the
  highest supported Luna effort. Use `gpt-5.6-sol` when the manager judges the
  task to require independent high-risk reasoning, especially numerical math,
  real-time/concurrency safety, interface review, or final integration review;
  reviewers use Sol/ultra by default.
