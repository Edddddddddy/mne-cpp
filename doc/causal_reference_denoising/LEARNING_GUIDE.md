# Causal Reference Denoising v1: Engineering Learning Guide

This guide is a compact map of the implemented v1. The numerical seam is a
single-stream, worker-owned Eigen module: it learns a causal linear model from
good reference rows and subtracts the prediction only from selected target
rows. Read the [v1 specification](./SPEC.md) for the full contract.

## Where to read the implementation

| Concern | Source |
| --- | --- |
| Public numerical interface and result contract | [`causalreferencedenoiser.h`](../../src/libraries/rtprocessing/causalreferencedenoiser.h) |
| State, EWLS update, solver, and hot path | [`causalreferencedenoiser.cpp`](../../src/libraries/rtprocessing/causalreferencedenoiser.cpp) |
| Deterministic public-interface example | [`ex_causal_reference_denoising/main.cpp`](../../src/examples/ex_causal_reference_denoising/main.cpp) |
| Numerical regression tests | [`test_causal_reference_denoiser.cpp`](../../src/testframes/test_causal_reference_denoiser/test_causal_reference_denoiser.cpp) |
| Plugin-private queue seam | [`adaptivedenoisingblockqueue.h`](../../src/applications/mne_scan/plugins/adaptivedenoising/adaptivedenoisingblockqueue.h) and [`adaptivedenoisingblockqueue.cpp`](../../src/applications/mne_scan/plugins/adaptivedenoising/adaptivedenoisingblockqueue.cpp) |
| Queue and adapter tests | [`test_adaptive_denoising_plugin.cpp`](../../src/testframes/test_adaptive_denoising_plugin/test_adaptive_denoising_plugin.cpp) |

## 1. Matrix shapes and row ownership

An input block is an Eigen matrix (X \in \mathbb{R}^{C \times N}): rows are
channels and columns are chronological samples. The configured row lists are:

- `referenceRows`: (R) reference rows, written (r(t) \in \mathbb{R}^R);
- `targetRows`: (M) target rows, whose raw values are (y(t) \in \mathbb{R}^M);
- every other row is preserved, including STIM, bad, and unrelated rows.

With (L) taps, the feature count is (P = R L). Reference and target rows
must be nonempty, in range, unique, and disjoint. The plugin selects good
`FIFFV_REF_MEG_CH` rows as references and good `FIFFV_MEG_CH` rows as targets;
the numerical module itself receives only integer row lists and has no FIFF or
Qt dependency.

Only target entries may be written. Reference and preserved rows are
value-identical after a valid call, not merely approximately unchanged.

## 2. Exact features and causal history

The feature order is tap-major, with the current sample first:

```text
phi(t) = [r0(t), r1(t), ..., r(R-1)(t),
          r0(t-1), r1(t-1), ...,
          ...,
          r0(t-L+1), ..., r(R-1)(t-L+1)]^T
```

Equivalently, feature index `lag * R + reference` holds
`r_reference(t - lag)`. It is not reference-major. The example's eight
coefficients are therefore ordered as
`[r0(t), r1(t), r0(t-1), r1(t-1), ...]`.

The implementation owns an (R \times (L-1)) history matrix. Before sample
(t), history column `lag` contains (r(t-1-\text{lag})); the current
reference vector is copied from the input, `buildFeature()` concatenates the
current vector and history, and `advanceHistory()` shifts older values right
before putting the current vector in column zero. History is retained across
calls, so splitting one stream into arbitrary block sizes produces the same
state and output as processing it in one call. The focused test checks this at
relative tolerance (10^{-10}).

After configure or reset, history is zero. The first (L-1) samples are
warmup: they advance history but do not apply a model or accumulate learning
statistics. A zero initial model therefore passes the first complete
adaptation epoch unchanged.

## 3. EWLS state, forgetting, and the epoch transaction

The committed model has (W \in \mathbb{R}^{M \times P}). For an eligible
sample, it predicts from the old committed model and subtracts that prediction:

```text
prediction(t) = W * phi(t)
output(t)     = y(t) - prediction(t)
```

Learning uses the raw target captured before the subtraction. With

```text
lambda = exp(-1 / (samplingFrequencyHz * memoryTimeSeconds))
```

each learning sample updates preallocated pending statistics:

```text
pending G <- lambda * pending G + phi(t) * phi(t)^T       (P x P)
pending H <- lambda * pending H + y(t)   * phi(t)^T       (M x P)
```

If an epoch has accumulated (n) eligible samples, its candidate recurrence is

```text
candidate G = lambda^n * committed G + pending G
candidate H = lambda^n * committed H + pending H
```

Thus forgetting is per eligible learning sample both within an epoch and when
the committed history is aged into the next candidate. `ApplyOnly` and
`BypassTrackHistory` pause the learning epoch; they do not silently age or
replace the model.

The solve uses relative diagonal loading. Let

```text
s = max(max_i abs(sym(candidate G)_ii), machine_epsilon)
D = s * I(P)
A = sym(candidate G) + regularization * D
```

The source solves

```text
A * candidate W^T = candidate H^T
```

with Eigen `LDLT`; it never forms an inverse. It checks finite matrices,
successful/positive decomposition, positive finite pivots, and finite solved
weights before committing. The loaded solve accepts rank-deficient data when
the candidate remains numerically valid.

`committed G/H/W`, `pending G/H`, and candidate `G/H/W` are separate storage.
At an accepted boundary, all committed `G`, `H`, and `W` change together and
`modelGeneration` increments. At a failed boundary, the old `W` is retained,
the committed `G/H` are aged by the elapsed pending decay, and the poisoned
pending epoch is discarded. A valid block remains `Processed` even when it has
rejected update events.

## 4. Warmup, modes, fixed epochs, and latency

The fixed epoch length is `adaptationIntervalSamples` ((K)). The sample at
which the (K)-th eligible statistic is accumulated is still processed with
the old model. The candidate is committed only after that sample, so the new
model can affect later samples, never the current or past samples. A single
call may cross multiple fixed boundaries; accepted and rejected counts are
reported for that call.

| Mode | Target action | History | Learning |
| --- | --- | --- | --- |
| `BypassTrackHistory` | Preserve the whole block exactly | Advances | Paused |
| `ApplyOnly` | Apply the committed model if ready | Advances | Paused |
| `ApplyAndLearn` | Apply the old committed model | Advances | Accumulates and may commit after (K) eligible samples |

The adapter maps enabled to `ApplyAndLearn`, frozen to `ApplyOnly`, and
disabled to `BypassTrackHistory`. `reset()` zeroes history, pending and
committed statistics, weights, scratch state, counters, and generation; the
next call starts with (L-1) warmup samples again. `configure()` builds a new
fixed-shape state and swaps it in transactionally. A rejected configuration
leaves the prior configuration and state untouched.

There is no lookahead in the numerical path. Useful latency terms are distinct:

1. **Model readiness:** (L-1) samples of causal history are needed after a
   fresh configure/reset.
2. **Adaptation age:** a new model becomes available only at a (K)-sample
   eligible boundary, and is future-only.
3. **Block scheduling:** the plugin worker can add block duration and queue
   backlog before `process()` runs; a full queue drops the newest block.

These are engineering components, not an end-to-end latency guarantee. The
focused tests and the example do not establish a universal real-time bound.

## 5. Configure-only allocation and the process hot path

`configure()` is the allocation phase. It validates the configuration, creates
the implementation, and preallocates the vectors, matrices, and `LDLT` storage
needed by future calls. The feature limit is (P \le 256). The adapter's
`AdaptiveDenoisingSettings` defaults are 4 taps, (K=128), 30 seconds of
memory, and relative regularization (10^{-3}).

The important resident shapes are:

| State | Shape |
| --- | --- |
| current references, feature | (R), (P) |
| causal history | (R \times (L-1)) |
| raw targets, prediction | (M), (M) |
| committed/pending/candidate Gram matrices | (P \times P) each |
| committed/pending/candidate cross matrices | (M \times P) each |
| committed weights | (M \times P) |
| solve matrix and LDLT factorization | (P \times P) storage |
| solve RHS and transposed candidate weights | (P \times M) each |

After configure, `process()` performs no heap allocation, locking, Qt/FIFF
access, or string construction. An Eigen malloc guard in the focused test
exercises both a learning call (including the loaded solve) and an apply-only
call.

The RMS diagnostics use scaled sum-of-squares accumulation instead of a raw
sum of squares, so large finite selected-target values do not overflow the
diagnostics. Input RMS uses original selected target values; output RMS uses
returned target values; estimated-noise RMS uses the prediction actually
subtracted. Bypass has equal input/output RMS and zero estimated noise.

Shape errors and selected reference/target NaN or Inf values return the whole
block unchanged and do not advance state. A finite input can still produce a
non-finite prediction or residual. In that case the application fallback is
sample-wide: all selected target values at that sample remain at their original
finite values, the prediction recorded for diagnostics is zero, history still
advances, and learning may still feed the raw sample into the normal
transactional epoch. This avoids partially corrupting a target row.

### Why the actual `noalias()` calls are safe

The source uses Eigen no-alias expressions at the fixed-shape boundaries:

```cpp
pendingGram.noalias() += feature * feature.transpose();
pendingCross.noalias() += rawTargets * feature.transpose();
candidateGram.noalias() = committedGram;
candidateCross.noalias() = committedCross;
solveRightHandSide.noalias() = candidateCross.transpose();
candidateWeightsTranspose.noalias() = decomposition.solve(solveRightHandSide);
committedGram.noalias() = candidateGram;
committedCross.noalias() = candidateCross;
weights.noalias() = candidateWeightsTranspose.transpose();
prediction.noalias() = weights * feature;
```

Each destination is a distinct preallocated vector or matrix from every operand
that contributes to its expression. For example, `feature * feature.transpose()`
is (P \times P), `rawTargets * feature.transpose()` is (M \times P), and
`weights * feature` is (M). `noalias()` lets Eigen use the direct product or
copy path without protecting against an overlap that the implementation has
already ruled out; it is useful for avoiding unnecessary temporaries on the
hot path, but it is not an allocation or correctness guarantee by itself.

It would be wrong to write `A.noalias() = A * B`, `A.noalias() += A * B`, or a
self-overlapping transpose/copy when the destination shares storage with an
operand. Such an expression can overwrite inputs before the product finishes.
Only use `noalias()` after checking storage disjointness, not merely because an
expression is mathematically valid.

## 6. Plugin-private queue seam

The plugin does not reuse the older global circular-buffer seam. The frozen
queue-v2 is a bounded single-producer/single-consumer queue that uses
always-lock-free unsigned producer and consumer sequence atomics; each side's
ring index is owner-local. Acquire/release ordering publishes filled slots and
makes slot reuse visible. Configure-time allocation preallocates fixed
(C_{max} \times N_{max}) Eigen slots, while each slot carries positive
`rowCount` and `sampleCount` extents plus native
`QSharedPointer<const FiffInfo>` metadata. The producer release-publishes a
slot and then makes exactly one nonblocking native signal attempt. Windows
uses a configure-created auto-reset event; POSIX uses a configure-created
nonblocking `CLOEXEC` pipe. If no slot is free it returns `Full` without
advancing or overwriting a ring position; the adapter drops that newest block
and increments `droppedBlocks`.

The producer's `tryPush` has no Qt semaphore or mutex, wait, retry,
post-configure C++ allocation, FIFF access, or string work. The consumer uses
a bounded wait followed by atomic rechecks, receives a maximum-sized
destination, both extents, and metadata in FIFO order, and copies only the
valid top-left rectangle; the destination's outside tail remains untouched.
At the block boundary, the worker reshapes/copies exactly that
`rowCount x sampleCount` top-left into the processing view before layout,
settings, and numerical processing. `stop()` is idempotent and wakes the
consumer; the caller must quiesce producer and consumer before reconfigure or
destruction. Focused sustained-SPSC coverage includes a hard zero producer-
allocation assertion plus FIFO/drop-newest, extents, metadata, deep-copy,
tail, latency, stop, and lifetime checks.

## 7. Complexity and memory scaling

For (N) samples, (P=RL), (M) targets, and update interval (K), the v1
amortized form is

```text
O(N(P^2 + MP)) + O(P^3 / K)
```

The first term is the per-sample outer-product and prediction work. If every
solve over a complete (N)-sample stream is counted explicitly, the solve term
is (O((N/K)P^3)), or (O(P^3/K)) per sample. Resident numerical state is
(O(P^2 + MP + R(L-1))) up to fixed multiple copies of the matrices listed
above. The plugin queue additionally scales as
(O(\text{capacity} \times C \times N_{max})) for slot data, plus metadata
handles.

## 8. Four-row, two-reference, two-tap walkthrough

Take four channel rows with references 0 and 1, target row 2, and preserved row
3. For two taps:

```text
C = 4, R = 2, M = 1, L = 2, P = R*L = 4
X(:, t) = [r0(t), r1(t), y(t), preserved(t)]^T
phi(t) = [r0(t), r1(t), r0(t-1), r1(t-1)]^T       (4 x 1)
G: 4 x 4, H: 1 x 4, W: 1 x 4
```

Suppose the first two columns are

```text
X(:, 0) = [1, 10, 100, 1000]^T
X(:, 1) = [2, 20,  42, 1001]^T
```

At (t=0), prehistory is zero, so the sample is warmup and the block is
unchanged. After advancing history, the complete feature at (t=1) is

```text
phi(1) = [2, 20, 1, 10]^T

phi(1) * phi(1)^T =
[[  4,  40,  2,  20],
 [ 40, 400, 20, 200],
 [  2,  20,  1,  10],
 [ 20, 200, 10, 100]]

y(1) * phi(1)^T = [84, 840, 42, 420]
```

The first quantity contributes to pending (G), the second to pending (H),
and the old zero (W) predicts zero, so target row 2 is still 42. With
(K=2), a next sample with references ([3,30]^T) has
(phi(2)=[3,30,2,20]^T); after that sample the candidate is solved and
committed, but its weights first affect sample (t=3). Rows 0, 1, and 3 stay
exactly as supplied throughout.

## 9. One block-boundary timeline

For (L=2) and (K=2), with a fresh `ApplyAndLearn` stream:

```text
reset/configure: W0 = 0, history empty, pending empty

block A, t=0: warmup; copy current references into history; output unchanged
block A, t=1: build phi(1); apply W0; pending count = 1
block A, t=2: build phi(2); apply W0; pending count = 2
               end of sample: solve/commit W1, generation increments

block B, t=3: build phi(3) from samples t=3 and t=2; apply W1
```

If `block A` is split as `[t=0]` and `[t=1,t=2]`, or as `[t=0,t=1]`
and `[t=2]`, the history, pending epoch, commit point, and output are the
same. An `ApplyOnly` or bypass call between these samples still advances
history but leaves the fixed learning epoch paused.

## 10. Effect evidence versus engineering verification

These are different kinds of evidence:

- **Effect evidence:** `meetsQuantitativeSyntheticAcceptance()` constructs a
  known multi-tap environmental component plus a clean component, trains over
  fixed blocks, and checks at least 10 dB synthetic noise reduction and at most
  2% clean-component projection error after warmup. The deterministic example
  also checks statuses, generation, warmup, finite targets, and exact row
  preservation. These are controlled synthetic results, not a guarantee for
  arbitrary recordings.
- **Engineering verification:** the focused numerical tests cover transactional
  configuration, tap ordering, causality, arbitrary chunk boundaries,
  forgetting, fixed modes, reset, rank-deficient loaded solves, rejected and
  recovered epochs, selected non-finite input, prediction fallback, scaled RMS,
  target-only writes, and the post-configure Eigen malloc guard. The focused
  plugin tests separately cover channel mapping and the queue seam. Passing
  these tests establishes the tested contracts; it does not create untested
  clinical, universal-denoising, or end-to-end scheduling guarantees.

### Engineering benchmark reproduction

From the repository root containing the existing
`build-causal-reference-denoising` directory, build the focused Release target
and run its benchmark entry point:

```powershell
cmake --build .\build-causal-reference-denoising --config Release --target ex_causal_reference_denoising
& .\out\Release\apps\ex_causal_reference_denoising.exe --benchmark
```

The deterministic workload is 1000 Hz, 270 rows, 128 samples per block, 16
reference rows (0--15), 250 target rows (16--265), 4 preserved rows (266--269),
4 taps, and (P=64). It runs 100 untimed warmup blocks followed by 1000 timed
`ApplyAndLearn` blocks. The recorded nearest-rank result is p50 **2.725 ms**,
p95 **4.317 ms**, and maximum **8.557 ms**; the strict p95 < 128 ms block
duration gate passes. This is development-machine engineering timing evidence
for the numerical hot path. It is not denoising-quality, clinical, universal,
OS-scheduling, or end-to-end queue/worker latency proof; wall-clock results can
vary with machine load. The command does not invoke `mne_rt_server` or full
`mne_scan`.

## 11. Research and license boundary

The design is informed by the [Time-Shift PCA paper by de Cheveigne and
Simon](https://pmc.ncbi.nlm.nih.gov/articles/PMC2018742/), the online MEG
precedent [MEG adaptive noise suppression using Fast LMS](https://doi.org/10.1109/CNE.2005.1419543),
and the BSD [MEEGkit TSPCA/TSR behavior reference](https://nbara.github.io/python-meegkit/modules/meegkit.tspca.html).
FieldTrip's [`ft_denoise_tsr`](https://github.com/fieldtrip/fieldtrip) is a GPL
research-only reference, not a runtime dependency.

The equations and implementation in MNE-CPP were written independently. No
GPL FieldTrip source is copied, translated, or linked into this repository;
the MEEGkit and FieldTrip links document behavior and research context only.

## 12. v1 non-goals

The v1 scope is deliberately narrower than a general artifact-removal system:

- no HFC, rSSS/tSSS or AdaptiveTSSS, ICA/ORICA, or synthetic-reference
  discovery;
- no `mne_rt_server` integration;
- no numerical strategy base, algorithm registry, Qt type, or FIFF type in the
  concrete numerical interface;
- no guarantee beyond the specific tested invariants and synthetic acceptance
  gates described above.

The implemented seam is causal reference regression with fixed update epochs,
explicit state ownership, and safe pass-through behavior. Future algorithms or
broader guarantees require a separately specified interface and evidence.
