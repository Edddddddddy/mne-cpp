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
